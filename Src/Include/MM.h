#ifndef _MM_H
#define _MM_H


#include "Type.h"
#include "../Arch/X86_64/VirtualMemory/Page.h"
#include "Lib/Memory.h"
#include "Irq.h"


//#define DEBUG
#include "Debug.h"

#define MM_ARRAY_BASE (PHYS2VIRT(0x800000))
#define MEMORY_UPPER_BOUND (1LL<<36)
#define PAGE_SIZE (1LL<<21)
#define MEMORY_UP 0xE0000000
#define MEMORY_TOP 0xFFFFFFFF

void MemoryManagerInit(void);
void * AllocatePage();
void FreePage(void * PhysAddr);

typedef struct _MEMORY_REGION
{
	UINT64 Base;
	UINT64 Length;
	UINT64 Attribute;
	UINT64 PageSize;
	INT8  * AllocatingTable;
	UINT64 AllocatingIndex;
	struct _MEMORY_REGION * Next;
}MEMORY_REGION;

typedef struct _PHYSICAL_MEMORY_AREA
{
	UINT64 PhysBaseAddr;
	UINT64 PhysTotalSize;
	UINT64 Allocated;
	UINT64 PageSize;
	MEMORY_REGION * MemoryRegion;
}PHYSICAL_MEMORY_AREA;

typedef struct _VIRTUAL_MEMORY_AREA
{
	UINT64 VmBaseAddr;
	UINT64 VmTotalSize;
	UINT64 Allocated;
	UINT64 PageSize;
	MEMORY_REGION * MemoryRegion;
}VIRTUAL_MEMORY_AREA;

#endif
