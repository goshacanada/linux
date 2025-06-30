// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include "cxi_com_cmd.h"
#include "cxi.h"

int cxi_ib_com_create_qp(struct cxi_dev *cxi_dev,
			 struct cxi_ib_create_qp_params *params,
			 struct cxi_ib_create_qp_result *result)
{
	/* For now, return a simple implementation.
	 * In a real implementation, this would interface with the CXI
	 * hardware to create queue pairs.
	 */
	if (!cxi_dev || !params || !result)
		return -EINVAL;

	/* Generate a simple QP handle and number */
	result->qp_handle = (u32)(uintptr_t)params; /* Simple handle generation */
	result->qp_num = result->qp_handle & 0xFFFF;
	result->sq_db_offset = 0x1000; /* Example doorbell offset */
	result->rq_db_offset = 0x2000;
	result->send_sub_cq_idx = params->send_cq_idx;
	result->recv_sub_cq_idx = params->recv_cq_idx;

	return 0;
}

int cxi_ib_com_modify_qp(struct cxi_dev *cxi_dev,
			 struct cxi_ib_modify_qp_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation - in reality this would modify
	 * the QP state in hardware
	 */
	return 0;
}

int cxi_ib_com_query_qp(struct cxi_dev *cxi_dev,
			struct cxi_ib_query_qp_params *params,
			struct cxi_ib_query_qp_result *result)
{
	if (!cxi_dev || !params || !result)
		return -EINVAL;

	/* Simple implementation */
	result->qp_state = IB_QPS_RTS; /* Ready to Send */
	result->qkey = 0;
	result->sq_psn = 0;
	result->rnr_retry = 7;

	return 0;
}

int cxi_ib_com_destroy_qp(struct cxi_dev *cxi_dev,
			  struct cxi_ib_destroy_qp_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation */
	return 0;
}

int cxi_ib_com_create_cq(struct cxi_dev *cxi_dev,
			 struct cxi_ib_create_cq_params *params,
			 struct cxi_ib_create_cq_result *result)
{
	if (!cxi_dev || !params || !result)
		return -EINVAL;

	/* Generate a simple CQ index */
	result->cq_idx = (u16)(uintptr_t)params & 0xFFFF;
	result->actual_depth = params->cq_depth;
	result->db_off = 0x3000; /* Example doorbell offset */
	result->db_valid = true;

	return 0;
}

int cxi_ib_com_destroy_cq(struct cxi_dev *cxi_dev,
			  struct cxi_ib_destroy_cq_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation */
	return 0;
}

int cxi_ib_com_create_ah(struct cxi_dev *cxi_dev,
			 struct cxi_ib_create_ah_params *params,
			 struct cxi_ib_create_ah_result *result)
{
	if (!cxi_dev || !params || !result)
		return -EINVAL;

	/* Generate a simple AH handle */
	result->ah = (u16)(uintptr_t)params & 0xFFFF;

	return 0;
}

int cxi_ib_com_destroy_ah(struct cxi_dev *cxi_dev,
			  struct cxi_ib_destroy_ah_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation */
	return 0;
}

int cxi_ib_com_reg_mr(struct cxi_dev *cxi_dev,
		      struct cxi_ib_reg_mr_params *params,
		      struct cxi_ib_reg_mr_result *result)
{
	if (!cxi_dev || !params || !result)
		return -EINVAL;

	/* Generate simple keys */
	result->l_key = (u32)(params->start >> 8) & 0xFFFFFF;
	result->r_key = result->l_key | 0x01000000;

	return 0;
}

int cxi_ib_com_dereg_mr(struct cxi_dev *cxi_dev,
			struct cxi_ib_dereg_mr_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation */
	return 0;
}

int cxi_ib_com_alloc_pd(struct cxi_dev *cxi_dev,
			struct cxi_ib_alloc_pd_result *result)
{
	static atomic_t pd_counter = ATOMIC_INIT(1);

	if (!cxi_dev || !result)
		return -EINVAL;

	/* Generate a simple PD number */
	result->pdn = (u16)atomic_inc_return(&pd_counter);

	return 0;
}

int cxi_ib_com_dealloc_pd(struct cxi_dev *cxi_dev,
			  struct cxi_ib_dealloc_pd_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation */
	return 0;
}

int cxi_ib_com_alloc_ucontext(struct cxi_dev *cxi_dev,
			      struct cxi_ib_alloc_ucontext_result *result)
{
	static atomic_t uarn_counter = ATOMIC_INIT(1);

	if (!cxi_dev || !result)
		return -EINVAL;

	/* Generate a simple UARN */
	result->uarn = (u16)atomic_inc_return(&uarn_counter);

	return 0;
}

int cxi_ib_com_dealloc_ucontext(struct cxi_dev *cxi_dev,
				struct cxi_ib_dealloc_ucontext_params *params)
{
	if (!cxi_dev || !params)
		return -EINVAL;

	/* Simple implementation */
	return 0;
}
