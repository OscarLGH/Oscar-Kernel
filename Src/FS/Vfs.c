#include "Vfs.h"

VFS_NODE * RootFs;

VFS_NODE * RamFsFindNode(CHAR8 * Path)
{
	VFS_NODE * VfsNode0 = NULL;
	VFS_NODE * VfsNode1 = NULL;
	CHAR8 Temp[256] = { 0 };
	CHAR8 * Front;
	CHAR8 * Rear;
	//KDEBUG("VFS:Path = %s\n", Path);
	if (Path[0] != '/')
		return NULL;

	if (strlen(Path) == 1)
		return RootFs;

	Rear = Front = &Path[1];
	VfsNode0 = RootFs->Child;
	while (1)
	{
		if (*Front != '/' && *Front != 0)
		{
			Front++;
		}
		else
		{
			strncpy(Temp, Rear, Front - Rear);
			Temp[Front - Rear] = 0;
			Front++;
			Rear = Front;
			//KDEBUG("%s:%d\n", __FUNCTION__, __LINE__);

			VfsNode1 = VfsNode0;
			while (VfsNode1)
			{
				if (strcmp(VfsNode1->FileName, Temp) == 0)
				{
					//KDEBUG("VFS:Directory '%s' found.\n", Temp);
					break;
				}
				VfsNode1 = VfsNode1->Next;
			}
			//KDEBUG("%s:%d\n", __FUNCTION__, __LINE__);
			if (VfsNode1 == NULL)
			{
				//KDEBUG("VFS:Directory '%s' not found.\n", Temp);
				return NULL;
			}
			else
			{
				VfsNode0 = VfsNode1->Child;
				if (*Front == 0)
					return VfsNode1;
			}
		}
	}
	
}

static
RETURN_STATUS RamFsOpen(VFS_NODE * File)
{
	return RETURN_SUCCESS;
}

static
RETURN_STATUS RamFsRead(VFS_NODE * File, UINT64 Offset, UINT64 Size, UINT8 * Buffer)
{
	UINT8 * Data = File->PrivateData;
	memcpy(Buffer, Data + Offset, Size);
	return RETURN_SUCCESS;
}

static
RETURN_STATUS RamFsWrite(VFS_NODE * File, UINT64 Offset, UINT64 Size, UINT8 * Buffer)
{
	UINT8 * Data = File->PrivateData;
	memcpy(Data + Offset, Buffer, Size);
	return RETURN_SUCCESS;
}

static
RETURN_STATUS RamFsClose(VFS_NODE * File)
{
	return RETURN_SUCCESS;
}

RETURN_STATUS RamFsCreate(VFS_NODE * File, CHAR8 * FilePath, UINT64 FileSize, UINT64 Attribute)
{
	//KDEBUG("Creating %s\n", FilePath);
	CHAR8 DirPath[256] = { 0 };
	CHAR8 FileName[256] = { 0 };
	CHAR8 * Dir = strrchr(FilePath, '/');
	CHAR8 * Name = Dir + 1;
	strncpy(DirPath, FilePath, (Dir - FilePath == 0 ? 1 : Dir - FilePath));
	//KDEBUG("VFS:DirPath = %s\n", DirPath);
	//strcpy(FileName, Name);
	VFS_NODE * VfsNode = RamFsFindNode(DirPath);
	//if (VfsNode == NULL)
	//	return RETURN_NOT_FOUND;

	//KDEBUG("VFS:VfsNode->FileName = %s\n", VfsNode->FileName);

	VFS_NODE * SubDirNode = VfsNode->Child;
	VFS_NODE * New = KMalloc(sizeof(*VfsNode->Child));
	
	strcpy(New->FileName, Name);
	New->Attributes = Attribute;
	New->FileSize = FileSize;
	New->VfsCreate = RamFsCreate;
	New->VfsOpen = RamFsOpen;
	New->VfsRead = RamFsRead;
	New->VfsWrite = RamFsWrite;
	New->VfsClose = RamFsClose;

	if (SubDirNode == NULL)
	{
		VfsNode->Child = New;
	}
	else
	{
		VFS_NODE * TempNode = SubDirNode;
		while (TempNode->Next)
		{
			TempNode = TempNode->Next;
		}
		TempNode->Next = New;
	}

	return RETURN_SUCCESS;
}

RETURN_STATUS VfsInit()
{
	RootFs = KMalloc(sizeof(*RootFs));
	strcpy(RootFs->FileName, "/");

	VfsFileCreate("/dev", 0);
	VfsFileCreate("/dev/disk", 0);
	VfsFileCreate("/dev/disk/sda1", 0);
	VfsFileCreate("/dev/disk/sda2", 0);
	VfsFileCreate("/etc", 0);
	VfsFileCreate("/bin", 0);
	VfsFileCreate("/sbin", 0);
	VfsFileCreate("/abcd/1234", 0);

}

RETURN_STATUS VfsDirCreate(CHAR8 * Path)
{
	if (RootFs == NULL)
		return RETURN_NOT_READY;

	return RootFs->VfsCreate(RootFs, Path, VFS_ATTRIBUTE_DIR, 0);
}

RETURN_STATUS VfsFileCreate(CHAR8 * Path, UINT64 Size)
{
	if (RootFs == NULL)
		return RETURN_NOT_READY;

	return RootFs->VfsCreate(RootFs, Path, VFS_ATTRIBUTE_FILE, Size);
}

