#ifndef _NETCARD_H
#define _NETCARD_H


#include "Device.h"
#include "Pci.h"


#define DEBUG
#include "Debug.h"

struct _NETCARD_NODE;

typedef RETURN_STATUS (*READMACADDRESS)(struct _NETCARD_NODE * Dev, UINT8 * Buffer);

typedef struct _NETCARD_DRIVER
{
	READMACADDRESS ReadMacAddress;
}NETCARD_DRIVER;

typedef struct _NETCARD_NODE
{
	DEVICE_NODE * Device;
	NETCARD_DRIVER * Operations;
	struct _NETCARD_NODE * Next;
}NETCARD_NODE;

extern DRIVER_NODE * DriverList[32];

void NetCardRegister(DEVICE_NODE * Dev);
NETCARD_NODE * OpenNetCard(UINT64 DevNum);


#endif
