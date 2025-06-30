// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include <linux/dma-buf.h>
#include <linux/dma-resv.h>
#include <linux/vmalloc.h>
#include <linux/log2.h>

#include <rdma/ib_addr.h>
#include <rdma/ib_umem.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_verbs.h>
#include <rdma/uverbs_ioctl.h>
#include <rdma/uverbs_named_ioctl.h>

#define UVERBS_MODULE_NAME cxi_ib
#include "cxi.h"

#define CXI_IB_DEFAULT_LINK_SPEED_GBPS   100

enum {
	CXI_IB_MMAP_DMA_PAGE = 0,
	CXI_IB_MMAP_IO_WC,
	CXI_IB_MMAP_IO_NC,
};

struct cxi_ib_user_mmap_entry {
	struct rdma_user_mmap_entry rdma_entry;
	u64 address;
	u8 mmap_flag;
};

int cxi_ib_query_device(struct ib_device *ibdev,
			struct ib_device_attr *props,
			struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibdev, struct cxi_ib_dev, ibdev);
	int err;

	err = cxi_ib_com_get_device_attr(dev, &dev->dev_info);
	if (err)
		return err;

	memset(props, 0, sizeof(*props));

	props->fw_ver = 0; /* TODO: Get from CXI device */
	props->sys_image_guid = dev->dev_info.nid; /* Use NID as GUID */
	props->max_mr_size = ~0ull;
	props->page_size_cap = PAGE_SIZE;
	props->vendor_id = 0x1590; /* HPE vendor ID */
	props->vendor_part_id = 0x0371; /* Cassini device ID */
	props->hw_ver = 1;
	props->max_qp = 1024;
	props->max_qp_wr = 1024;
	props->device_cap_flags = IB_DEVICE_CURR_QP_STATE_MOD |
				  IB_DEVICE_RC_RNR_NAK_GEN |
				  IB_DEVICE_LOCAL_DMA_LKEY;
	props->max_send_sge = 16;
	props->max_recv_sge = 16;
	props->max_sge_rd = 16;
	props->max_cq = 1024;
	props->max_cqe = 1024;
	props->max_mr = 1024;
	props->max_pd = 1024;
	props->max_qp_rd_atom = 16;
	props->max_qp_init_rd_atom = 16;
	props->atomic_cap = IB_ATOMIC_NONE;
	props->masked_atomic_cap = IB_ATOMIC_NONE;
	props->max_ah = 1024;
	props->max_srq = 0; /* SRQ not supported */
	props->max_srq_wr = 0;
	props->max_srq_sge = 0;
	props->local_ca_ack_delay = 0;
	props->max_pkeys = 1;

	return 0;
}

int cxi_ib_query_port(struct ib_device *ibdev, u32 port,
		      struct ib_port_attr *props)
{
	struct cxi_ib_dev *dev = container_of(ibdev, struct cxi_ib_dev, ibdev);

	memset(props, 0, sizeof(*props));

	props->state = IB_PORT_ACTIVE;
	props->max_mtu = IB_MTU_4096;
	props->active_mtu = IB_MTU_4096;
	props->gid_tbl_len = 1;
	props->port_cap_flags = IB_PORT_CM_SUP;
	props->max_msg_sz = 0x40000000;
	props->bad_pkey_cntr = 0;
	props->qkey_viol_cntr = 0;
	props->pkey_tbl_len = 1;
	props->lid = 0;
	props->sm_lid = 0;
	props->lmc = 0;
	props->max_vl_num = 1;
	props->sm_sl = 0;
	props->subnet_timeout = 0;
	props->init_type_reply = 0;
	props->active_width = IB_WIDTH_4X;
	props->active_speed = IB_SPEED_EDR;
	props->phys_state = IB_PORT_PHYS_STATE_LINK_UP;

	return 0;
}

int cxi_ib_query_gid(struct ib_device *ibdev, u32 port, int index,
		     union ib_gid *gid)
{
	struct cxi_ib_dev *dev = container_of(ibdev, struct cxi_ib_dev, ibdev);

