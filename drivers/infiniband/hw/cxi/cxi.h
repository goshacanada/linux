/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_IB_H_
#define _CXI_IB_H_

#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/xarray.h>

#include <rdma/ib_verbs.h>
#include <rdma/ib_user_verbs.h>

#include <linux/cxi/cxi.h>
#include "cxi_com_cmd.h"

#define DRV_MODULE_NAME         "cxi_ib"
#define DEVICE_NAME             "CXI (Cassini) InfiniBand Adapter"

#define CXI_IB_IRQNAME_SIZE     40

/* Don't use anything other than atomic64 */
struct cxi_ib_stats {
	atomic64_t alloc_pd_err;
	atomic64_t create_qp_err;
	atomic64_t create_cq_err;
	atomic64_t reg_mr_err;
	atomic64_t alloc_ucontext_err;
	atomic64_t create_ah_err;
	atomic64_t mmap_err;
	atomic64_t cxi_events_rcvd;
};

struct cxi_ib_dev {
	struct ib_device ibdev;
	struct cxi_dev *cxi_dev;

	/* Device attributes from CXI driver */
	struct cxi_dev_info dev_info;

	/* Statistics */
	struct cxi_ib_stats stats;

	/* Array of completion event queues */
	struct cxi_ib_eq *eqs;
	u32 neqs;

	/* Only stores CQs with interrupts enabled */
	struct xarray cqs_xa;

	/* Client registration with CXI core */
	struct cxi_client client;
};

struct cxi_ib_ucontext {
	struct ib_ucontext ibucontext;
	u16 uarn;
};

struct cxi_ib_pd {
	struct ib_pd ibpd;
	u16 pdn;
};

struct cxi_ib_mr {
	struct ib_mr ibmr;
	struct ib_umem *umem;
	struct cxi_md *cxi_md;
};

struct cxi_ib_cq {
	struct ib_cq ibcq;
	struct cxi_ib_ucontext *ucontext;
	struct cxi_eq *cxi_eq;
	struct rdma_user_mmap_entry *mmap_entry;
	struct rdma_user_mmap_entry *db_mmap_entry;
	size_t size;
	u16 cq_idx;
	/* NULL when no interrupts requested */
	struct cxi_ib_eq *eq;
};

struct cxi_ib_qp {
	struct ib_qp ibqp;
	struct cxi_cq *cxi_txq;
	struct cxi_cq *cxi_tgq;
	enum ib_qp_state state;

	/* Used for saving mmap_xa entries */
	struct rdma_user_mmap_entry *sq_db_mmap_entry;
	struct rdma_user_mmap_entry *rq_db_mmap_entry;
	struct rdma_user_mmap_entry *sq_mmap_entry;
	struct rdma_user_mmap_entry *rq_mmap_entry;

	u32 qp_handle;
	u32 max_send_wr;
	u32 max_recv_wr;
	u32 max_send_sge;
	u32 max_recv_sge;
	u32 max_inline_data;
};

struct cxi_ib_ah {
	struct ib_ah ibah;
	u16 ah;
	/* dest_addr */
	u8 id[16]; /* CXI GID size */
};

struct cxi_ib_eq {
	struct cxi_eq *cxi_eq;
	char irq_name[CXI_IB_IRQNAME_SIZE];
};

/* Function prototypes */
int cxi_ib_query_device(struct ib_device *ibdev,
			struct ib_device_attr *props,
			struct ib_udata *udata);
int cxi_ib_query_port(struct ib_device *ibdev, u32 port,
		      struct ib_port_attr *props);
int cxi_ib_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
		    int qp_attr_mask,
		    struct ib_qp_init_attr *qp_init_attr);
int cxi_ib_query_gid(struct ib_device *ibdev, u32 port, int index,
		     union ib_gid *gid);
int cxi_ib_query_pkey(struct ib_device *ibdev, u32 port, u16 index,
		      u16 *pkey);
int cxi_ib_alloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
int cxi_ib_dealloc_pd(struct ib_pd *ibpd, struct ib_udata *udata);
int cxi_ib_destroy_qp(struct ib_qp *ibqp, struct ib_udata *udata);
int cxi_ib_create_qp(struct ib_qp *ibqp, struct ib_qp_init_attr *init_attr,
		     struct ib_udata *udata);
int cxi_ib_destroy_cq(struct ib_cq *ibcq, struct ib_udata *udata);
int cxi_ib_create_cq(struct ib_cq *ibcq, const struct ib_cq_init_attr *attr,
		     struct uverbs_attr_bundle *attrs);
struct ib_mr *cxi_ib_reg_mr(struct ib_pd *ibpd, u64 start, u64 length,
			    u64 virt_addr, int access_flags,
			    struct ib_udata *udata);
struct ib_mr *cxi_ib_reg_user_mr_dmabuf(struct ib_pd *ibpd, u64 start,
					 u64 length, u64 virt_addr,
					 int fd, int access_flags,
					 struct uverbs_attr_bundle *attrs);
int cxi_ib_dereg_mr(struct ib_mr *ibmr, struct ib_udata *udata);
int cxi_ib_get_port_immutable(struct ib_device *ibdev, u32 port_num,
			      struct ib_port_immutable *immutable);
int cxi_ib_alloc_ucontext(struct ib_ucontext *ibucontext, struct ib_udata *udata);
void cxi_ib_dealloc_ucontext(struct ib_ucontext *ibucontext);
int cxi_ib_mmap(struct ib_ucontext *ibucontext,
		struct vm_area_struct *vma);
void cxi_ib_mmap_free(struct rdma_user_mmap_entry *rdma_entry);
int cxi_ib_create_ah(struct ib_ah *ibah,
		     struct rdma_ah_init_attr *init_attr,
		     struct ib_udata *udata);
int cxi_ib_destroy_ah(struct ib_ah *ibah, u32 flags);
int cxi_ib_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *qp_attr,
		     int qp_attr_mask, struct ib_udata *udata);
enum rdma_link_layer cxi_ib_port_link_layer(struct ib_device *ibdev,
					     u32 port_num);
struct rdma_hw_stats *cxi_ib_alloc_hw_port_stats(struct ib_device *ibdev, u32 port_num);
struct rdma_hw_stats *cxi_ib_alloc_hw_device_stats(struct ib_device *ibdev);
int cxi_ib_get_hw_stats(struct ib_device *ibdev, struct rdma_hw_stats *stats,
			u32 port_num, int index);

#endif /* _CXI_IB_H_ */
