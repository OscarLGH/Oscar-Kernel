#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>
#include <bitmap.h>
#include <string.h>
#include <math.h>
#include <fb.h>
#include <irq.h>
#include <cpu.h>

#pragma pack(1)

struct event_ring_segment_table_entry {
	u64 ring_segment_base_addr;
	u16 ring_segment_size;
	u16 reserved0;
	u32 reserved1;
};


struct trb_template {
	u64 parameter;
	u32 status;

	u32 c:1;
	u32 ent:1;
	u32 reserved1:8;
	u32 trb_type:6;
	u32 control:16;
};

struct transfer_trb {
	u64 addr;
	u32 trb_transfer_len:17;
	u32 td_size:5;
	u32 int_tar:10;

	u32 c:1;
	u32 ent:1;
	u32 isp:1;
	u32 ns:1;
	u32 ch:1;
	u32 ioc:1;
	u32 idt:1;
	u32 resvd:2;
	u32 bei:1;
	u32 trb_type:6;
	u32 reserved1:16;
};

struct port_status_change_event_trb {
	u32 rsvd0:24;
	u32 port_id:8;
	u32 rsvd1;
	u32 rsvd2:24;
	u32 completion_code:8;

	u32 c:1;
	u32 rsvd3:9;
	u32 trb_type:6;
	u32 rsvd4:16;
};

struct command_completion_event_trb {
	u32 command_trb_ptr_lo;
	u32 command_trb_ptr_hi;
	u32 command_completion_parameter:24;
	u32 completion_code:8;
	
	u32 c:1;
	u32 rsvd3:9;
	u32 trb_type:6;
	u32 vf_id:8;
	u32 slot_id:8;
};

struct transfer_event_trb {
	u64 trb_pointer;
	u32 trb_transfer_len:24;
	u32 completion_code:8;

	u32 c:1;
	u32 rsvd1:1;
	u32 ed:1;
	u32 rsvd2:7;
	u32 trb_type:6;
	u32 endpoint_id:5;
	u32 rsvd3:3;
	u32 slot_id:8;
};

struct address_device_trb {
	u32 input_context_ptr_lo;
	u32 input_context_ptr_hi;
	u32 rsvd;

	u32 c:1;
	u32 rsvd1:8;
	u32 bsr:1;
	u32 trb_type:6;
	u32 rsvd2:8;
	u32 slot_id:8;
};

struct slot_context {
	u32 route_string:20;
	u32 speed:4;
	u32 rsvdz:1;
	u32 mtt:1;
	u32 hub:1;
	u32 context_entries:5;

	u32 max_exit_latency:16;
	u32 root_hub_port_number:8;
	u32 number_of_ports:8;

	u32 tt_hub_slot_id:8;
	u32 tt_port_number:8;
	u32 ttt:2;
	u32 rsvdz1:4;
	u32 interrupt_target:10;

	u32 usb_dev_addr:8;
	u32 rsvdz2:19;
	u32 slot_state:5;

	u32 rsvd1;
	u32 rsvd2;
	u32 rsvd3;
	u32 rsvd4;
};

struct endpoint_context {
	u32 ep_state:3;
	u32 rsvdz1:5;
	u32 mult:2;
	u32 max_pstreams:5;
	u32 lsa:1;
	u32 interval:8;
	u32 max_esit_payload_hi:8;

	u32 rsvdz2:1;
	u32 cerr:2;
	u32 ep_type:3;
	u32 rsvdz3:1;
	u32 hid:1;
	u32 max_burst_size:8;
	u32 max_packet_size:16;

	u32 dcs:1;
	u32 rsvdz4:3;
	u32 tr_dequeue_pointer_lo:18;

	u32 tr_dequeue_pointer_hi;

	u32 average_trb_length:16;
	u32 max_esit_payload_lo:16;
	
	u32 rsvd2;
	u32 rsvd3;
	u32 rsvd4;
};

struct input_control_context {
	u32 drop_context_flags;
	u32 add_context_flags;
	u32 rsvd1;
	u32 rsvd2;
	u32 rsvd3;
	u32 rsvd4;
	u32 rsvd5;
	u32 configuration_value:8;
	u32 interface_number:8;
	u32 alternate_setting:8;
	u32 rsvdz:8;
};

struct device_context {
	struct slot_context slot_context;
	struct endpoint_context endpoint_context[31];
};

struct input_context {
	struct input_control_context input_ctrl_context;
	struct device_context dev_context;
};
#pragma pack(0)


struct xhci {
	struct pci_dev *pdev;
	volatile u32 *mmio_virt;
	u64 hc_op_reg_offset;
	u64 hc_rt_reg_offset;
	u64 hc_doorbell_reg_offset;
	u64 hc_vt_reg_offset;
	u64 hc_ext_reg_offset;
	u64 *dcbaa;
	u64 dcbaa_size;

	u64 nr_slot;
	u64 nr_port;
	u64 nr_intr;
	
	struct trb_template *cmd_ring;
	u64 cmd_ring_size;
	u64 cmd_ring_enqueue_ptr;
	struct trb_template *cmd_ring_dequeue_ptr;
	
	struct trb_template *event_ring;
	u64 event_ring_size;
	struct trb_template *event_ring_dequeue_ptr;
	

	struct event_ring_segment_table_entry *event_ring_seg_table;
};

int xhci_cmd_ring_insert(struct xhci *xhci, struct trb_template *cmd)
{
	xhci->cmd_ring[xhci->cmd_ring_enqueue_ptr] = *cmd;
	xhci->cmd_ring[xhci->cmd_ring_enqueue_ptr].c = 1;
	xhci->cmd_ring_enqueue_ptr++;
	return 0;
}

