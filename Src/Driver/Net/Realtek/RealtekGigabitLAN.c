#include "RealtekGigabitLAN.h"

RETURN_STATUS RealtekNetSupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if(REALTEK_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;
	
	if(PCI_CLASS_NETWORK != PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;
	
	KDEBUG("Realtek NetCard driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	NetCardRegister(Dev);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS RealtekNetStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	KDEBUG("Realtek NetCard Driver starting...\n");
	
	UINT16 PCICommand = PciCfgRead16(Dev->Device, PCI_CONF_COMMAND_OFF_16);
	PCICommand |= 0x2;
	PciCfgWrite16(Dev->Device, PCI_CONF_COMMAND_OFF_16, PCICommand);
	
	return RETURN_SUCCESS;
}

UINT32 RealtekRegRead(NETCARD_NODE * Dev, UINT64 Offset)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	volatile UINT64 BAR2 = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR2_OFF_32));
	BAR2 &= (~0xF);
	volatile UINT32 * Reg = (UINT32 *)(BAR2 + Offset);
	return *Reg;
}

void RealtekRegWrite(NETCARD_NODE * Dev, UINT32 Value, UINT64 Offset)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	volatile UINT64 BAR2 = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR2_OFF_32));
	BAR2 &= (~0xF);
	volatile UINT32 * Reg = (UINT32 *)(BAR2 + Offset);
	*Reg = Value;
}


RETURN_STATUS RealtekReadMacAddress(NETCARD_NODE * Dev, UINT8 * Buffer)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	UINT64 BAR2 = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR2_OFF_32));
	BAR2 &= 0xFFFFFFFFFFFFFFF0;
	memcpy(Buffer, (void *)(BAR2 + REALTEK_MAC_OFFSET), 6);
	
	return RETURN_SUCCESS;
}

NETCARD_DRIVER RealtekNETOperations = 
{
	.ReadMacAddress = RealtekReadMacAddress
};

DRIVER_NODE RealtekLanControllerDriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = RealtekNetSupported,
	.DriverStart = RealtekNetStart,
	.DriverStop = 0,
	.Operations = &RealtekNETOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void RealtekLanDriverInit()
{
	DriverRegister(&RealtekLanControllerDriver, &DriverList[PCI_CLASS_NETWORK]);
}

MODULE_INIT(RealtekLanDriverInit);