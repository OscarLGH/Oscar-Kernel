#include "MM.h"

void PageFaultHandler(void * Dev)
{
	printk("In PageFaultHandler\n");
	VOID * PageFaultAddr = GetPageFaultAddr();
	printk("PageFaultAddress:%016x\n",PageFaultAddr);
	VOID * PageBaseAddr = (VOID *)PHYS2VIRT(GetPageTableBase());
	VOID * NewPage;
	if((UINT64)PageFaultAddr >= (UINT64)&KERNEL_SPACE)
	{
		printk("Segment Fault!\n");
		asm("hlt");
	}
	else
	{
		NewPage = AllocatePage();
		printk("NewPageAddress:%016x\n",NewPage);
		MapPage(PageBaseAddr, PageFaultAddr, NewPage, 2, PG_RWW | PG_USU | PG_P);
		RefreshTLB(0xFFFFFFFFFFFFFFFF);
		//printk("Page Reallocated.\n");
	}
	asm("hlt");
}

MEMORY_REGION PhysicalMemoryRegions[256] = {0};
PHYSICAL_MEMORY_AREA PhysicalMemoryArea = {0};

void MemoryManagerInit()
{
	char * MMArray = (char *)MM_ARRAY_BASE;
	
	PhysicalMemoryArea.MemoryRegion = PhysicalMemoryRegions;
	SYSTEM_PARAMETERS *SysParam = (SYSTEM_PARAMETERS *)SYSTEMPARABASE;

	long long i, j;
	for (i = 0, j = 0; i < SysParam->Ardc_Cnt; i++)
	{
		/*KDEBUG("MMap:Region Start:%016x, Length = %016x, Type = %x\n", 
			SysParam->ArdcArray[i].BaseAddr, 
			SysParam->ArdcArray[i].Length, 
			SysParam->ArdcArray[i].Type);*/

		if (SysParam->ArdcArray[i].Type == 1)
		{
			KDEBUG("MMap:Available Region Start:%016x, Length = %016x\n", 
				SysParam->ArdcArray[i].BaseAddr,
				SysParam->ArdcArray[i].Length);

			PhysicalMemoryRegions[j].Base = SysParam->ArdcArray[i].BaseAddr;
			PhysicalMemoryRegions[j].Length = SysParam->ArdcArray[i].Length;
			PhysicalMemoryRegions[j].PageSize = PAGE_SIZE;
			PhysicalMemoryRegions[j].Next = &PhysicalMemoryRegions[j + 1];
			j++;
			PhysicalMemoryArea.PhysTotalSize += SysParam->ArdcArray[i].Length;
		}
	}
	PhysicalMemoryRegions[j - 1].Next = NULL;
	KDEBUG("MM:Memory Available:%dMB\n", PhysicalMemoryArea.PhysTotalSize >> 20);

	for(i = 0; i < MEMORY_UPPER_BOUND / PAGE_SIZE; i++)
	{
		MMArray[i] = 0;
	}
	
	MMArray[0] = 1;			//第一页0-1M保留不使用，1M以下存在BIOS只读区
	MMArray[1] = 1;			//第二页2-3M为内核初始化时使用
	MMArray[2] = 1;			//第三页4-5M用作内核数据
	MMArray[3] = 1;			//第四页6-7M用作内存管理
	MMArray[4] = 1;
	MMArray[5] = 1;
	
	
	for(i = MEMORY_UP; i < MEMORY_TOP; i += PAGE_SIZE)
	{
		MMArray[i / PAGE_SIZE] = 1;
	}
	
	IrqRegister(0xE, PageFaultHandler, NULL);
	return;
}

void * AllocatePage()
{
	char * MMArray = (char *)MM_ARRAY_BASE;
	long long i;
	
	for(i = 0; i < MEMORY_UPPER_BOUND / PAGE_SIZE; i++)
	{
		//printk("%d ",MMArray[i]);
		if(MMArray[i] == 0)
			break;
	}
	
	if(i == MEMORY_UPPER_BOUND / PAGE_SIZE)
		return (void *)0;
		
	MMArray[i] = 1;
	//printk("%x\n",i*PAGE_SIZE + KERNELSPACE);
	//while(1);
	//MapPage((void *)(i*PAGE_SIZE + KERNELSPACE),(void *)(i*PAGE_SIZE),2,PG_RWW | PG_USU | PG_P);
	KDEBUG("Allocate Page:0x%016x->0x%016x\n", i * PAGE_SIZE + KERNEL_SPACE, i * PAGE_SIZE);
	//while(1);
	
	return (void *)(i * PAGE_SIZE);
}

void FreePage(void * PhysAddr)
{
	char * MMArray = (char *)MM_ARRAY_BASE;
	int Index = (long long)PhysAddr/PAGE_SIZE;
	if(0 != MMArray[Index] && Index > 3)
	{
		MMArray[Index]--;
	}
	else
		printk("MM Error!");
}

RETURN_STATUS VmaInit(VIRTUAL_MEMORY_AREA * Vma, UINT64 VmaBase, UINT64 VmaSize, UINT64 PageSize)
{
	Vma->VmBaseAddr = VmaBase;
	Vma->VmTotalSize = VmaSize;
	return RETURN_SUCCESS;
}

UINT64 AddressSpaceAllocate(MEMORY_REGION * Vma, UINT64 Size)
{
	MEMORY_REGION * VmRegionPtr = Vma;

	while (VmRegionPtr)
	{

		VmRegionPtr = VmRegionPtr->Next;
	}

	UINT64 Pages = Size % Vma->PageSize ? Size / Vma->PageSize : Size / Vma->PageSize + 1;
	MEMORY_REGION * VmRegion = KMalloc(sizeof(*VmRegion));
	memset(VmRegion, 0, sizeof(*VmRegion));

	
}