#include "xhci.h"


struct command_completion_event_trb *xhci_wait_cmd_completion(struct xhci *xhci, int cmd_index, int timeout)
{
	u64 erdp;
	int i = 0, j = 0x10000000;
	struct trb_template *current_trb;
	erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	current_trb = (void *)PHYS2VIRT(erdp);
	u64 cmd_trb_phys = VIRT2PHYS(&xhci->cmd_ring[cmd_index]);
	int ccs = xhci->event_ring_ccs;
	while (j--) {
		ccs = xhci->event_ring_ccs;
		while (erdp + i * 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_COMMAND_COMPLETION_EVENT) && (current_trb[i].parameter == cmd_trb_phys)) {
					return (struct command_completion_event_trb *)&current_trb[i];
				}
				i++;
			}
		}
		current_trb = (void *)PHYS2VIRT(xhci->event_ring_seg_table[0].ring_segment_base_addr);
		ccs = ~ccs;
		i = 0;
		while (xhci->event_ring_seg_table[0].ring_segment_base_addr + i * 16 <= erdp - 16) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_COMMAND_COMPLETION_EVENT) && (current_trb[i].parameter == cmd_trb_phys)) {
					return (struct command_completion_event_trb *)&current_trb[i];
				}
				i++;
			}
		}
	}
		
	return NULL;
}

struct transfer_event_trb *xhci_wait_transfer_completion(struct xhci *xhci, u64 trb_phys_addr, int timeout)
{
	u64 erdp;
	int i = 0, j = 0x10000000;
	struct trb_template *current_trb;
	erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	current_trb = (void *)PHYS2VIRT(erdp);
	int ccs = xhci->event_ring_ccs;
	while (j--) {
		ccs = xhci->event_ring_ccs;
		while (erdp + i * 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_TRANSFER_EVENT) && (current_trb[i].parameter == trb_phys_addr)) {
					return (struct transfer_event_trb *)&current_trb[i];
				}
				i++;
			}
		}
		current_trb = (void *)PHYS2VIRT(xhci->event_ring_seg_table[0].ring_segment_base_addr);
		ccs = ~ccs;
		i = 0;
		while (xhci->event_ring_seg_table[0].ring_segment_base_addr + i * 16 <= erdp - 16) {
			if (current_trb[i].c == (ccs & BIT0)) {
				if ((current_trb[i].trb_type == TRB_TRANSFER_EVENT) && (current_trb[i].parameter == trb_phys_addr)) {
					return (struct transfer_event_trb *)&current_trb[i];
				}
				i++;
			}
		}
	}
	printk("trb %x time out.\n", trb_phys_addr);
	return NULL;
}


int xhci_enable_slot(struct xhci *xhci)
{
	struct trb_template cmd = {0};
	int cmd_ring_index;
	struct command_completion_event_trb *completion_trb;
	int i = 0;
	int slot = -1;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_ENABLE_SLOT_CMD;
	cmd_ring_index = xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		slot = -1;
	slot = completion_trb->slot_id;
	return slot;
}

int xhci_disable_slot(struct xhci *xhci, int slot)
{
	struct trb_template cmd = {0};
	int cmd_ring_index;
	struct command_completion_event_trb *completion_trb;
	int i = 0;
	memset(&cmd, 0, sizeof(cmd));
	cmd.trb_type = TRB_DISABLE_SLOT_CMD;
	cmd.control = (slot << 8);
	cmd_ring_index = xhci_cmd_ring_insert(xhci, &cmd);
	xhci_doorbell_reg_wr32(xhci, 0, 0);
	
	completion_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
	if ((completion_trb == NULL) || (completion_trb->completion_code != 0x1))
		return -1;
	return 0;
}

int xhci_cmd_ring_insert(struct xhci *xhci, struct trb_template *cmd)
{
	int index;
	index = xhci->cmd_ring_enqueue_ptr;
	printk("cmd index = %d\n", index);
	if (index == 0x1000 / sizeof(struct trb_template) - 1) {
		printk("cmd ring overrun.\n");
		memset(&xhci->cmd_ring[0], 0, 0x1000);
		struct link_trb link_trb = {0};
		link_trb.c = 1;
		link_trb.trb_type = TRB_LINK;
		link_trb.ring_seg_pointer = VIRT2PHYS(xhci->cmd_ring);
		xhci->cmd_ring[index] = *((struct trb_template *)&link_trb);
		index = 0;
		xhci->cmd_ring_enqueue_ptr = 0;
	}
	xhci->cmd_ring[index] = *cmd;
	xhci->cmd_ring[index].c = 1;
	xhci->cmd_ring_enqueue_ptr++;
	return index;
}

