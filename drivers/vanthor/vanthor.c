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
#include <linux/arm-smccc.h>
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
long vanthor_hypercall_gpa_to_hpa_batch(u64 *addr_array, u64 count)
{
	struct arm_smccc_res res;

	phys_addr_t array_gpa = virt_to_phys(addr_array);

	arm_smccc_1_1_invoke(ARM_SMCCC_VENDOR_HYP_GPA_TO_HPA_FUNC_ID, array_gpa,
			     count, &res);

	return res.a0; // 返回状态码
}
static void vathor_gpa2hpa_check(void)
{
	u64 *pages_array;
	void *test_pages[4]; // 假设我们要测试 4 页
	size_t count = 4;
	size_t i;
	long ret;

	// 1. 分配用于传递地址序列的缓冲区
	pages_array = kmalloc_array(count, sizeof(u64), GFP_KERNEL);
	if (!pages_array)
		return;

	// 2. 循环分配 4 个独立的页面并获取它们的 GPA
	for (i = 0; i < count; i++) {
		test_pages[i] = (void *)__get_free_page(GFP_KERNEL);
		if (!test_pages[i]) {
			// 如果中间分配失败，释放已分配的并退出
			while (i--)
				free_page((unsigned long)test_pages[i]);
			kfree(pages_array);
			return;
		}
		// 记录每一页的 GPA
		pages_array[i] = virt_to_phys(test_pages[i]);
		pr_info("VATHOR TEST [%zu]: Before HVC - GPA: %llx\n", i,
			pages_array[i]);
	}

	// 3. 发起批量 Hypercall
	ret = vanthor_hypercall_gpa_to_hpa_batch(pages_array, count);

	// 4. 验证结果
	if (ret == 0) {
		for (i = 0; i < count; i++) {
			pr_info("VATHOR TEST [%zu]: After HVC  - HPA: %llx\n",
				i, pages_array[i]);

			// 校验：HPA 和 GPA 的低 12 位必须始终一致 (0x000)
			if ((pages_array[i] & 0xfff) != 0) {
				pr_err("VATHOR TEST [%zu]: Alignment Error!\n",
				       i);
			}
		}
	} else {
		pr_err("VATHOR TEST: Multi-page Hypercall failed: %ld\n", ret);
	}

	// 5. 清理：释放所有页面和缓冲区
	for (i = 0; i < count; i++) {
		free_page((unsigned long)test_pages[i]);
	}
	kfree(pages_array);
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
	vathor_gpa2hpa_check();
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