	if (index != 0)
		return -EINVAL;

	/* Use the CXI device's MAC address to construct GID */
	memset(gid, 0, sizeof(*gid));
	memcpy(&gid->raw[8], dev->cxi_dev->mac_addr, ETH_ALEN);
	gid->raw[0] = 0xfe;
	gid->raw[1] = 0x80;

	return 0;
}

int cxi_ib_query_pkey(struct ib_device *ibdev, u32 port, u16 index,
		      u16 *pkey)
{
	if (index != 0)
		return -EINVAL;

	*pkey = 0xffff;
	return 0;
}

int cxi_ib_alloc_pd(struct ib_pd *ibpd, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibpd->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_pd *pd = container_of(ibpd, struct cxi_ib_pd, ibpd);
	struct cxi_ib_alloc_pd_result result;
	int err;

	err = cxi_ib_com_alloc_pd(dev->cxi_dev, &result);
	if (err) {
		atomic64_inc(&dev->stats.alloc_pd_err);
		return err;
	}

	pd->pdn = result.pdn;
	return 0;
}

int cxi_ib_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibpd->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_pd *pd = container_of(ibpd, struct cxi_ib_pd, ibpd);
	struct cxi_ib_dealloc_pd_params params = {
		.pdn = pd->pdn,
	};

	return cxi_ib_com_dealloc_pd(dev->cxi_dev, &params);
}

int cxi_ib_alloc_ucontext(struct ib_ucontext *ibucontext, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibucontext->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_ucontext *ucontext = container_of(ibucontext, struct cxi_ib_ucontext, ibucontext);
	struct cxi_ib_alloc_ucontext_result result;
	int err;

	err = cxi_ib_com_alloc_ucontext(dev->cxi_dev, &result);
	if (err) {
		atomic64_inc(&dev->stats.alloc_ucontext_err);
		return err;
	}

	ucontext->uarn = result.uarn;
	return 0;
}

