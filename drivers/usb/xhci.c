#include "xhci.h"

int xhci_cmd_ring_insert(struct xhci *xhci, struct trb_template *cmd)
{
	int index;
	index = xhci->cmd_ring_enqueue_ptr;
	//printk("cmd index = %d\n", index);
	if (index == xhci->cmd_ring_size / sizeof(struct trb_template) - 1) {
		printk("cmd ring wrapback.============================\n");
		struct link_trb link_trb = {0};
		link_trb.c = xhci->cmd_ring_dcs;
		link_trb.trb_type = TRB_LINK;
		link_trb.ring_seg_pointer = VIRT2PHYS(xhci->cmd_ring);
		xhci->cmd_ring[index] = *((struct trb_template *)&link_trb);
		//xhci->cmd_ring_dcs = ~xhci->cmd_ring_dcs;
		//xhci_doorbell_reg_wr32(xhci, 0, 0);
		//memset(&xhci->cmd_ring[index], 0, 16);
		index = 0;
		xhci->cmd_ring_enqueue_ptr = 0;
	}
	xhci->cmd_ring[index] = *cmd;
	xhci->cmd_ring[index].c = xhci->cmd_ring_dcs;
	xhci->cmd_ring_enqueue_ptr++;
	memset(&xhci->cmd_ring[index + 1], 0, 16);
	return index;
}

struct command_completion_event_trb *xhci_wait_cmd_completion(struct xhci *xhci, int cmd_index, int timeout)
{
	u64 erdp;
	int i = 0, j = timeout = 0xffffffff;
	struct trb_template *current_trb;
	erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	u64 cmd_trb_phys = VIRT2PHYS(&xhci->cmd_ring[cmd_index]);
	int ccs = xhci->event_ring_ccs;
	while (j--) {
		current_trb = (void *)PHYS2VIRT(erdp);
		i = 0;
		while (erdp + i * 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_COMMAND_COMPLETION_EVENT) && (current_trb[i].parameter == cmd_trb_phys)) {
					//xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (erdp + i * 16) | BIT3);
					//printk("erdp index = %d\n", (erdp + i * 16 - xhci->event_ring_seg_table[0].ring_segment_base_addr) / 16);
					return (struct command_completion_event_trb *)&current_trb[i];
				}
			}
			i++;
		}
		
		current_trb = (void *)PHYS2VIRT(xhci->event_ring_seg_table[0].ring_segment_base_addr);
		ccs = ~ccs;
		i = 0;
		while (xhci->event_ring_seg_table[0].ring_segment_base_addr + i * 16 <= erdp - 16) {
			if (1 || current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_COMMAND_COMPLETION_EVENT) && (current_trb[i].parameter == cmd_trb_phys)) {
					//xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (xhci->event_ring_seg_table[0].ring_segment_base_addr + i * 16) | BIT3);
					//printk("erdp index = %d\n", i);
					return (struct command_completion_event_trb *)&current_trb[i];
				}
				
			}
			i++;
		}
		
	}
	return NULL;
}


struct transfer_event_trb *xhci_wait_transfer_completion(struct xhci *xhci, u64 trb_phys_addr, int timeout)
{
	u64 erdp;
	int i = 0, j = timeout = 10000000;
	struct trb_template *current_trb;
	erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	int ccs;
	while (j--) {
		ccs = xhci->event_ring_ccs;
		current_trb = (void *)PHYS2VIRT(erdp);
		i = 0;
		while (erdp + i * 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_TRANSFER_EVENT) && (current_trb[i].parameter == trb_phys_addr)) {
					//xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (erdp + i * 16) | BIT3);
					//printk("erdp index = %d\n", (erdp + i * 16 - xhci->event_ring_seg_table[0].ring_segment_base_addr) / 16);
					return (struct transfer_event_trb *)&current_trb[i];
				}
			}
			i++;
		}

		current_trb = (void *)PHYS2VIRT(xhci->event_ring_seg_table[0].ring_segment_base_addr);
		ccs = ~ccs;
		i = 0;
		while (xhci->event_ring_seg_table[0].ring_segment_base_addr + i * 16 <= erdp - 16) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_TRANSFER_EVENT) && (current_trb[i].parameter == trb_phys_addr)) {
					//xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (xhci->event_ring_seg_table[0].ring_segment_base_addr + i * 16) | BIT3);
					//printk("erdp index = %d\n", i);
					return (struct transfer_event_trb *)&current_trb[i];
				}
				
			}
			i++;
		}
	}
	return NULL;
}


int xhci_cmd_enable_slot(struct xhci *xhci)
{
	struct trb_template cmd = {0};
	int cmd_ring_index;
	struct command_completion_event_trb *completion_trb;
	int slot = -1;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_ENABLE_SLOT_CMD;
	cmd_ring_index = xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 100000);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		slot = -1;
	slot = completion_trb->slot_id;
	return slot;
}

int xhci_cmd_disable_slot(struct xhci *xhci, int slot_id)
{
	struct trb_template cmd = {0};
	int cmd_ring_index;
	struct command_completion_event_trb *completion_trb;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_DISABLE_SLOT_CMD;
	cmd.control = (slot_id << 8);
	cmd_ring_index = xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 100000);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

