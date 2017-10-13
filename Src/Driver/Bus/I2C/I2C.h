#ifndef _I2C_H
#define _I2C_H

#include "Device.h"
#include "Pci.h"
#include "Lib/Memory.h"

struct _I2C_NODE;

typedef RETURN_STATUS (*I2COPEN) (struct _I2C_NODE * I2CDevice);
typedef RETURN_STATUS (*I2CREAD) (struct _I2C_NODE * I2CDevice, UINT32 SlaveAddress, UINT8 * Buffer, UINT32 Length);
typedef RETURN_STATUS (*I2CWRITE) (struct _I2C_NODE * I2CDevice, UINT32 SlaveAddress, UINT8 * Buffer, UINT32 Length);
typedef RETURN_STATUS (*I2CCLOSE) (struct _I2C_NODE * I2CDevice);

typedef struct _I2C_DRIVER
{
	I2COPEN I2COpen;
	I2CREAD I2CRead;
	I2CWRITE I2CWrite;
	I2CCLOSE I2CClose;
}I2CDRIVER;

typedef struct _I2C_NODE
{
	DEVICE_NODE * Device;
	RETURN_STATUS InitStatus;
	I2CDRIVER * Operations;
	struct _I2C_NODE * Next;
}I2CNODE;

extern DRIVER_NODE * DriverList[32];

void I2CInit();
void I2CRegister(DEVICE_NODE * Dev);
I2CNODE * I2COpen(UINT64 DevNum);

#endif