u64 xhci_insert_transfer_trb(struct xhci *xhci, int port, int endpoint, struct transfer_trb *trb_in)
{
	u64 phys;
	int index = xhci->port[port].transfer_ring_status[endpoint].enquene_pointer;
	struct transfer_trb *trb = &xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base[index];
	if (index == 0x1000 / sizeof(struct transfer_trb) - 1) {
		printk("transfer ring overrun.\n");
		memset(&xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base[0], 0, 0x1000);
		struct link_trb link_trb = {0};
		link_trb.c = 1;
		link_trb.trb_type = TRB_LINK;
		link_trb.ring_seg_pointer = VIRT2PHYS(xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base);
		*trb = *((struct transfer_trb *)&link_trb);
		index = 0;
		xhci->port[port].transfer_ring_status[endpoint].enquene_pointer = 0;
	}
	trb = &xhci->port[port].transfer_ring_status[endpoint].transfer_ring_base[index];
	*trb = *trb_in;
	phys = VIRT2PHYS(trb);
	//printk("trb phys:%x\n", VIRT2PHYS(trb));
	xhci->port[port].transfer_ring_status[endpoint].enquene_pointer++;

	return phys;
}

struct endpoint_context *usb_get_context(struct xhci *xhci, int slot_id, int endpoint)
{
	struct device_context *output_context = (void *)PHYS2VIRT(xhci->dcbaa[slot_id]);
	struct endpoint_context *context = &output_context->endpoint_context[ep_addr_to_dci(endpoint) - 1];
	return context;
}

int usb_control_transfer(struct xhci *xhci, int port, struct urb *urb, bool direction, void *data)
{
	struct setup_stage_trb *setup_trb = kmalloc(sizeof(*setup_trb), GFP_KERNEL);
	struct data_stage_trb *data_trb;
	struct status_stage_trb *status_trb = kmalloc(sizeof(*status_trb), GFP_KERNEL);
	u64 trb_phys;
	memset(setup_trb, 0, sizeof(*setup_trb));
	memset(status_trb, 0, sizeof(*status_trb));
	setup_trb->c = 1;
	setup_trb->b_request = urb->b_request;
	setup_trb->bm_request_type = urb->bm_request_type;
	setup_trb->w_value = urb->w_value;
	setup_trb->w_index = urb->w_index;
	setup_trb->w_length = urb->w_length;
	setup_trb->trb_type = TRB_SETUP_STAGE;
	setup_trb->trb_transfer_len = 8;
	setup_trb->idt = 1;

	/* 4.11.2.2 Table 4-7 */
	if (direction == 0) {
		if (urb->w_length == 0) {
			setup_trb->trt = 0;
		} else {
			setup_trb->trt = 2;
		}
		status_trb->dir = 1;
	} else {
		if (urb->w_length == 0) {
			setup_trb->trt = 0;
		} else {
			setup_trb->trt = 3;
		}
		status_trb->dir = 0;
	}	

	//setup_trb->ioc = 1;
	xhci_insert_transfer_trb(xhci, port, 1, (struct transfer_trb *)setup_trb);

	if (data != NULL) {
		data_trb = kmalloc(sizeof(*data_trb), GFP_KERNEL);
		memset(data_trb, 0, sizeof(*data_trb));
		data_trb->c = 1;
		data_trb->data_buffer_addr = VIRT2PHYS(data);
		data_trb->dir = status_trb->dir == 0 ? 1 : 0;
		data_trb->trb_type = TRB_DATA_STAGE;
		data_trb->trb_transfer_len = urb->w_length;
		data_trb->ioc = 1;
		trb_phys = xhci_insert_transfer_trb(xhci, port, 1, (struct transfer_trb *)data_trb);
		//printk("control transfer:data trb phys = %x\n", trb_phys);
	}

	status_trb->c = 1;
	status_trb->ch = 0;
	status_trb->trb_type = TRB_STATUS_STAGE;
	status_trb->ioc = 1;
	trb_phys = xhci_insert_transfer_trb(xhci, port, 1, (struct transfer_trb *)status_trb);
	//printk("control transfer trb phys:%x\n", trb_phys);
	//printk("xhci->port[port].slot_id = %d\n", xhci->port[port].slot_id);
	xhci_doorbell_reg_wr32(xhci, xhci->port[port].slot_id * 4, 1);
	xhci_wait_transfer_completion(xhci, trb_phys, 0);
	//for (int i = 0; i < 0x8000000; i++);
}

