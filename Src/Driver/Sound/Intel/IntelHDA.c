#include "Intel.h"

RETURN_STATUS IntelHDASupported(DEVICE_NODE * Dev)
{	
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;
	
	if(Dev->DeviceType != DEVICE_TYPE_MULTIMEDIA)
		return RETURN_UNSUPPORTED;

	if(INTEL_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;

	KDEBUG("Intel HDA driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));
	
	SoundCardRegister(Dev);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS IntelHDAStart(DEVICE_NODE * Dev)
{
	KDEBUG("Intel HDA driver starting...\n"); 
	return RETURN_SUCCESS;
}

SOUND_DRIVER IntelHDAOperations = 
{
	.abc = NULL
};


DRIVER_NODE IntelHDADriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = IntelHDASupported,
	.DriverStart = IntelHDAStart,
	.DriverStop = 0,
	.Operations = &IntelHDAOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void IntelHDADriverInit()
{
	DriverRegister(&IntelHDADriver, &DriverList[DEVICE_TYPE_MULTIMEDIA]);
}

MODULE_INIT(IntelHDADriverInit);