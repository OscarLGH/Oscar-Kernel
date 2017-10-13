//=====================================================================
//Page.h
//内存分页
//Oscar 2015.7
//=====================================================================
#ifndef _PAGE_H
#define _PAGE_H

#include "Type.h"
#include "SystemParameters.h"

//#define DEBUG
#include "Debug.h"

#define PG_P    (1LL<<0)
	
#define PG_RWW	(1LL<<1)
#define PG_RWR	(0LL<<1)

#define PG_USS	(0LL<<2)	
#define PG_USU	(1LL<<2)
	
#define PG_PWT	(1LL<<3)	
#define PG_PCD	(1LL<<4)	
#define PG_A	(1LL<<5)	
#define PG_D	(1LL<<6)	
#define PG_PAT	(1LL<<7)	
#define PG_G	(1LL<<8)	
#define PG_XD	(1LL<<63)

#define KERNEL_PAGE_BASE	0x10000

#define PML4T_OFFSET 0
#define PDPT_OFFSET  (PML4T_OFFSET + 0x1000)
#define PDT_OFFSET   (PDPT_OFFSET  + 0x1000 * 16)
#define PT_OFFSET    (PDT_OFFSET   + 0x10000)

#define PDPT_USER_START (PDPT_OFFSET + 0x1000)
#define PDPT_USER_END PDT_OFFSET

#define PDT_USER_START (PDT_OFFSET + 128 * 0x1000)		
#define PDT_USER_END 0x100000

#define PAGE_SIZE_4K 1
#define PAGE_SIZE_2M 2
#define PAGE_SIZE_1G 3

VOID * GetPageTableBase(void);
UINT64 SetPageTableBase(VOID * PageTableBase);
VOID * GetPageFaultAddr(void);
VOID * GetMaxPhysAddr(void);
VOID * GetMaxVirtAddr(void);
void RefreshTLB(UINT64 Addr);

void MapPage(UINT64 * PgdBase, VOID * VirtAddr, VOID * PhysAddr, UINT64 PageSize, UINT64 Attr);
int printk(const char *fmt, ...);

#endif
