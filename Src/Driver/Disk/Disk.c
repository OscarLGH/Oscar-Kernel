#include "Disk.h"

DISK_NODE * DiskList = NULL;

void DiskRegister(DISK_NODE * Disk)
{
	DISK_NODE * DiskPtr = DiskList;
	DISK_NODE * DiskNew = NULL;

	if((Disk->Device->DeviceType & (~VIRTUAL_DEVICE_MASK)) == DEVICE_TYPE_STORAGE)
	{
		if(DiskPtr != NULL)
		{
			while(DiskPtr->Next)
			{
				DiskPtr = DiskPtr->Next;
			}
			DiskPtr->Next = Disk;
		}
		else
			DiskList = Disk;

		//printk("Disk->Device->Position:%x\n", DiskList->Device->Position);
	}
}

DISK_NODE * DiskOpen(UINT64 DevNum)
{
	DISK_NODE * DiskNode = DiskList;
	
	if(DiskNode == NULL)
		return NULL;
	
	while(DiskNode)
	{
		if(DevNum == 0)
		{
			if(DiskNode->Device->Driver == NULL)
			{
				KDEBUG("Failed to open device %08x due to no driver binded.\n", DiskNode->Device->DeviceID);
				return NULL;
			}
			DiskNode->Operations = DiskNode->Device->Driver;

			return DiskNode;
		}
		DiskNode = DiskNode->Next;
		DevNum--;
	}
	return NULL;
}

RETURN_STATUS DiskRead(DISK_NODE * Disk, UINT64 Sector, UINT64 Offset, UINT64 Length, UINT8 * Buffer)
{
	RETURN_STATUS Status;

	//if (Disk->Device->DeviceType & VIRTUAL_DEVICE_MASK)
	//	KDEBUG("Virtual Disk Read.\n");

	if (Sector + Length / Disk->SectorSize > Disk->StartingSectors + Disk->NumberOfSectors)
		return RETURN_END_OF_MEDIA;

	UINT8 DiskBuffer[512 * 32];

	Sector += Disk->StartingSectors;
	Sector += (Offset / Disk->SectorSize);
	Offset %= Disk->SectorSize;

	//KDEBUG("%s:Sector = %d, Offset = %d, Length = %d\n", __FUNCTION__, Sector, Offset, Length);
	Status = Disk->Operations->DiskRead(Disk, Sector, Length, DiskBuffer);

	//KDEBUG("%s:Status = %x\n", __FUNCTION__, Status);
	if (Status)
		return Status;

	memcpy(Buffer, DiskBuffer + Offset, Length);

	return RETURN_SUCCESS;
}

RETURN_STATUS DiskWrite(DISK_NODE * Disk, UINT64 Sector, UINT64 Offset, UINT64 Length, UINT8 * Buffer)
{
	RETURN_STATUS Status;

	if (Disk->Device->DeviceType & VIRTUAL_DEVICE_MASK)
		KDEBUG("Virtual Disk Write.\n");

	if (Sector + Length / Disk->SectorSize > Disk->StartingSectors + Disk->NumberOfSectors)
		return RETURN_END_OF_MEDIA;

	UINT8 DiskBuffer[512 * 32];
	Sector += Disk->StartingSectors;
	Sector += (Offset / Disk->SectorSize);
	Offset %= Disk->SectorSize;
	Status = Disk->Operations->DiskRead(Disk, Sector, Length, DiskBuffer);
	if (Status)
		return Status;

	memcpy(DiskBuffer + Offset, Buffer, Length);

	Status = Disk->Operations->DiskWrite(Disk, Sector, Length, DiskBuffer);
	if (Status)
		return Status;

	return RETURN_SUCCESS;

}

RETURN_STATUS DiskClose(DISK_NODE * Disk)
{
	if (Disk->DiskCache)
		KFree(Disk->DiskCache);

	return RETURN_SUCCESS;
}