int xhci_cmd_address_device(struct xhci *xhci, struct input_context *input_ctx, int slot_id)
{
	struct address_device_trb address_dev_trb = {0};
	int cmd_ring_index = 0;
	struct command_completion_event_trb *completion_trb;
	address_dev_trb.input_context_ptr_lo = VIRT2PHYS(input_ctx);
	address_dev_trb.input_context_ptr_hi = VIRT2PHYS(input_ctx) >> 32;
	address_dev_trb.trb_type = TRB_ADDRESS_DEVICE_CMD;
	address_dev_trb.slot_id = slot_id;
	address_dev_trb.bsr = 0;
	cmd_ring_index = xhci_cmd_ring_insert(xhci, (struct trb_template *)&address_dev_trb);

	xhci_doorbell_reg_wr32(xhci, 0, 0);
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
	printk("completion_trb->completion_code = %d\n", completion_trb->completion_code);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

int xhci_cmd_configure_endpoint(struct xhci *xhci, struct input_context *input_ctx, int slot_id, bool dc)
{
	struct configure_endpoint_trb config_ep_trb = {0};
	int cmd_ring_index = 0;
	struct command_completion_event_trb *completion_trb;
	config_ep_trb.input_context_ptr_lo = VIRT2PHYS(input_ctx);
	config_ep_trb.input_context_ptr_hi = VIRT2PHYS(input_ctx) >> 32;
	config_ep_trb.trb_type = TRB_CONFIG_ENDPOINT_CMD;
	config_ep_trb.slot_id = slot_id;
	config_ep_trb.dc = 0;
	config_ep_trb.c = 1;
	cmd_ring_index = xhci_cmd_ring_insert(xhci, (struct trb_template *)&config_ep_trb);

	xhci_doorbell_reg_wr32(xhci, 0, 0);
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
	
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

int xhci_cmd_evaluate_context(struct xhci *xhci, struct input_context *input_ctx, int slot_id)
{
	struct evaluate_context_trb evaluate_context_trb = {0};
	int cmd_ring_index = 0;
	struct command_completion_event_trb *completion_trb;
	evaluate_context_trb.input_context_ptr_lo = VIRT2PHYS(input_ctx);
	evaluate_context_trb.input_context_ptr_hi = VIRT2PHYS(input_ctx) >> 32;
	evaluate_context_trb.trb_type = TRB_EVALUATE_CONTEXT_CMD;
	evaluate_context_trb.slot_id = slot_id;
	evaluate_context_trb.bsr = 0;
	cmd_ring_index = xhci_cmd_ring_insert(xhci, (struct trb_template *)&evaluate_context_trb);

	xhci_doorbell_reg_wr32(xhci, 0, 0);
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
	
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

int xhci_stop_endpoint(struct xhci *xhci, int slot_id, int endpoint_id)
{
	struct trb_template cmd = {0};
	int cmd_ring_index;
	struct command_completion_event_trb *completion_trb;
	int i = 0;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_STOP_ENDPOINT_CMD;
	cmd.control = (slot_id << 8) | endpoint_id;
	cmd_ring_index = xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 1000000);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

int xhci_reset_endpoint(struct xhci *xhci, int slot_id, int endpoint_id)
{
	struct trb_template cmd = {0};
	int cmd_ring_index;
	struct command_completion_event_trb *completion_trb;
	int i = 0;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_RESET_ENDPOINT_CMD;
	cmd.control = (slot_id << 8) | endpoint_id;
	cmd.reserved1 = (0 << 7);
	cmd_ring_index = xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 100000);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

struct endpoint_context *usb_get_context(struct xhci *xhci, int slot_id, int endpoint)
{
	struct device_context *output_context = (void *)PHYS2VIRT(xhci->dcbaa[slot_id]);
	struct endpoint_context *context = &output_context->endpoint_context[ep_addr_to_dci(endpoint) - 1];
	return context;
}

u64 xhci_insert_transfer_trb(struct xhci *xhci, int port, int endpoint, struct transfer_trb *trb_in)
{
	u64 phys;
	int index = xhci->port[port].transfer_ring_status[endpoint].enquene_pointer;
	struct transfer_trb *trb = &xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base[index];
	//printk("trb_in->c = %x\n", trb_in->c);
	//printk("index  = %d\n", index);
	if (index == 0x1000 / sizeof(struct transfer_trb) - 1) {
		//printk("transfer ring overrun.\n");
		//memset(&xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base[0], 0, 0x1000 - 0x10);
		struct link_trb link_trb = {0};
		link_trb.c = xhci->port[port].transfer_ring_status[endpoint].dcs;
		link_trb.tc = 1;
		link_trb.trb_type = TRB_LINK;
		link_trb.ring_seg_pointer = VIRT2PHYS(xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base);
		*trb = *((struct transfer_trb *)&link_trb);
		index = 0;
		xhci->port[port].transfer_ring_status[endpoint].enquene_pointer = 0;
		xhci->port[port].transfer_ring_status[endpoint].dcs = (~xhci->port[port].transfer_ring_status[endpoint].dcs) & 0x1;
		//printk("xhci->port[port].transfer_ring_status[endpoint].dcs = %x\n", xhci->port[port].transfer_ring_status[endpoint].dcs);
		//printk("ep state:%x\n", usb_get_context(xhci, xhci->port[port].slot_id, 0x81)->ep_state);
		
	}
	trb = &xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base[index];
	*trb = *trb_in;
	trb->c = xhci->port[port].transfer_ring_status[endpoint].dcs;
	phys = VIRT2PHYS(trb);
	//printk("trb phys:%x\n", VIRT2PHYS(trb));
	xhci->port[port].transfer_ring_status[endpoint].enquene_pointer++;
	return phys;
}


int usb_control_transfer(struct urb *urb)
{
	struct usb_device *udev = urb->dev;
	struct xhci *xhci = udev->host_controller_context;
	struct setup_stage_trb *setup_trb = kmalloc(sizeof(*setup_trb), GFP_KERNEL);
	struct data_stage_trb *data_trb;
	struct status_stage_trb *status_trb = kmalloc(sizeof(*status_trb), GFP_KERNEL);
	struct usb_ctrlrequest *req = (struct usb_ctrlrequest *)urb->setup_packet;
	bool direction = !!(urb->pipe & 0x80000000);
	int endpoint = urb->pipe & 0xffff;
	u64 trb_phys;
	int ret = 0;
	memset(setup_trb, 0, sizeof(*setup_trb));
	memset(status_trb, 0, sizeof(*status_trb));
	setup_trb->b_request = req->bRequest;
	setup_trb->bm_request_type = req->bRequestType;
	setup_trb->w_value = req->wValue;
	setup_trb->w_index = req->wIndex;
	setup_trb->w_length = req->wLength;
	setup_trb->trb_type = TRB_SETUP_STAGE;
	setup_trb->trb_transfer_len = 8;
	setup_trb->idt = 1;

	/* 4.11.2.2 Table 4-7 */
	if (direction == 0) {
		if (setup_trb->w_length == 0) {
			setup_trb->trt = 0;
		} else {
			setup_trb->trt = 2;
		}
		status_trb->dir = 1;
	} else {
		if (setup_trb->w_length == 0) {
			setup_trb->trt = 0;
		} else {
			setup_trb->trt = 3;
		}
		status_trb->dir = 0;
	}	

	//setup_trb->ioc = 1;
	xhci_insert_transfer_trb(xhci, udev->port, ep_addr_to_dci(endpoint) - 1, (struct transfer_trb *)setup_trb);

	if (urb->transfer_buffer != NULL) {
		data_trb = kmalloc(sizeof(*data_trb), GFP_KERNEL);
		memset(data_trb, 0, sizeof(*data_trb));
		data_trb->data_buffer_addr = VIRT2PHYS(urb->transfer_buffer);
		data_trb->dir = status_trb->dir == 0 ? 1 : 0;
		data_trb->trb_type = TRB_DATA_STAGE;
		data_trb->trb_transfer_len = setup_trb->w_length;
		data_trb->ioc = 1;
		trb_phys = xhci_insert_transfer_trb(xhci, udev->port, ep_addr_to_dci(endpoint) - 1, (struct transfer_trb *)data_trb);
		//printk("control transfer:data trb phys = %x\n", trb_phys);
	}

	status_trb->ch = 0;
	status_trb->trb_type = TRB_STATUS_STAGE;
	status_trb->ioc = 1;
	trb_phys = xhci_insert_transfer_trb(xhci, udev->port, ep_addr_to_dci(endpoint) - 1, (struct transfer_trb *)status_trb);
	//printk("control transfer trb phys:%x\n", trb_phys);
	//printk("xhci->port[port].slot_id = %d\n", xhci->port[port].slot_id);
	
	xhci_doorbell_reg_wr32(xhci, xhci->port[udev->port].slot_id * 4, ep_addr_to_dci(endpoint) - 1);
	if (urb->polling_wait == true)
		xhci_wait_transfer_completion(xhci, trb_phys, 10000000);

	return ret;
	//for (int i = 0; i < 0x8000000; i++);
}

int usb_bulk_transfer(struct urb *urb)
{
	struct usb_device *udev = urb->dev;
	struct xhci *xhci = udev->host_controller_context;
	struct transfer_event_trb *transfer_trb;
	struct transfer_trb *normal_trb = kmalloc(sizeof(*normal_trb), GFP_KERNEL);
	int endpoint = urb->pipe & 0xffff;
	u64 trb_phys = VIRT2PHYS(urb->transfer_buffer);
	memset(normal_trb, 0, sizeof(*normal_trb));
	normal_trb->addr = trb_phys;
	normal_trb->trb_transfer_len = urb->transfer_buffer_length;
	normal_trb->isp = 1;
	normal_trb->td_size = 0;
	normal_trb->int_tar = 0;
	normal_trb->ioc = 1;
	normal_trb->trb_type = TRB_NORMAL;
	//printk("dci = %d\n", ep_addr_to_dci(endpoint));
	trb_phys = xhci_insert_transfer_trb(xhci, udev->port, ep_addr_to_dci(endpoint) - 1, normal_trb);
	struct inflight_transfer *transfer = kmalloc(sizeof(*transfer), GFP_KERNEL);
	memset(transfer, 0, sizeof(*transfer));
	transfer->trb_phys = trb_phys;
	transfer->urb = urb;
	list_add(&transfer->list, &xhci->inflight_transfer_list);
	//printk("bulk transfer trb phys:%x\n", trb_phys);
	//printk("xhci->port[port].slot_id = %d\n", xhci->port[port].slot_id);
	//printk("doorbell target:%x\n", ep_addr_to_dci(endpoint));
	xhci_doorbell_reg_wr32(xhci, xhci->port[udev->port].slot_id * 4, ep_addr_to_dci(endpoint));
	if (urb->polling_wait == true)
		transfer_trb = xhci_wait_transfer_completion(xhci, trb_phys, 0xffffffff);
	//printk("transfer result:%d\n", transfer_trb->completion_code);
	//for (int i = 0; i < 0x10000000; i++);
	//return transfer_trb->completion_code;
	return 0;
}

int usb_interrupt_transfer(struct urb *urb)
{
	return usb_bulk_transfer(urb);
}


int usb_get_descriptor(struct usb_device *udev, int descriptor_type, int descriptor_index, void *data, int len)
{
	struct urb *urb = kmalloc(sizeof(*urb), GFP_KERNEL);
	struct usb_ctrlrequest req = {0};
	memset(urb, 0, sizeof(*urb));
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.bRequestType = 0x80;
	req.wIndex = 0;
	req.wLength = len;
	req.wValue = (descriptor_type << 8) | descriptor_index;
	urb->dev = udev;
	urb->setup_packet = (char *)&req;
	urb->polling_wait = true;
	urb->complete = NULL;
	urb->transfer_buffer = data;
	urb->pipe = 0x80000000 | 1;
	usb_control_transfer(urb);
	kfree(urb);
	return 0;
}

int usb_set_config(struct usb_device *udev, int configuration)
{
	struct urb *urb = kmalloc(sizeof(*urb), GFP_KERNEL);
	struct usb_ctrlrequest req = {0};
	memset(urb, 0, sizeof(*urb));
	req.bRequest = USB_REQ_SET_CONFIGURATION;
	req.bRequestType = 0;
	req.wIndex = 0;
	req.wLength = 0;
	urb->dev = udev;
	req.wValue = configuration;
	urb->setup_packet = (char *)&req;
	urb->polling_wait = true;
	urb->complete = NULL;
	urb->transfer_buffer = NULL;
	urb->pipe = 1;
	usb_control_transfer(urb);
	kfree(urb);
	return 0;
}

int usb_set_interface(struct usb_device *udev, int interface)
{
	struct urb *urb = kmalloc(sizeof(*urb), GFP_KERNEL);
	struct usb_ctrlrequest req = {0};
	memset(urb, 0, sizeof(*urb));
	req.bRequest = USB_REQ_SET_INTERFACE;
	req.bRequestType = 0;
	req.wIndex = 0;
	req.wLength = 0;
	req.wValue = interface;
	urb->dev = udev;
	urb->setup_packet = (char *)&req;
	urb->polling_wait = true;
	urb->complete = NULL;
	urb->transfer_buffer = NULL;
	urb->pipe = 1;
	usb_control_transfer(urb);
	kfree(urb);
	return 0;
}

#include <msr.h>
void delay(u64 us)
{
	u64 time1 = rdtscp();
	u64 time2;
	do {
		time2 = rdtscp();
	} while (time2 - time1 < us * 3600);
}

int xhci_address_device(struct xhci *xhci, int slot_id, int root_hub_port_num, int port_speed)
{
	int ret;
	struct input_context *input_context = kmalloc(0x1000, GFP_KERNEL);
	struct transfer_trb *transfer_ring = kmalloc(0x1000, GFP_KERNEL);
	struct device_context *output_context = kmalloc(0x1000, GFP_KERNEL);
	memset(input_context, 0, 0x1000);
	memset(output_context, 0, 0x1000);
	memset(transfer_ring, 0, 0x1000);
	input_context->input_ctrl_context.add_context_flags = 0x3;
	input_context->dev_context.slot_context.root_hub_port_number = root_hub_port_num;
	input_context->dev_context.slot_context.route_string = 0;
	input_context->dev_context.slot_context.context_entries = 1;
	input_context->dev_context.slot_context.speed = port_speed;

	//printk("slot context:\n");
	//hex_dump(&input_context->dev_context, 32);
	input_context->dev_context.endpoint_context[0].tr_dequeue_pointer_lo = VIRT2PHYS(transfer_ring) >> 4;
	input_context->dev_context.endpoint_context[0].tr_dequeue_pointer_hi = (VIRT2PHYS(transfer_ring) >> 32);
	input_context->dev_context.endpoint_context[0].ep_type = 4;
	if (port_speed == 1 || port_speed == 2) {
		input_context->dev_context.endpoint_context[0].max_packet_size = 8;
	} else if (port_speed == 3) {
		input_context->dev_context.endpoint_context[0].max_packet_size = 64;
	} else {
		input_context->dev_context.endpoint_context[0].max_packet_size = 512;
	}
	input_context->dev_context.endpoint_context[0].max_burst_size = 0;
	input_context->dev_context.endpoint_context[0].average_trb_length = 0x1000;
	input_context->dev_context.endpoint_context[0].dcs = 1;
	input_context->dev_context.endpoint_context[0].interval = 0;
	input_context->dev_context.endpoint_context[0].max_pstreams = 0;
	input_context->dev_context.endpoint_context[0].mult = 0;
	input_context->dev_context.endpoint_context[0].cerr = 3;

	//printk("ep0 context:\n");
	//hex_dump(&input_context->dev_context.endpoint_context[0], 32);

	xhci->dcbaa[slot_id] = VIRT2PHYS(output_context);

	xhci->port[root_hub_port_num - 1].input_context = input_context;
	xhci->port[root_hub_port_num - 1].slot_id = slot_id;
	xhci->port[root_hub_port_num - 1].transfer_ring_status[1].transfer_ring_base = transfer_ring;
	xhci->port[root_hub_port_num - 1].transfer_ring_status[1].enquene_pointer = 0;
	xhci->port[root_hub_port_num - 1].transfer_ring_status[1].dcs = 1;

	return xhci_cmd_address_device(xhci, input_context, slot_id);
}

int evaluate_context(struct usb_device *udev)
{
	struct usb_device_descriptor tmp_desc;
	struct xhci *xhci = udev->host_controller_context;
	int slot_id = xhci->port[udev->port].slot_id;
	struct input_context *input_ctx = xhci->port[udev->port].input_context;
	
	usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_DEVICE, 0, &tmp_desc, 8);
	if ((tmp_desc.bcd_usb >> 8) == 0x2 && udev->speed == 1)
		input_ctx->dev_context.endpoint_context[0].max_packet_size = tmp_desc.b_max_packet_size0;
	
	//printk("max_packet_size = %d\n", input_ctx->dev_context.endpoint_context[0].max_packet_size);
	return xhci_cmd_evaluate_context(xhci, input_ctx, slot_id);
}

int configure_all_endpoints(struct usb_device *udev)
{
	struct xhci *xhci = udev->host_controller_context;
	struct usb_endpoint *ep;
	struct usb_interface *interface;
	int i;
	int len, type,intfi;
	int ret;
	char ascii_str[256];
	u16 *string;
	int ep_addr, dci;
	struct usb_interface_descriptor *intf;
	struct usb_endpoint_descriptor *endp;
	
	struct input_context *input_context = kmalloc(0x1000, GFP_KERNEL);
	struct transfer_trb *transfer_ring = kmalloc(0x1000, GFP_KERNEL);
	struct device_context *output_context = kmalloc(0x1000, GFP_KERNEL);
	void *descriptor = kmalloc(512, GFP_KERNEL);
	memset(input_context, 0, 0x1000);
	memset(output_context, 0, 0x1000);
	memset(transfer_ring, 0, 0x1000);
	memset(descriptor, 0, 512);

	usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_DEVICE, 0, &udev->desc, sizeof(struct usb_device_descriptor));
	printk("USB VID = %04x PID = %04x configuration = %d class:%x subclass:%x protocol:%x\n", 
		udev->desc.id_vender, 
		udev->desc.id_product, 
		udev->desc.b_num_configurations, 
		udev->desc.class, 
		udev->desc.subclass, 
		udev->desc.b_device_protocol);

	string = kmalloc(128, GFP_KERNEL);

	if (udev->desc.i_manufacturer != 0) {
		memset(string, 0, 128);
		usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_STRING, udev->desc.i_manufacturer, string, 2);
		usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_STRING, udev->desc.i_manufacturer, string, ((u8 *)string)[0]);
		unicode_to_ascii(&string[1], ascii_str);
		printk("%s\n", ascii_str);
	}

	if (udev->desc.i_product != 0) {
		memset(string, 0, 128);
		usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_STRING, udev->desc.i_product, string, 2);
		usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_STRING, udev->desc.i_product, string, ((u8 *)string)[0]);
		unicode_to_ascii(&string[1], ascii_str);
		printk("%s\n", ascii_str);
	}

	if (udev->desc.i_serial_number != 0) {
		memset(string, 0, 128);
		usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_STRING, udev->desc.i_serial_number, string, 2);
		usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_STRING, udev->desc.i_serial_number, string, ((u8 *)string)[0]);
		unicode_to_ascii(&string[1], ascii_str);
		printk("%s\n", ascii_str);
	}
	memset(descriptor, 0, 512);
	usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, descriptor, sizeof(struct usb_configuration_descriptor));
	struct usb_configuration_descriptor *conf = descriptor;
	printk("w_total = %d, interfaces:%d configuration value:%x i conf:%d max power:%dmA\n", conf->w_total_lenth, conf->b_num_interfaces, conf->b_configuration_value, conf->i_configuration, conf->b_max_power * 2);
	usb_get_descriptor(udev, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, descriptor, conf->w_total_lenth);

	char *endpoint_type[] = {
		"Control",
		"Isochronous",
		"Bulk",
		"Interrupt"
	};

	usb_set_config(udev, conf->b_configuration_value);

	intfi = 0;
	for (i = conf->b_length; i < conf->w_total_lenth; ) {
		len = ((u8 *)descriptor)[i];
		type = ((u8 *)descriptor)[i + 1];
		if (type == USB_DESCRIPTOR_TYPE_INTERFACE) {
			intf = (void *)&((u8 *)descriptor)[i];
			printk("interface number = %d num of endpoints = %d alternate setting:%d class:%x subclass:%x protocol:%x\n", intf->b_interface_number, intf->b_num_endpoints, intf->b_alternate_setting, intf->b_interface_class, intf->b_interface_subclass, intf->b_interface_protocol);
			intfi++;
			interface = kmalloc(sizeof(*interface), GFP_KERNEL);
			memset(interface, 0, sizeof(*interface));
			memcpy(&interface->desc, intf, sizeof(struct usb_interface_descriptor));
			list_add(&interface->list, &udev->interface_head);
			INIT_LIST_HEAD(&interface->endpoint_list);
		}
		if (type == USB_DESCRIPTOR_TYPE_ENDPOINT) {
			endp = (void *)&((u8 *)descriptor)[i];
			printk("endpoint address = %x type = %s max packet size = %d interval = %d\n", endp->b_endpoint_addr, endpoint_type[endp->bm_attributes], endp->w_max_packet_size, endp->b_interval);
			int ep_addr = endp->b_endpoint_addr & 0x1f;
			int dci = ep_addr_to_dci(endp->b_endpoint_addr);
			ep_addr = dci - 1;
			//printk("configuring endpoint %d dci = %d\n", ep_addr, dci);
			transfer_ring = kmalloc(0x1000, GFP_KERNEL);
			memset(transfer_ring, 0, 0x1000);
			//printk("transfer ring:%x\n", transfer_ring);
			input_context = xhci->port[udev->port].input_context;
			memset(input_context, 0, 0x1000);
			input_context->input_ctrl_context.add_context_flags = (1 << dci);
			//printk("input_context->input_ctrl_context.add_context_flags = %x\n", input_context->input_ctrl_context.add_context_flags);
			input_context->dev_context.slot_context.root_hub_port_number = udev->port;
			input_context->dev_context.slot_context.route_string = 0;
			input_context->dev_context.slot_context.context_entries = 5;
			input_context->dev_context.slot_context.speed = udev->speed;
			input_context->dev_context.slot_context.max_exit_latency = 0;
			//printk("slot context:\n");
			//hex_dump(&input_context->dev_context, 32);
			input_context->input_ctrl_context.configuration_value = 0;//conf->b_configuration_value;
			input_context->dev_context.endpoint_context[ep_addr].tr_dequeue_pointer_lo = VIRT2PHYS(transfer_ring) >> 4;
			input_context->dev_context.endpoint_context[ep_addr].tr_dequeue_pointer_hi = (VIRT2PHYS(transfer_ring) >> 32);
			input_context->dev_context.endpoint_context[ep_addr].ep_type = endp->bm_attributes | (endp->b_endpoint_addr & 0x80 ? 0x4 : 0);
			//printk("ep type:%d\n", input_context->dev_context.endpoint_context[ep_addr].ep_type);
			input_context->dev_context.endpoint_context[ep_addr].max_packet_size = endp->w_max_packet_size;
			input_context->dev_context.endpoint_context[ep_addr].max_burst_size = (endp->w_max_packet_size & 0x1800) >> 11;
			input_context->dev_context.endpoint_context[ep_addr].average_trb_length = endp->w_max_packet_size;
			input_context->dev_context.endpoint_context[ep_addr].max_esit_payload_lo = endp->w_max_packet_size * (input_context->dev_context.endpoint_context[ep_addr].max_burst_size + 1);
			input_context->dev_context.endpoint_context[ep_addr].dcs = 1;
			input_context->dev_context.endpoint_context[ep_addr].interval = order_base_2(endp->b_interval) + 3;
			//printk("interval = %d\n", input_context->dev_context.endpoint_context[ep_addr].interval);
			input_context->dev_context.endpoint_context[ep_addr].max_pstreams = 0;
			input_context->dev_context.endpoint_context[ep_addr].mult = 0;
			input_context->dev_context.endpoint_context[ep_addr].cerr = 3;

			//printk("ep %d context:\n", ep_addr);
			//hex_dump(&input_context->dev_context.endpoint_context[ep_addr], 32);
			
			xhci->port[udev->port].slot_id = udev->slot;
			xhci->port[udev->port].transfer_ring_status[ep_addr].transfer_ring_base = transfer_ring;
			xhci->port[udev->port].transfer_ring_status[ep_addr].enquene_pointer = 0;
			xhci->port[udev->port].transfer_ring_status[ep_addr].dcs = 1;

			ret = xhci_cmd_configure_endpoint(xhci, input_context, udev->slot, 0);
			if (ret == 0) {
				ep = kmalloc(sizeof(*ep), GFP_KERNEL);
				memset(ep, 0, sizeof(*ep));
				memcpy(&ep->desc, endp, sizeof(struct usb_endpoint_descriptor));
				INIT_LIST_HEAD(&ep->urb_list);
				list_add(&ep->list, &interface->endpoint_list);
			}
		}
		if (len == 0)
			i += 1;
		i += len;
	}
}

