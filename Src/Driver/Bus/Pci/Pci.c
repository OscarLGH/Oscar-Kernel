/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
PCI.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Oscar 2015.9
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "Pci.h"



extern DEVICE_NODE DeviceRoot;

//Arch code will fill this struct
PCI_HOST_BRIDGE PciHostBridge;

//Device Class Strings
char * BaseClassString[256] = 
{
	"Device was built before Class Code",
	"Mass storage controller",
	"Network controller",
	"Display controller",
	"Multimedia device",
	"Memory controller",
	"Bridge device",
	"Simple communication controllers",
	"Base system perpherals",
	"Input devices",
	"Docking stations",
	"Processors",
	"Serial bus controllers",
	"Wireless controller",
	"Intelligent I/O controllers",
	"Satellite communication controllers",
	"Encryption/Decryption controllers",
	"Data acquisition and signal processing controllers",
	"Reserved"
};

UINT64 PCIClass2DeviceClass[256] = 
{
	DEVICE_TYPE_OTHER,
	DEVICE_TYPE_STORAGE,
	DEVICE_TYPE_NET,
	DEVICE_TYPE_DISPLAY,
	DEVICE_TYPE_MULTIMEDIA,
	DEVICE_TYPE_MEMORY,
	DEVICE_TYPE_BRIDGE,
	DEVICE_TYPE_COMMUCIATION,
	DEVICE_TYPE_GENERAL_SYSTEM_PERIPHERALS,
	DEVICE_TYPE_INPUT,
	DEVICE_TYPE_OTHER,
	DEVICE_TYPE_PROCESSOR,
	DEVICE_TYPE_SERIAL_BUS,
	DEVICE_TYPE_NET,
	DEVICE_TYPE_OTHER,
	DEVICE_TYPE_OTHER,
	DEVICE_TYPE_OTHER,
	DEVICE_TYPE_OTHER
};

UINT8 PciCfgRead8(PCI_DEVICE *PciDev, UINT64 Offset)
{
	UINT64 Data = 0;
	PciHostBridge.PciCfgRead(&PciHostBridge, PciDev, Offset, 1, &Data);
	
	return Data;
}

UINT16 PciCfgRead16(PCI_DEVICE *PciDev, UINT64 Offset)
{
	UINT64 Data = 0;
	
	PciHostBridge.PciCfgRead(&PciHostBridge, PciDev, Offset, 2, &Data);
	return Data;
}

UINT32 PciCfgRead32(PCI_DEVICE *PciDev, UINT64 Offset)
{
	UINT64 Data = 0;
	
	PciHostBridge.PciCfgRead(&PciHostBridge, PciDev, Offset, 4, &Data);
	return Data;
}

UINT64 PciCfgWrite8(PCI_DEVICE *PciDev, UINT64 Offset, UINT8 Value)
{
	PciHostBridge.PciCfgWrite(&PciHostBridge, PciDev, Offset, 1, &Value);
}

UINT64 PciCfgWrite16(PCI_DEVICE *PciDev, UINT64 Offset, UINT16 Value)
{
	PciHostBridge.PciCfgWrite(&PciHostBridge, PciDev, Offset, 2, &Value);
}

UINT64 PciCfgWrite32(PCI_DEVICE *PciDev, UINT64 Offset, UINT32 Value)
{
	PciHostBridge.PciCfgWrite(&PciHostBridge, PciDev, Offset, 4, &Value);
}


UINT64 PciGetBarSize(PCI_DEVICE *PciDev, UINT64 BarIndex)
{
	UINT16 CommandRegister = 0;
	UINT32 OriBarValueL = 0;
	UINT32 NewValueL = 0;
	UINT32 OriBarValueH = 0;
	UINT32 NewValueH = 0xFFFFFFFF;
	UINT64 NewValue = 0;

	CommandRegister = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, CommandRegister & (~0x3));

	OriBarValueL = PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32 + BarIndex * 4);
	PciCfgWrite32(PciDev, PCI_CONF_BAR0_OFF_32 + BarIndex * 4, OriBarValueL & (~0xF));
	PciCfgWrite32(PciDev, PCI_CONF_BAR0_OFF_32 + BarIndex * 4, 0xFFFFFFFF);
	NewValueL = PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32 + BarIndex * 4);
	PciCfgWrite32(PciDev, PCI_CONF_BAR0_OFF_32 + BarIndex * 4, OriBarValueL);
	//NewValueL &= (~0xF);
	//NewValueL = ~NewValueL + 1;

	if (OriBarValueL & BIT2)
	{
		OriBarValueH = PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32 + (BarIndex + 1) * 4);
		PciCfgWrite32(PciDev, PCI_CONF_BAR0_OFF_32 + (BarIndex + 1) * 4, OriBarValueH & (~0xF));
		PciCfgWrite32(PciDev, PCI_CONF_BAR0_OFF_32 + (BarIndex + 1) * 4, 0xFFFFFFFF);
		NewValueH = PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32 + (BarIndex + 1) * 4);
		PciCfgWrite32(PciDev, PCI_CONF_BAR0_OFF_32 + (BarIndex + 1) * 4, OriBarValueH);
		//NewValueH &= (~0xF);
		//NewValueH = ~NewValueH + 1;
	}

	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, CommandRegister);

	NewValue = NewValueL | ((UINT64)NewValueH << 32);
	NewValue &= (~0xF);
	NewValue = ~NewValue + 1;

	return NewValue;
}

