#include "xhci.h"


int xhci_enable_slot(struct xhci *xhci)
{
	u64 erdp;
	struct trb_template *current_trb;
	struct trb_template cmd = {0};
	struct command_completion_event_trb *completion_trb;
	int i = 0;
	u64 crcr = 0;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_ENABLE_SLOT_CMD;
	xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	//crcr = xhci_opreg_rd32(xhci, XHCI_HC_CRCR) & (~0x3f);
	//xhci_opreg_wr32(xhci, XHCI_HC_CRCR, (crcr + 16) | BIT0);

	erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	current_trb = (void *)PHYS2VIRT(erdp);

	printk("STS = %x\n", xhci_opreg_rd32(xhci, XHCI_HC_USBSTS));
	int slot = -1;

	for (i = 0; i < 16; i++) {
		if (current_trb[i].trb_type == 33) {
			completion_trb = (struct command_completion_event_trb *)&current_trb[i];
			slot = completion_trb->slot_id;
			goto done;
		}
	}
done:
	//crcr = xhci_opreg_rd32(xhci, XHCI_HC_CRCR);
	//xhci_opreg_wr64(xhci, XHCI_HC_CRCR, (crcr + 16) | BIT1);
	if (completion_trb->completion_code != 0x1)
		slot = -1;
	//xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (erdp + event * 0x10) | BIT3);
	return slot;
}

int xhci_disable_slot(struct xhci *xhci, int slot)
{
	u64 erdp;
	struct trb_template *current_trb;
	struct trb_template cmd = {0};
	struct command_completion_event_trb *completion_trb;
	int i = 0;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_DISABLE_SLOT_CMD;
	cmd.control = slot << 8;
	xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);

	erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	current_trb = (void *)PHYS2VIRT(erdp);

	while (1) {
		for (i = 0; i < 16; i++) {
			if (current_trb[i].trb_type == 33) {
				completion_trb = (struct command_completion_event_trb *)&current_trb[i];
				return completion_trb->slot_id;
			}
		}
	};
	//u64 crcr = xhci_opreg_rd32(xhci, XHCI_HC_CRCR) & (~0x3f);
	//xhci_opreg_wr64(xhci, XHCI_HC_CRCR, (crcr + 16) | BIT0);
	return -1;
}


int init_device_slot(struct xhci *xhci, int slot_id)
{
	struct input_context *input_context = kmalloc(0x1000, GFP_KERNEL);
	struct transfer_trb *transfer_ring = kmalloc(0x1000, GFP_KERNEL);
	struct device_context *output_context = kmalloc(0x1000, GFP_KERNEL);
	memset(input_context, 0, 0x1000);
	memset(output_context, 0, 0x1000);
	input_context->input_ctrl_context.add_context_flags = 0x3;
	input_context->dev_context.slot_context.root_hub_port_number = 3;
	input_context->dev_context.slot_context.route_string = 0;
	input_context->dev_context.slot_context.context_entries = 1;
	input_context->dev_context.endpoint_context[0].tr_dequeue_pointer_lo = PHYS2VIRT(transfer_ring);
	input_context->dev_context.endpoint_context[0].tr_dequeue_pointer_hi = PHYS2VIRT(transfer_ring) >> 32;
	input_context->dev_context.endpoint_context[0].ep_type = 4;
	input_context->dev_context.endpoint_context[0].max_packet_size = 64;
	input_context->dev_context.endpoint_context[0].max_burst_size = 0;
	input_context->dev_context.endpoint_context[0].dcs = 1;
	input_context->dev_context.endpoint_context[0].interval = 0;
	input_context->dev_context.endpoint_context[0].max_pstreams = 0;
	input_context->dev_context.endpoint_context[0].mult = 0;
	input_context->dev_context.endpoint_context[0].cerr = 3;

	xhci->dcbaa[slot_id] = VIRT2PHYS(output_context);

	struct address_device_trb address_dev_trb = {0};
	address_dev_trb.input_context_ptr_lo = VIRT2PHYS(input_context);
	address_dev_trb.input_context_ptr_hi = VIRT2PHYS(input_context) >> 32;
	address_dev_trb.trb_type = TRB_ADDRESS_DEVICE_CMD;
	address_dev_trb.slot_id = slot_id;
	address_dev_trb.bsr = 0;
	xhci_cmd_ring_insert(xhci, (struct trb_template *)&address_dev_trb);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	u64 crcr = xhci_opreg_rd32(xhci, XHCI_HC_CRCR) & (~0x3f);
	xhci_opreg_wr64(xhci, XHCI_HC_CRCR, (crcr + 16) | BIT0 | BIT1);
}