int usb_bulk_transfer(struct xhci *xhci, int port, int endpoint, void *data, int len)
{
	struct transfer_event_trb *transfer_trb;
	struct transfer_trb *normal_trb = kmalloc(sizeof(*normal_trb), GFP_KERNEL);
	u64 trb_phys = VIRT2PHYS(data);
	memset(normal_trb, 0, sizeof(*normal_trb));
	normal_trb->addr = trb_phys;
	normal_trb->trb_transfer_len = len;
	normal_trb->c = 1;
	normal_trb->isp = 1;
	normal_trb->td_size = 0;
	normal_trb->int_tar = 0;
	normal_trb->ioc = 1;
	normal_trb->trb_type = TRB_NORMAL;
	//printk("dci = %d\n", ep_addr_to_dci(endpoint));
	trb_phys = xhci_insert_transfer_trb(xhci, port, ep_addr_to_dci(endpoint) - 1, normal_trb);
	//printk("bulk transfer trb phys:%x\n", trb_phys);
	//printk("xhci->port[port].slot_id = %d\n", xhci->port[port].slot_id);
	//printk("doorbell target:%x\n", ep_addr_to_dci(endpoint));
	xhci_doorbell_reg_wr32(xhci, xhci->port[port].slot_id * 4, ep_addr_to_dci(endpoint));
	transfer_trb = xhci_wait_transfer_completion(xhci, trb_phys, 0);
	//printk("transfer result:%d\n", transfer_trb->completion_code);
	//for (int i = 0; i < 0x10000000; i++);
	return transfer_trb->completion_code;
}

int usb_interrupt_transfer(struct xhci *xhci, int port, int endpoint, void *data, int len)
{
	return usb_bulk_transfer(xhci, port, endpoint, data, len);
}


int usb_get_descriptor(struct xhci *xhci, int port, int descriptor_type, int descriptor_index, void *data, int len)
{
	struct urb *urb = kmalloc(sizeof(*urb), GFP_KERNEL);
	memset(urb, 0, sizeof(*urb));
	urb->b_request = USB_REQ_GET_DESCRIPTOR;
	urb->bm_request_type = 0x80;
	urb->w_value = (descriptor_type << 8) | descriptor_index;
	urb->w_index = 0;
	urb->w_length = len;
	usb_control_transfer(xhci, port, urb, 1, data);
	//kfree(urb);
	return 0;
}

int usb_set_config(struct xhci *xhci, int port, int configuration)
{
	struct urb *urb = kmalloc(sizeof(*urb), GFP_KERNEL);
	memset(urb, 0, sizeof(*urb));
	urb->b_request = USB_REQ_SET_CONFIGURATION;
	urb->bm_request_type = 0;
	urb->w_value = configuration;
	urb->w_index = 0;
	urb->w_length = 0;
	usb_control_transfer(xhci, port, urb, 0, NULL);
	//kfree(urb);
	return 0;
}