void cxi_ib_dealloc_ucontext(struct ib_ucontext *ibucontext)
{
	struct cxi_ib_dev *dev = container_of(ibucontext->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_ucontext *ucontext = container_of(ibucontext, struct cxi_ib_ucontext, ibucontext);
	struct cxi_ib_dealloc_ucontext_params params = {
		.uarn = ucontext->uarn,
	};

	cxi_ib_com_dealloc_ucontext(dev->cxi_dev, &params);
}

int cxi_ib_create_cq(struct ib_cq *ibcq, const struct ib_cq_init_attr *attr,
		     struct uverbs_attr_bundle *attrs)
{
	struct cxi_ib_dev *dev = container_of(ibcq->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_cq *cq = container_of(ibcq, struct cxi_ib_cq, ibcq);
	struct cxi_ib_create_cq_params params;
	struct cxi_ib_create_cq_result result;
	int err;

	if (attr->cqe > 1024) {
		atomic64_inc(&dev->stats.create_cq_err);
		return -EINVAL;
	}

	params.cq_depth = attr->cqe;
	params.uarn = 0; /* TODO: Get from ucontext */
	params.eqn = 0; /* TODO: Assign event queue */
	params.entry_size_in_bytes = 64;
	params.interrupt_mode_enabled = (attr->comp_vector != -1);

	err = cxi_ib_com_create_cq(dev->cxi_dev, &params, &result);
	if (err) {
		atomic64_inc(&dev->stats.create_cq_err);
		return err;
	}

	cq->cq_idx = result.cq_idx;
	cq->size = result.actual_depth * params.entry_size_in_bytes;

	return 0;
}

int cxi_ib_destroy_cq(struct ib_cq *ibcq, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibcq->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_cq *cq = container_of(ibcq, struct cxi_ib_cq, ibcq);
	struct cxi_ib_destroy_cq_params params = {
		.cq_idx = cq->cq_idx,
	};

	return cxi_ib_com_destroy_cq(dev->cxi_dev, &params);
}

int cxi_ib_create_qp(struct ib_qp *ibqp, struct ib_qp_init_attr *init_attr,
		     struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibqp->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_qp *qp = container_of(ibqp, struct cxi_ib_qp, ibqp);
	struct cxi_ib_create_qp_params params;
	struct cxi_ib_create_qp_result result;
	int err;

	if (init_attr->qp_type != IB_QPT_RC) {
		atomic64_inc(&dev->stats.create_qp_err);
		return -EINVAL;
	}

	params.send_cq_idx = init_attr->send_cq ? 
		container_of(init_attr->send_cq, struct cxi_ib_cq, ibcq)->cq_idx : 0;
	params.recv_cq_idx = init_attr->recv_cq ?
		container_of(init_attr->recv_cq, struct cxi_ib_cq, ibcq)->cq_idx : 0;
	params.sq_depth = init_attr->cap.max_send_wr;
	params.rq_depth = init_attr->cap.max_recv_wr;
	params.pd = init_attr->pd ?
		container_of(init_attr->pd, struct cxi_ib_pd, ibpd)->pdn : 0;
	params.uarn = 0; /* TODO: Get from ucontext */
	params.qp_type = init_attr->qp_type;
	params.sl = 0;

	err = cxi_ib_com_create_qp(dev->cxi_dev, &params, &result);
	if (err) {
		atomic64_inc(&dev->stats.create_qp_err);
		return err;
	}

	qp->qp_handle = result.qp_handle;
	qp->max_send_wr = params.sq_depth;
	qp->max_recv_wr = params.rq_depth;
	qp->max_send_sge = init_attr->cap.max_send_sge;
	qp->max_recv_sge = init_attr->cap.max_recv_sge;
	qp->max_inline_data = init_attr->cap.max_inline_data;
	qp->state = IB_QPS_RESET;

	ibqp->qp_num = result.qp_num;

	return 0;
}

int cxi_ib_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibqp->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_qp *qp = container_of(ibqp, struct cxi_ib_qp, ibqp);
	struct cxi_ib_destroy_qp_params params = {
		.qp_handle = qp->qp_handle,
	};

	return cxi_ib_com_destroy_qp(dev->cxi_dev, &params);
}

int cxi_ib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
		    int qp_attr_mask, struct ib_qp_init_attr *qp_init_attr)
{
	struct cxi_ib_dev *dev = container_of(ibqp->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_qp *qp = container_of(ibqp, struct cxi_ib_qp, ibqp);
	struct cxi_ib_query_qp_params params = {
		.qp_handle = qp->qp_handle,
	};
	struct cxi_ib_query_qp_result result;
	int err;

	err = cxi_ib_com_query_qp(dev->cxi_dev, &params, &result);
	if (err)
		return err;

	qp_attr->qp_state = result.qp_state;
	qp_attr->qkey = result.qkey;
	qp_attr->sq_psn = result.sq_psn;
	qp_attr->rnr_retry = result.rnr_retry;

	qp_init_attr->cap.max_send_wr = qp->max_send_wr;
	qp_init_attr->cap.max_recv_wr = qp->max_recv_wr;
	qp_init_attr->cap.max_send_sge = qp->max_send_sge;
	qp_init_attr->cap.max_recv_sge = qp->max_recv_sge;
	qp_init_attr->cap.max_inline_data = qp->max_inline_data;

	return 0;
}

int cxi_ib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
		     int qp_attr_mask, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibqp->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_qp *qp = container_of(ibqp, struct cxi_ib_qp, ibqp);
	struct cxi_ib_modify_qp_params params;
	int err;

	params.qp_handle = qp->qp_handle;
	params.modify_mask = qp_attr_mask;
	params.cur_qp_state = qp->state;

	if (qp_attr_mask & IB_QP_STATE) {
		params.qp_state = qp_attr->qp_state;
		qp->state = qp_attr->qp_state;
	}

	if (qp_attr_mask & IB_QP_QKEY)
		params.qkey = qp_attr->qkey;

	if (qp_attr_mask & IB_QP_SQ_PSN)
		params.sq_psn = qp_attr->sq_psn;

	if (qp_attr_mask & IB_QP_RNR_RETRY)
		params.rnr_retry = qp_attr->rnr_retry;

	err = cxi_ib_com_modify_qp(dev->cxi_dev, &params);
	if (err)
		return err;

	return 0;
}

