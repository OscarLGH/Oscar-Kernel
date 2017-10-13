#include "BaseLib.h"
#include "Disk.h"

#define DEBUG
#include "Debug.h"

#pragma pack(1)
typedef struct {
	UINT32 SectorsPerFAT;
	UINT32 FirstRootDirSector;
	UINT16 FSInfoSector;
	UINT16 BackupBootSector;
	UINT8  BIOSDrive;
	UINT8  ExtBootSignature;
	UINT32 VolumeSerialNumber;
}FAT32Section;


typedef struct {
	UINT16 BytesPerSector;
	UINT8  SectorsPerCluster;
	UINT16 ReservedSectors;
	UINT8  NumberOfFATs;

	UINT16 NumberOfSectorsSmall;
	UINT8  MediaDescriptor;
	UINT16 SectorsPerFATSmall;
	UINT16 SectorsPerTrack;
	UINT16 Heads;
	UINT32 HiddenSectors;
	UINT32 Sectors;

	FAT32Section Fat32Section;

}FATBootSector;

typedef struct {

};

typedef struct {
	
	char FileName[8];
	char ExtensionName[3];
	UINT8 Attributes;
	UINT8 Reserved;
	UINT8 CreateTime10ms;

	UINT16 CreateTimeHour : 5;
	UINT16 CreateTimeMinites : 6;
	UINT16 CreateTimeSeconds : 5;

	UINT16 CreateDateYear : 7;
	UINT16 CreateDateMonth : 4;
	UINT16 CreateDateDay : 5;

	UINT16 LastAccessYear : 7;
	UINT16 LastAccessMonth : 4;
	UINT16 LastAccessDay : 5;

	UINT16 FirstClusterNumberH16;

	UINT16 LastModificationTimeHour : 5;
	UINT16 LastModificationTimeMinites : 6;
	UINT16 LastModificationTimeSeconds : 5;

	UINT16 LastModificationDateYear : 7;
	UINT16 LastModificationDateMonth : 4;
	UINT16 LastModificationDateDay : 5;

	UINT16 FirstClusterNumberL16;

	UINT32 FileSizeBytes;
}FileDirEntry;

#pragma pack(0)