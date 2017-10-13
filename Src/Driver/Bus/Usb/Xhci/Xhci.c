#include "Xhci.h"

RETURN_STATUS XhciHostDriverSupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if (Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if (PCI_CLASS_SERIAL != PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;

	if (PCI_CLASS_SERIAL_USB != PciCfgRead8(PciDev, PCI_CONF_SUBCLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;

	if (PCI_IF_XHCI != PciCfgRead8(PciDev, PCI_CONF_INTERFACECODE_OFF_8))
		return RETURN_UNSUPPORTED;

	KDEBUG("XHCI Host driver supported on device:%02x:%02x.%02x\n",
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	return RETURN_SUCCESS;
}

void XhciIrq(void * Dev)
{
	KDEBUG("XHCI Interrupt.\n");
}

/**
Create and initialize a USB_XHCI_INSTANCE structure.

@param  PciIo                  The PciIo on this device.
@param  DevicePath             The device path of host controller.
@param  OriginalPciAttributes  Original PCI attributes.

@return The allocated and initialized USB_XHCI_INSTANCE structure if created,
otherwise NULL.

**/
USB_XHCI_INSTANCE*
XhcCreateUsbHc(
	IN PCI_DEVICE       *PciDev
	)
{
	USB_XHCI_INSTANCE       *Xhc;
	RETURN_STATUS           Status;
	UINT32                  PageSize;
	UINT16                  ExtCapReg;

	Xhc = KMalloc(sizeof(USB_XHCI_INSTANCE));

	if (Xhc == NULL) {
		return NULL;
	}

	//
	// Initialize private data structure
	//
	//Xhc->Signature = XHCI_INSTANCE_SIG;
	Xhc->PciDev = PciDev;
	//Xhc->DevicePath = DevicePath;
	//Xhc->OriginalPciAttributes = OriginalPciAttributes;
	//memcpy(&Xhc->Usb2Hc, &gXhciUsb2HcTemplate, sizeof(EFI_USB2_HC_PROTOCOL));

	//InitializeListHead(&Xhc->AsyncIntTransfers);

	//
	// Be caution that the Offset passed to XhcReadCapReg() should be Dword align
	//
	Xhc->CapLength = XhcReadCapReg8(Xhc, XHC_CAPLENGTH_OFFSET);
	Xhc->HcSParams1.Dword = XhcReadCapReg(Xhc, XHC_HCSPARAMS1_OFFSET);
	Xhc->HcSParams2.Dword = XhcReadCapReg(Xhc, XHC_HCSPARAMS2_OFFSET);
	Xhc->HcCParams.Dword = XhcReadCapReg(Xhc, XHC_HCCPARAMS_OFFSET);
	Xhc->DBOff = XhcReadCapReg(Xhc, XHC_DBOFF_OFFSET);
	Xhc->RTSOff = XhcReadCapReg(Xhc, XHC_RTSOFF_OFFSET);

	//
	// This PageSize field defines the page size supported by the xHC implementation.
	// This xHC supports a page size of 2^(n+12) if bit n is Set. For example,
	// if bit 0 is Set, the xHC supports 4k byte page sizes.
	//
	PageSize = XhcReadOpReg(Xhc, XHC_PAGESIZE_OFFSET) & XHC_PAGESIZE_MASK;
	
	Xhc->PageSize = 1 << (FirstBitSet(PageSize) + 12);

	ExtCapReg = (UINT16)(Xhc->HcCParams.Data.ExtCapReg);
	Xhc->ExtCapRegBase = ExtCapReg << 2;
	Xhc->UsbLegSupOffset = XhcGetCapabilityAddr(Xhc, XHC_CAP_USB_LEGACY);
	Xhc->DebugCapSupOffset = XhcGetCapabilityAddr(Xhc, XHC_CAP_USB_DEBUG);


	KDEBUG("XhcCreateUsb3Hc: Capability length 0x%x\n", Xhc->CapLength);
	KDEBUG("XhcCreateUsb3Hc: HcSParams1 0x%x\n", Xhc->HcSParams1);
	KDEBUG("XhcCreateUsb3Hc: HcSParams2 0x%x\n", Xhc->HcSParams2);
	KDEBUG("XhcCreateUsb3Hc: HcCParams 0x%x\n", Xhc->HcCParams);
	KDEBUG("XhcCreateUsb3Hc: DBOff 0x%x\n", Xhc->DBOff);
	KDEBUG("XhcCreateUsb3Hc: RTSOff 0x%x\n", Xhc->RTSOff);
	KDEBUG("XhcCreateUsb3Hc: UsbLegSupOffset 0x%x\n", Xhc->UsbLegSupOffset);
	KDEBUG("XhcCreateUsb3Hc: DebugCapSupOffset 0x%x\n", Xhc->DebugCapSupOffset);

	//
	// Create AsyncRequest Polling Timer
	//
	/*Status = gBS->CreateEvent(
		EVT_TIMER | EVT_NOTIFY_SIGNAL,
		TPL_NOTIFY,
		XhcMonitorAsyncRequests,
		Xhc,
		&Xhc->PollTimer
		);

	if (EFI_ERROR(Status)) {
		goto ON_ERROR;
	}*/

	return Xhc;

ON_ERROR:
	KFree(Xhc);
	return NULL;
}


RETURN_STATUS XhciHostDriverStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	USB_XHCI_INSTANCE       *Xhc;
	UINT64 Chipset = 0;
	KDEBUG("XHCI Host driver starting...\n");
	UINT16 PciCommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PciCommand |= (BIT0 | BIT1 | BIT2);	/* Enable bus master, memory, io */
	PciCommand ^= BIT10;			/* Enable Interrupt.*/
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PciCommand);

	IrqRegister(0x48, XhciIrq, PciDev);
	IrqRegister(0x49, XhciIrq, PciDev);
	IrqRegister(0x4A, XhciIrq, PciDev);
	IrqRegister(0x4B, XhciIrq, PciDev);
	IrqRegister(0x4C, XhciIrq, PciDev);
	IrqRegister(0x4D, XhciIrq, PciDev);
	IrqRegister(0x4E, XhciIrq, PciDev);
	IrqRegister(0x4F, XhciIrq, PciDev);

	PciMsiCheck(PciDev);
	UINT64 Irqs = PciMsiGetMultipleMessageCapable(PciDev);
	KDEBUG("XHCI:PCI-MSI:Request %d IRQs.\n", Irqs);
	PciMsiMultipleMessageCapableEnable(PciDev, Irqs);
	MSI_CAP_STRUCTURE_64_MASK PciMsi;
	PciMsi.MessageAddr = 0xFEE00000;
	PciMsi.MessageData = 0x48;
	PciMsiSet(PciDev, &PciMsi);
	PciMsiEnable(PciDev);

	//
	// Create then install USB2_HC_PROTOCOL
	//
	Xhc = XhcCreateUsbHc(PciDev);

	if (Xhc == NULL) {
		KDEBUG("XhcDriverBindingStart: failed to create USB2_HC\n");
		return RETURN_OUT_OF_RESOURCES;
	}
	
	//XhcClearBiosOwnership(Xhc);

	XhcResetHC(Xhc, XHC_RESET_TIMEOUT);
	//ASSERT(XhcIsHalt(Xhc));

	//
	// After Chip Hardware Reset wait until the Controller Not Ready (CNR) flag
	// in the USBSTS is '0' before writing any xHC Operational or Runtime registers.
	//
	//ASSERT(!(XHC_REG_BIT_IS_SET(Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_CNR)));

	//
	// Initialize the schedule
	//
	//XhcInitSched(Xhc);

	//
	// Start the Host Controller
	//
	XhcRunHC(Xhc, XHC_GENERIC_TIMEOUT);


	return RETURN_SUCCESS;
}

DRIVER_NODE XhciHostDriver =
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = XhciHostDriverSupported,
	.DriverStart = XhciHostDriverStart,
	.DriverStop = 0,
	.Operations = NULL,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void XhciHostDriverInit()
{
	DriverRegister(&XhciHostDriver, &DriverList[DEVICE_TYPE_SERIAL_BUS]);
}

MODULE_INIT(XhciHostDriverInit);