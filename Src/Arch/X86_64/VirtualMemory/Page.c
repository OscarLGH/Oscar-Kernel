//=====================================================================
//Page.c
//内存分页
//Oscar 2015.7
//=====================================================================


#include "Page.h"

static UINT64 FindFreePDPT(UINT64 * PgdBase)
{
	UINT64 * StartPos = PDPT_USER_START/8 + PgdBase;
	UINT64 * EndPos = PDPT_USER_END/8 + PgdBase;
	UINT64 * Ptr0 = StartPos;
	UINT64 * Ptr1 = StartPos;
	UINT64 Zero = 0;

	while (Ptr1 < EndPos)
	{
		Zero++;
		if (*Ptr1 != 0)
		{
			Zero = 0;
			Ptr0 += 512;
			Ptr1 = Ptr0;
			continue;
		}

		if (Zero == 512)
		{
			return VIRT2PHYS((UINT64)Ptr0);
		}
		Ptr1++;
	}
	return 0;
}

static UINT64 FindFreePDT(UINT64 * PgdBase)
{
	UINT64 * StartPos = PDT_USER_START / 8 + PgdBase;
	UINT64 * EndPos = PDT_USER_END / 8 + PgdBase;
	UINT64 * Ptr0 = StartPos;
	UINT64 * Ptr1 = StartPos;
	UINT64 Zero = 0;
	
	while (Ptr1 < EndPos)
	{
		Zero++;
		if (*Ptr1 != 0)
		{
			Zero = 0;
			Ptr0 += 512;
			Ptr1 = Ptr0;
			continue;
		}

		if (Zero == 512)
		{
			return VIRT2PHYS((UINT64)Ptr0);
		}
		Ptr1++;
	}
	return 0;
}

static UINT64 FindFreePT(UINT64 * PgdBase)
{
	UINT64 * StartPos = PDT_USER_START / 8 + PgdBase;
	UINT64 * EndPos = PDT_USER_END / 8 + PgdBase;
	UINT64 * Ptr0 = StartPos;
	UINT64 * Ptr1 = StartPos;
	UINT64 Zero = 0;

	while (Ptr1 < EndPos)
	{
		Zero++;
		if (*Ptr1 != 0)
		{
			Zero = 0;
			Ptr0 += 512;
			Ptr1 = Ptr0;
			continue;
		}

		if (Zero == 512)
		{
			return VIRT2PHYS((UINT64)Ptr0);
		}
		Ptr1++;
	}
	return 0;
}

void MapPage(UINT64 * PgdBase, VOID * VirtAddr, VOID * PhysAddr, UINT64 PageSize, UINT64 Attr)
{
	//各级页表的基地址
	volatile UINT64 *PML4;
	volatile UINT64 *PDPT;
	volatile UINT64 *PD;
	volatile UINT64 *PT;
	
	//各级页表项
	volatile UINT64 *PML4E;
	volatile UINT64 *PDPE;
	volatile UINT64 *PDE;
	volatile UINT64 *PTE;
	
	UINT64 Index1,Index2,Index3,Index4,Offset2M,Offset4K;
	
	UINT64 MaxVirtAddr = (UINT64)GetMaxVirtAddr();
	
	UINT64 DirAttr =  PG_RWW | PG_USS | PG_P;
	PhysAddr = (VOID *) ((UINT64)PhysAddr & (~0xFFF));
	VirtAddr = (VOID *) ((UINT64)VirtAddr & (~0xFFF));


	KDEBUG("VirtualAddress:%016x\n",VirtAddr);
	KDEBUG("PhysicalAddress:%016x\n",PhysAddr);

	//if((UINT64)VirtAddr > (UINT64)MaxVirtAddr)
	//	return;
	
	//计算虚拟地址在各级页表中索引
	Index1 = ((UINT64)VirtAddr>>39);
	Index2 = ((UINT64)VirtAddr>>30)&0x1FF;
	Index3 = ((UINT64)VirtAddr>>21)&0x1FF;
	Index4 = ((UINT64)VirtAddr>>12)&0x1FF;
	Offset2M = (UINT64)VirtAddr&((1<<21)-1);
	Offset4K = (UINT64)VirtAddr&((1<<12)-1);
	
	switch(PageSize)
	{
		case PAGE_SIZE_4K://4KB page
		{
			PML4 = PgdBase;
			PML4E = PML4 + Index1;
			
			if (*PML4E == 0)
			{
				*PML4E = (FindFreePDPT(PgdBase) | DirAttr);
			}
			KDEBUG("PML4T :%016x PML4T [%03d] = %016x\n", PML4, Index1, *PML4E);
			PDPT = (UINT64 *)PHYS2VIRT(*PML4E & (~0xFFF));
			PDPE = PDPT + Index2;
			
			if (*PDPE == 0)
			{
				*PDPE = (FindFreePDT(PgdBase) | DirAttr);
			}
			KDEBUG("PDPT  :%016x PDPT  [%03d] = %016x\n", PDPT, Index2, *PDPE);
			PD = (UINT64 *)PHYS2VIRT(*PDPE & (~0xFFF));
			PDE = PD + Index3;
			
			if (*PDE == 0)
			{
				*PDE = (FindFreePT(PgdBase) | DirAttr);
			}
			KDEBUG("PD    :%016x PD    [%03d] = %016x\n", PD, Index3, *PDE);
			
			PT = (UINT64 *)PHYS2VIRT(*PDE & (~0xFFF));
			PTE = PT + Index4;
			*PTE = (UINT64)PhysAddr | Attr | PG_PAT;

			KDEBUG("PT    :%016x PT    [%03d] = %016x\n", PD, Index3, *PDE);
			break;
		}
		case PAGE_SIZE_2M://2MB page
		{
			PML4 = PgdBase;
			PML4E = PML4 + Index1;
			
			if (*PML4E == 0)
			{
				*PML4E = (FindFreePDPT(PgdBase) | DirAttr);
			}
			KDEBUG("PML4T :%016x PML4T [%03d] = %016x\n", PML4, Index1, *PML4E);
			PDPT = (UINT64 *)PHYS2VIRT(*PML4E & (~0xFFF));
			PDPE = PDPT + Index2;
			
			if (*PDPE == 0)
			{
				*PDPE = (FindFreePDT(PgdBase) | DirAttr);
			}
			KDEBUG("PDPT  :%016x PDPT  [%03d] = %016x\n", PDPT, Index2, *PDPE);

			PD = (UINT64 *)(PHYS2VIRT(*PDPE & (~0xFFF)));
			PDE = PD + Index3;
			*PDE = (UINT64)PhysAddr | Attr | PG_PAT;
			KDEBUG("PD    :%016x PD    [%03d] = %016x\n", PD, Index3, *PDE);
			break;
		}
		case PAGE_SIZE_1G://1GB page
		{
			PML4 = PgdBase;
			PML4E = PML4 + Index1;
			
			if (*PML4E == 0)
			{
				*PML4E = (FindFreePDPT(PgdBase) | DirAttr);
			}
			KDEBUG("PML4T :%016x PML4T [%03d] = %016x\n", PML4, Index1, *PML4E);
			PDPT = (UINT64 *)PHYS2VIRT(*PML4E & (~0xFFF));
			PDPE = PDPT + Index2;
			*PDPE = (UINT64)PhysAddr | Attr | PG_PAT;
			KDEBUG("PDPT  :%016x PDPT  [%03d] = %016x\n", PDPT, Index2, *PDPE);
			break;
		}
	}
}
