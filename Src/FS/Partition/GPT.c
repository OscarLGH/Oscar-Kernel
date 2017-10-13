#include "GPT.h"

RETURN_STATUS GPTPartitionProbe(DISK_NODE * Disk)
{
	RETURN_STATUS Status;
	MBR Mbr;
	GPT_HEADER GptHeader;
	GPT_ENTRY GptEnrty[256];
	DISK_NODE *VirtDisk;

	KDEBUG("GPT Partition Probing...\n");

	//Skip virtual disks
	if (Disk->Device->DeviceType != DEVICE_TYPE_STORAGE)
		return RETURN_UNSUPPORTED;

	Status = DiskRead(Disk, MBR_SECTOR_NUM, 0, 512, (UINT8 *)&Mbr);

	if (Status)
		return RETURN_DEVICE_ERROR;

	if (Mbr.Signature != 0xAA55)
		return RETURN_UNSUPPORTED;
	
	if (Mbr.PartitionTable[0].BootIndicator != 0 || Mbr.PartitionTable[0].PartitionType != 0xEE)
		return RETURN_UNSUPPORTED;

	Status = DiskRead(Disk, GPT_HEADER_SECTOR, 0, 512, (UINT8 *)&GptHeader);

	if (Status)
		return RETURN_DEVICE_ERROR;

	if (memcmp(GptHeader.Header.Header, "EFI PART", 8))
		return RETURN_UNSUPPORTED;

	GptHeader.NumberOfPartitionEntries;

	Status = DiskRead(
		Disk, 
		GPT_STARTING_SECTOR,
		0, 
		GptHeader.NumberOfPartitionEntries * GptHeader.SizeOfPartitionEntry,
		(UINT8 *)GptEnrty
		);

	if (Status)
		return RETURN_DEVICE_ERROR;

	UINT32 i;
	for (i = 0; i < GptHeader.NumberOfPartitionEntries; i++)
	{
		if (GptEnrty[i].StartingLBA != 0)
		{
			KDEBUG("GPT Partition found:\nTypeGUID = %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\nStartingLBA = %016x, SectorsCount = %016x\n",
				GptEnrty[i].PartitionTypeGUID.Data1,
				GptEnrty[i].PartitionTypeGUID.Data2,
				GptEnrty[i].PartitionTypeGUID.Data3,
				GptEnrty[i].PartitionTypeGUID.Data4[0],
				GptEnrty[i].PartitionTypeGUID.Data4[1],
				GptEnrty[i].PartitionTypeGUID.Data4[2],
				GptEnrty[i].PartitionTypeGUID.Data4[3],
				GptEnrty[i].PartitionTypeGUID.Data4[4],
				GptEnrty[i].PartitionTypeGUID.Data4[5],
				GptEnrty[i].PartitionTypeGUID.Data4[6],
				GptEnrty[i].PartitionTypeGUID.Data4[7],
				GptEnrty[i].StartingLBA,
				GptEnrty[i].EndingLBA - GptEnrty[i].StartingLBA);

			VirtDisk = KMalloc(sizeof(DISK_NODE));
			VirtDisk->Device = KMalloc(sizeof(DEVICE_NODE));

			memcpy(VirtDisk->Device, Disk->Device, sizeof(DEVICE_NODE));
			VirtDisk->Device->DeviceType = Disk->Device->DeviceType | VIRTUAL_DEVICE_MASK;
			
			VirtDisk->StartingSectors = GptEnrty[i].StartingLBA;
			VirtDisk->NumberOfSectors = GptEnrty[i].EndingLBA - GptEnrty[i].StartingLBA;
			VirtDisk->SectorSize = Disk->SectorSize;
			//VirtDisk->Operations = Disk->Operations;

			VirtDisk->Next = NULL;

			DeviceRegister(VirtDisk->Device);
			DiskRegister(VirtDisk);

		}
	}

	return RETURN_SUCCESS;
}

