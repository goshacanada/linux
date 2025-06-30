/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef CXI_ABI_USER_H
#define CXI_ABI_USER_H

#include <linux/types.h>
#include <rdma/ib_user_ioctl_cmds.h>

/*
 * Increment this value if any changes that break userspace ABI
 * compatibility are made.
 */
#define CXI_UVERBS_ABI_VERSION 1

/*
 * Keep structs aligned to 8 bytes.
 * Keep reserved fields as arrays of __u8 named reserved_XXX where XXX is the
 * hex bit offset of the field.
 */

/* CXI user context allocation command */
struct cxi_ibv_alloc_ucontext_cmd {
	__u32 comp_mask;
	__u8 reserved_20[4];
};

/* CXI user context allocation response */
struct cxi_ibv_alloc_ucontext_resp {
	__u32 comp_mask;
	__u16 uarn;
	__u8 reserved_22[6];
};

/* CXI protection domain allocation response */
struct cxi_ibv_alloc_pd_resp {
	__u32 comp_mask;
	__u16 pdn;
	__u8 reserved_22[6];
};

/* CXI completion queue creation command */
struct cxi_ibv_create_cq_cmd {
	__u32 comp_mask;
	__u32 cq_depth;
	__u16 eqn;
	__u8 reserved_26[6];
};

/* CXI completion queue creation response */
struct cxi_ibv_create_cq_resp {
	__u32 comp_mask;
	__u16 cq_idx;
	__u16 actual_depth;
	__u32 db_off;
	__u8 reserved_30[4];
};

/* CXI queue pair creation command */
struct cxi_ibv_create_qp_cmd {
	__u32 comp_mask;
	__u32 sq_depth;
	__u32 rq_depth;
	__u16 send_cq_idx;
	__u16 recv_cq_idx;
	__u8 reserved_34[4];
};

/* CXI queue pair creation response */
struct cxi_ibv_create_qp_resp {
	__u32 comp_mask;
	__u32 qp_handle;
	__u32 qp_num;
	__u32 sq_db_offset;
	__u32 rq_db_offset;
	__u8 reserved_38[4];
};

/* CXI memory region registration command */
struct cxi_ibv_reg_mr_cmd {
	__u32 comp_mask;
	__aligned_u64 start;
	__aligned_u64 length;
	__aligned_u64 virt_addr;
	__u32 access_flags;
	__u8 reserved_44[4];
};

/* CXI memory region registration response */
struct cxi_ibv_reg_mr_resp {
	__u32 comp_mask;
	__u32 l_key;
	__u32 r_key;
	__u8 reserved_30[4];
};

/* CXI Method 1 response structure - Device information */
struct cxi_ibv_method1_resp {
	__u32 comp_mask;
	__u32 nic_addr;
	__u32 pid_granule;
	__u32 pid_count;
	__u32 pid_bits;
	__u32 min_free_shift;
	__u8 reserved_48[4];
};

/* CXI Method 2 response structure - Memory region information */
struct cxi_ibv_method2_resp {
	__u32 comp_mask;
	__u32 md_handle;
	__aligned_u64 iova;
	__aligned_u64 length;
	__u32 access_flags;
	__u8 reserved_54[4];
};

/* CXI Method 3 response structure - Queue pair information */
struct cxi_ibv_method3_resp {
	__u32 comp_mask;
	__u32 txq_handle;
	__u32 tgq_handle;
	__u32 cmdq_handle;
	__u32 eq_handle;
	__u32 state;
	__u8 reserved_50[4];
};

/* CXI device capabilities flags */
enum {
	CXI_DEVICE_CAP_ATOMIC_OPS = 1 << 0,
	CXI_DEVICE_CAP_RDMA_READ = 1 << 1,
	CXI_DEVICE_CAP_RDMA_WRITE = 1 << 2,
	CXI_DEVICE_CAP_MULTICAST = 1 << 3,
	CXI_DEVICE_CAP_TRIGGERED_OPS = 1 << 4,
	CXI_DEVICE_CAP_RESTRICTED_MEMBERS = 1 << 5,
};

/* CXI memory region access flags */
enum {
	CXI_MR_ACCESS_LOCAL_READ = 1 << 0,
	CXI_MR_ACCESS_LOCAL_WRITE = 1 << 1,
	CXI_MR_ACCESS_REMOTE_READ = 1 << 2,
	CXI_MR_ACCESS_REMOTE_WRITE = 1 << 3,
	CXI_MR_ACCESS_REMOTE_ATOMIC = 1 << 4,
};

/* CXI queue pair states */
enum {
	CXI_QP_STATE_RESET = 0,
	CXI_QP_STATE_INIT = 1,
	CXI_QP_STATE_RTR = 2,
	CXI_QP_STATE_RTS = 3,
	CXI_QP_STATE_SQD = 4,
	CXI_QP_STATE_SQE = 5,
	CXI_QP_STATE_ERR = 6,
};

/* CXI vendor-specific generic object attributes */
enum cxi_generic_attrs {
	/* Method 1 attributes - Device query */
	CXI_IB_ATTR_METHOD1_RESP_NIC_ADDR = (1U << UVERBS_ID_NS_SHIFT),
	CXI_IB_ATTR_METHOD1_RESP_PID_GRANULE,
	CXI_IB_ATTR_METHOD1_RESP_PID_COUNT,
	CXI_IB_ATTR_METHOD1_RESP_PID_BITS,
	CXI_IB_ATTR_METHOD1_RESP_MIN_FREE_SHIFT,

	/* Method 2 attributes - Memory region query */
	CXI_IB_ATTR_METHOD2_MR_HANDLE,
	CXI_IB_ATTR_METHOD2_RESP_MD_HANDLE,
	CXI_IB_ATTR_METHOD2_RESP_IOVA,
	CXI_IB_ATTR_METHOD2_RESP_LENGTH,
	CXI_IB_ATTR_METHOD2_RESP_ACCESS_FLAGS,

	/* Method 3 attributes - Queue pair query */
	CXI_IB_ATTR_METHOD3_QP_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_TXQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_TGQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_CMDQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_EQ_HANDLE,
	CXI_IB_ATTR_METHOD3_RESP_STATE,
};

/* CXI vendor-specific generic object methods */
enum cxi_generic_methods {
	CXI_IB_METHOD_1 = (1U << UVERBS_ID_NS_SHIFT),
	CXI_IB_METHOD_2,
	CXI_IB_METHOD_3,
};

/* CXI vendor-specific generic object */
enum cxi_objects {
	CXI_IB_OBJECT_GENERIC = (1U << UVERBS_ID_NS_SHIFT),
};

#endif /* CXI_ABI_USER_H */
