// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright 2024 Hewlett Packard Enterprise Development LP
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <rdma/ib_user_verbs.h>
#include <rdma/uverbs_ioctl.h>

#include "cxi.h"

/* PCI Vendor and Device IDs for CXI devices */
#define PCI_VENDOR_ID_HPE		0x1590
#define PCI_VENDOR_ID_CRAY		0x17db
#define PCI_DEVICE_ID_CASSINI_1		0x0501
#define PCI_DEVICE_ID_CASSINI_2		0x0371

/* External declaration for vendor-specific UAPI definitions */
extern const struct uapi_definition cxi_ib_uapi_defs[];

MODULE_AUTHOR("Hewlett Packard Enterprise Development LP");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION(DEVICE_NAME);

/* PCI Device ID table for CXI devices */
static const struct pci_device_id cxi_ib_pci_table[] = {
	{ PCI_VDEVICE(HPE, PCI_DEVICE_ID_CASSINI_2), 0 },	/* Cassini 2 */
	{ PCI_VDEVICE(CRAY, PCI_DEVICE_ID_CASSINI_1), 0 },	/* Cassini 1 */
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, cxi_ib_pci_table);

/* Async event handler - placeholder for future implementation */
static void cxi_ib_async_event_handler(struct cxi_ib_dev *dev, int event_type)
{
	if (!dev)
		return;

	switch (event_type) {
	case 1: /* Link up */
		ibdev_info(&dev->ibdev, "Link up event received\n");
		break;
	case 2: /* Link down */
		ibdev_info(&dev->ibdev, "Link down event received\n");
		break;
	default:
		ibdev_warn(&dev->ibdev, "Unknown async event %d\n", event_type);
		break;
	}

	atomic64_inc(&dev->stats.cxi_events_rcvd);
}

static void cxi_ib_stats_init(struct cxi_ib_dev *dev)
{
	atomic64_t *s = (atomic64_t *)&dev->stats;
	int i;

	for (i = 0; i < sizeof(dev->stats) / sizeof(*s); i++, s++)
		atomic64_set(s, 0);
}

static const struct ib_device_ops cxi_ib_dev_ops = {
	.owner = THIS_MODULE,
	.driver_id = RDMA_DRIVER_UNKNOWN, /* Will need to register with RDMA core */
	.uverbs_abi_ver = 1,
	.uapi = cxi_ib_uapi_defs,

	.alloc_hw_port_stats = cxi_ib_alloc_hw_port_stats,
	.alloc_hw_device_stats = cxi_ib_alloc_hw_device_stats,
	.alloc_pd = cxi_ib_alloc_pd,
	.alloc_ucontext = cxi_ib_alloc_ucontext,
	.create_cq = cxi_ib_create_cq,
	.create_qp = cxi_ib_create_qp,
	.create_user_ah = cxi_ib_create_ah,
	.dealloc_pd = cxi_ib_dealloc_pd,
	.dealloc_ucontext = cxi_ib_dealloc_ucontext,
	.dereg_mr = cxi_ib_dereg_mr,
	.destroy_ah = cxi_ib_destroy_ah,
	.destroy_cq = cxi_ib_destroy_cq,
	.destroy_qp = cxi_ib_destroy_qp,
	.get_hw_stats = cxi_ib_get_hw_stats,
	.get_link_layer = cxi_ib_port_link_layer,
	.get_port_immutable = cxi_ib_get_port_immutable,
	.mmap = cxi_ib_mmap,
	.mmap_free = cxi_ib_mmap_free,
	.modify_qp = cxi_ib_modify_qp,
	.query_device = cxi_ib_query_device,
	.query_gid = cxi_ib_query_gid,
	.query_pkey = cxi_ib_query_pkey,
	.query_port = cxi_ib_query_port,
	.query_qp = cxi_ib_query_qp,
	.reg_user_mr = cxi_ib_reg_mr,
	.reg_user_mr_dmabuf = cxi_ib_reg_user_mr_dmabuf,

	INIT_RDMA_OBJ_SIZE(ib_ah, cxi_ib_ah, ibah),
	INIT_RDMA_OBJ_SIZE(ib_cq, cxi_ib_cq, ibcq),
	INIT_RDMA_OBJ_SIZE(ib_pd, cxi_ib_pd, ibpd),
	INIT_RDMA_OBJ_SIZE(ib_qp, cxi_ib_qp, ibqp),
	INIT_RDMA_OBJ_SIZE(ib_ucontext, cxi_ib_ucontext, ibucontext),
};