int usb_set_interface(struct xhci *xhci, int port, int interface)
{
	struct urb *urb = kmalloc(sizeof(*urb), GFP_KERNEL);
	memset(urb, 0, sizeof(*urb));
	urb->b_request = USB_REQ_SET_INTERFACE;
	urb->bm_request_type = 0;
	urb->w_value = interface;
	urb->w_index = 0;
	urb->w_length = 0;
	usb_control_transfer(xhci, port, urb, 0, NULL);
	//kfree(urb);
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

int init_device_slot(struct xhci *xhci, int slot_id, int root_hub_port_num, int port_speed)
{
	int dev_type = -1;
	struct command_completion_event_trb *comp_trb;
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
	input_context->dev_context.slot_context.speed = 0;
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

	xhci->dcbaa[slot_id] = VIRT2PHYS(output_context);

	struct address_device_trb address_dev_trb = {0};
	address_dev_trb.input_context_ptr_lo = VIRT2PHYS(input_context);
	address_dev_trb.input_context_ptr_hi = VIRT2PHYS(input_context) >> 32;
	address_dev_trb.trb_type = TRB_ADDRESS_DEVICE_CMD;
	address_dev_trb.slot_id = slot_id;
	address_dev_trb.bsr = 0;
	u64 cmd_ring_index = xhci_cmd_ring_insert(xhci, (struct trb_template *)&address_dev_trb);

	xhci->port[root_hub_port_num - 1].slot_id = slot_id;
	xhci->port[root_hub_port_num - 1].transfer_ring_status[1].transfer_ring_base = transfer_ring;
	xhci->port[root_hub_port_num - 1].transfer_ring_status[1].enquene_pointer = 0;

	xhci_doorbell_reg_wr32(xhci, 0, 0);
	comp_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
	printk("%d:trb result:%d\n", __LINE__, comp_trb->completion_code);
	// device context debug
	//int i;
	//for (i = 0; i < 0x100; i++) {
	//	printk("%08x,", ((u32 *)output_context)[i]);
	//}

#if 0
	struct no_op_trb *no_op_trb = (struct no_op_trb *)&transfer_ring[0];
	memset(no_op_trb, 0, sizeof(*no_op_trb));
	no_op_trb->c = 1;
	no_op_trb->ent = 0;
	no_op_trb->int_tar = 0;
	no_op_trb->ch = 0;
	no_op_trb->ioc = 1;
	no_op_trb->trb_type = TRB_NO_OP;
	xhci_doorbell_reg_wr32(xhci, slot_id * 4, 1);
#endif

#if 1
	int i;
	void *descriptor = kmalloc(512, GFP_KERNEL);
	memset(descriptor, 0, 512);

	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_DEVICE, 0, descriptor, sizeof(struct usb_device_descriptor));
	struct usb_device_descriptor *dev_desc = descriptor;
	printk("USB VID = %04x PID = %04x configuration = %d class:%x subclass:%x protocol:%x\n", dev_desc->id_vender, dev_desc->id_product, dev_desc->b_num_configurations, dev_desc->class, dev_desc->subclass, dev_desc->b_device_protocol);
	//for (int i = 0; i < 18; i++) {
	//	printk("%02x ", ((u8 *)descriptor)[i]);
	//}
	//printk("\n");
	u16 *string = kmalloc(128, GFP_KERNEL);
	char ascii_str[256];
	memset(string, 0, 128);
	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_STRING, dev_desc->i_manufacturer, string, 2);
	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_STRING, dev_desc->i_manufacturer, string, ((u8 *)string)[0]);
	unicode_to_ascii(&string[1], ascii_str);
	printk("%s ", ascii_str);

	memset(string, 0, 128);
	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_STRING, dev_desc->i_product, string, 2);
	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_STRING, dev_desc->i_product, string, ((u8 *)string)[0]);
	unicode_to_ascii(&string[1], ascii_str);
	printk("%s ", ascii_str);

	if (dev_desc->i_serial_number != 0) {
		memset(string, 0, 128);
		usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_STRING, dev_desc->i_serial_number, string, 2);
		usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_STRING, dev_desc->i_serial_number, string, ((u8 *)string)[0]);
		unicode_to_ascii(&string[1], ascii_str);
		printk("%s", ascii_str);
	}
	printk("\n");
	memset(descriptor, 0, 512);
	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, descriptor, sizeof(struct usb_configuration_descriptor));
	struct usb_configuration_descriptor *conf = descriptor;
	printk("w_total = %d, interfaces:%d configuration value:%x i conf:%d max power:%dmA\n", conf->w_total_lenth, conf->b_num_interfaces, conf->b_configuration_value, conf->i_configuration, conf->b_max_power * 2);
	usb_get_descriptor(xhci, root_hub_port_num - 1, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, descriptor, conf->w_total_lenth);
	for (int i = 0; i < conf->w_total_lenth; i++) {
		printk("%02x ", ((u8 *)descriptor)[i]);
	}
	printk("\n");
	char *endpoint_type[] = {
		"Control",
		"Isochronous",
		"Bulk",
		"Interrupt"
	};
