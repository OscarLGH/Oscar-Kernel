#include <mm.h>
#include <task.h>
#include <paging.h>
#include <cr.h>
#include <string.h>

int alloc_new_page(u64 addr, int page_size, int cpl, bool rd_only, bool xd)
{
	u64 *pml4t;
	u64 index1;
	u64 pml4e;
	u64 *pdpt;
	u64 index2;
	u64 pdpte;
	u64 *pdt;
	u64 index3;
	u64 pde;
	u64 *pt;
	u64 index4;
	u64 pte;
	u64 offset;
	void *ptr = NULL;
	u64 attr;

	attr = ((cpl == 0) ? PG_PT_SUPERVISOR : PG_PT_USER) |
		((rd_only == 0) ? PG_PT_READ_WRITE : PG_PT_READ_ONLY) |
		((xd == 1) ? PG_PT_EXECUTE_DISABLE : 0);
	

	index1 = PML4T_INDEX(addr);
	index2 = PDPT_INDEX(addr);
	index3 = PD_INDEX(addr);
	index4 = PT_INDEX(addr);

	pml4t = (u64 *)PHYS2VIRT(PT_ENTRY_ADDR(read_cr3()));

	pml4e = pml4t[index1];
	if (PT_ENTRY_ADDR(pml4e) == 0) {
		ptr = kmalloc(0x1000, GFP_KERNEL);
		memset(ptr, 0, 0x1000);
		pml4t[index1] = VIRT2PHYS(ptr) | PG_PML4E_PRESENT | PG_PML4E_READ_WRITE | PG_PML4E_USER;
		pml4e = pml4t[index1];
	}

	pdpt = (u64 *)PHYS2VIRT(PT_ENTRY_ADDR(pml4e));

	pdpte = pdpt[index2];
	if (PT_ENTRY_ADDR(pdpte) == 0) {
		if (page_size == 0x40000000) {
			ptr = kmalloc(0x40000000, GFP_KERNEL);
			pdpt[index2] = VIRT2PHYS(ptr) | PG_PDPTE_PRESENT | PG_PDPTE_1GB_PAGE | attr;
			return 0;
		} else {
			ptr = kmalloc(0x1000, GFP_KERNEL);
			memset(ptr, 0, 0x1000);
			pdpt[index2] = VIRT2PHYS(ptr) | PG_PDPTE_PRESENT | PG_PDPTE_READ_WRITE | PG_PDPTE_USER;
			pdpte = pdpt[index2];
		}
	}

	pdt = (u64 *)PHYS2VIRT(PT_ENTRY_ADDR(pdpte));

	pde = pdt[index3];
	if (PT_ENTRY_ADDR(pde) == 0) {
		if (page_size == 0x200000) {
			ptr = kmalloc(0x200000, GFP_KERNEL);
			pdt[index3] = VIRT2PHYS(ptr) | PG_PDE_PRESENT | PG_PDE_2MB_PAGE | attr;
			return 0;
		} else {
			ptr = kmalloc(0x1000, GFP_KERNEL);
			memset(ptr, 0, 0x1000);
			pdt[index3] = VIRT2PHYS(ptr) | PG_PDE_PRESENT | PG_PDE_READ_WRITE | PG_PDE_USER;
			pde = pdt[index3];
		}
	}

	pt = (u64 *)PHYS2VIRT(PT_ENTRY_ADDR(pde));

	pte = pt[index4];
	if (PT_ENTRY_ADDR(pte) == 0) {
		ptr = kmalloc(0x1000, GFP_KERNEL);
		memset(ptr, 0, 0x1000);
		pt[index4] = VIRT2PHYS(ptr) | PG_PT_PRESENT | attr;
	}

	return 0;
}


int address_check(u64 addr, int cpl)
{
	if (cpl != 0) {
		if (VIRT2PHYS(addr) > addr) {
			return -EINVAL;
		}
	}

	if (addr == 0xffffffff00000000)
		return -EINVAL;

	return 0;
}

int page_fault_mm(u64 addr, u64 err_code, int cpl)
{
	int ret;
	struct task_struct *task_ptr = get_current_task();
	printk("handle page fault @0x%x error code 0x%x cpl = %d\n", addr, err_code, cpl);
	/* Checking addr */
	ret = address_check(addr, cpl);
	if (ret) {
		printk("task %d killed.\n", task_ptr->id);
		task_exit(task_ptr);
		return ret;
	}

	ret = alloc_new_page(addr, 0x1000, cpl, 0, 0);
	return ret;
}