char *usb_port_speed_info[] = {
	"",
	"Full-speed, 12 MB/s, USB 2.0",
	"Low-speed, 1.5 Mb/s, USB 2.0",
	"High-speed, 480 Mb/s, USB 2.0",
	"SuperSpeed Gen1 x1,5 Gb/s, USB3.x",
	"SuperSpeedPlus Gen2 x1, 10 Gb/s USB 3.1",
	"SuperSpeedPlus Gen1 x2, 5 Gb/s, USB 3.2",
	"SuperSpeedPlus Gen2 x2, 10 Gb/s USB 3.2",
};

void usb_kbd_irq(struct urb* urb)
{
	char *key_data = urb->transfer_buffer;
	printk("key recevied.%02x %02x %02x %02x %02x %02x %02x %02x\n", 
		key_data[0], 
		key_data[1], 
		key_data[2], 
		key_data[3], 
		key_data[4], 
		key_data[5],
		key_data[6],
		key_data[7]);
	usb_interrupt_transfer(urb);
}

void usb_mouse_irq(struct urb* urb)
{
	char *key_data = urb->transfer_buffer;
	printk("mouse coordinates recevied.%02x %02x %02x %02x\n", 
		key_data[0], 
		key_data[1], 
		key_data[2], 
		key_data[3]);
	usb_interrupt_transfer(urb);
}

