#include <paging.h>
#include <vmx.h>
#include <kernel.h>
#include "vmcs12.h"

int read_guest_memory_gpa(struct vmx_vcpu *vcpu, u64 gpa, u64 size, void *buffer);

int paging64_gva_to_gpa(struct vmx_vcpu *vcpu, gva_t gva, gpa_t *gpa)
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
	u64 hpa;
	int ret;

	index1 = PML4T_INDEX(gva);
	index2 = PDPT_INDEX(gva);
	index3 = PD_INDEX(gva);
	index4 = PT_INDEX(gva);

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(vcpu->guest_state.ctrl_regs.cr3), &hpa);
	if (ret != 0) {
		printk("soft mmu:transfer PML4T gpa 0x%x to hpa failed.\n", (gpa_t)PT_ENTRY_ADDR(vcpu->guest_state.ctrl_regs.cr3));
		return -EINVAL;
	}

	pml4t = (u64 *)PHYS2VIRT(hpa);
	pml4e = pml4t[index1];

	if ((pml4e & PG_PML4E_PRESENT) == 0) {
		return -EINVAL;
	}

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(pml4e), &hpa);
	if (ret != 0) {
		printk("soft mmu:transfer PDPT gpa 0x%x to hpa failed.\n", PT_ENTRY_ADDR(pml4e));
		return -EINVAL;
	}

	pdpt = (u64 *)PHYS2VIRT(hpa);
	pdpte = pdpt[index2];

	if ((pdpte & PG_PDPTE_PRESENT) == 0) {
		return -EINVAL;
	}
	if (pdpte & PG_PDPTE_1GB_PAGE) {
		*gpa = PG_PDPTE_1GB_PFN(pdpte) + PG_PDPTE_1GB_OFFSET(gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(pdpte), &hpa);
	if (ret != 0) {
		printk("soft mmu:transfer PDT gpa 0x%x to hpa failed.\n", PT_ENTRY_ADDR(pdpte));
		return -EINVAL;
	}

	pdt = (u64 *)PHYS2VIRT(hpa);
	pde = pdt[index3];
	if ((pde & PG_PDE_PRESENT) == 0) {
		return -EINVAL;
	}
	if (pde & PG_PDE_2MB_PAGE) {
		*gpa = PG_PDE_2MB_PFN(pdpte) + PG_PDE_2MB_OFFSET(gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(pde), &hpa);
	if (ret != 0) {
		printk("soft mmu:transfer PT gpa 0x%x to hpa failed.\n", PT_ENTRY_ADDR(pdpte));
		return -EINVAL;
	}
	pt = (u64 *)PHYS2VIRT(hpa);

	pte = pt[index4];
	if ((pte & PG_PT_PRESENT) == 0) {
		return -EINVAL;
	}
	
	*gpa = PG_PT_4KB_PFN(pte) + PG_PT_4KB_OFFSET(gva);
	return 0;
}

int ept_gpa_to_hpa(struct vmx_vcpu *vcpu, gpa_t gpa, hpa_t *hpa)
{
	u64 index1, index2, index3, index4, offset_1g, offset_2m, offset_4k;
	u64 *pml4t, *pdpt, *pdt, *pt;
	u64 pml4e, pdpte, pde, pte;
	u64 *virt;

	pml4t = (u64 *)vcpu->eptp_base;

	index1 = PML4T_INDEX(gpa);
	index2 = PDPT_INDEX(gpa);
	index3 = PD_INDEX(gpa);
	index4 = PT_INDEX(gpa);

	pml4e = pml4t[index1];
	if ((pml4e & (EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE)) == 0) {
		return -EINVAL;
	}

	pdpt = (u64 *)(PHYS2VIRT(PT_ENTRY_ADDR(pml4e)));
	pdpte = pdpt[index2];

	if ((pdpte & (EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	if (pdpte & EPT_PDPTE_1GB_PAGE) {
		*hpa = EPT_PDPTE_1GB_PFN(pdpte) + EPT_PDPTE_1GB_OFFSET(gpa);
		return 0;
	}

	pdt = (u64 *)(PHYS2VIRT(PT_ENTRY_ADDR(pdpte)));
	pde = pdt[index3];

	if ((pde & (EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	if (pde & EPT_PDE_2MB_PAGE) {
		*hpa = EPT_PDE_2MB_PFN(pdpte) + EPT_PDE_2MB_OFFSET(gpa);
		return 0;
	}

	pt = (u64 *)(PHYS2VIRT(PT_ENTRY_ADDR(pde)));
	pte = pt[index4];

	if ((pte & (EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	*hpa = EPT_PT_4KB_PFN(pte) + EPT_PT_4KB_OFFSET(gpa);
	return 0;
}

int ept_gpa_to_hpa_any(u64 *eptp_base, gpa_t gpa, hpa_t *hpa)
{
	u64 index1, index2, index3, index4, offset_1g, offset_2m, offset_4k;
	u64 *pml4t, *pdpt, *pdt, *pt;
	u64 pml4e, pdpte, pde, pte;
	u64 *virt;

	pml4t = eptp_base;

	index1 = PML4T_INDEX(gpa);
	index2 = PDPT_INDEX(gpa);
	index3 = PD_INDEX(gpa);
	index4 = PT_INDEX(gpa);

	pml4e = pml4t[index1];
	if ((pml4e & (EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE)) == 0) {
		return -EINVAL;
	}

	pdpt = (u64 *)(PHYS2VIRT(PT_ENTRY_ADDR(pml4e)));
	pdpte = pdpt[index2];

	if ((pdpte & (EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	if (pdpte & EPT_PDPTE_1GB_PAGE) {
		*hpa = EPT_PDPTE_1GB_PFN(pdpte) + EPT_PDPTE_1GB_OFFSET(gpa);
		return 0;
	}

	pdt = (u64 *)(PHYS2VIRT(PT_ENTRY_ADDR(pdpte)));
	pde = pdt[index3];

	if ((pde & (EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	if (pde & EPT_PDE_2MB_PAGE) {
		*hpa = EPT_PDE_2MB_PFN(pdpte) + EPT_PDE_2MB_OFFSET(gpa);
		return 0;
	}

	pt = (u64 *)(PHYS2VIRT(PT_ENTRY_ADDR(pde)));
	pte = pt[index4];

	if ((pte & (EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	*hpa = EPT_PT_4KB_PFN(pte) + EPT_PT_4KB_OFFSET(gpa);
	return 0;
}


int nested_ept_l2gpa_to_l1gpa(struct vmx_vcpu *vcpu, struct vmcs12 *vmcs12, gpa_t l2gpa, gpa_t *l1gpa)
{
	u64 index1, index2, index3, index4, offset_1g, offset_2m, offset_4k;
	u64 *pml4t, *pdpt, *pdt, *pt;
	u64 pml4e, pdpte, pde, pte;
	u64 *virt;
	int ret;
	hpa_t hpa;
	ret = ept_gpa_to_hpa(vcpu, (gpa_t)PT_ENTRY_ADDR(vmcs12->ept_pointer), &hpa);
	if (ret) {
		printk("nested ept mmu:transfer PML4T gpa 0x%x to hpa failed.\n", (gpa_t)PT_ENTRY_ADDR(vmcs12->ept_pointer));
		return -EINVAL;
	}
	pml4t = (u64 *)PHYS2VIRT(hpa);

	index1 = PML4T_INDEX(l2gpa);
	index2 = PDPT_INDEX(l2gpa);
	index3 = PD_INDEX(l2gpa);
	index4 = PT_INDEX(l2gpa);

	pml4e = pml4t[index1];
	if ((pml4e & (EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE)) == 0) {
		return -EINVAL;
	}

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(pml4e), &hpa);
	if (ret) {
		printk("nested ept mmu:transfer PDPT gpa 0x%x to hpa failed.\n", PT_ENTRY_ADDR(pml4e));
		return -EINVAL;
	}
	pdpt = (u64 *)(PHYS2VIRT(hpa));
	pdpte = pdpt[index2];

	if ((pdpte & (EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	if (pdpte & EPT_PDPTE_1GB_PAGE) {
		*l1gpa = EPT_PDPTE_1GB_PFN(pdpte) + EPT_PDPTE_1GB_OFFSET(l2gpa);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(pdpte), &hpa);
	if (ret) {
		printk("nested ept mmu:transfer PDT gpa 0x%x to hpa failed.\n", PT_ENTRY_ADDR(pdpte));
		return -EINVAL;
	}
	pdt = (u64 *)(PHYS2VIRT(hpa));
	pde = pdt[index3];

	if ((pde & (EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	if (pde & EPT_PDE_2MB_PAGE) {
		*l1gpa = EPT_PDE_2MB_PFN(pdpte) + EPT_PDE_2MB_OFFSET(l2gpa);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, PT_ENTRY_ADDR(pde), &hpa);
	if (ret) {
		printk("nested ept mmu:transfer PT gpa 0x%x to hpa failed.\n", PT_ENTRY_ADDR(pde));
		return -EINVAL;
	}
	pt = (u64 *)(PHYS2VIRT(hpa));
	pte = pt[index4];

	if ((pte & (EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE)) == 0) {
		return -EINVAL;
	}

	*l1gpa = EPT_PT_4KB_PFN(pte) + EPT_PT_4KB_OFFSET(l2gpa);
	return 0;
}

