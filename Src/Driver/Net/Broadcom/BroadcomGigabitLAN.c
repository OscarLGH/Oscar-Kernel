#include "BroadcomGigabitLAN.h"

RETURN_STATUS BroadcomNetSupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if(BROADCOM_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;
	
	if(PCI_CLASS_NETWORK != PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;
	
	KDEBUG("Broadcom NetCard driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	NetCardRegister(Dev);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS BroadcomNetStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	KDEBUG("Broadcom NetCard Driver starting...\n");
	UINT16 PCICommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PCICommand |= 0x2;
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PCICommand);
	return RETURN_SUCCESS;
}

RETURN_STATUS BroadcomReadMacAddress(NETCARD_NODE * Dev, UINT8 * Buffer)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	UINT8 * BAR0 = (UINT8 *)(PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32))&(~0xF));
	memcpy(Buffer, BAR0 + BROADCOM_MAC_OFFSET, 6);
	
	return RETURN_SUCCESS;
}

NETCARD_DRIVER BroadcomNETOperations = 
{
	.ReadMacAddress = BroadcomReadMacAddress
};

DRIVER_NODE BroadcomLANControllerDriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = BroadcomNetSupported,
	.DriverStart = BroadcomNetStart,
	.DriverStop = 0,
	.Operations = &BroadcomNETOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