UINT64 PciGetBarBase(PCI_DEVICE *PciDev, UINT64 BarIndex)
{
	UINT32 BarValueL = 0;
	UINT32 BarValueH = 0;

	BarValueL = PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32 + BarIndex * 4);
	
	if (BarValueL & BIT2)
	{
		BarValueH = PciCfgRead32(PciDev, PCI_CONF_BAR0_OFF_32 + (BarIndex + 1) * 4);
	}

	return (BarValueL & (~0xF)) | ((UINT64)BarValueH << 32);
}

UINT8 PciCapabilityFind(PCI_DEVICE *PciDev, UINT8 CapId)
{
	if (PciDev == NULL)
		return 0;

	UINT8 NextPtr = PciCfgRead8(PciDev, PCI_CONF_CAPABILITIESPOINTER_OFF_32);
	while (NextPtr)
	{
		if (CapId == PciCfgRead8(PciDev, NextPtr))
		{
			return NextPtr;
		}
		NextPtr = PciCfgRead8(PciDev, NextPtr + 1);
	}
	return 0;
}

RETURN_STATUS PciMsiCheck(PCI_DEVICE *PciDev)
{
	UINT8 MsiOffset = 0;
	UINT16 MessageControl = 0;

	if (PciDev == NULL)
		return RETURN_INVALID_PARAMETER;

	PciDev->InterruptCap = 0;

	MsiOffset = PciCapabilityFind(PciDev, PCI_MSIX_CAP_ID);
	if (MsiOffset != 0)
	{
		PciDev->InterruptCap |= BIT2;
	}
	else
	{
		MsiOffset = PciCapabilityFind(PciDev, PCI_MSI_CAP_ID);
		if (MsiOffset != 0)
		{
			PciDev->InterruptCap = BIT1;
		}
		else
		{
			PciDev->InterruptCap = BIT0;
			return RETURN_UNSUPPORTED;
		}
	}

	MessageControl = PciCfgRead16(PciDev, MsiOffset + 2);
	MessageControl &= (BIT7 | BIT8);
	MessageControl >> 7;
	
	PciDev->InterruptCap |= MessageControl;
	PciDev->InterruptCap |= (MsiOffset << 16);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS PciMsiSet(PCI_DEVICE *PciDev, MSI_CAP_STRUCTURE_64_MASK * MsiData)
{
	UINT8 MsiOffset = 0;
	UINT8 MaskOffset = 0;
	if (PciDev == NULL)
		return RETURN_INVALID_PARAMETER;

	MsiOffset = (PciDev->InterruptCap >> 16);

	if (PciDev->InterruptCap & BIT1)
	{
		PciCfgWrite32(PciDev, MsiOffset + 0x4, MsiData->MessageAddr);
		if (PciDev->InterruptCap & BIT7)
		{
			PciCfgWrite32(PciDev, MsiOffset + 0x8, MsiData->MessageAddr >> 32);
			PciCfgWrite16(PciDev, MsiOffset + 0xc, MsiData->MessageData);
			MaskOffset = 0x10;
		}
		else
		{
			PciCfgWrite16(PciDev, MsiOffset + 0x8, MsiData->MessageData);
			MaskOffset = 0xc;
		}

		if (PciDev->InterruptCap & BIT8)
		{
			PciCfgWrite16(PciDev, MsiOffset + MaskOffset, MsiData->MaskBits);
			PciCfgWrite16(PciDev, MsiOffset + MaskOffset + 0x4, MsiData->PendingBits);
		}
	}
	else
	{
		return RETURN_UNSUPPORTED;
	}
}

UINT8 PciMsiGetMultipleMessageCapable(PCI_DEVICE *PciDev)
{
	UINT8 CapOffset = 0;
	UINT16 MessageControl = 0;
	UINT32 Order = 0;
	UINT32 Vectors = 1;

	if (PciDev->InterruptCap & BIT1)
	{
		CapOffset = (PciDev->InterruptCap >> 16);
		MessageControl = PciCfgRead16(PciDev, CapOffset + 2);
		Order = ((MessageControl & (BIT1 | BIT2 | BIT3)) >> 1);
		KDEBUG("%s:MessageControl:%04x\n", __FUNCTION__, MessageControl);
		while (Order--)
		{
			Vectors *= 2;
		}
		return Vectors;
	}
	else
	{
		return 0;
	}
}

RETURN_STATUS PciMsiMultipleMessageCapableEnable(PCI_DEVICE *PciDev, UINT8 Vectors)
{
	UINT8 CapOffset = 0;
	UINT16 MessageControl = 0;
	UINT32 Order = 0;
	
	if (PciDev == NULL)
		return RETURN_INVALID_PARAMETER;

	if (PciDev->InterruptCap & BIT1)
	{
		CapOffset = (PciDev->InterruptCap >> 16);
		MessageControl = PciCfgRead16(PciDev, CapOffset + 2);
		
		while (Vectors)
		{
			Order++;
			Vectors >>= 1;
		}

		MessageControl |= (Order << 4);
		PciCfgWrite16(PciDev, CapOffset + 2, MessageControl);
	}
	else
	{
		return RETURN_UNSUPPORTED;
	}
}

RETURN_STATUS PciMsiEnable(PCI_DEVICE *PciDev)
{
	UINT8 CapOffset = 0;
	UINT16 MessageControl = 0;
	
	if (PciDev->InterruptCap & BIT1)
	{
		CapOffset = (PciDev->InterruptCap >> 16);
		
		MessageControl = PciCfgRead16(PciDev, CapOffset + 2);
		MessageControl |= BIT0;
		PciCfgWrite16(PciDev, CapOffset + 2, MessageControl);

		return RETURN_SUCCESS;
	}
	else
	{
		return RETURN_UNSUPPORTED;
	}
}



void PciScan()
{
	UINT32 b,d,f;
	PCI_DEVICE PciTemp;
	PCI_DEVICE *PciDev = &PciTemp;
	PCI_DEVICE *NewPciDev = NULL;
	DEVICE_NODE * Dev;
	
	for(b = 0; b <= PciHostBridge.EndBusNum; b++)
	{
		for(d = 0; d < 32; d++)
		{
			for(f = 0; f < 8; f++)
			{
				PciDev->PciAddress.Bus = b;
				PciDev->PciAddress.Device = d;
				PciDev->PciAddress.Function = f;
				if(PciCfgRead32(PciDev, PCI_CONF_VIDDID_OFF_32) != 0xFFFFFFFF)
				{
					NewPciDev = (PCI_DEVICE *)KMalloc(sizeof(PCI_DEVICE));
					memcpy(NewPciDev, PciDev, sizeof(PCI_DEVICE));
					NewPciDev->PciCfgIoBase = 0;
					//NewPciDev->PciCfgMmioBase = GetPciCfgAddress(PciDev, 0);

					Dev = (DEVICE_NODE *)KMalloc(sizeof(DEVICE_NODE));
					Dev->Device = NewPciDev;
					Dev->DeviceType = PCIClass2DeviceClass[PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8)];
					Dev->DeviceID = PciCfgRead32(PciDev, PCI_CONF_VIDDID_OFF_32);
					Dev->BusType = BUS_TYPE_PCI;
					Dev->Position = (b << 8) + (d << 3) + f;
					Dev->Driver = 0;
					Dev->ParentDevice = &DeviceRoot;
					Dev->ChildDevice = 0;
					Dev->PrevDevice = 0;
					Dev->NextDevice = 0;
					
					DeviceRegister(Dev);
					
					KDEBUG("PCI: %02x:%02x.%x,VendorID:%04x,DeviceID:%04x,Int_Line:%02x %s\n",\
						PciDev->PciAddress.Bus,\
						PciDev->PciAddress.Device,\
						PciDev->PciAddress.Function,\
						PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16),
						PciCfgRead16(PciDev, PCI_CONF_DEVICEID_OFF_16),
						//PciCfgRead8(PciDev->PciAddress, PCI_CONF_BASECLASSCODE_OFF_8)
						PciCfgRead8(PciDev, PCI_CONF_INTERRUPTLINE_OFF_8),
						BaseClassString[PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8)]
					);
					//Delay(2000000);
					/*
					printk("BAR0:%08x,BAR1:%08x,BAR2:%08x,BAR3:%08x,BAR4:%08x,BAR5:%08x\n",
						PciCfgRead32(PciDev->PciAddress, PCI_CONF_BAR0_OFF_32),
						PciCfgRead32(PciDev->PciAddress, PCI_CONF_BAR1_OFF_32),
						PciCfgRead32(PciDev->PciAddress, PCI_CONF_BAR2_OFF_32),
						PciCfgRead32(PciDev->PciAddress, PCI_CONF_BAR3_OFF_32),
						PciCfgRead32(PciDev->PciAddress, PCI_CONF_BAR4_OFF_32),
						PciCfgRead32(PciDev->PciAddress, PCI_CONF_BAR5_OFF_32)
					);
					*/
				}
				if(!(PciCfgRead8(PciDev, PCI_CONF_HEADERTYPE_OFF_8) & (1<<7) ))
					break;
			}
		}
	}	
}