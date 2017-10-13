#ifndef _FILE_H
#define _FILE_H

#include "Disk.h"
#include "FileSystem.h"

//#define DEBUG
#include "Debug.h"

struct _FILESYSTEM;

typedef struct
{
	char FileName[256];
	DISKNODE * Disk;
	UINT64 FileSize;
	UINT64 Attributes;
	UINT64 CurrentOffset;
	UINT64 SpinLock;
	UINT64 ReferenceCount;
	void * FSPrivateData;
	struct _FILESYSTEM FileSystem;
}FILE;


typedef RETURN_STATUS(*FSPROBE)(DISKNODE * Disk);
typedef RETURN_STATUS(*FSOPEN)(FILE * File, char * Path);
typedef RETURN_STATUS(*FSREAD)(FILE * File, UINT64 Offset, UINT64 Size, UINT8 *Buffer);
typedef RETURN_STATUS(*FSWRITE)(FILE * File, UINT64 Offset, UINT64 Size, UINT8 *Buffer);
typedef RETURN_STATUS(*FSCLOSE)(FILE * File);
typedef RETURN_STATUS(*FSREMOVE)(FILE * File);
typedef RETURN_STATUS(*FSFORMAT)(DISKNODE * Disk);


typedef struct _FILESYSTEM
{
	char FSName[32];
	UINT64 Attributes;
	UINT64 Reserved;

	FSPROBE FSProbe;
	FSOPEN  FSOpen;
	FSREAD  FSRead;
	FSWRITE  FSWrite;
	FSCLOSE  FSClose;
	FSREMOVE FSRemove;
	FSFORMAT FSFormat;

	struct _FILESYSTEM *Next;

}FILESYSTEM;

void FileSystemRegister(FILESYSTEM * FileSystem);

FILE * FileOpen(char * FileName);
UINT64 FileRead(FILE * File, UINT64 Offset, UINT64 Length, UINT8 * Buffer);
UINT64 FileWrite(FILE * File, UINT64 Offset, UINT64 Length, UINT8 * Buffer);
RETURN_STATUS FileClose(FILE * File);
RETURN_STATUS FileRemove(FILE * File);

#endif