int usb_kbd_test(struct usb_device *udev, int endpoint)
{
	u8 *data = kmalloc(0x1000, GFP_KERNEL);
	struct urb *kbd_cr_urb = kmalloc(sizeof(*kbd_cr_urb), GFP_KERNEL);
	memset(kbd_cr_urb, 0, sizeof(*kbd_cr_urb));
	struct urb *kbd_urb = kmalloc(sizeof(*kbd_cr_urb), GFP_KERNEL);
	memset(kbd_urb, 0, sizeof(*kbd_cr_urb));
	struct usb_ctrlrequest *ureq = kmalloc(sizeof(*ureq), GFP_KERNEL);
	ureq->bRequest = 0x9;
	ureq->bRequestType = (1 << 5) | 1;
	ureq->wValue = 0x200;
	ureq->wLength = 1;
	ureq->wIndex = 0;
	kbd_cr_urb->dev = udev;
	kbd_cr_urb->setup_packet = (char *)ureq;
	kbd_cr_urb->transfer_buffer = data;
	kbd_cr_urb->pipe = 1;
	kbd_cr_urb->polling_wait = true;
	*data = 0x7;
	printk("sending kbd led cmd...\n");
	usb_control_transfer(kbd_cr_urb);
	
	ureq->bRequest = 0xa;
	ureq->bRequestType = (1 << 5) | 1;
	ureq->wValue = 0 << 8;
	ureq->wLength = 0;
	ureq->wIndex = 0;
	usb_control_transfer(kbd_cr_urb);

	memset(data, 0, 8);
	kbd_urb->dev = udev;
	kbd_urb->pipe = endpoint;
	kbd_urb->complete = usb_kbd_irq;
	kbd_urb->polling_wait = false;
	kbd_urb->transfer_buffer = data;
	kbd_urb->transfer_buffer_length = 8;

	usb_interrupt_transfer(kbd_urb);
}

