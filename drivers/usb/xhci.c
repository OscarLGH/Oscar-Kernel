#include "xhci.h"

int xhci_intr(int irq, void *data)
{
	struct xhci *xhci = data;
	u32 usb_sts = xhci_opreg_rd32(xhci, XHCI_HC_USBSTS);
	u32 port;
	u32 port_status;
	u64 erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP);
	//printk("xhci_intr.USB STS = %x\n", usb_sts);
	struct port_status_change_event_trb *event_trbs = (void *)PHYS2VIRT(erdp & (~0xf));
	if (event_trbs[0].trb_type == 34) {
		if (port != 0) {
			port = event_trbs[0].port_id - 1;
			port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
			printk("XHCI Port %x %s\n", port, port_status & 0x1 ? "connected" : "disconnected");
			xhci_opreg_wr32(xhci, 0x400 + port * 0x10, xhci_opreg_rd32(xhci, 0x400 + port * 0x10) | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22);
		}
	}
	xhci_opreg_wr32(xhci, XHCI_HC_USBSTS, usb_sts | BIT3 | BIT4);
	u32 ir = xhci_rtreg_rd32(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP);
	//printk("ERDP:%x\n", xhci_rtreg_rd32(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP));
	if (ir & BIT3) {
		if (event_trbs[0].trb_type == 34) {
			erdp += 16;
		}
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (erdp) | BIT3);
	}

	
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
	printk("xhci:reg:%x len:%x\n", pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	
	pdev->private_data = xhci;

	u32 len_version = xhci_cap_rd32(xhci, XHCI_CAPLENGTH);
	xhci->hc_op_reg_offset = len_version & 0xff;
	printk("Cap len:%d\n", len_version & 0xff);
	printk("HCI version:%x\n", len_version >> 16);
	u32 hcs_params1 = xhci_cap_rd32(xhci, XHCI_HCCPARAMS1);
	printk("Number of slots:%d\n", hcs_params1 & 0xff);
	printk("Number of Interrupts:%d\n", (hcs_params1 >> 8) & 0x3ff);
	printk("Number of Ports:%d\n", hcs_params1 >> 24);

	xhci->hc_doorbell_reg_offset = xhci_cap_rd32(xhci, XHCI_DBOFF);
	xhci->hc_rt_reg_offset = xhci_cap_rd32(xhci, XHCI_RTSOFF);
	xhci->hc_vt_reg_offset = xhci_cap_rd32(xhci, 0x20);

	printk("doorbell offset:%x\n", xhci->hc_doorbell_reg_offset);
	printk("runtime_reg_offset offset:%x\n", xhci->hc_rt_reg_offset);
	printk("xhci_vtio_offset offset:%x\n", xhci->hc_vt_reg_offset);

	ret = pci_enable_msix(pdev, 0, 2);
	if (ret < 0) {
		ret = pci_enable_msi(pdev);
	}
	list_for_each_entry(irq_desc, &pdev->irq_list, list) {
		request_irq_smp(get_cpu(), irq_desc->vector, xhci_intr, 0, "xhci", xhci);
	}

	u32 page_size = xhci_opreg_rd32(xhci, 0x8);
	printk("page size:%x\n", page_size);

	xhci->dcbaa = kmalloc(0x1000, GFP_KERNEL);
	memset(xhci->dcbaa, 0, 0x1000);
	xhci_opreg_wr64(xhci, XHCI_HC_DCBAAP, VIRT2PHYS(xhci->dcbaa));
	
	xhci->cmd_ring = kmalloc(0x1000, GFP_KERNEL);
	memset(xhci->cmd_ring, 0, 0x1000);
	xhci_opreg_wr64(xhci, XHCI_HC_CRCR, VIRT2PHYS(xhci->cmd_ring));

	xhci->event_ring_seg_table = kmalloc(0x1000, GFP_KERNEL);
	memset(xhci->event_ring_seg_table, 0, 0x1000);
	struct port_status_change_event_trb *event_trbs = kmalloc(0x1000, GFP_KERNEL);
	memset(event_trbs, 0, 0x1000);
	xhci->event_ring_seg_table[0].ring_segment_base_addr = VIRT2PHYS(event_trbs);
	xhci->event_ring_seg_table[0].ring_segment_size = 0x1000 / 16;
	for (int i = 0; i < 119; i++) {
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTSZ, 1);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERDP, xhci->event_ring_seg_table[0].ring_segment_base_addr | BIT3);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTBA, VIRT2PHYS(xhci->event_ring_seg_table));
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMOD, 4000);
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMAN, XHCI_HC_IR_IMAN_IE);
	}

	xhci_opreg_wr32(xhci, XHCI_HC_USBCMD, XHCI_HC_USBCMD_RUN | XHCI_HC_USBCMD_INTE | XHCI_HC_USBCMD_HSEE);

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