struct ib_mr *cxi_ib_reg_mr(struct ib_pd *ibpd, u64 start, u64 length,
			    u64 virt_addr, int access_flags,
			    struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibpd->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_pd *pd = container_of(ibpd, struct cxi_ib_pd, ibpd);
	struct cxi_ib_mr *mr;
	struct cxi_ib_reg_mr_params params;
	struct cxi_ib_reg_mr_result result;
	int err;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		atomic64_inc(&dev->stats.reg_mr_err);
		return ERR_PTR(-ENOMEM);
	}

	mr->umem = ib_umem_get(udata, start, length, access_flags);
	if (IS_ERR(mr->umem)) {
		err = PTR_ERR(mr->umem);
		atomic64_inc(&dev->stats.reg_mr_err);
		goto err_free_mr;
	}

	params.start = start;
	params.length = length;
	params.virt_addr = virt_addr;
	params.access_flags = access_flags;
	params.pdn = pd->pdn;

	err = cxi_ib_com_reg_mr(dev->cxi_dev, &params, &result);
	if (err) {
		atomic64_inc(&dev->stats.reg_mr_err);
		goto err_release_umem;
	}

	mr->ibmr.lkey = result.l_key;
	mr->ibmr.rkey = result.r_key;
	mr->ibmr.length = length;
	mr->ibmr.iova = virt_addr;

	return &mr->ibmr;

err_release_umem:
	ib_umem_release(mr->umem);
err_free_mr:
	kfree(mr);
	return ERR_PTR(err);
}

struct ib_mr *cxi_ib_reg_user_mr_dmabuf(struct ib_pd *ibpd, u64 start,
					 u64 length, u64 virt_addr,
					 int fd, int access_flags,
					 struct uverbs_attr_bundle *attrs)
{
	/* For now, return not supported */
	return ERR_PTR(-EOPNOTSUPP);
}

int cxi_ib_dereg_mr(struct ib_mr *ibmr, struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibmr->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_mr *mr = container_of(ibmr, struct cxi_ib_mr, ibmr);
	struct cxi_ib_dereg_mr_params params = {
		.l_key = ibmr->lkey,
	};
	int err;

	err = cxi_ib_com_dereg_mr(dev->cxi_dev, &params);
	if (err)
		return err;

	if (mr->umem)
		ib_umem_release(mr->umem);

	kfree(mr);
	return 0;
}

int cxi_ib_create_ah(struct ib_ah *ibah,
		     struct rdma_ah_init_attr *init_attr,
		     struct ib_udata *udata)
{
	struct cxi_ib_dev *dev = container_of(ibah->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_ah *ah = container_of(ibah, struct cxi_ib_ah, ibah);
	struct cxi_ib_create_ah_params params;
	struct cxi_ib_create_ah_result result;
	struct rdma_ah_attr *ah_attr = init_attr->ah_attr;
	int err;

	if (ah_attr->type != RDMA_AH_ATTR_TYPE_IB) {
		atomic64_inc(&dev->stats.create_ah_err);
		return -EINVAL;
	}

	params.pdn = container_of(init_attr->ah_attr->pd, struct cxi_ib_pd, ibpd)->pdn;
	memcpy(params.dest_addr, ah_attr->grh.dgid.raw, CXI_IB_GID_SIZE);

	err = cxi_ib_com_create_ah(dev->cxi_dev, &params, &result);
	if (err) {
		atomic64_inc(&dev->stats.create_ah_err);
		return err;
	}

	ah->ah = result.ah;
	memcpy(ah->id, params.dest_addr, CXI_IB_GID_SIZE);

	return 0;
}

int cxi_ib_destroy_ah(struct ib_ah *ibah, u32 flags)
{
	struct cxi_ib_dev *dev = container_of(ibah->device, struct cxi_ib_dev, ibdev);
	struct cxi_ib_ah *ah = container_of(ibah, struct cxi_ib_ah, ibah);
	struct cxi_ib_destroy_ah_params params = {
		.ah = ah->ah,
	};

	return cxi_ib_com_destroy_ah(dev->cxi_dev, &params);
}

int cxi_ib_get_port_immutable(struct ib_device *ibdev, u32 port_num,
			      struct ib_port_immutable *immutable)
{
	struct ib_port_attr attr;
	int err;

