#include "Partition.h"
#include "MBR.h"

#define DEBUG
#include "Debug.h"


#pragma pack(1)
typedef struct
{
	UINT8  Header[8];
	UINT32 Version;
	UINT32 BytesOfHeader;
	UINT32 HeaderCRC;
	UINT32 Reserved;
}EFI_TABLE_HEADER;

typedef struct
{
	EFI_TABLE_HEADER Header;
	UINT64 MyLBA;
	UINT64 AlternateLBA;
	UINT64 FirstUsableLBA;
	UINT64 LastUsableLBA;
	GUID   DiskGUID;
	UINT64 PartitionEntryLBA;
	UINT32 NumberOfPartitionEntries;
	UINT32 SizeOfPartitionEntry;
	UINT32 PartitionEntryArrayCRC32;
	UINT8  Reserved[512 - 92];
}GPT_HEADER;

typedef struct
{
	GUID   PartitionTypeGUID;
	GUID   UniquePartitionGUID;
	UINT64 StartingLBA;
	UINT64 EndingLBA;
	UINT64 Attributes;
	INT16  PartitionName[36];
}GPT_ENTRY;
#pragma pack(0)

#define GPT_MBR_SECTOR 0
#define GPT_HEADER_SECTOR 1
#define GPT_STARTING_SECTOR 2
#define GPT_ENDING_SECTOR 33

RETURN_STATUS GPTPartitionProbe(DISK_NODE * Disk);



