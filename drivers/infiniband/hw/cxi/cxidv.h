/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef CXIDV_H
#define CXIDV_H

#include <stdint.h>
#include <infiniband/verbs.h>
#include <rdma/cxi-abi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CXI Direct Verbs - Vendor-specific extensions */

/* CXI Method 1 attributes structure - Device information */
typedef struct cxi_ibv_method1_resp cxidv_method1_attr;

/* CXI Method 2 attributes structure - Memory region information */
typedef struct cxi_ibv_method2_resp cxidv_method2_attr;

/* CXI Method 3 attributes structure - Queue pair information */
typedef struct cxi_ibv_method3_resp cxidv_method3_attr;

/* CXI device capabilities flags - use UAPI definitions */
#define CXIDV_DEVICE_CAP_ATOMIC_OPS         CXI_DEVICE_CAP_ATOMIC_OPS
#define CXIDV_DEVICE_CAP_RDMA_READ          CXI_DEVICE_CAP_RDMA_READ
#define CXIDV_DEVICE_CAP_RDMA_WRITE         CXI_DEVICE_CAP_RDMA_WRITE
#define CXIDV_DEVICE_CAP_MULTICAST          CXI_DEVICE_CAP_MULTICAST
#define CXIDV_DEVICE_CAP_TRIGGERED_OPS      CXI_DEVICE_CAP_TRIGGERED_OPS
#define CXIDV_DEVICE_CAP_RESTRICTED_MEMBERS CXI_DEVICE_CAP_RESTRICTED_MEMBERS

/* CXI memory region access flags - use UAPI definitions */
#define CXIDV_MR_ACCESS_LOCAL_READ    CXI_MR_ACCESS_LOCAL_READ
#define CXIDV_MR_ACCESS_LOCAL_WRITE   CXI_MR_ACCESS_LOCAL_WRITE
#define CXIDV_MR_ACCESS_REMOTE_READ   CXI_MR_ACCESS_REMOTE_READ
#define CXIDV_MR_ACCESS_REMOTE_WRITE  CXI_MR_ACCESS_REMOTE_WRITE
#define CXIDV_MR_ACCESS_REMOTE_ATOMIC CXI_MR_ACCESS_REMOTE_ATOMIC

/* CXI queue pair states - use UAPI definitions */
#define CXIDV_QP_STATE_RESET CXI_QP_STATE_RESET
#define CXIDV_QP_STATE_INIT  CXI_QP_STATE_INIT
#define CXIDV_QP_STATE_RTR   CXI_QP_STATE_RTR
#define CXIDV_QP_STATE_RTS   CXI_QP_STATE_RTS
#define CXIDV_QP_STATE_SQD   CXI_QP_STATE_SQD
#define CXIDV_QP_STATE_SQE   CXI_QP_STATE_SQE
#define CXIDV_QP_STATE_ERR   CXI_QP_STATE_ERR

/**
 * cxidv_method1 - CXI Method 1 - Query device information
 * @context: InfiniBand context
 * @attr: Method 1 attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_method1(struct ibv_context *context,
		  struct cxidv_method1_attr *attr,
		  uint32_t inlen);

/**
 * cxidv_method2 - CXI Method 2 - Query memory region information
 * @mr: Memory region to query
 * @attr: Method 2 attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_method2(struct ibv_mr *mr,
		  struct cxidv_method2_attr *attr,
		  uint32_t inlen);

/**
 * cxidv_method3 - CXI Method 3 - Query queue pair information
 * @qp: Queue pair to query
 * @attr: Method 3 attributes structure to fill
 * @inlen: Size of the attributes structure
 *
 * Returns 0 on success, errno on failure
 */
int cxidv_method3(struct ibv_qp *qp,
		  struct cxidv_method3_attr *attr,
		  uint32_t inlen);

/**
 * cxidv_is_supported - Check if the device supports CXI direct verbs
 * @context: InfiniBand context
 *
 * Returns 1 if supported, 0 if not supported
 */
static inline int cxidv_is_supported(struct ibv_context *context)
{
	/* Check if this is a CXI device by looking at the device name */
	return (strncmp(context->device->name, "cxi_", 4) == 0);
}

/**
 * cxidv_get_version - Get the CXI direct verbs library version
 *
 * Returns version string
 */
static inline const char *cxidv_get_version(void)
{
	return "1.0.0";
}

/* Helper macros for checking field availability */
#define CXIDV_FIELD_AVAIL(type, field, inlen) \
	(offsetof(type, field) + sizeof(((type *)0)->field) <= (inlen))

#define CXIDV_METHOD1_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxi_ibv_method1_resp, field, inlen)

#define CXIDV_METHOD2_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxi_ibv_method2_resp, field, inlen)

#define CXIDV_METHOD3_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxi_ibv_method3_resp, field, inlen)

#ifdef __cplusplus
}
#endif

#endif /* CXIDV_H */
