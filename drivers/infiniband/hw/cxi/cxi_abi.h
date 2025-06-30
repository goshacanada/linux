/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef CXI_IB_ABI_H
#define CXI_IB_ABI_H

#include <linux/types.h>
#include <rdma/ib_user_ioctl_cmds.h>

/*
 * Increment this value if any changes that break userspace ABI
 * compatibility are made.
 */
#define CXI_IB_UVERBS_ABI_VERSION 1

/*
 * Keep structs aligned to 8 bytes.
 * Keep reserved fields as arrays of __u8 named reserved_XXX where XXX is the
 * hex bit offset of the field.
 */

/* CXI vendor-specific device query attributes */
enum cxi_ib_query_device_attrs {
	CXI_IB_ATTR_QUERY_DEVICE_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	CXI_IB_ATTR_QUERY_DEVICE_RESP_NIC_ADDR,
	CXI_IB_ATTR_QUERY_DEVICE_RESP_PID_GRANULE,
	CXI_IB_ATTR_QUERY_DEVICE_RESP_PID_COUNT,
	CXI_IB_ATTR_QUERY_DEVICE_RESP_PID_BITS,
	CXI_IB_ATTR_QUERY_DEVICE_RESP_MIN_FREE_SHIFT,
};

/* CXI vendor-specific memory region query attributes */
enum cxi_ib_query_mr_attrs {
	CXI_IB_ATTR_QUERY_MR_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	CXI_IB_ATTR_QUERY_MR_RESP_MD_HANDLE,
	CXI_IB_ATTR_QUERY_MR_RESP_IOVA,
	CXI_IB_ATTR_QUERY_MR_RESP_LENGTH,
	CXI_IB_ATTR_QUERY_MR_RESP_ACCESS_FLAGS,
};

/* CXI vendor-specific queue pair query attributes */
enum cxi_ib_query_qp_attrs {
	CXI_IB_ATTR_QUERY_QP_HANDLE = (1U << UVERBS_ID_NS_SHIFT),
	CXI_IB_ATTR_QUERY_QP_RESP_TXQ_HANDLE,
	CXI_IB_ATTR_QUERY_QP_RESP_TGQ_HANDLE,
	CXI_IB_ATTR_QUERY_QP_RESP_CMDQ_HANDLE,
	CXI_IB_ATTR_QUERY_QP_RESP_EQ_HANDLE,
	CXI_IB_ATTR_QUERY_QP_RESP_STATE,
};

/* CXI vendor-specific device methods */
enum cxi_ib_device_methods {
	CXI_IB_METHOD_QUERY_DEVICE = (1U << UVERBS_ID_NS_SHIFT),
};

/* CXI vendor-specific memory region methods */
enum cxi_ib_mr_methods {
	CXI_IB_METHOD_MR_QUERY = (1U << UVERBS_ID_NS_SHIFT),
};

/* CXI vendor-specific queue pair methods */
enum cxi_ib_qp_methods {
	CXI_IB_METHOD_QP_QUERY = (1U << UVERBS_ID_NS_SHIFT),
};

/* CXI device query response structure */
struct cxi_ib_query_device_resp {
	__u32 comp_mask;
	__u32 nic_addr;
	__u32 pid_granule;
	__u32 pid_count;
	__u32 pid_bits;
	__u32 min_free_shift;
	__u8 reserved_18[4];
};

/* CXI memory region query response structure */
struct cxi_ib_query_mr_resp {
	__u32 comp_mask;
	__u32 md_handle;
	__aligned_u64 iova;
	__aligned_u64 length;
	__u32 access_flags;
	__u8 reserved_1c[4];
};

/* CXI queue pair query response structure */
struct cxi_ib_query_qp_resp {
	__u32 comp_mask;
	__u32 txq_handle;
	__u32 tgq_handle;
	__u32 cmdq_handle;
	__u32 eq_handle;
	__u32 state;
	__u8 reserved_18[4];
};

/* CXI device capabilities flags */
enum {
	CXI_IB_DEVICE_CAP_ATOMIC_OPS = 1 << 0,
	CXI_IB_DEVICE_CAP_RDMA_READ = 1 << 1,
	CXI_IB_DEVICE_CAP_RDMA_WRITE = 1 << 2,
	CXI_IB_DEVICE_CAP_MULTICAST = 1 << 3,
	CXI_IB_DEVICE_CAP_TRIGGERED_OPS = 1 << 4,
	CXI_IB_DEVICE_CAP_RESTRICTED_MEMBERS = 1 << 5,
};

/* CXI memory region access flags */
enum {
	CXI_IB_MR_ACCESS_LOCAL_READ = 1 << 0,
	CXI_IB_MR_ACCESS_LOCAL_WRITE = 1 << 1,
	CXI_IB_MR_ACCESS_REMOTE_READ = 1 << 2,
	CXI_IB_MR_ACCESS_REMOTE_WRITE = 1 << 3,
	CXI_IB_MR_ACCESS_REMOTE_ATOMIC = 1 << 4,
};

/* CXI queue pair states */
enum {
	CXI_IB_QP_STATE_RESET = 0,
	CXI_IB_QP_STATE_INIT = 1,
	CXI_IB_QP_STATE_RTR = 2,
	CXI_IB_QP_STATE_RTS = 3,
	CXI_IB_QP_STATE_SQD = 4,
	CXI_IB_QP_STATE_SQE = 5,
	CXI_IB_QP_STATE_ERR = 6,
};

#endif /* CXI_IB_ABI_H */