int xhci_intr(int irq, void *data)
{
	struct xhci *xhci = data;
	u32 usb_sts = xhci_opreg_rd32(xhci, XHCI_HC_USBSTS);
	u32 port;
	u32 port_status;
	u32 slot;
	u64 erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	printk("xhci_intr.USB STS = %x\n", usb_sts);
	struct trb_template *current_trb = (void *)PHYS2VIRT(erdp);

	if (current_trb[0].trb_type == 34) {
		struct port_status_change_event_trb *port_ch_trb = (struct port_status_change_event_trb *)current_trb;
		port = port_ch_trb[0].port_id - 1;
		port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
		printk("XHCI Port %x %s\n", port, port_status & 0x1 ? "connected" : "disconnected");
		printk("port_status = %x\n", port_status);
		//xhci_opreg_wr32(xhci, 0x400 + port * 0x10, xhci_opreg_rd32(xhci, 0x400 + port * 0x10) | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22);

		if (port_status & 0x1) {
			xhci_opreg_wr32(xhci, 0x400 + port * 0x10, XHCI_PORTSC_PR | XHCI_PORTSC_PP);
			while (1) {
				port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
				
				if ((port_status & XHCI_PORTSC_PRC) != 0) {
					xhci_opreg_wr32(xhci, 0x400 + port * 0x10, XHCI_PORTSC_PRC | XHCI_PORTSC_CSC | XHCI_PORTSC_PP);
					break;
				}
			}
			port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
			printk("port_status after reset = %x\n", port_status);
			slot = xhci_enable_slot(xhci);
			printk("available slot:%d\n", slot);
			init_device_slot(xhci, slot);
		} else {
			xhci_disable_slot(xhci, 2);
			xhci_opreg_wr32(xhci, 0x400 + port * 0x10, XHCI_PORTSC_PRC | XHCI_PORTSC_CSC | XHCI_PORTSC_PP);
		}
		port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
		printk("port_status = %x\n", port_status);
	}


	u32 ir = xhci_rtreg_rd32(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP);

	int i;
	for (i = 0; i < 4; i++) {
		if (current_trb[i].c == 1) {
			//printk("event ring %d, type = %d\n", i, current_trb[i].trb_type);
			u32 *trb = (u32 *)&current_trb[i];
			//printk("%08x %08x %08x %08x\n", trb[0], trb[1], trb[2], trb[3]);
			if (erdp + 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
				erdp += 16;
				xhci->event_ring_dequeue_ptr++;
			}
		}
	}
	//xhci_opreg_wr32(xhci, 0x400 + port * 0x10, BIT9 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22);
	xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (erdp) | BIT3);
	xhci_opreg_wr32(xhci, XHCI_HC_USBSTS, usb_sts | BIT3 | BIT4);
	return 0;
}

