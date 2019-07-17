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

struct xhci {
	struct pci_dev *pdev;
	volatile u32 *mmio_virt;
	u64 hc_op_reg_offset;
	u64 hc_rt_reg_offset;
	u64 hc_doorbell_reg_offset;
	u64 hc_vt_reg_offset;
	u64 *dcbaa;
	u64 dcbaa_size;
	
	struct trb_template *cmd_ring;
	u64 cmd_ring_size;
	
	struct trb_template *event_ring;
	u64 event_ring_size;

	struct event_ring_segment_table_entry *event_ring_seg_table;
};



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
#define XCHC_HC_PORT_REG 0x400

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



