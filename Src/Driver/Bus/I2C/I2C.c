#include "I2C.h"

I2CNODE * I2CList = NULL;

extern DRIVER_NODE AtomE3800I2CDriver;

void I2CRegister(DEVICE_NODE * Dev)
{
	I2CNODE * I2C = I2CList;
	I2CNODE * I2CNew = NULL;

	//if(Dev->DeviceType == DEVICE_TYPE_SERIAL_BUS)
	if(1)
	{
		I2CNew = (I2CNODE *)KMalloc(sizeof(I2CNODE));
		I2CNew->Device = Dev;
		I2CNew->Operations = (I2CDRIVER *)Dev->Driver;
		I2CNew->Next = NULL;
		
		if(I2C != NULL)
		{
			while(I2C->Next)
			{
				I2C = I2C->Next;
			}
			I2C->Next = I2CNew;
		}
		else
			I2CList = I2CNew;

		//printk("I2C->Device->Position:%x\n", I2CList->Device->Position);
	}
}

I2CNODE * I2COpen(UINT64 DevNum)
{
	I2CNODE * I2CNode = I2CList;
	
	if(I2CNode == NULL)
		return NULL;
	
	while(I2CNode)
	{
		if(DevNum == 0)
		{
			if(I2CNode->Device->Driver == NULL)
			{
				KDEBUG("Failed to open device %08x due to no driver binded.\n", I2CNode->Device->DeviceID);
				return NULL;
			}
			I2CNode->Operations = I2CNode->Device->Driver;
			return I2CNode;
		}
		I2CNode = I2CNode->Next;
		DevNum--;
	}
	return NULL;
}