#endif
	//for (int i = 0; i < 0x40000000; i++);

#if 1
	usb_set_config(xhci, root_hub_port_num - 1, conf->b_configuration_value);

	//usb_set_interface(xhci, root_hub_port_num - 1, 2);
	int intfi = 0;
	for (int i = conf->b_length; i < conf->w_total_lenth; ) {
		int len = ((u8 *)descriptor)[i];
		int type = ((u8 *)descriptor)[i + 1];
		if (type == USB_DESCRIPTOR_TYPE_INTERFACE) {
			struct usb_interface_descriptor *intf = (void *)&((u8 *)descriptor)[i];
			printk("interface number = %d num of endpoints = %d alternate setting:%d class:%x subclass:%x protocol:%x\n", intf->b_interface_number, intf->b_num_endpoints, intf->b_alternate_setting, intf->b_interface_class, intf->b_interface_subclass, intf->b_interface_protocol);
			intfi++;
			if (intf->b_interface_class == 3)
				dev_type = 0;
			if (intf->b_interface_class == 8)
				dev_type = 1;
			//if (intfi == 2)
			//	break;
		}
		if (type == USB_DESCRIPTOR_TYPE_ENDPOINT) {
			struct usb_endpoint_descriptor *endp = (void *)&((u8 *)descriptor)[i];
			printk("endpoint address = %x type = %s max packet size = %d interval = %d\n", endp->b_endpoint_addr, endpoint_type[endp->bm_attributes], endp->w_max_packet_size, endp->b_interval);
			int ep_addr = endp->b_endpoint_addr & 0x1f;
			int dci = ep_addr_to_dci(endp->b_endpoint_addr);
			ep_addr = dci - 1;
			printk("configuring endpoint %d dci = %d\n", ep_addr, dci);
			transfer_ring = kmalloc(0x1000, GFP_KERNEL);
			memset(transfer_ring, 0, 0x1000);
			printk("transfer ring:%x\n", transfer_ring);
			input_context = kmalloc(0x1000, GFP_KERNEL);
			memset(input_context, 0, 0x1000);
			input_context->input_ctrl_context.add_context_flags = (1 << dci);
			printk("input_context->input_ctrl_context.add_context_flags = %x\n", input_context->input_ctrl_context.add_context_flags);
			input_context->dev_context.slot_context.root_hub_port_number = root_hub_port_num;
			input_context->dev_context.slot_context.route_string = 0;
			input_context->dev_context.slot_context.context_entries = 5;
			input_context->dev_context.slot_context.speed = 0;
			input_context->dev_context.slot_context.max_exit_latency = 0;
			input_context->input_ctrl_context.configuration_value = 0;//conf->b_configuration_value;
			input_context->dev_context.endpoint_context[ep_addr].tr_dequeue_pointer_lo = VIRT2PHYS(transfer_ring) >> 4;
			input_context->dev_context.endpoint_context[ep_addr].tr_dequeue_pointer_hi = (VIRT2PHYS(transfer_ring) >> 32);
			input_context->dev_context.endpoint_context[ep_addr].ep_type = endp->bm_attributes | (endp->b_endpoint_addr & 0x80 ? 0x4 : 0);
			printk("ep type:%d\n", input_context->dev_context.endpoint_context[ep_addr].ep_type);
			input_context->dev_context.endpoint_context[ep_addr].max_packet_size = endp->w_max_packet_size;
			input_context->dev_context.endpoint_context[ep_addr].max_burst_size = (endp->w_max_packet_size & 0x1800) >> 11;
			input_context->dev_context.endpoint_context[ep_addr].average_trb_length = 0x1000;
			input_context->dev_context.endpoint_context[ep_addr].max_esit_payload_lo = endp->w_max_packet_size * (input_context->dev_context.endpoint_context[ep_addr].max_burst_size + 1);
			input_context->dev_context.endpoint_context[ep_addr].dcs = 1;
			input_context->dev_context.endpoint_context[ep_addr].interval = 6;
			input_context->dev_context.endpoint_context[ep_addr].max_pstreams = 0;
			input_context->dev_context.endpoint_context[ep_addr].mult = 0;
			input_context->dev_context.endpoint_context[ep_addr].cerr = 3;
			
			struct configure_endpoint_trb config_ep_trb = {0};
			config_ep_trb.input_context_ptr_lo = VIRT2PHYS(input_context);
			config_ep_trb.input_context_ptr_hi = VIRT2PHYS(input_context) >> 32;
			config_ep_trb.trb_type = TRB_CONFIG_ENDPOINT_CMD;
			config_ep_trb.slot_id = slot_id;
			config_ep_trb.dc = 0;
			config_ep_trb.c = 1;
			cmd_ring_index = xhci_cmd_ring_insert(xhci, (struct trb_template *)&config_ep_trb);
			xhci->port[root_hub_port_num - 1].slot_id = slot_id;
			xhci->port[root_hub_port_num - 1].transfer_ring_status[ep_addr].transfer_ring_base = transfer_ring;
			xhci->port[root_hub_port_num - 1].transfer_ring_status[ep_addr].enquene_pointer = 0;

			xhci_doorbell_reg_wr32(xhci, 0, 0);
			comp_trb = xhci_wait_cmd_completion(xhci, cmd_ring_index, 0);
			printk("%d:trb result:%d\n", __LINE__, comp_trb->completion_code);
			//break;
		}
		if (len == 0)
			i += 1;
		i += len;
	}
	//