	err = cxi_ib_query_port(ibdev, port_num, &attr);
	if (err)
		return err;

	immutable->pkey_tbl_len = attr.pkey_tbl_len;
	immutable->gid_tbl_len = attr.gid_tbl_len;
	immutable->core_cap_flags = RDMA_CORE_PORT_IBA_IB;
	immutable->max_mad_size = IB_MGMT_MAD_SIZE;

	return 0;
}

enum rdma_link_layer cxi_ib_port_link_layer(struct ib_device *ibdev,
					     u32 port_num)
{
	return IB_LINK_LAYER_INFINIBAND;
}

int cxi_ib_mmap(struct ib_ucontext *ibucontext, struct vm_area_struct *vma)
{
	struct cxi_ib_dev *dev = container_of(ibucontext->device, struct cxi_ib_dev, ibdev);
	struct rdma_user_mmap_entry *rdma_entry;
	struct cxi_ib_user_mmap_entry *entry;
	int err = 0;

	rdma_entry = rdma_user_mmap_entry_get_pgoff(ibucontext, vma->vm_pgoff);
	if (!rdma_entry) {
		atomic64_inc(&dev->stats.mmap_err);
		return -EINVAL;
	}

	entry = container_of(rdma_entry, struct cxi_ib_user_mmap_entry, rdma_entry);

	switch (entry->mmap_flag) {
	case CXI_IB_MMAP_DMA_PAGE:
		err = rdma_user_mmap_io(ibucontext, vma, entry->address,
					rdma_entry->npages * PAGE_SIZE,
					pgprot_noncached(vma->vm_page_prot),
					rdma_entry);
		break;
	case CXI_IB_MMAP_IO_WC:
		err = rdma_user_mmap_io(ibucontext, vma, entry->address,
					rdma_entry->npages * PAGE_SIZE,
					pgprot_writecombine(vma->vm_page_prot),
					rdma_entry);
		break;
	case CXI_IB_MMAP_IO_NC:
		err = rdma_user_mmap_io(ibucontext, vma, entry->address,
					rdma_entry->npages * PAGE_SIZE,
					pgprot_noncached(vma->vm_page_prot),
					rdma_entry);
		break;
	default:
		err = -EINVAL;
		break;
	}

	rdma_user_mmap_entry_put(rdma_entry);

	if (err)
		atomic64_inc(&dev->stats.mmap_err);

	return err;
}

void cxi_ib_mmap_free(struct rdma_user_mmap_entry *rdma_entry)
{
	struct cxi_ib_user_mmap_entry *entry;

	entry = container_of(rdma_entry, struct cxi_ib_user_mmap_entry, rdma_entry);
	kfree(entry);
}

struct rdma_hw_stats *cxi_ib_alloc_hw_port_stats(struct ib_device *ibdev, u32 port_num)
{
	/* For now, return NULL - no port-specific stats */
	return NULL;
}

struct rdma_hw_stats *cxi_ib_alloc_hw_device_stats(struct ib_device *ibdev)
{
	/* For now, return NULL - no device-specific stats */
	return NULL;
}

int cxi_ib_get_hw_stats(struct ib_device *ibdev, struct rdma_hw_stats *stats,
			u32 port_num, int index)
{
	/* For now, return 0 - no stats to report */
	return 0;
}

/* Vendor-specific ioctl handler 1: CXI Method 1 - Device information query */
static int UVERBS_HANDLER(CXI_IB_METHOD_1)(struct uverbs_attr_bundle *attrs)
{
	struct ib_device *ibdev = attrs->context->device;
	struct cxi_ib_dev *dev = container_of(ibdev, struct cxi_ib_dev, ibdev);
	struct cxi_ib_method1_resp resp = {};
	int ret;

	/* Fill device-specific information */
	resp.comp_mask = 0;
	resp.nic_addr = dev->dev_info.nid; /* Use NID as NIC address */
	resp.pid_granule = dev->dev_info.pid_granule;
	resp.pid_count = dev->dev_info.pid_count;
	resp.pid_bits = dev->dev_info.pid_bits;
	resp.min_free_shift = dev->dev_info.min_free_shift;

	/* Copy response to user space */
	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD1_RESP_NIC_ADDR,
			     &resp.nic_addr, sizeof(resp.nic_addr));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD1_RESP_PID_GRANULE,
			     &resp.pid_granule, sizeof(resp.pid_granule));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD1_RESP_PID_COUNT,
			     &resp.pid_count, sizeof(resp.pid_count));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD1_RESP_PID_BITS,
			     &resp.pid_bits, sizeof(resp.pid_bits));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD1_RESP_MIN_FREE_SHIFT,
			     &resp.min_free_shift, sizeof(resp.min_free_shift));
	if (ret)
		return ret;

	ibdev_dbg(ibdev, "CXI Method 1: NIC addr=0x%x, PID granule=%u\n",
		  resp.nic_addr, resp.pid_granule);

	return 0;
}

