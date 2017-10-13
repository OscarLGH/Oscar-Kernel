#ifndef _DEVICE_H
#define _DEVICE_H

#include "Type.h"
#include "MM.h"
#include "Lib/Memory.h"
#include "Lib/String.h"
#include "Lib/Integer.h"
#include "Lib/List.h"
#include "Irq.h"
#include "InitCall.h"

#include "SystemParameters.h"
#include "Delay.h"

//#define DEBUG
#include "Debug.h"

typedef enum
{
	DEVICE_TYPE_ROOT = 0,
	DEVICE_TYPE_PROCESSOR,
	DEVICE_TYPE_BRIDGE,
	DEVICE_TYPE_MEMORY,
	DEVICE_TYPE_STORAGE,
	DEVICE_TYPE_DISPLAY,
	DEVICE_TYPE_MULTIMEDIA,
	DEVICE_TYPE_NET,
	DEVICE_TYPE_GENERAL_SYSTEM_PERIPHERALS,
	DEVICE_TYPE_SERIAL_BUS,
	DEVICE_TYPE_INPUT,
	DEVICE_TYPE_COMMUCIATION,
	DEVICE_TYPE_CRYPTION,
	DEVICE_TYPE_OTHER
}DEVICE_TYPE;

typedef enum
{
	BUS_TYPE_PCI = 0,
	BUS_TYPE_SCSI,
	BUS_TYPE_SATA,
	BUS_TYPE_IDE,
	BUS_TYPE_USB,
	BUS_TYPE_THUNDER,
	BUS_TYPE_IEEE1394,
	BUS_TYPE_DVI,
	BUS_TYPE_VGA,
	BUS_TYPE_DISPLAYPORT,
	BUS_TYPE_HDMI,
	BUS_TYPE_OTHER
}BUS_TYPE;

#define VIRTUAL_DEVICE_MASK 0x80000000

typedef struct _DEVICE_NODE
{
	UINT64 DeviceType;
	UINT64 DeviceID;
	UINT64 BusType;
	UINT64 Position;	//B/D/F for pci devices, Ports for other devices

	void * Device;		// Point to real device structure
	void * PrivateData;
	void * Status;
	void * Driver;
	
	SPIN_LOCK SpinLock;
	
	struct _DEVICE_NODE * ParentDevice;
	struct _DEVICE_NODE * ChildDevice;
	struct _DEVICE_NODE * PrevDevice;
	struct _DEVICE_NODE * NextDevice;
	
}DEVICE_NODE;


typedef RETURN_STATUS (*DRIVER_SUPPORTED)(DEVICE_NODE * Dev);
typedef RETURN_STATUS (*DRIVER_START)(DEVICE_NODE * Dev);
typedef RETURN_STATUS (*DRIVER_STOP)(DEVICE_NODE * Dev);

typedef struct _DRIVER_NODE
{
	UINT64 BusType;
	DRIVER_SUPPORTED DriverSupported;
	DRIVER_START DriverStart;
	DRIVER_STOP DriverStop;
	void * Operations;
	
	struct _DRIVER_NODE * PrevDriver;
	struct _DRIVER_NODE * NextDriver;
	
}DRIVER_NODE;

void DeviceRegister(DEVICE_NODE * Device);
void DeviceUnregister(DEVICE_NODE * Device);
void SearchSupportedDriver(DEVICE_NODE * Dev, DRIVER_NODE * Drv);

void DriverBinding(DEVICE_NODE * Device, DRIVER_NODE * Driver);

void DriverRegister(DRIVER_NODE * Driver, DRIVER_NODE ** DriverList);
void DriverUnregister(DRIVER_NODE * Driver, DRIVER_NODE ** DriverList);
void SearchSupportedDevice(DEVICE_NODE * Dev, DRIVER_NODE * Drv);

extern DRIVER_NODE * DriverList[32];

#endif





