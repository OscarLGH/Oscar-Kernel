#include "Nvidia.h"

RETURN_STATUS NvidiaHDASupported(DEVICE_NODE * Dev)
{	
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;
	
	if(Dev->DeviceType != DEVICE_TYPE_MULTIMEDIA)
		return RETURN_UNSUPPORTED;

	if(NVIDIA_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;

	KDEBUG("NVIDIA HDMI Audio driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));
	
	SoundCardRegister(Dev);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS NvidiaHDAStart(DEVICE_NODE * Dev)
{
	KDEBUG("NVIDIA HDMI Audio driver starting...\n"); 
	return RETURN_SUCCESS;
}

SOUND_DRIVER NvidiaHDAOperations = 
{
	.abc = NULL
};

DRIVER_NODE NvidiaHDADriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = NvidiaHDASupported,
	.DriverStart = NvidiaHDAStart,
	.DriverStop = 0,
	.Operations = &NvidiaHDAOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void NvidiaHDADriverInit()
{
	DriverRegister(&NvidiaHDADriver, &DriverList[DEVICE_TYPE_MULTIMEDIA]);
}

MODULE_INIT(NvidiaHDADriverInit);