int xhci_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	u32 len_version, hcs_params1, hcs_params2, hcs_params3, hcc_params1;
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

	len_version = xhci_cap_rd32(xhci, XHCI_CAPLENGTH);
	xhci->hc_op_reg_offset = len_version & 0xff;
	printk("Cap len:%d\n", len_version & 0xff);
	printk("HCI version:%x\n", len_version >> 16);
	hcs_params1 = xhci_cap_rd32(xhci, XHCI_HCSPARAMS1);
	xhci->nr_slot = hcs_params1 & 0xff;
	xhci->nr_intr = (hcs_params1 >> 8) & 0x3ff;
	xhci->nr_port = hcs_params1 >> 24;
	printk("Number of slots:%d\n", xhci->nr_slot);
	printk("Number of Interrupts:%d\n", xhci->nr_intr);
	printk("Number of Ports:%d\n", xhci->nr_port);
	hcs_params2 = xhci_cap_rd32(xhci, XHCI_HCSPARAMS2);
	xhci->max_scratch_buffer_cnt = (hcs_params2 >> 27) | (((hcs_params2 >> 21) & 0x1f) << 5);
	printk("max scratch buffers:%d\n", xhci->max_scratch_buffer_cnt);
	hcs_params3 = xhci_cap_rd32(xhci, XHCI_HCSPARAMS3);
	hcc_params1 = xhci_cap_rd32(xhci, XHCI_HCCPARAMS1);
	xhci->hc_ext_reg_offset = hcc_params1 >> 16;
	printk("xHCI extended ptr:%x\n", xhci->hc_ext_reg_offset);

	xhci->hc_doorbell_reg_offset = xhci_cap_rd32(xhci, XHCI_DBOFF);
	xhci->hc_rt_reg_offset = xhci_cap_rd32(xhci, XHCI_RTSOFF);
	xhci->hc_vt_reg_offset = xhci_cap_rd32(xhci, 0x20);

	printk("doorbell offset:%x\n", xhci->hc_doorbell_reg_offset);
	printk("runtime_reg_offset offset:%x\n", xhci->hc_rt_reg_offset);
	printk("xhci_vtio_offset offset:%x\n", xhci->hc_vt_reg_offset);

	xhci_opreg_wr32(xhci, XHCI_HC_USBCMD, XHCI_HC_USBCMD_RESET);
	/* VMware stuck here. */
	while (1) {
		if ((xhci_opreg_rd32(xhci, XHCI_HC_USBCMD) & XHCI_HC_USBCMD_RESET) == 0)
			break;
	}

	ret = pci_enable_msix(pdev, 0, 2);
	if (ret < 0) {
		ret = pci_enable_msi(pdev);
	}
	list_for_each_entry(irq_desc, &pdev->irq_list, list) {
		request_irq_smp(get_cpu(), irq_desc->vector, xhci_intr, 0, "xhci", xhci);
	}

	xhci_opreg_wr32(xhci, XHCI_HC_CONFIG, xhci->nr_slot);

	xhci->dcbaa = kmalloc(0x1000, GFP_KERNEL);
	for (int i = 0; i < 256; i++) {
		xhci->dcbaa[i] = 0;
	}

	u64 *scrach_buffer = kmalloc(0x1000, GFP_KERNEL);
	memset(scrach_buffer, 0, 0x1000);
	for (int i = 0; i < xhci->max_scratch_buffer_cnt; i++) {
		u64 *buffer_sc = kmalloc(0x1000, GFP_KERNEL);
		memset(buffer_sc, 0, 0x1000);
		scrach_buffer[i] = VIRT2PHYS(buffer_sc);
	}
	xhci->dcbaa[0] = VIRT2PHYS(scrach_buffer);
	
	xhci_opreg_wr64(xhci, XHCI_HC_DCBAAP, VIRT2PHYS(xhci->dcbaa));
	//printk("dcbaap = %x\n", VIRT2PHYS(xhci->dcbaa));

	xhci->cmd_ring_size = 0x1000;
	xhci->cmd_ring = kmalloc(xhci->cmd_ring_size, GFP_KERNEL);
	xhci->cmd_ring_enqueue_ptr = 0;
	memset(xhci->cmd_ring, 0, xhci->cmd_ring_size);
	xhci_opreg_wr64(xhci, XHCI_HC_CRCR, VIRT2PHYS(xhci->cmd_ring) | BIT0);

	xhci->event_ring_seg_table = kmalloc(0x1000, GFP_KERNEL);
	memset(xhci->event_ring_seg_table, 0, 0x1000);
	xhci->event_ring_size = 0x1000;
	xhci->event_ring = kmalloc(xhci->event_ring_size, GFP_KERNEL);
	xhci->event_ring_dequeue_ptr = xhci->event_ring;
	memset(xhci->event_ring, 0, xhci->event_ring_size);
	xhci->event_ring_seg_table[0].ring_segment_base_addr = VIRT2PHYS(xhci->event_ring);
	xhci->event_ring_seg_table[0].ring_segment_size = xhci->event_ring_size / 16;
	for (int i = 0; i < xhci->nr_intr; i++) {
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTSZ, 1);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERDP, xhci->event_ring_seg_table[0].ring_segment_base_addr | BIT3);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTBA, VIRT2PHYS(xhci->event_ring_seg_table));
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMOD, 1000);
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMAN, XHCI_HC_IR_IMAN_IE);
	}

	xhci_opreg_wr32(xhci, XHCI_HC_DNCTRL, 0);
	xhci_opreg_wr32(xhci, XHCI_HC_USBCMD, XHCI_HC_USBCMD_RUN | XHCI_HC_USBCMD_INTE | XHCI_HC_USBCMD_HSEE);

	return 0;
}

void xhci_remove(struct pci_dev *pdev)
{
	
}


struct pci_device_id xhci_ids[64] = {
	{0x9d2f8086, 0x0c0330, 0xffffffff, 0x0},
	{0xa2af8086, 0x0c0330, 0xffffffff, 0x0},
	{0x077915ad, 0x0c0330, 0xffffffff, 0x0}
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