//TODO define other TRBs


#define XHCI_CAPLENGTH 0x0
#define XHCI_HCIVERSION 0x2
#define XHCI_HCSPARAMS1 0x4
#define XHCI_HCSPARAMS2 0x8
#define XHCI_HCSPARAMS3 0xc
#define XHCI_HCCPARAMS1 0x10
#define XHCI_DBOFF 0x14
#define XHCI_RTSOFF 0x18
#define XHCI_HCCPARAMS2 0x1c

//Host Controller Operational Registers
#define XHCI_HC_USBCMD 0x0
	#define XHCI_HC_USBCMD_RUN BIT0
	#define XHCI_HC_USBCMD_RESET BIT1
	#define XHCI_HC_USBCMD_INTE BIT2
	#define XHCI_HC_USBCMD_HSEE BIT3
#define XHCI_HC_USBSTS 0x4
#define XHCI_HC_PAGESIZE 0x8
#define XHCI_HC_DNCTRL 0x14
#define XHCI_HC_CRCR 0x18
#define XHCI_HC_DCBAAP 0x30
#define XHCI_HC_CONFIG 0x38
#define XHCI_HC_PORT_REG 0x400

#define XHCI_PORTSC_CCS BIT0
#define XHCI_PORTSC_PED BIT1
#define XHCI_PORTSC_OCA BIT3
#define XHCI_PORTSC_PR BIT4
#define XHCI_PORTSC_PP BIT9
#define XHCI_PORTSC_LWS BIT16
#define XHCI_PORTSC_CSC BIT17
#define XHCI_PORTSC_PEC BIT18
#define XHCI_PORTSC_WRC BIT19
#define XHCI_PORTSC_OCC BIT20
#define XHCI_PORTSC_PRC BIT21
#define XHCI_PORTSC_CEC BIT23
#define XHCI_PORTSC_CAS BIT24
#define XHCI_PORTSC_WCE BIT25
#define XHCI_PORTSC_WDE BIT26
#define XHCI_PORTSC_WOE BIT27
#define XHCI_PORTSC_DR BIT30
#define XHCI_PORTSC_WPR BIT31


//Host Controller Runtime Resgistes
#define XHCI_HC_MFINDEX 0x0
#define XHCI_HC_IR(x) (0x20 + (x)*0x20)
	#define XHCI_HC_IR_IMAN 0x0
		#define XHCI_HC_IR_IMAN_IE 0x2
		#define XHCI_HC_IR_IMAN_IP 0x1
	#define XHCI_HC_IR_IMOD 0x4
	#define XHCI_HC_IR_ERSTSZ 0x8
	#define XHCI_HC_IR_ERSTBA 0x10
	#define XHCI_HC_IR_ERDP 0x18


// Data structures
#define TRB_NORMAL 1
#define TRB_SETUP_STAGE 2
#define TRB_DATA_STAGE 3
#define TRB_STATUS_STAGE 4
#define TRB_ISOCH 5
#define TRB_LINK 6
#define TRB_EVENT_DATA 7
#define TRB_NO_OP 8
#define TRB_ENABLE_SLOT_CMD 9
#define TRB_DISABLE_SLOT_CMD 10
#define TRB_ADDRESS_DEVICE_CMD 11
#define TRB_CONFIG_ENDPOINT_CMD 12

#define TRB_NO_OP_CMD 23
#define TRB_TRANSFER_EVENT 32
#define TRB_COMMAND_COMPLETION_EVENT 33
#define TRB_PORT_STATUS_CHANGE_EVENT 34


u32 xhci_cap_rd32(struct xhci *xhci, u64 offset)
{
	return xhci->mmio_virt[offset / 4];
}

void xhci_cap_wr32(struct xhci *xhci, u64 offset, u32 value)
{
	xhci->mmio_virt[offset / 4] = value;
}

u32 xhci_opreg_rd32(struct xhci *xhci, u64 offset)
{
	return xhci->mmio_virt[(xhci->hc_op_reg_offset + offset) / 4];
}

void xhci_opreg_wr32(struct xhci *xhci, u64 offset, u32 value)
{
	xhci->mmio_virt[(xhci->hc_op_reg_offset + offset) / 4] = value;
}

void xhci_opreg_wr64(struct xhci *xhci, u64 offset, u64 value)
{
	xhci_opreg_wr32(xhci, offset, value);
	xhci_opreg_wr32(xhci, offset + 4, value >> 32);
}

u32 xhci_rtreg_rd32(struct xhci *xhci, u64 offset)
{
	return xhci->mmio_virt[(xhci->hc_rt_reg_offset + offset) / 4];
}

void xhci_rtreg_wr32(struct xhci *xhci, u64 offset, u32 value)
{
	xhci->mmio_virt[(xhci->hc_rt_reg_offset + offset) / 4] = value;
}

u64 xhci_rtreg_rd64(struct xhci *xhci, u64 offset)
{
	u64 lo,hi;
	lo = xhci_rtreg_rd32(xhci, offset);
	hi = xhci_rtreg_rd32(xhci, offset + 4);
	return lo | (hi << 32);
}

void xhci_rtreg_wr64(struct xhci *xhci, u64 offset, u64 value)
{
	xhci_rtreg_wr32(xhci, offset, value);
	xhci_rtreg_wr32(xhci, offset + 4, value >> 32);
}

void xhci_doorbell_reg_wr32(struct xhci *xhci, u64 offset, u32 value)
{
	xhci->mmio_virt[(xhci->hc_doorbell_reg_offset + offset) / 4] = value;
}