#endif
	
#if 1
	if (dev_type == 0) {
		usb_set_config(xhci, root_hub_port_num - 1, conf->b_configuration_value);
		u8 *data = kmalloc(0x1000, GFP_KERNEL);
		struct urb kbd_urb;
	
		kbd_urb.b_request = 0x9;
		kbd_urb.bm_request_type = (1 << 5) | 1;
		kbd_urb.w_value = 0x200;
		kbd_urb.w_index = 0;
		kbd_urb.w_length = 1;
		*data = 0x7;
		printk("sending kbd led cmd...\n");
		//while(1) {
		printk("on\n");
		usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 0, data);
		//for (i = 0; i < 0x80000000; i++);
		//printk("off\n");
		kbd_urb.b_request = 0x1;
		kbd_urb.bm_request_type = (1 << 5) | 1 | (1 << 7);
		kbd_urb.w_value = 0x200;
		kbd_urb.w_index = 0;
		kbd_urb.w_length = 1;
		*data = 0x20;
		//usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 1, data);
		//printk("Keyboard LED status:%x\n", data[0]);
		//}

		kbd_urb.b_request = 0xB;
		kbd_urb.bm_request_type = (1 << 5) | 1;
		kbd_urb.w_value = 0;
		kbd_urb.w_index = 0;
		kbd_urb.w_length = 0;
		usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 0, NULL);

		kbd_urb.b_request = 0x3;
		kbd_urb.bm_request_type = (1 << 5) | 1 | (1 << 7);
		kbd_urb.w_value = 0;
		kbd_urb.w_index = 0;
		kbd_urb.w_length = 1;
		//usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 1, data);
		//printk("protocol:%d\n", data[0]);
		
		kbd_urb.b_request = 0xa;
		kbd_urb.bm_request_type = (1 << 5) | 1;
		kbd_urb.w_value = 0 << 8;
		kbd_urb.w_index = 0;
		kbd_urb.w_length = 0;
		usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 0, NULL);

		kbd_urb.b_request = 0x2;
		kbd_urb.bm_request_type = (1 << 5) | 1 | (1 << 7);
		kbd_urb.w_value = 0 << 8;
		kbd_urb.w_index = 0;
		kbd_urb.w_length = 1;
		//usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 1, data);
		//printk("get idle = %d\n", data[0]);
		int j = 0;
		//while (1) {
		//	delay(8000);
			//printk("kbd int endpoint transfer...\n");
		//	memset(data, 0, 8);
		//	usb_interrupt_transfer(xhci, root_hub_port_num - 1, 0x81, data, 8);
		//	hex_dump(data, 8);
			//kbd_urb.b_request = 0x9;
			//kbd_urb.bm_request_type = (1 << 5) | 1;
			//kbd_urb.w_value = 0x200;
			//kbd_urb.w_index = 0;
			//kbd_urb.w_length = 1;
			//*data = 0x7;
			//usb_control_transfer(xhci, root_hub_port_num - 1, &kbd_urb, 0, data);
	    //}
	    printk("ep state:%x\n", usb_get_context(xhci, xhci->port[root_hub_port_num - 1].slot_id, 0x81)->ep_state);
		printk("ep interval:%d\n", usb_get_context(xhci, xhci->port[root_hub_port_num - 1].slot_id, 0x81)->interval);
		
	}
