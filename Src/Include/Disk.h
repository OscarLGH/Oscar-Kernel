#ifndef _DISK_H
#define _DISK_H

#include "Device.h"

#define DEBUG
#include "Debug.h"

struct _DISK_NODE;

typedef RETURN_STATUS(*DISK_OPEN) (struct _DISK_NODE * Disk);
typedef RETURN_STATUS(*GET_DISK_INFO) (struct _DISK_NODE * Disk, UINT8 * Buffer);
typedef RETURN_STATUS(*DISK_READ) (struct _DISK_NODE * Disk, UINT64 Sector, UINT64 Length, UINT8 * Buffer);
typedef RETURN_STATUS(*DISK_WRITE) (struct _DISK_NODE * Disk, UINT64 Sector, UINT64 Length, UINT8 * Buffer);
typedef RETURN_STATUS(*DISK_CLOSE) (struct _DISK_NODE * Disk);

typedef struct _DISK_DRIVER
{
	DISK_OPEN DiskOpen;
	GET_DISK_INFO GetDiskInfo;
	DISK_READ DiskRead;
	DISK_WRITE DiskWrite;
	DISK_CLOSE DiskClose;
}DISK_DRIVER;

typedef struct _DISK_NODE
{
	DEVICE_NODE * Device;
	char	   * DiskName[32];
	DISK_DRIVER * Operations;
	UINT64       StartingSectors;
	UINT64       NumberOfSectors;
	UINT64       SectorSize;
	void       * DiskCache;
	//FILESYSTEM * FileSystem;			//For physical disk, this should be NULL
	struct _DISK_NODE * Next;
}DISK_NODE;

extern DRIVER_NODE * DriverList[32];
extern DISK_NODE * DiskList;

void DiskInit();
void DiskRegister(DISK_NODE * Dev);
DISK_NODE * DiskOpen(UINT64 DevNum);
RETURN_STATUS DiskRead(DISK_NODE * Disk, UINT64 Sector, UINT64 Offset, UINT64 Length, UINT8 * Buffer);
RETURN_STATUS DiskWrite(DISK_NODE * Disk, UINT64 Sector, UINT64 Offset, UINT64 Length, UINT8 * Buffer);
RETURN_STATUS DiskClose(DISK_NODE * Disk);



#endif
