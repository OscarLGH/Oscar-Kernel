#ifndef MBR_H
#define MBR_H

#include "Partition.h"

#define DEBUG
#include "Debug.h"


#pragma pack(1)
typedef struct
{
	UINT8 BootIndicator;
	UINT8 StartingCHS[3];
	UINT8 PartitionType;
	UINT8 EndingCHS[3];

	UINT32 StartingLBA;
	UINT32 SectorsCnt;
}MBR_PARTITION_TABLE;
#pragma pack(0)

#pragma pack(1)
typedef struct
{
	UINT8  BootCode[440];
	UINT32 MBRIdentifier;
	UINT16 Reserved;
	MBR_PARTITION_TABLE PartitionTable[4];
	UINT16 Signature;
}MBR;
#pragma pack(0)

#define MBR_SECTOR_NUM   0

RETURN_STATUS MBRPartitionProbe(DISK_NODE * Disk);

#endif