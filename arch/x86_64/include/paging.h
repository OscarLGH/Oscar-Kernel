#ifndef _PAGING_H_
#define _PAGING_H_

#include <types.h>

/* Page attributes for paging */
#define PG_PML4E_PRESENT (1 << 0)
#define PG_PML4E_READ_WRITE (1 << 1)
#define PG_PML4E_READ_ONLY (0　<< 1)
#define PG_PML4E_USER (1 << 2)
#define PG_PML4E_SUPERVISOR (0 << 2)
#define PG_PML4E_CACHE_WT (1 << 3)
#define PG_PML4E_CACHE_DISABLE (1 << 4)
#define PG_PML4E_ACCESS_FLAG (1 << 5)
#define PG_PML4E_EXECUTE_DISABLE (1 << 63)

#define PG_PDPTE_PRESENT (1 << 0)
#define PG_PDPTE_READ_WRITE (1 << 1)
#define PG_PDPTE_READ_ONLY (0　<< 1)
#define PG_PDPTE_USER (1 << 2)
#define PG_PDPTE_SUPERVISOR (0 << 2)
#define PG_PDPTE_CACHE_WT (1 << 3)
#define PG_PDPTE_CACHE_DISABLE (1 << 4)
#define PG_PDPTE_ACCESS_FLAG (1 << 5)
#define PG_PDPTE_DIRTY_FLAG (1 << 6)
#define PG_PDPTE_1GB_PAGE (1 << 7)
#define PG_PDPTE_1GB_GLOBAL (1 << 8)
#define PG_PDPTE_1GB_PAT (1 << 12)
#define PG_PDPTE_1GB_OFFSET(x) ((x) & 0xffffffffc0000000)
#define PG_PDPTE_1BG_PROTECTION_KEY(x) ((x & 0xf) << 59)
#define PG_PDPTE_EXECUTE_DISABLE (1 << 63)

#define PG_PDE_PRESENT (1 << 0)
#define PG_PDE_READ_WRITE (1 << 1)
#define PG_PDE_READ_ONLY (0　<< 1)
#define PG_PDE_USER (1 << 2)
#define PG_PDE_SUPERVISOR (0 << 2)
#define PG_PDE_CACHE_WT (1 << 3)
#define PG_PDE_CACHE_DISABLE (1 << 4)
#define PG_PDE_ACCESS_FLAG (1 << 5)
#define PG_PDE_DIRTY_FLAG (1 << 6)
#define PG_PDE_2MB_PAGE (1 << 7)
#define PG_PDE_2MB_GLOBAL (1 << 8)
#define PG_PDE_2MB_PAT (1 << 12)
#define PG_PDE_2MB_OFFSET(x) ((x) & 0xffffffffffe00000)
#define PG_PDE_2MB_PROTECTION_KEY(x) ((x & 0xf) << 59)
#define PG_PDE_EXECUTE_DISABLE (1 << 63)

#define PG_PT_PRESENT (1 << 0)
#define PG_PT_READ_WRITE (1 << 1)
#define PG_PT_READ_ONLY (0　<< 1)
#define PG_PT_USER (1 << 2)
#define PG_PT_SUPERVISOR (0 << 2)
#define PG_PT_CACHE_WT (1 << 3)
#define PG_PT_CACHE_DISABLE (1 << 4)
#define PG_PT_ACCESS_FLAG (1 << 5)
#define PG_PT_DIRTY_FLAG (1 << 6)
#define PG_PT_PAT (1 << 7)
#define PG_PT_GLOBAL (1 << 8)
#define PG_PT_4KB_OFFSET(x) ((x) & 0xfffffffffffff000)
#define PG_PT_PROTECTION_KEY(x) ((x & 0xf) << 59)
#define PG_PT_EXECUTE_DISABLE (1 << 63)

#define PG_FAULT_PRESENT (1 << 0)
#define PG_FAULT_WR (1 << 1)
#define PG_FAULT_US (1 << 2)
#define PG_FAULT_RSVD (1 << 3)
#define PG_FAULT_ID (1 << 4)
#define PG_FAULT_PK (1 << 5)
#define PG_FAULT_SGX (1 << 15)

