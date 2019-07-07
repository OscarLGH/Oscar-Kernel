#include "xhci.h"

int xhci_intr(int irq, void *data)
{
	struct xhci *xhci = data;
	printk("xhci_intr.\n");
	return 0;
}

int xhci_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	int ret;
	struct pci_irq_desc *irq_desc;
	struct xhci *xhci = kmalloc(sizeof(*xhci), GFP_KERNEL);
	memset(xhci, 0, sizeof(*xhci));
	xhci->pdev = pdev;
	pci_enable_device(pdev);
	pci_set_master(pdev);
	xhci->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	printk("xhci:reg:%x len = %x\n", pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	
	pdev->private_data = xhci;

	u32 len_version = xhci_cap_rd32(xhci, XHCI_CAPLENGTH);
	xhci->hc_op_reg_offset = len_version & 0xff;
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

	ret = pci_enable_msix(pdev, 0, 2);
	if (ret < 0) {
		ret = pci_enable_msi(pdev);
	}
	list_for_each_entry(irq_desc, &pdev->irq_list, list) {
		request_irq_smp(get_cpu(), irq_desc->vector, xhci_intr, 0, "xhci", xhci);
	}

	u32 page_size = xhci_opreg_rd32(xhci, 0x8);
	printk("page size:%x\n", page_size);
	xhci_opreg_wr32(xhci, 0, 0x4);

	int i;
	u32 status;
	//while (1) {
		for (i = 0; i < 32; i++) {
			status = xhci_opreg_rd32(xhci, 0x400 + i * 0x10);
			if (status & 0x1)
				printk("Port %02d SC:%x\n", i, status);
		}
	//}

	

	return 0;
}

void xhci_remove(struct pci_dev *pdev)
{
	
}


struct pci_device_id xhci_ids[64] = {
	{0x9d2f8086, 0x0c0330, 0xffffffff, 0x0},
	{0xa2af8086, 0x0c0330, 0xffffffff, 0x0}
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

