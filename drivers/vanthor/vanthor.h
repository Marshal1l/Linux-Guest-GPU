#define DRV_NAME "vanthor"

struct vanthor_device {
	struct device *dev;
	/** @phys_addr: Physical address of the iomem region. */
	phys_addr_t phys_addr;
	/** @iomem: CPU mapping of the IOMEM region. */
	void __iomem *iomem;
};
