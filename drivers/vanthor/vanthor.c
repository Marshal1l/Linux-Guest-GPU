// SPDX-License-Identifier: GPL-2.0
/*
 * vanthor - minimal stub driver for Mali Valhall (CSF)
 *
 * This driver only:
 *  - matches DT nodes
 *  - parses reg / interrupt resources
 *  - prints information
 *
 * It does NOT:
 *  - ioremap
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

#define DRV_NAME "vanthor"

static int vanthor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int irq;
	int i;

	dev_info(dev, "vanthor probe start\n");

	/* ---- reg ---- */
	for (i = 0;; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res)
			break;

		dev_info(dev, "reg[%d]: start=0x%pa size=0x%llx\n", i,
			 &res->start, (unsigned long long)resource_size(res));
	}

	if (i == 0)
		dev_warn(dev, "no reg resources found\n");

	/* ---- interrupts ---- */
	for (i = 0;; i++) {
		irq = platform_get_irq(pdev, i);
		if (irq < 0)
			break;

		dev_info(dev, "irq[%d]: %d\n", i, irq);
	}

	if (i == 0)
		dev_warn(dev, "no irq resources found\n");

	/* ---- clocks (optional) ---- */
	for (i = 0;; i++) {
		struct clk *clk;

		clk = devm_clk_get_optional(dev, NULL);
		if (IS_ERR(clk))
			break;

		if (!clk)
			break;

		dev_info(dev, "clock found\n");
		break;
	}

	dev_info(dev, "vanthor probe done (no binding performed)\n");
	return 0;
}

static void vanthor_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "vanthor remove\n");
	return;
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