static inline u64 read_cr3()
{
	u64 cr3;
	asm volatile("movq %%cr3, %0\n\t"
				: "=a"(cr3) :
		);
	return cr3;
}

static inline void write_cr3(u64 cr3)
{
	asm volatile("movq %0, %%cr3\n\t"
				: : "rdi"(cr3)
		);
}


static inline u64 read_cr2()
{
	u64 cr2;
	/* Stop speculative execution by lfence */
	asm volatile("movq %%cr2, %0\n\t"
				: "=a"(cr2) :
		);
	return cr2;
}

static inline void write_pg(u64 *table, u64 index, u64 value)
{
	table[index] = value;
}

static inline u64 read_pg(u64 *table, u64 index)
{
	return table[index];
}


/* EPTP Attributes. */
#define EPTP_CACHE_WB 0x6
#define EPTP_UNCACHEABLE 0x0
#define EPTP_PAGE_WALK_LEN(x) (((x)&0x7)<<3)
#define EPTP_ENABLE_A_D_FLAGS (1 << 6)
#define EPT_IGNORE_PAT (1 << 6)

/* EPT Attributes. */
#define EPT_PML4E_READ (1 << 0)
#define EPT_PML4E_WRITE (1 << 1)
#define EPT_PML4E_EXECUTE (1 << 2)
#define EPT_PML4E_ACCESS_FLAG (1 << 8)

#define EPT_PDPTE_READ (1 << 0)
#define EPT_PDPTE_WRITE (1 << 1)
#define EPT_PDPTE_EXECUTE (1 << 2)
#define EPT_PDPTE_UNCACHEABLE (0 << 3)
#define EPT_PDPTE_CACHE_WC (1 << 3)
#define EPT_PDPTE_CACHE_WT (4 << 3)
#define EPT_PDPTE_CACHE_WP (5 << 3)
#define EPT_PDPTE_CACHE_WB (6 << 3)
#define EPT_PDPTE_1GB_IGNORE_PAT (1 << 6)
#define EPT_PDPTE_1GB_PAGE (1 << 7)
#define EPT_PDPTE_ACCESS_FLAG (1 << 8)
#define EPT_PDPTE_DIRTY_FLAG (1 << 9)
#define EPT_PDPTE_1GB_OFFSET(x) ((x) & 0xffffffffc0000000)


#define EPT_PDE_READ (1 << 0)
#define EPT_PDE_WRITE (1 << 1)
#define EPT_PDE_EXECUTE (1 << 2)
#define EPT_PDE_UNCACHEABLE (0 << 3)
#define EPT_PDE_CACHE_WC (1 << 3)
#define EPT_PDE_CACHE_WT (4 << 3)
#define EPT_PDE_CACHE_WP (5 << 3)
#define EPT_PDE_CACHE_WB (6 << 3)
#define EPT_PDE_2MB_IGNORE_PAT (1 << 6)
#define EPT_PDE_2MB_PAGE (1 << 7)
#define EPT_PDE_ACCESS_FLAG (1 << 8)
#define EPT_PDE_DIRTY_FLAG (1 << 9)
#define EPT_PDE_2MB_OFFSET(x) ((x) & 0xffffffffffe00000)


#define EPT_PTE_READ (1 << 0)
#define EPT_PTE_WRITE (1 << 1)
#define EPT_PTE_EXECUTE (1 << 2)
#define EPT_PTE_UNCACHEABLE (0 << 3)
#define EPT_PTE_CACHE_WC (1 << 3)
#define EPT_PTE_CACHE_WT (4 << 3)
#define EPT_PTE_CACHE_WP (5 << 3)
#define EPT_PTE_CACHE_WB (6 << 3)
#define EPT_PTE_CACHE_IGNORE_PAT (1 << 6)
#define EPT_PTE_ACCESS_FLAG (1 << 8)
#define EPT_PTE_DIRTY_FLAG (1 << 9)
#define EPT_PT_4KB_OFFSET(x) ((x) & 0xfffffffffffff000)

#endif