int usb_mouse_test(struct usb_device *udev, int endpoint)
{
	u8 *data = kmalloc(0x1000, GFP_KERNEL);
	struct urb *mouse_cr_urb = kmalloc(sizeof(*mouse_cr_urb), GFP_KERNEL);
	memset(mouse_cr_urb, 0, sizeof(*mouse_cr_urb));
	struct urb *mouse_urb = kmalloc(sizeof(*mouse_urb), GFP_KERNEL);
	memset(mouse_urb, 0, sizeof(*mouse_urb));
	
	memset(data, 0, 8);
	mouse_urb->dev = udev;
	mouse_urb->pipe = endpoint;
	mouse_urb->complete = usb_mouse_irq;
	mouse_urb->polling_wait = false;
	mouse_urb->transfer_buffer = data;
	mouse_urb->transfer_buffer_length = 8;

	usb_interrupt_transfer(mouse_urb);
}


void mass_storage_cmd_irq(struct urb* urb)
{
	printk("usb mass storage:transfer cmd complete.\n");
}

void mass_storage_data_irq(struct urb* urb)
{
	int i;
	printk("usb mass storage:transfer data complete.\n");
	hex_dump(urb->transfer_buffer + 512 - 16, 16);
}

void mass_storage_status_irq(struct urb* urb)
{
	struct usbmassbulk_csw *csw = urb->transfer_buffer;
	printk("transfer status complete.\n");
	printk("csw.tag = %x\n", csw->tag);
	printk("csw.status = %x\n", csw->status);
}

