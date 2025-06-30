/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#ifndef _CXI_IB_COM_H_
#define _CXI_IB_COM_H_

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/sched.h>

#include <rdma/ib_verbs.h>
#include <linux/cxi/cxi.h>

/* Don't use anything other than atomic64 */
struct cxi_ib_com_stats {
	atomic64_t submitted_cmd;
	atomic64_t completed_cmd;
	atomic64_t cmd_err;
	atomic64_t no_completion;
};

struct cxi_ib_com_dev {
	struct cxi_dev *cxi_dev;
	struct cxi_ib_dev *cxi_ib_dev;
	struct cxi_lni *lni;
	struct cxi_cp *cp;
	struct cxi_ib_com_stats stats;
};

/* Event handling */
typedef void (*cxi_ib_event_handler)(struct cxi_ib_dev *dev,
				      enum cxi_async_event event);

/* Communication layer initialization and cleanup */
int cxi_ib_com_dev_init(struct cxi_ib_com_dev *com_dev,
			struct cxi_dev *cxi_dev,
			struct cxi_ib_dev *cxi_ib_dev);
void cxi_ib_com_dev_cleanup(struct cxi_ib_com_dev *com_dev);

/* Event queue management */
int cxi_ib_com_eq_init(struct cxi_ib_dev *dev, struct cxi_ib_eq *eq,
		       unsigned int eq_depth, unsigned int msix_vec);
void cxi_ib_com_eq_destroy(struct cxi_ib_dev *dev, struct cxi_ib_eq *eq);

/* Command queue management */
int cxi_ib_com_cq_init(struct cxi_ib_dev *dev, struct cxi_cq **cq,
		       unsigned int cq_depth, struct cxi_eq *eq);
void cxi_ib_com_cq_destroy(struct cxi_ib_dev *dev, struct cxi_cq *cq);

/* Memory management */
int cxi_ib_com_map_memory(struct cxi_ib_dev *dev, void *addr, size_t len,
			  u32 flags, struct cxi_md **md);
void cxi_ib_com_unmap_memory(struct cxi_ib_dev *dev, struct cxi_md *md);

/* Device information */
int cxi_ib_com_get_device_attr(struct cxi_ib_dev *dev,
			       struct cxi_dev_info *dev_info);

/* Statistics */
void cxi_ib_com_get_stats(struct cxi_ib_com_dev *com_dev,
			  struct cxi_ib_com_stats *stats);

/* Utility functions */
static inline struct cxi_ib_dev *cxi_to_cxi_ib_dev(struct cxi_dev *cxi_dev)
{
	/* This assumes the cxi_ib_dev is stored as driver data */
	return dev_get_drvdata(&cxi_dev->pdev->dev);
}

#endif /* _CXI_IB_COM_H_ */