/* Vendor-specific ioctl handler 2: CXI Method 2 - Memory region information query */
static int UVERBS_HANDLER(CXI_IB_METHOD_2)(struct uverbs_attr_bundle *attrs)
{
	struct ib_mr *ibmr = uverbs_attr_get_obj(attrs, CXI_IB_ATTR_METHOD2_MR_HANDLE);
	struct cxi_ib_mr *mr = container_of(ibmr, struct cxi_ib_mr, ibmr);
	struct cxi_ib_method2_resp resp = {};
	int ret;

	if (!ibmr) {
		return -EINVAL;
	}

	/* Fill memory region specific information */
	resp.comp_mask = 0;
	resp.md_handle = mr->cxi_md ? (u32)(uintptr_t)mr->cxi_md : 0;
	resp.iova = ibmr->iova;
	resp.length = ibmr->length;
	resp.access_flags = 0; /* Convert from IB access flags if needed */

	/* Copy response to user space */
	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD2_RESP_MD_HANDLE,
			     &resp.md_handle, sizeof(resp.md_handle));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD2_RESP_IOVA,
			     &resp.iova, sizeof(resp.iova));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD2_RESP_LENGTH,
			     &resp.length, sizeof(resp.length));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD2_RESP_ACCESS_FLAGS,
			     &resp.access_flags, sizeof(resp.access_flags));
	if (ret)
		return ret;

	ibdev_dbg(ibmr->device, "CXI Method 2: handle=0x%x, IOVA=0x%llx, len=%llu\n",
		  resp.md_handle, resp.iova, resp.length);

	return 0;
}