int usb_mass_storage_read_cap(struct usb_device *udev)
{
	struct urb *blkin_data_urb = kmalloc(sizeof(*blkin_data_urb), GFP_KERNEL);
	struct urb *blkin_status_urb = kmalloc(sizeof(*blkin_status_urb), GFP_KERNEL);
	struct urb *blkout_urb = kmalloc(sizeof(*blkout_urb), GFP_KERNEL);
	struct usbmassbulk_cbw *cbw = kmalloc(sizeof(*cbw), GFP_KERNEL);
	struct usbmassbulk_csw *csw = kmalloc(sizeof(*csw), GFP_KERNEL);
	struct ufi_cmd scsi_cmd = {0};
	u8 *disk_buf;
	u64 cap;

	memset(cbw, 0, sizeof(*cbw));
	memset(csw, 0, sizeof(*csw));
	cbw->sig = 0x43425355;
	cbw->tag = 0x12345678;
	cbw->flags = (1 << 7);
	cbw->length = 0xc;
	cbw->data_transfer_len = 8;
	
	scsi_cmd.logical_block_addr = 0;
	scsi_cmd.logical_unit_number = 0;
	scsi_cmd.op_code = SCSI_READ_CAPACITY;
	scsi_cmd.parameter = 0x01000000;
	memcpy(&cbw->cb, &scsi_cmd, sizeof(scsi_cmd));
	disk_buf = kmalloc(0x2000, GFP_KERNEL);
	memset(disk_buf, 0, 0x2000);

	blkout_urb->dev = udev;
	blkout_urb->pipe = 0x2;
	blkout_urb->setup_packet = NULL;
	blkout_urb->polling_wait = true;
	blkout_urb->transfer_buffer = cbw;
	blkout_urb->transfer_buffer_length = sizeof(*cbw);
	blkout_urb->complete = NULL;
	usb_bulk_transfer(blkout_urb);


	blkin_data_urb->dev = udev;
	blkin_data_urb->pipe = 0x80000000 | 0x81;
	blkin_data_urb->setup_packet = NULL;
	blkin_data_urb->polling_wait = true;
	blkin_data_urb->transfer_buffer = disk_buf;
	blkin_data_urb->transfer_buffer_length = 8;
	blkin_data_urb->complete = NULL;
	usb_bulk_transfer(blkin_data_urb);

	blkin_status_urb->dev = udev;
	blkin_status_urb->pipe = 0x80000000 | 0x81;
	blkin_status_urb->setup_packet = NULL;
	blkin_status_urb->polling_wait = true;
	blkin_status_urb->transfer_buffer = csw;
	blkin_status_urb->transfer_buffer_length = sizeof(*csw);
	blkin_status_urb->complete = NULL;
	usb_bulk_transfer(blkin_status_urb);

	cap = disk_buf[3] | (disk_buf[2] << 8) | (disk_buf[1] << 16) | (disk_buf[0] << 24);
	cap *= disk_buf[5] | (disk_buf[6] << 8) | (disk_buf[7] << 16) | (disk_buf[8] << 24);
	printk("CAPACITY:%d GB.\n", cap / 1024 / 1024 / 1024);
}

int usb_mass_storage_read_sector(struct usb_device *udev, int sector)
{
	struct urb *blkin_data_urb = kmalloc(sizeof(*blkin_data_urb), GFP_KERNEL);
	struct urb *blkin_status_urb = kmalloc(sizeof(*blkin_status_urb), GFP_KERNEL);
	struct urb *blkout_urb = kmalloc(sizeof(*blkout_urb), GFP_KERNEL);
	struct usbmassbulk_cbw *cbw = kmalloc(sizeof(*cbw), GFP_KERNEL);
	struct usbmassbulk_csw *csw = kmalloc(sizeof(*csw), GFP_KERNEL);
	struct ufi_cmd scsi_cmd = {0};
	u8 *disk_buf = kmalloc(0x2000, GFP_KERNEL);;
	u64 cap;

	memset(cbw, 0, sizeof(*cbw));
	memset(csw, 0, sizeof(*csw));
	cbw->sig = 0x43425355;
	cbw->tag = 0x12345678;
	cbw->flags = (1 << 7);
	cbw->length = 0xc;
	cbw->data_transfer_len = 0x200;
	scsi_cmd.logical_block_addr = sector;
	scsi_cmd.logical_unit_number = 0;
	scsi_cmd.op_code = SCSI_READ;
	scsi_cmd.parameter = 0x01000000;
	memcpy(&cbw->cb, &scsi_cmd, sizeof(scsi_cmd));
	memset(disk_buf, 0, 0x2000);
	blkout_urb->dev = udev;
	blkout_urb->pipe = 0x2;
	blkout_urb->setup_packet = NULL;
	blkout_urb->polling_wait = true;
	blkout_urb->transfer_buffer = cbw;
	blkout_urb->transfer_buffer_length = sizeof(*cbw);
	blkout_urb->complete = NULL;
	blkout_urb->polling_wait = false;
	usb_bulk_transfer(blkout_urb);

	blkin_data_urb->dev = udev;
	blkin_data_urb->pipe = 0x80000000 | 0x81;
	blkin_data_urb->setup_packet = NULL;
	blkin_data_urb->transfer_buffer = disk_buf;
	blkin_data_urb->transfer_buffer_length = 0x200;
	blkin_data_urb->polling_wait = false;
	blkin_data_urb->complete = mass_storage_data_irq;
	usb_bulk_transfer(blkin_data_urb);

	blkin_status_urb->dev = udev;
	blkin_status_urb->pipe = 0x80000000 | 0x81;
	blkin_status_urb->setup_packet = NULL;
	blkin_status_urb->transfer_buffer = csw;
	blkin_status_urb->transfer_buffer_length = sizeof(*csw);
	blkin_status_urb->polling_wait = false;
	blkin_status_urb->complete = mass_storage_status_irq;
	usb_bulk_transfer(blkin_status_urb);
}