#endif

#if 1
	if (dev_type == 1) {
		struct urb bbb_urb;
		bbb_urb.b_request = 0xff;
		bbb_urb.bm_request_type = (1 << 5) | 1;
		bbb_urb.w_value = 0;
		bbb_urb.w_index = 0;
		bbb_urb.w_length = 0;
		usb_control_transfer(xhci, root_hub_port_num - 1, &bbb_urb, 0, NULL);
		struct usbmassbulk_cbw *cbw = kmalloc(sizeof(*cbw), GFP_KERNEL);
		struct usbmassbulk_csw *csw = kmalloc(sizeof(*csw), GFP_KERNEL);
		memset(cbw, 0, sizeof(*cbw));
		memset(csw, 0, sizeof(*csw));
		cbw->sig = 0x43425355;
		cbw->tag = 0x12345678;
		cbw->flags = (1 << 7);
		cbw->length = 0xc;
		cbw->data_transfer_len = 8;
		struct ufi_cmd scsi_cmd = {0};
		scsi_cmd.logical_block_addr = 0;
		scsi_cmd.logical_unit_number = 0;
		scsi_cmd.op_code = SCSI_READ_CAPACITY;
		scsi_cmd.parameter = 0x01000000;
		memcpy(&cbw->cb, &scsi_cmd, sizeof(scsi_cmd));
		u8 *disk_buf = kmalloc(0x2000, GFP_KERNEL);
		memset(disk_buf, 0, 0x2000);
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x2, cbw, sizeof(*cbw));
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x81, disk_buf, 8);
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x81, csw, sizeof(*csw));
		printk("csw.tag = %x\n", csw->tag);
		printk("csw.status = %x\n", csw->status);
		u64 cap = disk_buf[3] | (disk_buf[2] << 8) | (disk_buf[1] << 16) | (disk_buf[0] << 24);
		cap *= disk_buf[5] | (disk_buf[6] << 8) | (disk_buf[7] << 16) | (disk_buf[8] << 24);
		printk("CAPACITY:%d GB.\n", cap / 1024 / 1024 / 1024);

		memset(cbw, 0, sizeof(*cbw));
		memset(csw, 0, sizeof(*csw));
		cbw->sig = 0x43425355;
		cbw->tag = 0x12345678;
		cbw->flags = (1 << 7);
		cbw->length = 0xc;
		cbw->data_transfer_len = 0x200;
		scsi_cmd.logical_block_addr = 0;
		scsi_cmd.logical_unit_number = 0;
		scsi_cmd.op_code = SCSI_READ;
		scsi_cmd.parameter = 0x01000000;
		memcpy(&cbw->cb, &scsi_cmd, sizeof(scsi_cmd));
		memset(disk_buf, 0, 0x2000);
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x2, cbw, sizeof(*cbw));
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x81, disk_buf, 0x200);
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x81, csw, sizeof(*csw));
		hex_dump(disk_buf + 512 - 16, 16);

		cbw->sig = 0x43425355;
		cbw->tag = 0x12345678;
		cbw->flags = (1 << 7);
		cbw->length = 0xc;
		cbw->data_transfer_len = 0x200;
		scsi_cmd.logical_block_addr = 1;
		scsi_cmd.logical_unit_number = 0;
		scsi_cmd.op_code = SCSI_READ;
		scsi_cmd.parameter = 0x01000000;
		memcpy(&cbw->cb, &scsi_cmd, sizeof(scsi_cmd));
		memset(disk_buf, 0, 0x2000);
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x2, cbw, sizeof(*cbw));
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x81, disk_buf, 0x200);
		usb_bulk_transfer(xhci, root_hub_port_num - 1, 0x81, csw, sizeof(*csw));
		hex_dump(disk_buf + 512 - 16, 16);
	}
#endif
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

