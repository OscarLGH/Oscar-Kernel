#include "MBR.h"

RETURN_STATUS MBRPartitionProbe(DISK_NODE * Disk)
{
	RETURN_STATUS Status;
	MBR Mbr;
	DISK_NODE *VirtDisk;
	KDEBUG("MBR Partition Probing...\n");

	//Skip virtual disks
	if (Disk->Device->DeviceType != DEVICE_TYPE_STORAGE)
		return RETURN_UNSUPPORTED;

	Status = DiskRead(Disk, MBR_SECTOR_NUM, 0, 512, (UINT8 *)&Mbr);

	if (Status)
		return RETURN_DEVICE_ERROR;

	if (Mbr.Signature != 0xAA55)
		return RETURN_UNSUPPORTED;

	PARTITION_MAP_ENRTY * PartitionMap = NULL;
	PARTITION_MAP_ENRTY * PartitionMapPtr0 = NULL;
	PARTITION_MAP_ENRTY * PartitionMapPtr1 = NULL;

	UINT32 i;
	for (i = 0; i < 4; i++)
	{
		if (Mbr.PartitionTable[i].StartingLBA != 0 && Mbr.PartitionTable[i].PartitionType != 0xEE)
		{
			KDEBUG("MBR Partition found:Type = %02x, StartingLBA = %016x, SectorsCount = %016x\n",
			Mbr.PartitionTable[i].PartitionType,
			Mbr.PartitionTable[i].StartingLBA,
			Mbr.PartitionTable[i].SectorsCnt);

			VirtDisk = KMalloc(sizeof(DISK_NODE));
			VirtDisk->Device = KMalloc(sizeof(DEVICE_NODE));

			memcpy(VirtDisk->Device, Disk->Device, sizeof(DEVICE_NODE));
			VirtDisk->Device->DeviceType = Disk->Device->DeviceType | VIRTUAL_DEVICE_MASK;
			
			VirtDisk->StartingSectors = Mbr.PartitionTable[i].StartingLBA;
			VirtDisk->NumberOfSectors = Mbr.PartitionTable[i].SectorsCnt;
			VirtDisk->SectorSize = Disk->SectorSize;
			VirtDisk->Operations = Disk->Operations;

			VirtDisk->Next = NULL;

			DeviceRegister(VirtDisk->Device);
			DiskRegister(VirtDisk);
		}
	}

	return RETURN_SUCCESS;
}