int usb_massstorage_test(struct usb_device *udev)
{
	struct urb *bbb_urb = kmalloc(sizeof(*bbb_urb), GFP_KERNEL);
	struct usb_ctrlrequest *ureq = kmalloc(sizeof(*ureq), GFP_KERNEL);

	ureq->bRequest = 0xff;
	ureq->bRequestType = (1 << 5) | 1;
	ureq->wValue = 0 << 8;
	ureq->wLength = 0;
	ureq->wIndex = 0;
	bbb_urb->dev = udev;
	bbb_urb->pipe = 1;
	bbb_urb->setup_packet = (char *)ureq;
	bbb_urb->transfer_buffer = NULL;
	bbb_urb->polling_wait = true;
	usb_control_transfer(bbb_urb);

	usb_mass_storage_read_cap(udev);
	usb_mass_storage_read_sector(udev, 0);
}

int usb_device_init(struct xhci *xhci, int slot_id, int root_hub_port_num, int port_speed)
{
	struct usb_device *udev;
	int ret;
	ret = xhci_address_device(xhci, slot_id, root_hub_port_num, port_speed);
	if (ret != 0) {
		printk("address device failed!\n");
		return -EFAULT;
	}
	udev = kmalloc(sizeof(*udev), GFP_KERNEL);
	memset(udev, 0, sizeof(*udev));
	INIT_LIST_HEAD(&udev->interface_head);
	INIT_LIST_HEAD(&udev->list);
	udev->port = root_hub_port_num - 1;
	udev->speed = port_speed;
	udev->slot = slot_id;
	udev->host_controller_context = xhci;
	printk("usb device at port %d slot:%d speed %s registered.\n", 
		udev->port, 
		udev->slot, 
		usb_port_speed_info[udev->speed]
	);

	list_add(&udev->list, &usb_dev_list);
	evaluate_context(udev);
	configure_all_endpoints(udev);

	struct usb_interface *intf;
	struct usb_endpoint *endp;
	list_for_each_entry(intf, &udev->interface_head, list) {
		if ((intf->desc.b_interface_class == 3) && 
			(intf->desc.b_interface_subclass == 1) && 
			(intf->desc.b_interface_protocol == 1)) {
			list_for_each_entry(endp, &intf->endpoint_list, list) {
				if (endp->desc.bm_attributes == 3)
					usb_kbd_test(udev, endp->desc.b_endpoint_addr);
			}
		}
		if ((intf->desc.b_interface_class == 3) && 
			(intf->desc.b_interface_subclass == 1) && 
			(intf->desc.b_interface_protocol == 2)) {
			list_for_each_entry(endp, &intf->endpoint_list, list) {
				if (endp->desc.bm_attributes == 3)
					usb_mouse_test(udev, endp->desc.b_endpoint_addr);
			}
		}
		if ((intf->desc.b_interface_class == 8) && 
			(intf->desc.b_interface_subclass == 6) && 
			(intf->desc.b_interface_protocol == 0x50)) {
			usb_massstorage_test(udev);
		}
	}
	return 0;
}

int usb_device_remove(struct xhci *xhci, int slot_id, int root_hub_port_num)
{
	struct usb_device *udev, *udevp;
	void *slot_output = (void *)PHYS2VIRT(xhci->dcbaa[slot_id]);
	list_for_each_entry(udevp, &usb_dev_list, list) {
		if (udevp->port == root_hub_port_num - 1) {
			udev = udevp;
			break;
		}
	}

	if (udev != NULL) {
		printk("usb device remove.port %d\n", udev->port);
		kfree(slot_output);
		//TODO free all allocated resources.
		
		list_del(&udev->list);
		kfree(udev);
	}
}

int submit_urb(struct urb *urb)
{
	switch (urb->ep->desc.b_descriptor_type) {
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
	}
}

int handle_transfer_completion(struct xhci *xhci, struct transfer_event_trb *event_trb)
{
	//printk("transfer completion. trb_pointer = %x completion code = %d\n", event_trb->trb_pointer, event_trb->completion_code);
	u8 data[128];
	struct trb *trb = (void *)PHYS2VIRT(event_trb->trb_pointer);
	struct inflight_transfer *transfer;
	struct inflight_transfer *transfer_tmp;
	struct urb *urb = NULL;
	list_for_each_entry_safe(transfer, transfer_tmp, &xhci->inflight_transfer_list, list) {
		if (transfer->trb_phys == event_trb->trb_pointer) {
			urb = transfer->urb;
			list_del(&transfer->list);
			kfree(transfer);
			if (urb->complete)
				urb->complete(urb);
		}
	}
	return 0;
}

int handle_port_status_change(struct xhci *xhci, struct port_status_change_event_trb *port_ch_trb)
{
	int port;
	int slot;
	u32 port_status;
	int port_speed;
	port = port_ch_trb[0].port_id - 1;
	port_status = xhci_opreg_rd32(xhci, XHCI_HC_PORT_REG + port * 0x10);
	port_speed = (port_status >> 10) & 0xf;
	//printk("XHCI Port %x %s\n", port, port_status & 0x1 ? "connected" : "disconnected");
	//printk("port_status = %x\n", port_status);

	if (port_status & XHCI_PORTSC_CCS) {
		/* USB3 device doesn't need to be reset. */
		if (port_speed < 4) {
			xhci_opreg_wr32(xhci, 0x400 + port * 0x10, XHCI_PORTSC_PR | XHCI_PORTSC_PP);
			while (1) {
				port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
				
				if ((port_status & XHCI_PORTSC_PRC) != 0) {
					break;
				}
			}
		}
		xhci_opreg_wr32(xhci, XHCI_HC_PORT_REG + port * 0x10, XHCI_PORTSC_PRC | XHCI_PORTSC_CSC | XHCI_PORTSC_PP);
		delay(1000);
		slot = xhci_cmd_enable_slot(xhci);
		//printk("available slot:%d\n", slot);
		usb_device_init(xhci, slot, port + 1, (port_status >> 10) & 0xf);
		xhci->port[port].slot_id = slot;
	} else {
		xhci_cmd_disable_slot(xhci, xhci->port[port].slot_id);
		usb_device_remove(xhci, slot, port + 1);
		xhci_opreg_wr32(xhci, XHCI_HC_PORT_REG + port * 0x10, port_status | XHCI_PORTSC_PRC | XHCI_PORTSC_CSC | XHCI_PORTSC_PP);
	}
}