/* Vendor-specific ioctl handler 3: CXI Method 3 - Queue pair information query */
static int UVERBS_HANDLER(CXI_IB_METHOD_3)(struct uverbs_attr_bundle *attrs)
{
	struct ib_qp *ibqp = uverbs_attr_get_obj(attrs, CXI_IB_ATTR_METHOD3_QP_HANDLE);
	struct cxi_ib_qp *qp = container_of(ibqp, struct cxi_ib_qp, ibqp);
	struct cxi_ib_method3_resp resp = {};
	int ret;

	if (!ibqp) {
		return -EINVAL;
	}

	/* Fill queue pair specific information */
	resp.comp_mask = 0;
	resp.txq_handle = qp->cxi_txq ? (u32)(uintptr_t)qp->cxi_txq : 0;
	resp.tgq_handle = qp->cxi_tgq ? (u32)(uintptr_t)qp->cxi_tgq : 0;
	resp.cmdq_handle = qp->qp_handle;
	resp.eq_handle = 0; /* Event queue handle if available */
	resp.state = qp->state;

	/* Copy response to user space */
	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD3_RESP_TXQ_HANDLE,
			     &resp.txq_handle, sizeof(resp.txq_handle));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD3_RESP_TGQ_HANDLE,
			     &resp.tgq_handle, sizeof(resp.tgq_handle));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD3_RESP_CMDQ_HANDLE,
			     &resp.cmdq_handle, sizeof(resp.cmdq_handle));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD3_RESP_EQ_HANDLE,
			     &resp.eq_handle, sizeof(resp.eq_handle));
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, CXI_IB_ATTR_METHOD3_RESP_STATE,
			     &resp.state, sizeof(resp.state));
	if (ret)
		return ret;

	ibdev_dbg(ibqp->device, "CXI Method 3: TXQ=0x%x, TGQ=0x%x, state=%u\n",
		  resp.txq_handle, resp.tgq_handle, resp.state);

	return 0;
}

/* Declare vendor-specific method 1: CXI generic method 1 */
DECLARE_UVERBS_NAMED_METHOD(CXI_IB_METHOD_1,
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD1_RESP_NIC_ADDR,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD1_RESP_PID_GRANULE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD1_RESP_PID_COUNT,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD1_RESP_PID_BITS,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD1_RESP_MIN_FREE_SHIFT,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY));

/* Declare vendor-specific method 2: CXI generic method 2 */
DECLARE_UVERBS_NAMED_METHOD(CXI_IB_METHOD_2,
			    UVERBS_ATTR_IDR(CXI_IB_ATTR_METHOD2_MR_HANDLE,
					    UVERBS_OBJECT_MR,
					    UVERBS_ACCESS_READ,
					    UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD2_RESP_MD_HANDLE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD2_RESP_IOVA,
						UVERBS_ATTR_TYPE(u64),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD2_RESP_LENGTH,
						UVERBS_ATTR_TYPE(u64),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD2_RESP_ACCESS_FLAGS,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY));

/* Declare vendor-specific method 3: CXI generic method 3 */
DECLARE_UVERBS_NAMED_METHOD(CXI_IB_METHOD_3,
			    UVERBS_ATTR_IDR(CXI_IB_ATTR_METHOD3_QP_HANDLE,
					    UVERBS_OBJECT_QP,
					    UVERBS_ACCESS_READ,
					    UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD3_RESP_TXQ_HANDLE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD3_RESP_TGQ_HANDLE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD3_RESP_CMDQ_HANDLE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD3_RESP_EQ_HANDLE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY),
			    UVERBS_ATTR_PTR_OUT(CXI_IB_ATTR_METHOD3_RESP_STATE,
						UVERBS_ATTR_TYPE(u32),
						UA_MANDATORY));

/* Define CXI generic object with all three methods */
DECLARE_UVERBS_NAMED_OBJECT(CXI_IB_OBJECT_GENERIC,
			     UVERBS_TYPE_ALLOC_IDR(0, 1),
			     &UVERBS_METHOD(CXI_IB_METHOD_1),
			     &UVERBS_METHOD(CXI_IB_METHOD_2),
			     &UVERBS_METHOD(CXI_IB_METHOD_3));

/* CXI vendor-specific UAPI definitions */
const struct uapi_definition cxi_ib_uapi_defs[] = {
	UAPI_DEF_CHAIN_OBJ_TREE_NAMED(CXI_IB_OBJECT_GENERIC,
				      &UVERBS_OBJECT(CXI_IB_OBJECT_GENERIC)),
	{},
};
