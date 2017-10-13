#include "Amd.h"

RETURN_STATUS AmdHDASupported(DEVICE_NODE * Dev)
{	
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;
	
	if(Dev->DeviceType != DEVICE_TYPE_MULTIMEDIA)
		return RETURN_UNSUPPORTED;

	if(AMD_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;

	KDEBUG("AMD HDMI Audio driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));
	
	SoundCardRegister(Dev);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS AmdHDAStart(DEVICE_NODE * Dev)
{
	KDEBUG("AMD HDMI Audio driver starting...\n"); 
	return RETURN_SUCCESS;
}

SOUND_DRIVER AmdHDAOperations = 
{
	.abc = NULL
};

DRIVER_NODE AmdHDADriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = AmdHDASupported,
	.DriverStart = AmdHDAStart,
	.DriverStop = 0,
	.Operations = &AmdHDAOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void AmdHDADriverInit()
{
	DriverRegister(&AmdHDADriver, &DriverList[DEVICE_TYPE_MULTIMEDIA]);
}

MODULE_INIT(AmdHDADriverInit);