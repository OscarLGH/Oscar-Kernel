#ifndef _VFS_H
#define _VFS_H

#include "BaseLib.h"

#define DEBUG
#include "Debug.h"

typedef struct _FILE_SYSTEM_TYPE
{
	char * Name;
	struct _FILE_SYSTEM_TYPE * Next;
}FILE_SYSTEM_TYPE;

typedef struct _VFS_NODE
{
	char FileName[256];
	
	UINT64 FileSize;
	UINT64 Attributes;
	
	UINT64 ReferenceCount;
	VOID * PrivateData;

	SPIN_LOCK SpinLock;

	struct _VFS_NODE * Next;
	struct _VFS_NODE * Prev;
	struct _VFS_NODE * Parent;
	struct _VFS_NODE * Child;

	struct _VFS_NODE * (*VfsFindPath)(struct _VFS_NODE * File, CHAR8 * Path);
	RETURN_STATUS(*VfsCreate)(struct _VFS_NODE * File, CHAR8 * FilePath, UINT64 FileSize, UINT64 Attribute);
	RETURN_STATUS(*VfsOpen)(struct _VFS_NODE * File);
	RETURN_STATUS(*VfsRead)(struct _VFS_NODE * File, UINT64 Offset, UINT64 Size, UINT8 * Buffer);
	RETURN_STATUS(*VfsWrite)(struct _VFS_NODE * File, UINT64 Offset, UINT64 Size, UINT8 * Buffer);
	RETURN_STATUS(*VfsClose)(struct _VFS_NODE * File);

}VFS_NODE;

#define VFS_ATTRIBUTE_FILE 0
#define VFS_ATTRIBUTE_DIR  BIT0

RETURN_STATUS VfsInit();
RETURN_STATUS VfsFileCreate(char * FileName, UINT64 Attribute);
RETURN_STATUS VfsMount(VFS_NODE * Vfs, char * FileName, UINT64 Attribute);
RETURN_STATUS VfsUnmount(VFS_NODE * Vfs, char * FileName, UINT64 Attribute);

#endif