static int cxi_ib_device_add(struct cxi_ib_dev *dev)
{
	int err;

	cxi_ib_stats_init(dev);

	err = cxi_dev_info_get(dev->cxi_dev, &dev->dev_info);
	if (err) {
		ibdev_err(&dev->ibdev, "Failed to get device info: %d\n", err);
		return err;
	}

	dev->ibdev.node_type = RDMA_NODE_UNSPECIFIED;
	dev->ibdev.phys_port_cnt = 1;
	dev->ibdev.num_comp_vectors = dev->neqs ?: 1;
	dev->ibdev.dev.parent = &dev->cxi_dev->pdev->dev;

	ib_set_device_ops(&dev->ibdev, &cxi_ib_dev_ops);

	err = ib_register_device(&dev->ibdev, "cxi_%d", &dev->cxi_dev->pdev->dev);
	if (err) {
		ibdev_err(&dev->ibdev, "Failed to register IB device: %d\n", err);
		return err;
	}

	ibdev_info(&dev->ibdev, "CXI IB device registered\n");
	return 0;
}

static void cxi_ib_device_remove(struct cxi_ib_dev *dev)
{
	ibdev_info(&dev->ibdev, "Unregister CXI IB device\n");
	ib_unregister_device(&dev->ibdev);
}

static int cxi_ib_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct cxi_ib_dev *dev;
	int err;

	/* Enable PCI device */
	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "Failed to enable PCI device: %d\n", err);
		return err;
	}

	/* Set DMA mask */
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(&pdev->dev, "Failed to set DMA mask: %d\n", err);
		goto err_disable_device;
	}

	/* Set PCI master */
	pci_set_master(pdev);

	/* Allocate IB device */
	dev = ib_alloc_device(cxi_ib_dev, ibdev);
	if (!dev) {
		dev_err(&pdev->dev, "Failed to allocate IB device\n");
		err = -ENOMEM;
		goto err_disable_device;
	}

	/* Initialize device */
	dev->pdev = pdev;
	xa_init(&dev->cqs_xa);
	cxi_ib_stats_init(dev);

	/* Initialize device info with default values */
	dev->dev_info.nid = 0x1000; /* Default NID */
	dev->dev_info.pid_granule = 1;
	dev->dev_info.pid_count = 1024;
	dev->dev_info.pid_bits = 10;
	dev->dev_info.min_free_shift = 6;
	dev->dev_info.max_msg_size = 65536;
	dev->dev_info.max_eq_size = 1024;
	dev->dev_info.max_ct_size = 1024;
	dev->dev_info.max_trig_size = 1024;
	dev->dev_info.max_ptlte_size = 1024;

	/* Store the cxi_ib_dev as driver data for easy access */
	pci_set_drvdata(pdev, dev);

	err = cxi_ib_device_add(dev);
	if (err) {
		dev_err(&pdev->dev, "Failed to add IB device: %d\n", err);
		goto err_dealloc;
	}

	dev_info(&pdev->dev, "CXI InfiniBand device added\n");
	return 0;

err_dealloc:
	xa_destroy(&dev->cqs_xa);
	ib_dealloc_device(&dev->ibdev);
err_disable_device:
	pci_disable_device(pdev);
	return err;
}

static void cxi_ib_pci_remove(struct pci_dev *pdev)
{
	struct cxi_ib_dev *dev = pci_get_drvdata(pdev);

	if (!dev)
		return;

	dev_info(&pdev->dev, "Removing CXI InfiniBand device\n");

	cxi_ib_device_remove(dev);
	xa_destroy(&dev->cqs_xa);
	ib_dealloc_device(&dev->ibdev);
	pci_disable_device(pdev);
}

/* PCI driver structure */
static struct pci_driver cxi_ib_pci_driver = {
	.name		= "cxi_ib",
	.id_table	= cxi_ib_pci_table,
	.probe		= cxi_ib_pci_probe,
	.remove		= cxi_ib_pci_remove,
};

static int __init cxi_ib_init(void)
{
	int ret;

	ret = pci_register_driver(&cxi_ib_pci_driver);
	if (ret) {
		pr_err("Failed to register CXI IB PCI driver: %d\n", ret);
		return ret;
	}

	pr_info("CXI InfiniBand driver loaded\n");
	return 0;
}

static void __exit cxi_ib_exit(void)
{
	pci_unregister_driver(&cxi_ib_pci_driver);
	pr_info("CXI InfiniBand driver unloaded\n");
}

module_init(cxi_ib_init);
module_exit(cxi_ib_exit);