int xhci_intr(int irq, void *data)
{
	struct xhci *xhci = data;
	u32 usb_sts = xhci_opreg_rd32(xhci, XHCI_HC_USBSTS);
	u32 port;
	u32 port_status;
	u32 slot;
	u64 erdp = xhci_rtreg_rd64(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP) & (~0xf);
	printk("xhci_intr (%d).USB STS = %x\n", irq, usb_sts);
	struct trb_template *current_trb = (void *)PHYS2VIRT(erdp);

	if (current_trb[0].trb_type == TRB_PORT_STATUS_CHANGE_EVENT) {
		struct port_status_change_event_trb *port_ch_trb = (struct port_status_change_event_trb *)current_trb;
		port = port_ch_trb[0].port_id - 1;
		port_status = xhci_opreg_rd32(xhci, 0x400 + port * 0x10);
		printk("XHCI Port %x %s\n", port, port_status & 0x1 ? "connected" : "disconnected");
		printk("port_status = %x\n", port_status);

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
			printk("port speed = %d, %s\n", (port_status >> 10) & 0xf, usb_port_speed_info[(port_status >> 10) & 0xf]);
			slot = xhci_enable_slot(xhci);
			printk("available slot:%d\n", slot);
			init_device_slot(xhci, slot, port + 1, (port_status >> 10) & 0xf);
			xhci->port[port].slot_id = slot;
		} else {
			xhci_disable_slot(xhci, xhci->port[port].slot_id);
			xhci_opreg_wr32(xhci, 0x400 + port * 0x10, XHCI_PORTSC_PRC | XHCI_PORTSC_CSC | XHCI_PORTSC_PP);
		}
	}


	u32 ir = xhci_rtreg_rd32(xhci, XHCI_HC_IR(0) + XHCI_HC_IR_ERDP);

	int i = 0;
	u64 last_erdp = erdp;
	while (erdp + 16 != last_erdp) {
		if (current_trb[i].c == (xhci->event_ring_ccs & BIT0)) {
			u32 *trb = (u32 *)&current_trb[i];
			printk("%08x %08x %08x %08x\n", trb[0], trb[1], trb[2], trb[3]);
			if (erdp + 16 < xhci->event_ring_seg_table[0].ring_segment_base_addr + xhci->event_ring_size) {
				erdp += 16;
				xhci->event_ring_dequeue_ptr++;
			} else {
				printk("event ring wrap back.\n");
				erdp = xhci->event_ring_seg_table->ring_segment_base_addr;
				xhci->event_ring_ccs = ~xhci->event_ring_ccs;
				xhci->event_ring_dequeue_ptr = 0;
				current_trb = (void *)PHYS2VIRT(erdp);
			}

			if (current_trb[i].trb_type == TRB_HOST_CONTROLLER_EVENT) {
				struct host_controller_event_trb *host_ctrl_trb = (struct host_controller_event_trb *)&current_trb[i];
				if (host_ctrl_trb[i].completion_code == TRB_COMPLETION_EVENT_RING_FULL_ERROR) {
					printk("XHCI event ring full.\n");
					xhci->event_ring_ccs = ~xhci->event_ring_ccs;
					erdp = xhci->event_ring_seg_table->ring_segment_base_addr;
				}
			}
			i++;
		}else {
			break;
		}
	}
	//usb_sts = xhci_opreg_rd32(xhci, XHCI_HC_USBSTS);
	//printk("xhci_intr.USB STS = %x\n", usb_sts);
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
	u32 len_version, hcs_params1, hcs_params2, hcs_params3, hcc_params1, hcc_params2;
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
	xhci->event_ring_ccs = 1;
	for (int i = 0; i < xhci->nr_intr; i++) {
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTSZ, 1);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERDP, xhci->event_ring_seg_table[0].ring_segment_base_addr | BIT3);
		xhci_rtreg_wr64(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_ERSTBA, VIRT2PHYS(xhci->event_ring_seg_table));
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMOD, 1000);
		xhci_rtreg_wr32(xhci, XHCI_HC_IR(i) + XHCI_HC_IR_IMAN, XHCI_HC_IR_IMAN_IE);
	}

	xhci_opreg_wr32(xhci, XHCI_HC_DNCTRL, 0xffff);
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
