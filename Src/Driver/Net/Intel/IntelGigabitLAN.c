#include "IntelGigabitLAN.h"

RETURN_STATUS IntelNetSupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if(INTEL_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;
	
	if(PCI_CLASS_NETWORK != PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;
	
	KDEBUG("Intel NetCard driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	NetCardRegister(Dev);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS IntelNetStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	KDEBUG("Intel NetCard Driver starting...\n");
	UINT16 PCICommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PCICommand |= 0x2;
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PCICommand);
	return RETURN_SUCCESS;
}

UINT32 IntelRegRead(NETCARD_NODE * Dev, UINT64 Offset)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	volatile UINT64 BAR0 = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32));
	BAR0 &= (~0xF);
	volatile UINT32 * Reg = (UINT32 *)(BAR0 + Offset);
	return *Reg;
}

void IntelRegWrite(NETCARD_NODE * Dev, UINT32 Value, UINT64 Offset)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	volatile UINT64 BAR0 = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32));
	BAR0 &= (~0xF);
	volatile UINT32 * Reg = (UINT32 *)(BAR0 + Offset);
	*Reg = Value;
}


RETURN_STATUS IntelReadMacAddress(NETCARD_NODE * Dev, UINT8 * Buffer)
{
	PCI_DEVICE * PciDev = Dev->Device->Device;
	UINT64 BAR0 = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32));
	BAR0 &= 0xFFFFFFFFFFFFFFF0;
	memcpy(Buffer, (void *)(BAR0 + INTEL_MAC_OFFSET), 6);
	
	return RETURN_SUCCESS;
}

NETCARD_DRIVER IntelNETOperations = 
{
	.ReadMacAddress = IntelReadMacAddress
};

DRIVER_NODE IntelLanControllerDriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = IntelNetSupported,
	.DriverStart = IntelNetStart,
	.DriverStop = 0,
	.Operations = &IntelNETOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};


static void IntelLanControllerDriverInit()
{
	DriverRegister(&IntelLanControllerDriver, &DriverList[PCI_CLASS_NETWORK]);
}

MODULE_INIT(IntelLanControllerDriverInit);