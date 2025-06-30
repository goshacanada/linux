/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_IB_COM_CMD_H_
#define _CXI_IB_COM_CMD_H_

#include "cxi_com.h"

#define CXI_IB_GID_SIZE 16

struct cxi_ib_create_qp_params {
	u32 send_cq_idx;
	u32 recv_cq_idx;
	u32 sq_depth;
	u32 rq_depth;
	u16 pd;
	u16 uarn;
	u8 qp_type;
	u8 sl;
};

struct cxi_ib_create_qp_result {
	u32 qp_handle;
	u32 qp_num;
	u32 sq_db_offset;
	u32 rq_db_offset;
	u16 send_sub_cq_idx;
	u16 recv_sub_cq_idx;
};

struct cxi_ib_modify_qp_params {
	u32 modify_mask;
	u32 qp_handle;
	u32 qp_state;
	u32 cur_qp_state;
	u32 qkey;
	u32 sq_psn;
	u8 rnr_retry;
};

struct cxi_ib_query_qp_params {
	u32 qp_handle;
};

struct cxi_ib_query_qp_result {
	u32 qp_state;
	u32 qkey;
	u32 sq_psn;
	u8 rnr_retry;
};

struct cxi_ib_destroy_qp_params {
	u32 qp_handle;
};

struct cxi_ib_create_cq_params {
	/* cq physical base address in OS memory */
	dma_addr_t dma_addr;
	/* completion queue depth in # of entries */
	u16 cq_depth;
	u16 uarn;
	u16 eqn;
	u8 entry_size_in_bytes;
	u8 interrupt_mode_enabled : 1;
};

struct cxi_ib_create_cq_result {
	/* cq identifier */
	u16 cq_idx;
	/* actual cq depth in # of entries */
	u16 actual_depth;
	u32 db_off;
	bool db_valid;
};

struct cxi_ib_destroy_cq_params {
	u16 cq_idx;
};

struct cxi_ib_create_ah_params {
	u16 pdn;
	/* Destination address in network byte order */
	u8 dest_addr[CXI_IB_GID_SIZE];
};

struct cxi_ib_create_ah_result {
	u16 ah;
};

struct cxi_ib_destroy_ah_params {
	u16 ah;
};

struct cxi_ib_reg_mr_params {
	u64 start;
	u64 length;
	u64 virt_addr;
	u32 access_flags;
	u16 pdn;
};

struct cxi_ib_reg_mr_result {
	u32 l_key;
	u32 r_key;
};

struct cxi_ib_dereg_mr_params {
	u32 l_key;
};

struct cxi_ib_alloc_pd_result {
	u16 pdn;
};

struct cxi_ib_dealloc_pd_params {
	u16 pdn;
};

struct cxi_ib_alloc_ucontext_result {
	u16 uarn;
};

struct cxi_ib_dealloc_ucontext_params {
	u16 uarn;
};

/* Function prototypes for command operations */
int cxi_ib_com_create_qp(struct cxi_dev *cxi_dev,
			 struct cxi_ib_create_qp_params *params,
			 struct cxi_ib_create_qp_result *result);
int cxi_ib_com_modify_qp(struct cxi_dev *cxi_dev,
			 struct cxi_ib_modify_qp_params *params);
int cxi_ib_com_query_qp(struct cxi_dev *cxi_dev,
			struct cxi_ib_query_qp_params *params,
			struct cxi_ib_query_qp_result *result);
int cxi_ib_com_destroy_qp(struct cxi_dev *cxi_dev,
			  struct cxi_ib_destroy_qp_params *params);
int cxi_ib_com_create_cq(struct cxi_dev *cxi_dev,
			 struct cxi_ib_create_cq_params *params,
			 struct cxi_ib_create_cq_result *result);
int cxi_ib_com_destroy_cq(struct cxi_dev *cxi_dev,
			  struct cxi_ib_destroy_cq_params *params);
int cxi_ib_com_create_ah(struct cxi_dev *cxi_dev,
			 struct cxi_ib_create_ah_params *params,
			 struct cxi_ib_create_ah_result *result);
int cxi_ib_com_destroy_ah(struct cxi_dev *cxi_dev,
			  struct cxi_ib_destroy_ah_params *params);
int cxi_ib_com_reg_mr(struct cxi_dev *cxi_dev,
		      struct cxi_ib_reg_mr_params *params,
		      struct cxi_ib_reg_mr_result *result);
int cxi_ib_com_dereg_mr(struct cxi_dev *cxi_dev,
			struct cxi_ib_dereg_mr_params *params);
int cxi_ib_com_alloc_pd(struct cxi_dev *cxi_dev,
			struct cxi_ib_alloc_pd_result *result);
int cxi_ib_com_dealloc_pd(struct cxi_dev *cxi_dev,
			  struct cxi_ib_dealloc_pd_params *params);
int cxi_ib_com_alloc_ucontext(struct cxi_dev *cxi_dev,
			      struct cxi_ib_alloc_ucontext_result *result);
int cxi_ib_com_dealloc_ucontext(struct cxi_dev *cxi_dev,
				struct cxi_ib_dealloc_ucontext_params *params);

#endif /* _CXI_IB_COM_CMD_H_ */
