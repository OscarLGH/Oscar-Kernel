#include "Device.h"

DEVICE_NODE DeviceRoot = 
{
	.DeviceType = DEVICE_TYPE_ROOT,
	.DeviceID = 0,
	.BusType = 0,
	.Position = 0,
	.Driver = 0,
	.SpinLock = 0,
	.ParentDevice = 0,
	.ChildDevice = 0,
	.PrevDevice = 0,
	.NextDevice = 0
};

DRIVER_NODE * DriverList[32] = {0};

void DeviceRegister(DEVICE_NODE * Device)
{
	DEVICE_NODE * dev;

	//SpinLockIrqSave(&DeviceRoot.SpinLock);

	if(Device == 0 || Device->ParentDevice == 0)
	{
	//	SpinUnlockIrqRestore(&DeviceRoot.SpinLock);
		KDEBUG("Bad device node!\n");
		return;
	}

	SearchSupportedDriver(Device, DriverList[Device->DeviceType & (~VIRTUAL_DEVICE_MASK)]);
	dev = Device->ParentDevice->ChildDevice;
	
	if(dev)
	{
		while(dev->NextDevice)
		{
			dev = dev->NextDevice;
		}

		Device->PrevDevice = dev;
		dev->NextDevice = Device;
	}
	else
	{
		Device->ParentDevice->ChildDevice = Device;
	}
	//SpinUnlockIrqRestore(&DeviceRoot.SpinLock);
}

void DeviceUnregister(DEVICE_NODE * Device)
{
	if(!Device)
	{
		KDEBUG("Bad device node!\n");
		return;
	}

	DeviceUnregister(Device->ChildDevice);
	
	if(!Device->ChildDevice)
	{
		DEVICE_NODE * DevDel = Device;
		DEVICE_NODE * DevTmp;
		
		while(DevDel)
		{
			DevTmp = DevDel->NextDevice;
			//DevDel->Driver->DriverStop(DevDel);
			KFree(DevDel);
			DevDel = DevTmp;
		}
	}
}

void SearchSupportedDriver(DEVICE_NODE * Dev, DRIVER_NODE * Drv)
{
	if(!Dev || !Drv)
	{
		//KDEBUG("Bad device/driver node!\n");
		return;
	}
	
	if(RETURN_SUCCESS == Drv->DriverSupported(Dev))
	{
		Dev->Driver = Drv->Operations;
		Drv->DriverStart(Dev);
	}
	SearchSupportedDriver(Dev, Drv->NextDriver);
}

void DriverRegister(DRIVER_NODE * Driver, DRIVER_NODE ** DriverList)
{
	
	//KDEBUG("%s\n", __FUNCTION__);
	if(!Driver)
	{
		KDEBUG("Bad driver node!\n");
		return;
	}
	SpinLockIrqSave(&DeviceRoot.SpinLock);
	Driver->PrevDriver = 0;
	Driver->NextDriver = *DriverList;
	
	*DriverList = Driver;
	SpinUnlockIrqRestore(&DeviceRoot.SpinLock);
	//KDEBUG("Driver = %x\n",Driver);
	SearchSupportedDevice(&DeviceRoot, Driver);
}

void DriverUnregister(DRIVER_NODE * Driver, DRIVER_NODE ** DriverList)
{
	if(!Driver)
	{
		KDEBUG("Bad driver node!\n");
		return;
	}

	if(Driver->NextDriver)
		Driver->NextDriver->PrevDriver = Driver->PrevDriver;
	
	if(Driver->PrevDriver)
		Driver->PrevDriver->NextDriver = Driver->NextDriver;

	KFree(Driver);
}

void SearchSupportedDevice(DEVICE_NODE * Dev, DRIVER_NODE * Drv)
{
	if(!Drv)
	{
		KDEBUG("Bad device/driver node!\n");
		return;
	}

	if(!Dev)
		return;
	
	if(RETURN_SUCCESS == Drv->DriverSupported(Dev))
	{
		SpinLockIrqSave(&DeviceRoot.SpinLock);
		Dev->Driver = Drv->Operations;
		SpinUnlockIrqRestore(&DeviceRoot.SpinLock);

		Drv->DriverStart(Dev);
	}
	
	SearchSupportedDevice(Dev->NextDevice, Drv);
	SearchSupportedDevice(Dev->ChildDevice, Drv);
}

