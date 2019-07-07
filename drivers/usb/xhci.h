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

struct xhci {
	struct pci_dev *pdev;
	volatile u32 *mmio_virt;
	volatile u64 hc_op_reg_offset;
	
};

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
#define XHCI_HC_USBSTS 0x4
#define XHCI_HC_PAGESIZE 0x8
#define XHCI_HC_DNCTRL 0x14
#define XHCI_HC_CRCR 0x18
#define XHCI_HC_DCBAAP 0x30
#define XHCI_HC_CONFIG 0x38
#define XCHC_HC_PORT_REG 0x400



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
