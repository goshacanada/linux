/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef CXIDV_H
#define CXIDV_H

#include <stdint.h>
#include <infiniband/verbs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CXI Direct Verbs - Vendor-specific extensions */

/* CXI Method 1 attributes structure - Device information */
struct cxidv_method1_attr {
	uint64_t comp_mask;
	uint32_t nic_addr;
	uint32_t pid_granule;
	uint32_t pid_count;
	uint32_t pid_bits;
	uint32_t min_free_shift;
	uint8_t reserved[4];
};

/* CXI Method 2 attributes structure - Memory region information */
struct cxidv_method2_attr {
	uint64_t comp_mask;
	uint32_t md_handle;
	uint64_t iova;
	uint64_t length;
	uint32_t access_flags;
	uint8_t reserved[4];
};

/* CXI Method 3 attributes structure - Queue pair information */
struct cxidv_method3_attr {
	uint64_t comp_mask;
	uint32_t txq_handle;
	uint32_t tgq_handle;
	uint32_t cmdq_handle;
	uint32_t eq_handle;
	uint32_t state;
	uint8_t reserved[4];
};

/* CXI device capabilities flags */
enum {
	CXIDV_DEVICE_CAP_ATOMIC_OPS = 1 << 0,
	CXIDV_DEVICE_CAP_RDMA_READ = 1 << 1,
	CXIDV_DEVICE_CAP_RDMA_WRITE = 1 << 2,
	CXIDV_DEVICE_CAP_MULTICAST = 1 << 3,
	CXIDV_DEVICE_CAP_TRIGGERED_OPS = 1 << 4,
	CXIDV_DEVICE_CAP_RESTRICTED_MEMBERS = 1 << 5,
};

/* CXI memory region access flags */
enum {
	CXIDV_MR_ACCESS_LOCAL_READ = 1 << 0,
	CXIDV_MR_ACCESS_LOCAL_WRITE = 1 << 1,
	CXIDV_MR_ACCESS_REMOTE_READ = 1 << 2,
	CXIDV_MR_ACCESS_REMOTE_WRITE = 1 << 3,
	CXIDV_MR_ACCESS_REMOTE_ATOMIC = 1 << 4,
};

/* CXI queue pair states */
enum {
	CXIDV_QP_STATE_RESET = 0,
	CXIDV_QP_STATE_INIT = 1,
	CXIDV_QP_STATE_RTR = 2,
	CXIDV_QP_STATE_RTS = 3,
	CXIDV_QP_STATE_SQD = 4,
	CXIDV_QP_STATE_SQE = 5,
	CXIDV_QP_STATE_ERR = 6,
};

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
	CXIDV_FIELD_AVAIL(struct cxidv_method1_attr, field, inlen)

#define CXIDV_METHOD2_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxidv_method2_attr, field, inlen)

#define CXIDV_METHOD3_ATTR_FIELD_AVAIL(field, inlen) \
	CXIDV_FIELD_AVAIL(struct cxidv_method3_attr, field, inlen)

#ifdef __cplusplus
}
#endif

#endif /* CXIDV_H */
