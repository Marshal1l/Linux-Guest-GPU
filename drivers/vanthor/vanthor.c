// SPDX-License-Identifier: GPL-2.0
/*
 * vanthor - minimal stub driver for Mali Valhall (CSF)
 *
 * This driver now:
 *  - matches DT nodes
 *  - parses reg / interrupt resources
 *  - prints information
 *  - maps the first memory region (index 0) using devm_ioremap_resource
 *  - stores phys_addr and iomem in private data
 *
 * It does NOT:
 *  - map additional memory regions (if any)
 *  - request_irq
 *  - bind to DRM / GPU
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/err.h>
#include "vanthor.h"
#include "vanthor_regs.h"
static void vanthor_gpu_id_check(struct vanthor_device *ptdev)
{
	u32 id1, id2;

	id1 = gpu_read(ptdev, GPU_ID);
	id2 = gpu_read(ptdev, GPU_ID);

	dev_info(ptdev->dev, "GPU_ID = 0x%08x (stable=%s)\n", id1,
		 (id1 == id2) ? "yes" : "NO");
}

static int vanthor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct vanthor_device *vdev;
	struct resource *res;
	int irq;
	int i;
	int ret;

	dev_info(dev, "vanthor probe start\n");

	vdev = devm_kzalloc(dev, sizeof(*vdev), GFP_KERNEL);
	if (!vdev)
		return -ENOMEM;

	vdev->dev = dev;
	dev_set_drvdata(dev, vdev);

	/* Map the first memory resource (index 0) */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		vdev->phys_addr = res->start;
		vdev->iomem = devm_ioremap_resource(dev, res);
		if (IS_ERR(vdev->iomem)) {
			ret = PTR_ERR(vdev->iomem);
			dev_err(dev, "failed to ioremap resource 0: %d\n", ret);
			return ret;
		}
		dev_info(dev, "mapped reg[0]: phys=0x%pa virt=%p size=0x%llx\n",
			 &vdev->phys_addr, vdev->iomem,
			 (unsigned long long)resource_size(res));
	} else {
		dev_info(dev,
			 "no memory resource at index 0, skipping mapping\n");
	}

	/* Print all memory resources (including the mapped one) */
	for (i = 0;; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;

		dev_info(dev, "reg[%d]: start=0x%pa size=0x%llx%s\n", i,
			 &res->start, (unsigned long long)resource_size(res),
			 (i == 0 && vdev->iomem &&
			  res->start == vdev->phys_addr) ?
				 " (mapped)" :
				 "");
	}

	if (i == 0)
		dev_warn(dev, "no reg resources found\n");

	/* Print all IRQs */
	for (i = 0;; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq < 0)
			break;

		dev_info(dev, "irq[%d]: %d\n", i, irq);
	}

	if (i == 0)
		dev_warn(dev, "no irq resources found\n");
	vanthor_gpu_id_check(vdev);
	dev_info(dev, "vanthor probe done (no binding performed)\n");
	return 0;
}

static void vanthor_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "vanthor remove\n");
}

static const struct of_device_id vanthor_of_match[] = {
	{ .compatible = "rockchip,rk3588-mali" },
	{ .compatible = "arm,mali-valhall-csf" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vanthor_of_match);

static struct platform_driver vanthor_driver = {
	.probe  = vanthor_probe,
	.remove = vanthor_remove,
	.driver = {
		.name           = DRV_NAME,
		.of_match_table = vanthor_of_match,
	},
};

static int __init vanthor_init(void)
{
	pr_info("[MZH] vanthor init\n");
	return platform_driver_register(&vanthor_driver);
}
module_init(vanthor_init);

static void __exit vanthor_exit(void)
{
	pr_info("[MZH] vanthor exit\n");
	platform_driver_unregister(&vanthor_driver);
}
module_exit(vanthor_exit);

MODULE_DESCRIPTION("Minimal Mali Valhall DT stub driver");
MODULE_AUTHOR("You");
MODULE_LICENSE("GPL");