int handle_cmd_completion(struct xhci *xhci, struct port_status_change_event_trb *port_ch_trb)
{
	//printk("xhci command completion\n");
	return 0;
}

int xhci_handle_event(struct xhci *xhci, struct trb_template *event)
{
	int ret = 0;
	//printk("event->trb_type = %d\n", event->trb_type);
	switch(event->trb_type) {
		case TRB_PORT_STATUS_CHANGE_EVENT:
			ret = handle_port_status_change(xhci, (void *)event);
			break;
		case TRB_COMMAND_COMPLETION_EVENT:
			ret = handle_cmd_completion(xhci, (void *)event);
			break;
		case TRB_TRANSFER_EVENT:
			ret = handle_transfer_completion(xhci, (void *)event);
			break;
		default:
			printk("unknow event %d\n", event->trb_type);
			break;
	}
	return ret;
}

int xhci_intr(int irq, void *data)
{
	struct xhci *xhci = data;
	u32 usb_sts = xhci_opreg_rd32(xhci, XHCI_HC_USBSTS);
	u32 port;
	u32 port_status;
	u32 slot;
	u64 erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	//printk("xhci_intr (%d).USB STS = %x\n", irq, usb_sts);
	struct trb_template *current_trb = (void *)PHYS2VIRT(erdp);

	u32 ir = xhci_rtreg_rd32(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP);
	//erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);

	int i = 0;
	u64 last_erdp = erdp;
	u64 delta = 0;
	//printk("erdp = %x\n", erdp);
	while (erdp + 16 != last_erdp) {
		if (erdp + i * 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
			if (current_trb[i].c == (xhci->event_ring_ccs & BIT0)) {
				u32 *trb = (u32 *)&current_trb[i];
				//printk("%d:%08x %08x %08x %08x\n", (erdp + i * 16 - xhci->event_ring_seg_table[0].ring_segment_base_addr) / 16, trb[0], trb[1], trb[2], trb[3]);
				xhci_handle_event(xhci, &current_trb[i]);
				delta += 16;
				xhci->event_ring_dequeue_ptr++;
				i++;
			} else {
				break;
			}
		}else {
			//printk("event ring wrap back.\n");
			erdp = xhci->event_ring_seg_table->ring_segment_base_addr;
			xhci->event_ring_ccs = ~xhci->event_ring_ccs;
			xhci->event_ring_dequeue_ptr = 0;
			current_trb = (void *)PHYS2VIRT(erdp);
			delta = 0;
			i = 0;
		}
	}
	//usb_sts = xhci_opreg_rd32(xhci, XHCI_HC_USBSTS);
	//printk("xhci_intr.USB STS = %x\n", usb_sts);
	//xhci_opreg_wr32(xhci, XHCI_HC_PORT_REG + port * 0x10, BIT9 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22);
	xhci_rtreg_wr64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP, (erdp + delta) | BIT3);
	xhci_opreg_wr32(xhci, XHCI_HC_USBSTS, usb_sts | BIT3 | BIT4);
	return 0;
}

int xhci_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	u32 len_version, hcs_params1, hcs_params2, hcs_params3, hcc_params1, hcc_params2;
	int ret, i;
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
	hcc_params2 = xhci_cap_rd32(xhci, XHCI_HCCPARAMS2);
	printk("XHCI HCSPARAMS1:%x\n", hcs_params1);
	printk("XHCI HCSPARAMS2:%x\n", hcs_params2);
	printk("XHCI HCSPARAMS3:%x\n", hcs_params3);
	printk("XHCI HCCPARAMS1:%x\n", hcc_params1);
	printk("XHCI HCCPARAMS2:%x\n", hcc_params2);
	xhci->hc_ext_reg_offset = hcc_params1 >> 16;
	printk("xHCI extended ptr:%x\n", xhci->hc_ext_reg_offset);

	xhci->hc_doorbell_reg_offset = xhci_cap_rd32(xhci, XHCI_DBOFF);
	xhci->hc_rt_reg_offset = xhci_cap_rd32(xhci, XHCI_RTSOFF);
	xhci->hc_vt_reg_offset = xhci_cap_rd32(xhci, 0x20);

	printk("doorbell offset:%x\n", xhci->hc_doorbell_reg_offset);
	printk("runtime_reg_offset offset:%x\n", xhci->hc_rt_reg_offset);
	printk("xhci_vtio_offset offset:%x\n", xhci->hc_vt_reg_offset);

	xhci_opreg_wr32(xhci, XHCI_HC_USBCMD, 0);
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
	for (i = 0; i < 256; i++) {
		xhci->dcbaa[i] = 0;
	}

	u64 *scrach_buffer = kmalloc(0x1000, GFP_KERNEL);
	memset(scrach_buffer, 0, 0x1000);
	for (i = 0; i < xhci->max_scratch_buffer_cnt; i++) {
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
	xhci->cmd_ring_dcs = 1;
	memset(xhci->cmd_ring, 0, xhci->cmd_ring_size);
	xhci_opreg_wr64(xhci, XHCI_HC_CRCR, VIRT2PHYS(xhci->cmd_ring) | (xhci->cmd_ring_dcs));

	xhci->event_ring_seg_table = kmalloc(0x1000, GFP_KERNEL);
	memset(xhci->event_ring_seg_table, 0, 0x1000);
	xhci->event_ring_size = 0x1000;
	xhci->event_ring = kmalloc(xhci->event_ring_size, GFP_KERNEL);
	xhci->event_ring_dequeue_ptr = xhci->event_ring;
	memset(xhci->event_ring, 0, xhci->event_ring_size);
	xhci->event_ring_seg_table[0].ring_segment_base_addr = VIRT2PHYS(xhci->event_ring);
	xhci->event_ring_seg_table[0].ring_segment_size = xhci->event_ring_size / 16;
	xhci->event_ring_ccs = 1;
	for (i = 0; i < xhci->nr_intr; i++) {
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTSZ, 1);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERDP, xhci->event_ring_seg_table[0].ring_segment_base_addr | BIT3);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTBA, VIRT2PHYS(xhci->event_ring_seg_table));
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMOD, 1000);
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMAN, XHCI_HC_IR_IMAN_IE);
	}

	xhci_opreg_wr32(xhci, XHCI_HC_DNCTRL, 0x2);
	xhci_opreg_wr32(xhci, XHCI_HC_USBCMD, XHCI_HC_USBCMD_RUN | XHCI_HC_USBCMD_INTE | XHCI_HC_USBCMD_HSEE);

	INIT_LIST_HEAD(&xhci->inflight_transfer_list);
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
	usb_core_init();
	pci_register_driver(&xhci_host_driver);
}

module_init(xhci_init);
