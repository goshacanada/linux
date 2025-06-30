// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include "cxi_com.h"
#include "cxi.h"

int cxi_ib_com_dev_init(struct cxi_ib_com_dev *com_dev,
			struct cxi_dev *cxi_dev,
			struct cxi_ib_dev *cxi_ib_dev)
{
	int ret;

	if (!com_dev || !cxi_dev || !cxi_ib_dev)
		return -EINVAL;

	com_dev->cxi_dev = cxi_dev;
	com_dev->cxi_ib_dev = cxi_ib_dev;

	/* Allocate LNI (Logical Network Interface) */
	com_dev->lni = cxi_lni_alloc(cxi_dev, 1); /* Use service ID 1 */
	if (IS_ERR(com_dev->lni)) {
		ret = PTR_ERR(com_dev->lni);
		ibdev_err(&cxi_ib_dev->ibdev, "Failed to allocate LNI: %d\n", ret);
		return ret;
	}

	/* Allocate Communication Profile */
	com_dev->cp = cxi_cp_alloc(com_dev->lni, 0, CXI_TC_BEST_EFFORT,
				   CXI_TC_TYPE_DEFAULT);
	if (IS_ERR(com_dev->cp)) {
		ret = PTR_ERR(com_dev->cp);
		ibdev_err(&cxi_ib_dev->ibdev, "Failed to allocate CP: %d\n", ret);
		goto err_free_lni;
	}

	/* Initialize statistics */
	atomic64_set(&com_dev->stats.submitted_cmd, 0);
	atomic64_set(&com_dev->stats.completed_cmd, 0);
	atomic64_set(&com_dev->stats.cmd_err, 0);
	atomic64_set(&com_dev->stats.no_completion, 0);

	return 0;

err_free_lni:
	cxi_lni_free(com_dev->lni);
	return ret;
}

void cxi_ib_com_dev_cleanup(struct cxi_ib_com_dev *com_dev)
{
	if (!com_dev)
		return;

	if (com_dev->cp) {
		cxi_cp_free(com_dev->cp);
		com_dev->cp = NULL;
	}

	if (com_dev->lni) {
		cxi_lni_free(com_dev->lni);
		com_dev->lni = NULL;
	}
}

int cxi_ib_com_eq_init(struct cxi_ib_dev *dev, struct cxi_ib_eq *eq,
		       unsigned int eq_depth, unsigned int msix_vec)
{
	struct cxi_eq_attr eq_attr = {
		.queue_len = eq_depth,
		.flags = 0,
	};
	void *eq_buf;
	struct cxi_md *eq_md;
	int ret;

	if (!dev || !eq)
		return -EINVAL;

	/* Allocate event queue buffer */
	eq_buf = dma_alloc_coherent(&dev->cxi_dev->pdev->dev,
				    eq_depth * sizeof(union c_event),
				    &eq_attr.queue_dma_addr, GFP_KERNEL);
	if (!eq_buf) {
		ibdev_err(&dev->ibdev, "Failed to allocate EQ buffer\n");
		return -ENOMEM;
	}

	eq_attr.queue = eq_buf;

	/* Map the buffer for CXI */
	ret = cxi_map(NULL, (uintptr_t)eq_buf, eq_depth * sizeof(union c_event),
		      CXI_MAP_PIN | CXI_MAP_WRITE, NULL, &eq_md);
	if (ret) {
		ibdev_err(&dev->ibdev, "Failed to map EQ buffer: %d\n", ret);
		goto err_free_buf;
	}

	/* Allocate the event queue */
	eq->cxi_eq = cxi_eq_alloc(NULL, eq_md, &eq_attr, NULL, NULL);
	if (IS_ERR(eq->cxi_eq)) {
		ret = PTR_ERR(eq->cxi_eq);
		ibdev_err(&dev->ibdev, "Failed to allocate EQ: %d\n", ret);
		goto err_unmap;
	}

	snprintf(eq->irq_name, CXI_IB_IRQNAME_SIZE, "cxi-ib-eq%d@pci:%s",
		 msix_vec, pci_name(dev->cxi_dev->pdev));

	return 0;

err_unmap:
	cxi_unmap(eq_md);
err_free_buf:
	dma_free_coherent(&dev->cxi_dev->pdev->dev,
			  eq_depth * sizeof(union c_event),
			  eq_buf, eq_attr.queue_dma_addr);
	return ret;
}

void cxi_ib_com_eq_destroy(struct cxi_ib_dev *dev, struct cxi_ib_eq *eq)
{
	if (!dev || !eq || !eq->cxi_eq)
		return;

	cxi_eq_free(eq->cxi_eq);
	eq->cxi_eq = NULL;
}

int cxi_ib_com_cq_init(struct cxi_ib_dev *dev, struct cxi_cq **cq,
		       unsigned int cq_depth, struct cxi_eq *eq)
{
	struct cxi_cq_alloc_opts opts = {
		.count = cq_depth,
		.flags = 0, /* Use default flags */
	};

	if (!dev || !cq)
		return -EINVAL;

	/* For now, we'll use a simple approach and let the CXI driver
	 * handle the command queue allocation details
	 */
	*cq = cxi_cq_alloc(NULL, eq, &opts);
	if (IS_ERR(*cq)) {
		int ret = PTR_ERR(*cq);
		ibdev_err(&dev->ibdev, "Failed to allocate CQ: %d\n", ret);
		return ret;
	}

	return 0;
}

void cxi_ib_com_cq_destroy(struct cxi_ib_dev *dev, struct cxi_cq *cq)
{
	if (!dev || !cq)
		return;

	cxi_cq_free(cq);
}

int cxi_ib_com_map_memory(struct cxi_ib_dev *dev, void *addr, size_t len,
			  u32 flags, struct cxi_md **md)
{
	int ret;

	if (!dev || !addr || !md)
		return -EINVAL;

	ret = cxi_map(NULL, (uintptr_t)addr, len, flags, NULL, md);
	if (ret) {
		ibdev_err(&dev->ibdev, "Failed to map memory: %d\n", ret);
		return ret;
	}

	return 0;
}

void cxi_ib_com_unmap_memory(struct cxi_ib_dev *dev, struct cxi_md *md)
{
	if (!dev || !md)
		return;

	cxi_unmap(md);
}

int cxi_ib_com_get_device_attr(struct cxi_ib_dev *dev,
			       struct cxi_dev_info *dev_info)
{
	if (!dev || !dev_info)
		return -EINVAL;

	return cxi_dev_info_get(dev->cxi_dev, dev_info);
}

void cxi_ib_com_get_stats(struct cxi_ib_com_dev *com_dev,
			  struct cxi_ib_com_stats *stats)
{
	if (!com_dev || !stats)
		return;

	*stats = com_dev->stats;
}
