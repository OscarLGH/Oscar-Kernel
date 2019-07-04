#include "xhci.h"

int xhci_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	u32 boot0, strap;
	struct xhci *xhci = kmalloc(sizeof(*xhci), GFP_KERNEL);
	memset(xhci, 0, sizeof(*xhci));
	xhci->pdev = pdev;
	pci_read_config_word(pdev, PCI_COMMAND, &pci_command_reg);
	pci_command_reg |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_write_config_word(pdev, PCI_COMMAND, pci_command_reg);
	xhci->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	printk("xhci:reg:%x len = %x\n", pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	
	pdev->private_data = xhci;

	u32 len_version = xhci_cap_rd32(xhci, XHCI_CAPLENGTH);
	printk("Cap len:%d\n", len_version & 0xff);
	printk("HCI version:%x\n", len_version >> 16);
	u32 hcs_params1 = xhci_cap_rd32(xhci, XHCI_HCCPARAMS1);
	printk("Number of slots:%d\n", hcs_params1 & 0xff);
	printk("Number of Interrupts:%d\n", (hcs_params1 >> 8) & 0x3ff);
	printk("Number of Ports:%d\n", hcs_params1 >> 24);

	u32 doorbell_offset = xhci_cap_rd32(xhci, XHCI_DBOFF);
	u32 runtime_reg_offset = xhci_cap_rd32(xhci, XHCI_RTSOFF);
	u32 xhci_vtio_offset = xhci_cap_rd32(xhci, 0x20);

	printk("doorbell offset:%x\n", doorbell_offset);
	printk("runtime_reg_offset offset:%x\n", runtime_reg_offset);
	printk("xhci_vtio_offset offset:%x\n", xhci_vtio_offset);

	return 0;
}

void xhci_remove(struct pci_dev *pdev)
{
	
}


struct pci_device_id xhci_ids[64] = {
	{0x9d2f8086, 0x0c0330, 0xffffffff, 0x0}
};

struct pci_driver xhci_host_driver = {
	.name = "xhci_host_driver",
	.id_array = xhci_ids,
	.pci_probe = xhci_probe,
	.pci_remove = xhci_remove
};

void xhci_init()
{
	pci_register_driver(&xhci_host_driver);
}

module_init(xhci_init);

