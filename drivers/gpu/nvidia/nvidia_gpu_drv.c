#include "nvidia_gpu.h"

int nvidia_gpu_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	u32 boot0, strap;
	struct nvidia_gpu *gpu = kmalloc(sizeof(*gpu), GFP_KERNEL);
	printk("%s\n", __FUNCTION__);
	pci_read_config_word(pdev, PCI_COMMAND, &pci_command_reg);
	pci_command_reg |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_write_config_word(pdev, PCI_COMMAND, pci_command_reg);
	gpu->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	gpu->chipset = (nvkm_rd(gpu, 0) >> 20) & 0x1ff;
	printk("nvidia chipset = %x\n", gpu->chipset);

	pdev->private_data = gpu;
}

void nvidia_gpu_remove(struct pci_dev *pdev)
{
	
}


struct pci_device_id nvidia_ids[64] = {
	{0x134f10de, 0x030200, 0xffffffff, 0x0}
};

struct pci_driver nvidia_gpu_driver = {
	.name = "nvidia_gpu_driver",
	.id_array = nvidia_ids,
	.pci_probe = nvidia_gpu_probe,
	.pci_remove = nvidia_gpu_remove
};

void nvidia_gpu_init()
{
	pci_register_driver(&nvidia_gpu_driver);
}

module_init(nvidia_gpu_init);

