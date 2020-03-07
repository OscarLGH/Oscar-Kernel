#include <vmx.h>
#include <cpuid.h>
#include <paging.h>
#include <msr.h>
#include <cr.h>
#include <kernel.h>
#include <string.h>
#include <mm.h>
#include <math.h>
#include <list.h>
#include <fpu.h>

int vmx_enable()
{
	u32 regs[4] = {0};
	u64 msr;
	u64 vmx_msr;
	cpuid(0x00000001, 0x00000000, &regs[0]);
	if ((regs[2] & CPUID_FEATURES_ECX_VMX) == 0) {
		printk("VMX is unsupported on this CPU!.\n");
		return -1;
	}

	msr = rdmsr(MSR_IA32_FEATURE_CONTROL);
	vmx_msr = FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX | FEATURE_CONTROL_VMXON_ENABLED_INSIDE_SMX;

	if ((msr & FEATURE_CONTROL_LOCKED) == FEATURE_CONTROL_LOCKED) {
		if ((msr & vmx_msr) == 0) {
			printk("VMX is supported, but disabled in BIOS.\n");
			return -2;
		}
	} else {
		msr |= (FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX | FEATURE_CONTROL_LOCKED);
		wrmsr(MSR_IA32_FEATURE_CONTROL, msr);
	}

	cr0_set_bits(CR0_NE | CR0_MP);
	cr4_set_bits(CR4_VMXE);
	return 0;
}

int vmx_disable()
{
	cr4_clear_bits(CR4_VMXE);
	return 0;
}

u64 vcpu_cnt = 0;
struct vmx_vcpu *vmx_preinit()
{
	int ret;
	struct vmx_vcpu *vcpu_ptr = kmalloc(sizeof(*vcpu_ptr), GFP_KERNEL);
	memset(vcpu_ptr, 0, sizeof(*vcpu_ptr));
	INIT_LIST_HEAD(&vcpu_ptr->guest_memory_list);
	vcpu_ptr->vmxon_region = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu_ptr->vmxon_region, 0, 0x1000);
	vcpu_ptr->vmxon_region_phys = VIRT2PHYS(vcpu_ptr->vmxon_region);
	vcpu_ptr->vmcs01 = kmalloc(0x1000, GFP_KERNEL);
	vcpu_ptr->vmcs01_phys = VIRT2PHYS(vcpu_ptr->vmcs01);
	memset(vcpu_ptr->vmcs01, 0, 0x1000);
	vcpu_ptr->vmcs02 = kmalloc(0x1000, GFP_KERNEL);
	vcpu_ptr->vmcs02_phys = VIRT2PHYS(vcpu_ptr->vmcs02);
	memset(vcpu_ptr->vmcs02, 0, 0x1000);
	vcpu_ptr->virtual_processor_id = ++vcpu_cnt;

	ret = vmx_enable();
	if (ret) {
		return NULL;
	}

	vcpu_ptr->vmxon_region->revision_id = rdmsr(MSR_IA32_VMX_BASIC);
	vcpu_ptr->vmcs01->revision_id = rdmsr(MSR_IA32_VMX_BASIC);
	vcpu_ptr->vmcs02->revision_id = rdmsr(MSR_IA32_VMX_BASIC);
	vmx_on(vcpu_ptr->vmxon_region_phys);
	vmclear(vcpu_ptr->vmcs01_phys);
	vmclear(vcpu_ptr->vmcs02_phys);
	vmptr_load(vcpu_ptr->vmcs01_phys);
	return vcpu_ptr;
}

int vmx_init(struct vmx_vcpu * vcpu)
{
	vcpu->vapic_page = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->vapic_page, 0, 0x1000);
	vcpu->io_bitmap_a = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->io_bitmap_a, 0, 0x1000);
	vcpu->io_bitmap_b = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->io_bitmap_b, 0, 0x1000);
	vcpu->msr_bitmap = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->msr_bitmap, 0, 0x1000);
	vcpu->eptp_base = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->eptp_base, 0, 0x1000);
	vcpu->host_state.fp_regs = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->host_state.fp_regs, 0, 0x1000);
	vcpu->host_state.msr = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->host_state.msr, 0, 0x1000);
	vcpu->guest_state.fp_regs = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->guest_state.fp_regs, 0, 0x1000);
	vcpu->guest_state.msr = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->guest_state.msr, 0, 0x1000);
	vcpu->posted_intr_addr = kmalloc(0x1000, GFP_KERNEL);
	memset(vcpu->posted_intr_addr, 0, 0x1000);

	return 0;
}

int vmx_set_ctrl_state(struct vmx_vcpu *vcpu)
{
	u64 pin_based_vm_exec_ctrl;
	u64 cpu_based_vm_exec_ctrl;
	u64 cpu_based_vm_exec_ctrl2;
	u64 vm_entry_ctrl;
	u64 vm_exit_ctrl;

	u32 pin_based_allow1_mask;
	u32 pin_based_allow0_mask;
	u32 cpu_based_allow1_mask;
	u32 cpu_based_allow0_mask;
	u32 cpu_based2_allow1_mask;
	u32 cpu_based2_allow0_mask;
	u32 vm_entry_allow1_mask;
	u32 vm_entry_allow0_mask;
	u32 vm_exit_allow1_mask;
	u32 vm_exit_allow0_mask;

	vmcs_write(VIRTUAL_PROCESSOR_ID, vcpu->virtual_processor_id);
	vmcs_write(IO_BITMAP_A, VIRT2PHYS(vcpu->io_bitmap_a));
	vmcs_write(IO_BITMAP_B, VIRT2PHYS(vcpu->io_bitmap_b));
	vmcs_write(MSR_BITMAP, VIRT2PHYS(vcpu->msr_bitmap));
	vmcs_write(VM_ENTRY_MSR_LOAD_ADDR, VIRT2PHYS(vcpu->guest_state.msr));
	vmcs_write(VM_EXIT_MSR_STORE_ADDR, VIRT2PHYS(vcpu->guest_state.msr));
	vmcs_write(VM_EXIT_MSR_LOAD_ADDR, VIRT2PHYS(vcpu->host_state.msr));

	vmcs_write(VIRTUAL_APIC_PAGE_ADDR, VIRT2PHYS(vcpu->vapic_page));
	vmcs_write(EPT_POINTER, VIRT2PHYS(vcpu->eptp_base) | 0x5e);

	pin_based_vm_exec_ctrl = rdmsr(MSR_IA32_VMX_PINBASED_CTLS);
	cpu_based_vm_exec_ctrl = rdmsr(MSR_IA32_VMX_PROCBASED_CTLS);
	cpu_based_vm_exec_ctrl2 = rdmsr(MSR_IA32_VMX_PROCBASED_CTLS2);
	vm_entry_ctrl = rdmsr(MSR_IA32_VMX_ENTRY_CTLS);
	vm_exit_ctrl = rdmsr(MSR_IA32_VMX_EXIT_CTLS);

	pin_based_allow1_mask = pin_based_vm_exec_ctrl >> 32;
	pin_based_allow0_mask = pin_based_vm_exec_ctrl;
	cpu_based_allow1_mask = cpu_based_vm_exec_ctrl >> 32;
	cpu_based_allow0_mask = cpu_based_vm_exec_ctrl;
	cpu_based2_allow1_mask = cpu_based_vm_exec_ctrl2 >> 32;
	cpu_based2_allow0_mask = cpu_based_vm_exec_ctrl2;
	vm_entry_allow1_mask = vm_entry_ctrl >> 32;
	vm_entry_allow0_mask = vm_entry_ctrl;
	vm_exit_allow1_mask = vm_exit_ctrl >> 32;
	vm_exit_allow0_mask = vm_exit_ctrl;

	pin_based_vm_exec_ctrl =  PIN_BASED_ALWAYSON_WITHOUT_TRUE_MSR
		| PIN_BASED_EXT_INTR_MASK
		| PIN_BASED_POSTED_INTR
		//| PIN_BASED_VMX_PREEMPTION_TIMER
		;

	vmcs_write(VMX_PREEMPTION_TIMER_VALUE, 2100000000 / 128);

	if ((pin_based_vm_exec_ctrl | pin_based_allow1_mask) != pin_based_allow1_mask) {
		printk("Warning:setting pin_based_vm_exec_control:%x unsupported.\n", 
			(pin_based_vm_exec_ctrl & pin_based_allow1_mask) ^ pin_based_vm_exec_ctrl);
		pin_based_vm_exec_ctrl |= pin_based_allow0_mask;
		pin_based_vm_exec_ctrl &= pin_based_allow1_mask;
	}
	vmcs_write(PIN_BASED_VM_EXEC_CONTROL, pin_based_vm_exec_ctrl);
	cpu_based_vm_exec_ctrl = CPU_BASED_FIXED_ONES
		| CPU_BASED_ACTIVATE_SECONDARY_CONTROLS
		//| CPU_BASED_USE_IO_BITMAPS
		| CPU_BASED_USE_MSR_BITMAPS
		| CPU_BASED_TPR_SHADOW
		| CPU_BASED_UNCOND_IO_EXITING
		//| CPU_BASED_CR3_LOAD_EXITING
		//| CPU_BASED_MONITOR_TRAP_FLAG
		;
	if ((cpu_based_vm_exec_ctrl | cpu_based_allow1_mask) != cpu_based_allow1_mask) {
		printk("Warning:setting cpu_based_vm_exec_control:%x unsupported.\n", 
			(cpu_based_vm_exec_ctrl & cpu_based_allow1_mask) ^ cpu_based_vm_exec_ctrl);
		cpu_based_vm_exec_ctrl |= cpu_based_allow0_mask;
		cpu_based_vm_exec_ctrl &= cpu_based_allow1_mask;
	}
	vmcs_write(CPU_BASED_VM_EXEC_CONTROL, cpu_based_vm_exec_ctrl);

	cpu_based_vm_exec_ctrl2 = SECONDARY_EXEC_UNRESTRICTED_GUEST
		| SECONDARY_EXEC_ENABLE_EPT
		| SECONDARY_EXEC_ENABLE_VPID
		| SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES
		| SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY
		| SECONDARY_EXEC_APIC_REGISTER_VIRT
		;
	if ((cpu_based_vm_exec_ctrl2 | cpu_based2_allow1_mask) != cpu_based2_allow1_mask) {
		printk("Warning:setting secondary_vm_exec_control:%x unsupported.\n", 
			(cpu_based_vm_exec_ctrl2 & cpu_based2_allow1_mask) ^ cpu_based_vm_exec_ctrl2);
		cpu_based_vm_exec_ctrl2 |= cpu_based2_allow0_mask;
		cpu_based_vm_exec_ctrl2 &= cpu_based2_allow1_mask;
	}
	vmcs_write(SECONDARY_VM_EXEC_CONTROL, cpu_based_vm_exec_ctrl2);

	vm_entry_ctrl = VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR
		| VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL
		| VM_ENTRY_LOAD_IA32_EFER;
	if ((vm_entry_ctrl | vm_entry_allow1_mask) != vm_entry_allow1_mask) {
		printk("Warning:setting vm_entry_controls:%x unsupported.\n", 
			(vm_entry_ctrl & vm_entry_allow1_mask) ^ vm_entry_ctrl);
		vm_entry_ctrl |= vm_entry_allow0_mask;
		vm_entry_ctrl &= vm_entry_allow1_mask;
	}
	vmcs_write(VM_ENTRY_CONTROLS, vm_entry_ctrl);

	vm_exit_ctrl = VM_EXIT_ALWAYSON_WITHOUT_TRUE_MSR
		| VM_EXIT_SAVE_IA32_EFER
		| VM_EXIT_LOAD_IA32_EFER
		| VM_EXIT_ACK_INTR_ON_EXIT
		| VM_EXIT_HOST_ADDR_SPACE_SIZE
		//| VM_EXIT_SAVE_VMX_PREEMPTION_TIMER
		;
	if ((vm_exit_ctrl | vm_exit_allow1_mask) != vm_exit_allow1_mask) {
		printk("Warning:setting vm_exit_controls:%x unsupported.\n", 
			(vm_exit_ctrl & vm_exit_allow1_mask) ^ vm_exit_ctrl);
		vm_exit_ctrl |= vm_exit_allow0_mask;
		vm_exit_ctrl &= vm_exit_allow1_mask;
	}
	vmcs_write(VM_EXIT_CONTROLS, vm_exit_ctrl);
	vmcs_write(POSTED_INTR_DESC_ADDR, VIRT2PHYS(vcpu->posted_intr_addr));
	vmcs_write(CR3_TARGET_COUNT, 0);
	vmcs_write(CR0_GUEST_HOST_MASK, rdmsr(MSR_IA32_VMX_CR0_FIXED0) & rdmsr(MSR_IA32_VMX_CR0_FIXED1) & 0xfffffffe);
	vmcs_write(CR4_GUEST_HOST_MASK, rdmsr(MSR_IA32_VMX_CR4_FIXED0) & rdmsr(MSR_IA32_VMX_CR4_FIXED1));
	vmcs_write(EXCEPTION_BITMAP, 0xffffffff);

	return 0;
}

int vmx_save_host_state(struct vmx_vcpu *vcpu)
{
	struct gdtr host_gdtr;
	struct idtr host_idtr;
	u64 host_rsp;
	extern void vm_exit();

	vmcs_write(HOST_CR0, read_cr0());
	vmcs_write(HOST_CR3, read_cr3());
	vmcs_write(HOST_CR4, read_cr4());

	sgdt(&host_gdtr);
	sidt(&host_idtr);

	vmcs_write(HOST_GDTR_BASE, host_gdtr.base);
	vmcs_write(HOST_IDTR_BASE, host_idtr.base);

	vmcs_write(HOST_CS_SELECTOR, save_cs());
	vmcs_write(HOST_DS_SELECTOR, save_ds());
	vmcs_write(HOST_ES_SELECTOR, save_es());
	vmcs_write(HOST_FS_SELECTOR, save_fs());
	vmcs_write(HOST_GS_SELECTOR, save_gs());
	vmcs_write(HOST_SS_SELECTOR, save_ss());
	vmcs_write(HOST_TR_SELECTOR, str());
	vmcs_write(HOST_GS_BASE, rdmsr(MSR_GS_BASE));
	vmcs_write(HOST_FS_BASE, rdmsr(MSR_FS_BASE));
	vmcs_write(HOST_IA32_SYSENTER_CS, rdmsr(MSR_IA32_SYSENTER_CS));
	vmcs_write(HOST_IA32_SYSENTER_ESP, rdmsr(MSR_IA32_SYSENTER_ESP));
	vmcs_write(HOST_IA32_SYSENTER_EIP, rdmsr(MSR_IA32_SYSENTER_EIP));
	vmcs_write(HOST_IA32_EFER, rdmsr(MSR_EFER));
	vmcs_write(HOST_IA32_PAT, rdmsr(MSR_IA32_CR_PAT));
	vmcs_write(HOST_IA32_PERF_GLOBAL_CTRL, rdmsr(MSR_IA32_PERF_CTL));

	asm volatile("mov %%rsp, %0\n\t"
		:"=r"(host_rsp):);
	vmcs_write(HOST_RSP, host_rsp + 8);
	vmcs_write(HOST_RIP, (u64)vm_exit);

	/* save host FPU states...*/
	vcpu->host_state.ctrl_regs.xcr0 = xgetbv(0);
	xsave(vcpu->host_state.fp_regs, vcpu->host_state.ctrl_regs.xcr0);
	return 0;
}

int vmx_save_guest_state(struct vmx_vcpu *vcpu)
{
	vcpu->guest_state.rip = vmcs_read(GUEST_RIP);
	vcpu->guest_state.ctrl_regs.cr0 = vmcs_read(GUEST_CR0);
	vcpu->guest_state.ctrl_regs.cr2 = read_cr2();
	vcpu->guest_state.ctrl_regs.cr3 = vmcs_read(GUEST_CR3);
	vcpu->guest_state.ctrl_regs.cr4 = vmcs_read(GUEST_CR4);
	vcpu->guest_state.ctrl_regs.cr8 = read_cr8();
	vcpu->guest_state.ctrl_regs.xcr0 = xgetbv(0);
	xsave(vcpu->guest_state.fp_regs, vcpu->guest_state.ctrl_regs.xcr0);
	return 0;
}

void dump_guest_state(struct vmx_vcpu *vcpu);

int vmx_set_guest_state(struct vmx_vcpu *vcpu)
{
	u64 cr0, cr4;
	cr0 = vcpu->guest_state.ctrl_regs.cr0 | rdmsr(MSR_IA32_VMX_CR0_FIXED0) & rdmsr(MSR_IA32_VMX_CR0_FIXED1);

	if ((vmcs_read(SECONDARY_VM_EXEC_CONTROL) & SECONDARY_EXEC_UNRESTRICTED_GUEST) != 0) {
		cr0 &= 0x7ffffffe;
	}

	cr4 = rdmsr(MSR_IA32_VMX_CR4_FIXED0) & rdmsr(MSR_IA32_VMX_CR4_FIXED1);
	vmcs_write(GUEST_CR0, cr0);
	vmcs_write(CR0_READ_SHADOW, vcpu->guest_state.cr0_read_shadow);
	vmcs_write(GUEST_CR3, vcpu->guest_state.ctrl_regs.cr3);
	vmcs_write(GUEST_CR4, cr4);
	vmcs_write(CR4_READ_SHADOW, vcpu->guest_state.cr4_read_shadow);

	vmcs_write(GUEST_RIP, vcpu->guest_state.rip);
	vmcs_write(GUEST_RFLAGS, vcpu->guest_state.rflags);
	
	vmcs_write(GUEST_RSP, vcpu->guest_state.gr_regs.rsp);
	vmcs_write(GUEST_DR7, 0);
	vmcs_write(GUEST_IA32_DEBUGCTL, 0);

	vmcs_write(GUEST_CS_SELECTOR, vcpu->guest_state.cs.selector);
	vmcs_write(GUEST_CS_BASE, vcpu->guest_state.cs.base);
	vmcs_write(GUEST_CS_LIMIT, vcpu->guest_state.cs.limit);
	vmcs_write(GUEST_CS_AR_BYTES, vcpu->guest_state.cs.ar_bytes);

	vmcs_write(GUEST_DS_SELECTOR, vcpu->guest_state.ds.selector);
	vmcs_write(GUEST_DS_BASE, vcpu->guest_state.ds.base);
	vmcs_write(GUEST_DS_LIMIT, vcpu->guest_state.ds.limit);
	vmcs_write(GUEST_DS_AR_BYTES, vcpu->guest_state.ds.ar_bytes);

	vmcs_write(GUEST_ES_SELECTOR, vcpu->guest_state.es.selector);
	vmcs_write(GUEST_ES_BASE, vcpu->guest_state.es.base);
	vmcs_write(GUEST_ES_LIMIT, vcpu->guest_state.es.limit);
	vmcs_write(GUEST_ES_AR_BYTES, vcpu->guest_state.es.ar_bytes);

	vmcs_write(GUEST_FS_SELECTOR, vcpu->guest_state.fs.selector);
	vmcs_write(GUEST_FS_BASE, vcpu->guest_state.fs.base);
	vmcs_write(GUEST_FS_LIMIT, vcpu->guest_state.fs.limit);
	vmcs_write(GUEST_FS_AR_BYTES, vcpu->guest_state.fs.ar_bytes);

	vmcs_write(GUEST_GS_SELECTOR, vcpu->guest_state.gs.selector);
	vmcs_write(GUEST_GS_BASE, vcpu->guest_state.gs.base);
	vmcs_write(GUEST_GS_LIMIT, vcpu->guest_state.gs.limit);
	vmcs_write(GUEST_GS_AR_BYTES, vcpu->guest_state.gs.ar_bytes);

	vmcs_write(GUEST_SS_SELECTOR, vcpu->guest_state.ss.selector);
	vmcs_write(GUEST_SS_BASE, vcpu->guest_state.ss.base);
	vmcs_write(GUEST_SS_LIMIT, vcpu->guest_state.ss.limit);
	vmcs_write(GUEST_SS_AR_BYTES, vcpu->guest_state.ss.ar_bytes);	

	vmcs_write(GUEST_TR_SELECTOR, vcpu->guest_state.tr.selector);
	vmcs_write(GUEST_TR_BASE, vcpu->guest_state.tr.base);
	vmcs_write(GUEST_TR_LIMIT, vcpu->guest_state.tr.limit);
	vmcs_write(GUEST_TR_AR_BYTES, vcpu->guest_state.tr.ar_bytes);
	
	vmcs_write(GUEST_LDTR_SELECTOR, vcpu->guest_state.ldtr.selector);
	vmcs_write(GUEST_LDTR_BASE, vcpu->guest_state.ldtr.base);
	vmcs_write(GUEST_LDTR_LIMIT, vcpu->guest_state.ldtr.limit);
	vmcs_write(GUEST_LDTR_AR_BYTES, vcpu->guest_state.ldtr.ar_bytes);

	vmcs_write(GUEST_GDTR_BASE, vcpu->guest_state.gdtr.base);
	vmcs_write(GUEST_GDTR_LIMIT, vcpu->guest_state.gdtr.limit);
	vmcs_write(GUEST_IDTR_BASE, vcpu->guest_state.idtr.base);
	vmcs_write(GUEST_IDTR_LIMIT, vcpu->guest_state.idtr.limit);

	vmcs_write(GUEST_SYSENTER_ESP, 0);
	vmcs_write(GUEST_SYSENTER_EIP, 0);
	vmcs_write(GUEST_SYSENTER_CS, 0);
	vmcs_write(GUEST_IA32_DEBUGCTL, 0);
	vmcs_write(GUEST_IA32_PERF_GLOBAL_CTRL, 0);
	vmcs_write(GUEST_IA32_EFER, 0);
	vmcs_write(GUEST_IA32_PAT, 0);
	
	vmcs_write(GUEST_ACTIVITY_STATE, 0);

	vmcs_write(GUEST_PDPTR0, vcpu->guest_state.pdpte0);
	vmcs_write(GUEST_PDPTR1, vcpu->guest_state.pdpte1);
	vmcs_write(GUEST_PDPTR2, vcpu->guest_state.pdpte2);
	vmcs_write(GUEST_PDPTR3, vcpu->guest_state.pdpte3);

	vmcs_write(GUEST_INTERRUPTIBILITY_INFO, 0);
	vmcs_write(VM_ENTRY_INTR_INFO_FIELD, 0);
	vmcs_write(GUEST_PENDING_DBG_EXCEPTIONS, 0);
	vmcs_write(GUEST_BNDCFGS, 0);

	vmcs_write(VMCS_LINK_POINTER, 0xffffffffffffffff);

	vmcs_write(APIC_ACCESS_ADDR, 0xfee00000);

	xsetbv(0, vcpu->guest_state.ctrl_regs.xcr0);
	//xrstor(vcpu->guest_state.fp_regs, vcpu->guest_state.ctrl_regs.xcr0);
	/* Use host XCRO features to zero out all host fpu regs. */
	xrstor(vcpu->guest_state.fp_regs, vcpu->host_state.ctrl_regs.xcr0);
	//xrstor(vcpu->guest_state.fp_regs, XCR0_X87 | XCR0_SSE | XCR0_AVX);

	return 0;
}

int vmx_resume_host_state(struct vmx_vcpu *vcpu)
{
	xsetbv(0, vcpu->host_state.ctrl_regs.xcr0);
	xrstor(vcpu->host_state.fp_regs, vcpu->host_state.ctrl_regs.xcr0);
	return 0;
}

int ept_map_page(u64 *eptp_base, u64 gpa, u64 hpa, u64 page_size, u64 attribute)
{
	//printk("ept map:gpa = %x hpa = %x page_size = %x\n", gpa, hpa, page_size);
	u64 index1, index2, index3, index4, offset_1g, offset_2m, offset_4k;
	u64 *pml4t, *pdpt, *pdt, *pt;
	u64 pml4e, pdpte, pde, pte;
	u64 pml4e_attr, pdpte_attr, pde_attr, pte_attr;
	u64 *virt;
	pml4e_attr = EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE | EPT_PML4E_ACCESS_FLAG;
	pdpte_attr = EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_ACCESS_FLAG;
	pde_attr = EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_ACCESS_FLAG;
	pte_attr = EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_ACCESS_FLAG;
	pml4t = eptp_base;

	index1 = (gpa >> 39) & 0x1ff;
	index2 = (gpa >> 30) & 0x1ff;
	index3 = (gpa >> 21) & 0x1ff;
	index4 = (gpa >> 12) & 0x1ff;
	offset_1g = gpa & ((1 << 30) - 1);
	offset_2m = gpa & ((1 << 21) - 1);
	offset_4k = gpa & ((1 << 12) - 1);

	pml4e = pml4t[index1];
	if (pml4e == 0) {
		virt = kmalloc(0x1000, GFP_KERNEL);
		memset(virt, 0, 0x1000);
		pml4t[index1] = VIRT2PHYS(virt) | pml4e_attr;
	}

	pdpt = (u64 *)(PHYS2VIRT(pml4t[index1] & ~0xfffULL));
	pdpte = pdpt[index2];

	if (page_size == 0x40000000) {
		pdpt[index2] = hpa | attribute | EPT_PDPTE_1GB_PAGE;
		invept(VMX_EPT_EXTENT_CONTEXT, VIRT2PHYS(eptp_base), 0);
		return 0;
	}

	if (pdpte == 0) {
		virt = kmalloc(0x1000, GFP_KERNEL);
		memset(virt, 0, 0x1000);
		pdpt[index2] = VIRT2PHYS(virt) | pdpte_attr;
	}

	pdt = (u64 *)(PHYS2VIRT(pdpt[index2] & ~0xfffULL));
	pde = pdt[index3];

	if (page_size == 0x200000) {
		pdt[index3] = hpa | attribute | EPT_PDE_2MB_PAGE;
		invept(VMX_EPT_EXTENT_CONTEXT, VIRT2PHYS(eptp_base), 0);
		return 0;
	}

	if (pde == 0) {
		virt = kmalloc(0x1000, GFP_KERNEL);
		memset(virt, 0, 0x1000);
		pdt[index3] = VIRT2PHYS(virt) | pde_attr;
	}

	pt = (u64 *)(PHYS2VIRT(pdt[index3] & ~0xfffULL));
	pte = pdt[index4];

	if (page_size == 0x1000) {
		pt[index4] = hpa | attribute;
		invept(VMX_EPT_EXTENT_CONTEXT, VIRT2PHYS(eptp_base), 0);
		return 0;
	}
	
	return 0;
}

int map_guest_memory(struct vmx_vcpu *vcpu, u64 gpa, u64 hpa, u64 len, u64 attr)
{
	u64 page_size;
	s64 remain_len = len;
	while (remain_len > 0) {
		//if (remain_len >= 0x40000000) {
		//	ept_map_page(vcpu, gpa, hpa, 0x40000000, attr);
		//	remain_len -= 0x40000000;
		//	gpa += 0x40000000;
		//	hpa += 0x40000000;
		//} else if (remain_len >= 0x200000) {
		//	ept_map_page(vcpu, gpa, hpa, 0x200000, attr);
		//	remain_len -= 0x200000;
		//	gpa += 0x200000;
		//	hpa += 0x200000;
		//} else {
			ept_map_page(vcpu->eptp_base, gpa, hpa, 0x1000, attr);
			remain_len -= 0x1000;
			gpa += 0x1000;
			hpa += 0x1000;
		//}
	}
}

int alloc_guest_memory(struct vmx_vcpu *vcpu, u64 gpa, u64 size)
{
	u64 hpa;
	u64 *virt;
	struct guest_memory_zone *zone = kmalloc(sizeof(*zone), GFP_KERNEL);
	if (zone == NULL)
		return -1;
	memset(zone, 0, sizeof(*zone));
	size = roundup(size, 0x1000);
	virt = kmalloc(size, GFP_KERNEL);
	printk("vm virt = %x\n", virt);
	if (virt == NULL)
		return -1;
	memset(virt, 0, size);
	hpa = VIRT2PHYS(virt);
	zone->hpa = hpa;
	zone->page_nr = size / 0x1000;
	zone->gpa = gpa;
	map_guest_memory(vcpu, gpa, hpa, size, EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE | EPT_PTE_CACHE_WB);
	list_add_tail(&zone->list, &vcpu->guest_memory_list);
	return 0;
}

int read_guest_memory_gpa(struct vmx_vcpu *vcpu, u64 gpa, u64 size, void *buffer)
{
	hpa_t hpa;
	int ret = 0;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret)
		return ret;
	memcpy(buffer, (void *)PHYS2VIRT(hpa), size);
}

int write_guest_memory_gpa(struct vmx_vcpu *vcpu, u64 gpa, u64 size, void *buffer)
{
	hpa_t hpa;
	int ret = 0;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret)
		return ret;
	memcpy((void *)PHYS2VIRT(hpa), buffer, size);
}

int vmx_handle_vm_entry_failed(struct vmx_vcpu *vcpu)
{
	u32 instruction_error = vmcs_read(VM_INSTRUCTION_ERROR);
	printk("VM-Entry failed.\n");
	printk("vm instruction error:%d\n", instruction_error);
	return 0;
}

int vmx_handle_exception(struct vmx_vcpu *vcpu)
{
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	u64 interruption_info = vmcs_read(VM_EXIT_INTR_INFO);
	u64 err_code = vmcs_read(VM_EXIT_INTR_ERROR_CODE);
	u64 cr0 = vmcs_read(GUEST_CR0);
	printk("VM-Exit:Guest exception (%d) @ 0x%x.type:%d error code:%x CR2 = 0x%x\n", 
		interruption_info & 0xff,
		vcpu->guest_state.rip,
		(interruption_info >> 8) & 0x7,
		err_code,
		vcpu->guest_state.ctrl_regs.cr2);
	//printk("rax = 0x%x\n", vcpu->guest_state.gr_regs.rax);
		/* real mode never use error code */
	if (cr0 & CR0_PE == 0) {
		interruption_info &= (~BIT11);
	}

	while (1);
	//vmcs_write(VM_ENTRY_INTR_INFO_FIELD, interruption_info);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_external_interrupt(struct vmx_vcpu *vcpu)
{
	u64 interruption_info = vmcs_read(VM_EXIT_INTR_INFO);
	u8 vector = interruption_info & 0xff;
	printk("VM-Exit:External interrupt (%d)\n", vector);
	soft_irq_call(vector);
	asm("sti");
	return 0;
}

int vmx_handle_triple_fault(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:Triple fault.RIP = %x\n", vcpu->guest_state.rip);
	while (1);
	return 0;
}

int vmx_handle_pending_interrupt(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:pending interrupt.\n");
	return 0;
}

int vmx_handle_cpuid(struct vmx_vcpu *vcpu)
{
	u32 buffer[4];
	//printk("VM-Exit:cpuid.rip = 0x%x\n", vcpu->guest_state.rip);
	/* Passthrough host cpuid. */
	cpuid(vcpu->guest_state.gr_regs.rax, vcpu->guest_state.gr_regs.rcx, buffer);
	vcpu->guest_state.gr_regs.rax = buffer[0];
	vcpu->guest_state.gr_regs.rbx = buffer[1];
	vcpu->guest_state.gr_regs.rcx = buffer[2];
	vcpu->guest_state.gr_regs.rdx = buffer[3];
	//printk("eax = 0x%x, ebx = 0x%x ecx = 0x%x edx = 0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_rdpmc(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:RDPMC.\n");
	return 0;
}

int vmx_handle_rdtsc(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:RDTSC.\n");
	return 0;
}

int vmx_handle_rdtscp(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:RDTSCP.\n");
	return 0;
}

void dump_guest_state(struct vmx_vcpu *vcpu)
{
	printk("RAX:%016x RBX:%016x RCX:%016x RDX:%016x\n", 
		vcpu->guest_state.gr_regs.rax,
		vcpu->guest_state.gr_regs.rbx,
		vcpu->guest_state.gr_regs.rcx,
		vcpu->guest_state.gr_regs.rdx
		);
	printk("RSI:%016x RDI:%016x RSP:%016x RBP:%016x\n", 
		vcpu->guest_state.gr_regs.rsi,
		vcpu->guest_state.gr_regs.rdi,
		vcpu->guest_state.gr_regs.rsp,
		vcpu->guest_state.gr_regs.rbp
		);
	printk("R8:%016x R9:%016x R10:%016x R11:%016x\n", 
		vcpu->guest_state.gr_regs.r8,
		vcpu->guest_state.gr_regs.r9,
		vcpu->guest_state.gr_regs.r10,
		vcpu->guest_state.gr_regs.r11
		);
	printk("R12:%016x R13:%016x R14:%016x R15:%016x\n", 
		vcpu->guest_state.gr_regs.r12,
		vcpu->guest_state.gr_regs.r13,
		vcpu->guest_state.gr_regs.r14,
		vcpu->guest_state.gr_regs.r15
		);
	printk("CR0:%016x CR2:%016x CR3:%016x CR4:%016x CR8:%016x\n", 
		vcpu->guest_state.ctrl_regs.cr0,
		vcpu->guest_state.ctrl_regs.cr2,
		vcpu->guest_state.ctrl_regs.cr3,
		vcpu->guest_state.ctrl_regs.cr4,
		vcpu->guest_state.ctrl_regs.cr8
		);
	printk("ZMM0:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[0], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[0], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[0], 16);
	printk("\nZMM1:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[1], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[1], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[1], 16);
	printk("\nZMM2:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[2], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[2], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[2], 16);
	printk("\nZMM3:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[3], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[3], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[3], 16);
	printk("\nZMM4:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[4], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[4], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[4], 16);
	printk("\nZMM5:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[5], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[5], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[5], 16);
	printk("\nZMM6:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[6], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[6], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[6], 16);
	printk("\nZMM7:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[7], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[7], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[7], 16);
	printk("\nZMM8:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[8], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[8], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[8], 16);
	printk("\nZMM9:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[9], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[9], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[9], 16);
	printk("\nZMM10:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[10], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[10], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[10], 16);
	printk("\nZMM11:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[11], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[11], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[11], 16);
	printk("\nZMM12:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[12], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[12], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[12], 16);
	printk("\nZMM13:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[13], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[13], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[13], 16);
	printk("\nZMM14:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[14], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[14], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[14], 16);
	printk("\nZMM15:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.zmm_hi256[15], 32);
	long_int_print(&vcpu->guest_state.fp_regs->avx_state.ymm[15], 16);
	long_int_print(&vcpu->guest_state.fp_regs->legacy_region.xmm_reg[15], 16);
	printk("\nZMM16:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[0], 64);
	printk("\nZMM17:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[1], 64);
	printk("\nZMM18:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[2], 64);
	printk("\nZMM19:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[3], 64);
	printk("\nZMM20:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[4], 64);
	printk("\nZMM21:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[5], 64);
	printk("\nZMM22:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[6], 64);
	printk("\nZMM23:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[7], 64);
	printk("\nZMM24:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[8], 64);
	printk("\nZMM25:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[9], 64);
	printk("\nZMM26:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[10], 64);
	printk("\nZMM27:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[11], 64);
	printk("\nZMM28:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[12], 64);
	printk("\nZMM29:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[13], 64);
	printk("\nZMM30:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[14], 64);
	printk("\nZMM31:");
	long_int_print(&vcpu->guest_state.fp_regs->avx512_state.hi16_zmm[15], 64);
	printk("\n");
}

int vmx_handle_vmcall(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMCALL.\n");
	//dump_guest_state(vcpu);
	//printk("LDTR limit:%x\n", vmcs_read(GUEST_LDTR_LIMIT));
	//printk("LDTR AR BYTES:%x\n", vmcs_read(GUEST_LDTR_AR_BYTES));
	//vmcs_write(GUEST_LDTR_AR_BYTES, 0x10000);
	//u64 pin_based_vm_exec_ctrl = vmcs_read(PIN_BASED_VM_EXEC_CONTROL);
	//pin_based_vm_exec_ctrl |= PIN_BASED_VMX_PREEMPTION_TIMER;
	//vmcs_write(PIN_BASED_VM_EXEC_CONTROL, pin_based_vm_exec_ctrl);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}
int vmx_handle_vmxon(struct vmx_vcpu *vcpu);
int vmx_handle_vmxoff(struct vmx_vcpu *vcpu);
int vmx_handle_vmclear(struct vmx_vcpu *vcpu);
int vmx_handle_vmlaunch(struct vmx_vcpu *vcpu);
int vmx_handle_vmresume(struct vmx_vcpu *vcpu);
int vmx_handle_vmptrld(struct vmx_vcpu *vcpu);
int vmx_handle_vmptrst(struct vmx_vcpu *vcpu);
int vmx_handle_vmread(struct vmx_vcpu *vcpu);
int vmx_handle_vmwrite(struct vmx_vcpu *vcpu);
int vmx_handle_invept(struct vmx_vcpu *vcpu);
int handle_vmx_instructions(struct vmx_vcpu *vcpu);

static int vmx_enter_longmode(struct vmx_vcpu *vcpu)
{
	u64 efer = vmcs_read(GUEST_IA32_EFER);
	u64 cr0 = vmcs_read(GUEST_CR0);
	u64 cr4 = vmcs_read(GUEST_CR4);
	vmcs_write(GUEST_CR0, cr0 | CR0_PE | CR0_PG);
	vmcs_write(GUEST_CR4, cr4 | CR4_PAE);
	vmcs_write(GUEST_TR_AR_BYTES, VMX_AR_P_MASK | VMX_AR_TYPE_BUSY_64_TSS);
	vmcs_write(VM_ENTRY_CONTROLS,
		vmcs_read(VM_ENTRY_CONTROLS) | VM_ENTRY_IA32E_MODE
		);
	vmcs_write(GUEST_IA32_EFER, efer | 0x500);
	printk("vmx:enter long mode.\n");
	return 0;
}

int vmx_handle_cr_access(struct vmx_vcpu *vcpu)
{
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	u64 access_type = (exit_qualification >> 4) & 0x3;
	u64 cr = exit_qualification & 0xf;
	u64 reg = (exit_qualification >> 8) & 0xf;
	u64 val = 0;
	printk("VM-Exit:CR%d access.\n", cr);
	switch(reg) {
		case 0:
			val = vcpu->guest_state.gr_regs.rax;
			break;
		case 1:
			val = vcpu->guest_state.gr_regs.rcx;
			break;
		case 2:
			val = vcpu->guest_state.gr_regs.rdx;
			break;
		case 3:
			val = vcpu->guest_state.gr_regs.rbx;
			break;
		case 4:
			val = vcpu->guest_state.gr_regs.rsp;
			break;
		case 5:
			val = vcpu->guest_state.gr_regs.rbp;
			break;
		case 6:
			val = vcpu->guest_state.gr_regs.rsi;
			break;
		case 7:
			val = vcpu->guest_state.gr_regs.rdi;
			break;
		case 8:
			val = vcpu->guest_state.gr_regs.r8;
			break;
		case 9:
			val = vcpu->guest_state.gr_regs.r9;
			break;
		case 10:
			val = vcpu->guest_state.gr_regs.r10;
			break;
		case 11:
			val = vcpu->guest_state.gr_regs.r11;
			break;
		case 12:
			val = vcpu->guest_state.gr_regs.r12;
			break;
		case 13:
			val = vcpu->guest_state.gr_regs.r13;
			break;
		case 14:
			val = vcpu->guest_state.gr_regs.r14;
			break;
		case 15:
			val = vcpu->guest_state.gr_regs.r15;
			break;		
	}
	printk("VM-Exit:CR%d %s, val = 0x%x\n", cr, access_type ? "read" : "write", val);
	if (cr == 0) {
		vcpu->guest_state.cr0_read_shadow = val;
		vmcs_write(CR0_READ_SHADOW, vcpu->guest_state.cr0_read_shadow);
		if ((val & CR0_PG) && (vmcs_read(GUEST_IA32_EFER) & BIT8)) {
			vmx_enter_longmode(vcpu);
		}
	}

	if (cr == 3) {
		vmcs_write(GUEST_CR3, val);
	}

	if (cr == 4) {
		vcpu->guest_state.cr4_read_shadow = val;
		vmcs_write(CR4_READ_SHADOW, vcpu->guest_state.cr0_read_shadow);
	}
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_rdmsr(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:RDMSR.\n");
	return 0;
}

int vmx_handle_wrmsr(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:WRMSR.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_mtf(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:Monitor trap.RIP = %x\n", vcpu->guest_state.rip);
	return 0;
}

int vmx_handle_apic_access(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:APIC access.\n");
	return 0;
}

int vmx_handle_eoi(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:EOI.\n");
	return 0;
}

int vmx_handle_ept_volation(struct vmx_vcpu *vcpu)
{
	//printk("VM-Exit:EPT Volation.\n");
	u64 gpa = vmcs_read(GUEST_PHYSICAL_ADDRESS);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	void *page;
	//printk("gpa = 0x%x\n", gpa);
	//printk("exit qualification:0x%x\n", exit_qualification);
	if (vcpu->guest_mode)
		return nested_vmx_handle_ept_volation(vcpu);
	else {
		/*TODO: check gpa */
		page = kmalloc(0x1000, GFP_KERNEL);
		return ept_map_page(vcpu->eptp_base,
								gpa,
								VIRT2PHYS(page),
								0x1000,
								EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE | EPT_PTE_CACHE_WB);
	}

	return 0;
}

int vmx_handle_io(struct vmx_vcpu *vcpu)
{
	//printk("VM-Exit:I/O.\n");
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	u64 port = (exit_qualification >> 16) & 0xffff;
	bool direction = exit_qualification & BIT3;
	bool is_string = exit_qualification & BIT4;
	bool is_rep = exit_qualification & BIT5;
	bool is_imme = exit_qualification & BIT6;

	if (!is_rep) {
		vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	} else {
		vcpu->guest_state.gr_regs.rcx--;
		/* String IO handle...*/
		if (vcpu->guest_state.gr_regs.rcx == 0)
			vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	}
	//printk("Port = 0x%x, val = %x\n", port, vcpu->guest_state.gr_regs.rax);
	if (port == 0x3f8) {
		printk("%c", vcpu->guest_state.gr_regs.rax);
	}

	if (port == 0xcfc) {
		vcpu->guest_state.gr_regs.rax = 0xffffffff;
	}

	if (port != 0x3f8 && port != 0xcfc && port != 0xcf8) {
		printk("Port = 0x%x, val = %x\n", port, vcpu->guest_state.gr_regs.rax);
	}
	return 0;
}

int vmx_handle_xsetbv(struct vmx_vcpu *vcpu)
{
	u32 cpuid_regs[4] = {0};
	u64 xcr0_mask = 0;

	u64 g_cr0 = vmcs_read(GUEST_CR0);
	u64 cr0_guest_host_mask = vmcs_read(CR0_GUEST_HOST_MASK);
	u64 cr0_read_shadow = vmcs_read(CR0_READ_SHADOW);
	u64 cr0 = g_cr0 & (~cr0_guest_host_mask) | (cr0_read_shadow & cr0_read_shadow);

	u64 g_cr4 = vmcs_read(GUEST_CR4);
	u64 cr4_guest_host_mask = vmcs_read(CR4_GUEST_HOST_MASK);
	u64 cr4_read_shadow = vmcs_read(CR4_READ_SHADOW);
	u64 cr4 = g_cr4 & (~cr4_guest_host_mask) | (cr4_read_shadow & cr4_read_shadow);

	u64 index = vcpu->guest_state.gr_regs.rcx;
	u64 xcr0 = vcpu->guest_state.gr_regs.rax | (vcpu->guest_state.gr_regs.rdx << 32);

	printk("VM-Exit:xsetbv.\n");
	printk("index(ecx) = 0x%x, eax = 0x%x, edx = 0x%x\n", 
		vcpu->guest_state.gr_regs.rcx,
		vcpu->guest_state.gr_regs.rax,
		vcpu->guest_state.gr_regs.rdx
		);

	cpuid(0x0000000d, 0x00000000, &cpuid_regs[0]);
	xcr0_mask = cpuid_regs[0];
	printk("xsetbv:cr0:0x%x, cr4:0x%x\n", cr0, cr4);
	if (index != 0 || ((xcr0 | xcr0_mask) != xcr0_mask) || 
		!(xcr0 & 0x1) || ((xcr0 & 0x6) == 0x4) ||
		/* TODO: Guest CPL check */
		!(cr0 & CR0_MP) || !(cr0 & CR0_NE) || (cr0 & CR0_EM) || !(cr4 & CR4_OSXSAVE)) {
		printk("invalid guest XCR0.\n");
		/* TODO: inject #GP to guest.*/
	} else {
		xsetbv(index, xcr0);
		vcpu->guest_state.ctrl_regs.xcr0 = xcr0;
	}

	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_xsaves(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:xsave.\n");
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_xrstors(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:xrstore.\n");
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_preemption_timer(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:Guest timeout.\n");
	vmcs_write(VMX_PREEMPTION_TIMER_VALUE, 2100000000 / 128);
	return 0;
}



int vm_exit_handler(struct vmx_vcpu *vcpu)
{
	int ret = 0;
	u32 exit_reason = vmcs_read(VM_EXIT_REASON);
	u32 instruction_error = vmcs_read(VM_INSTRUCTION_ERROR);
	vcpu->guest_state.rip = vmcs_read(GUEST_RIP);
	vcpu->guest_state.gr_regs.rsp = vmcs_read(GUEST_RSP);

	if (vcpu->guest_mode && nested_vm_exit_reflected(vcpu, exit_reason))
		return nested_vmx_reflect_vmexit(vcpu, exit_reason);
	else {
		if (exit_reason & 0x80000000) {
			printk("vm entry failed.\n");
			vmx_handle_vm_entry_failed(vcpu);
		} else {
			switch(exit_reason & 0xff) {
			case EXIT_REASON_EXCEPTION_NMI:
				ret = vmx_handle_exception(vcpu);
				break;
			case EXIT_REASON_EXTERNAL_INTERRUPT:
				ret = vmx_handle_external_interrupt(vcpu);
				break;
			case EXIT_REASON_TRIPLE_FAULT:
				ret = vmx_handle_triple_fault(vcpu);
				break;
			case EXIT_REASON_PENDING_INTERRUPT:
				ret = vmx_handle_pending_interrupt(vcpu);
				break;
			case EXIT_REASON_NMI_WINDOW:
				break;
			case EXIT_REASON_TASK_SWITCH:
				break;
			case EXIT_REASON_CPUID:
				ret = vmx_handle_cpuid(vcpu);
				break;
			case EXIT_REASON_HLT:
				break;
			case EXIT_REASON_INVD:
				break;
			case EXIT_REASON_INVLPG:
				break;
			case EXIT_REASON_RDPMC:
				ret = vmx_handle_rdpmc(vcpu);
				break;
			case EXIT_REASON_RDTSC:
				ret = vmx_handle_rdtsc(vcpu);
				break;
			case EXIT_REASON_VMCALL:
				ret = vmx_handle_vmcall(vcpu);
				break;
			case EXIT_REASON_VMCLEAR:
				ret = vmx_handle_vmclear(vcpu);
				break;
			case EXIT_REASON_VMLAUNCH:
				ret = vmx_handle_vmlaunch(vcpu);
				break;
			case EXIT_REASON_VMPTRLD:
				ret = vmx_handle_vmptrld(vcpu);
				break;
			case EXIT_REASON_VMPTRST:
				ret = vmx_handle_vmptrst(vcpu);
				break;
			case EXIT_REASON_VMREAD:
				ret = vmx_handle_vmread(vcpu);
				break;
			case EXIT_REASON_VMRESUME:
				ret = vmx_handle_vmresume(vcpu);
				break;
			case EXIT_REASON_VMWRITE:
				ret = vmx_handle_vmwrite(vcpu);
				break;
			case EXIT_REASON_VMOFF:
				ret = vmx_handle_vmxoff(vcpu);
				break;
			case EXIT_REASON_VMON:
				ret = vmx_handle_vmxon(vcpu);
				break;
			case EXIT_REASON_CR_ACCESS:
				ret = vmx_handle_cr_access(vcpu);
				break;
			case EXIT_REASON_DR_ACCESS:
				break;
			case EXIT_REASON_IO_INSTRUCTION:
				ret = vmx_handle_io(vcpu);
				break;
			case EXIT_REASON_MSR_READ:
				ret = vmx_handle_rdmsr(vcpu);
				break;
			case EXIT_REASON_MSR_WRITE:
				ret = vmx_handle_wrmsr(vcpu);
				break;
			case EXIT_REASON_INVALID_STATE:
				printk("VM Exit:Invalid Guest State.\n");
				break;
			case EXIT_REASON_MSR_LOAD_FAIL:
				break;
			case EXIT_REASON_MWAIT_INSTRUCTION:
				break;
			case EXIT_REASON_MONITOR_TRAP_FLAG:
				ret = vmx_handle_mtf(vcpu);
				break;
			case EXIT_REASON_MONITOR_INSTRUCTION:
				break;
			case EXIT_REASON_PAUSE_INSTRUCTION:
				break;
			case EXIT_REASON_MCE_DURING_VMENTRY:
				break;
			case EXIT_REASON_TPR_BELOW_THRESHOLD:
				break;
			case EXIT_REASON_APIC_ACCESS:
				ret = vmx_handle_apic_access(vcpu);
				break;
			case EXIT_REASON_EOI_INDUCED:
				ret = vmx_handle_eoi(vcpu);
				break;
			case EXIT_REASON_GDTR_IDTR:
				break;
			case EXIT_REASON_LDTR_TR:
				break;
			case EXIT_REASON_EPT_VIOLATION:
				ret = vmx_handle_ept_volation(vcpu);
				break;
			case EXIT_REASON_EPT_MISCONFIG:
				break;
			case EXIT_REASON_INVEPT:
				ret = vmx_handle_invept(vcpu);
				break;
			case EXIT_REASON_RDTSCP:
				ret = vmx_handle_rdtscp(vcpu);
				break;
			case EXIT_REASON_PREEMPTION_TIMER:
				ret = vmx_handle_preemption_timer(vcpu);
				break;
			case EXIT_REASON_INVVPID:
				break;
			case EXIT_REASON_WBINVD:
				break;
			case EXIT_REASON_XSETBV:
				ret = vmx_handle_xsetbv(vcpu);
				break;
			case EXIT_REASON_APIC_WRITE:
				break;
			case EXIT_REASON_RDRAND:
				break;
			case EXIT_REASON_INVPCID:
				break;
			case EXIT_REASON_VMFUNC:
				break;
			case EXIT_REASON_ENCLS:
				break;
			case EXIT_REASON_RDSEED:
				break;
			case EXIT_REASON_PML_FULL:
				break;
			case EXIT_REASON_XSAVES:
				ret = vmx_handle_xsaves(vcpu);
				break;
			case EXIT_REASON_XRSTORS:
				ret = vmx_handle_xrstors(vcpu);
				break;
			default:
				printk("VM-Exit:Unhandled exit reason:%d\n", exit_reason & 0xff);
				break;
			}
		}
	}
	//printk("Next RIP:%x\n", vcpu->guest_state.rip);
	vmcs_write(GUEST_RIP, vcpu->guest_state.rip);
	return ret;
}

void vmx_set_bist_state(struct vmx_vcpu *vcpu)
{
	vcpu->guest_state.cs.selector = 0;
	vcpu->guest_state.cs.base = 0;
	vcpu->guest_state.cs.limit = 0xffff;
	vcpu->guest_state.cs.ar_bytes = VMX_AR_P_MASK 
		| VMX_AR_TYPE_READABLE_CODE_MASK 
		| VMX_AR_TYPE_CODE_MASK 
		| VMX_AR_TYPE_ACCESSES_MASK 
		| VMX_AR_S_MASK;

	vcpu->guest_state.ds.selector = 0;
	vcpu->guest_state.ds.base = 0;
	vcpu->guest_state.ds.limit = 0xffff;
	vcpu->guest_state.ds.ar_bytes = VMX_AR_P_MASK
		| VMX_AR_TYPE_WRITABLE_DATA_MASK
		| VMX_AR_TYPE_ACCESSES_MASK
		| VMX_AR_S_MASK;

	vcpu->guest_state.es.selector = 0;
	vcpu->guest_state.es.base = 0;
	vcpu->guest_state.es.limit = 0xffff;
	vcpu->guest_state.es.ar_bytes = VMX_AR_P_MASK
		| VMX_AR_TYPE_WRITABLE_DATA_MASK
		| VMX_AR_TYPE_ACCESSES_MASK
		| VMX_AR_S_MASK;

	vcpu->guest_state.fs.selector = 0;
	vcpu->guest_state.fs.base = 0;
	vcpu->guest_state.fs.limit = 0xffff;
	vcpu->guest_state.fs.ar_bytes = VMX_AR_P_MASK
		| VMX_AR_TYPE_WRITABLE_DATA_MASK
		| VMX_AR_TYPE_ACCESSES_MASK
		| VMX_AR_S_MASK;

	vcpu->guest_state.gs.selector = 0;
	vcpu->guest_state.gs.base = 0;
	vcpu->guest_state.gs.limit = 0xffff;
	vcpu->guest_state.gs.ar_bytes = VMX_AR_P_MASK
		| VMX_AR_TYPE_WRITABLE_DATA_MASK
		| VMX_AR_TYPE_ACCESSES_MASK
		| VMX_AR_S_MASK;

	vcpu->guest_state.ss.selector = 0;
	vcpu->guest_state.ss.base = 0;
	vcpu->guest_state.ss.limit = 0xffff;
	vcpu->guest_state.ss.ar_bytes = VMX_AR_P_MASK
		| VMX_AR_TYPE_WRITABLE_DATA_MASK
		| VMX_AR_TYPE_ACCESSES_MASK
		| VMX_AR_S_MASK;

	vcpu->guest_state.tr.selector = 0;
	vcpu->guest_state.tr.base = 0;
	vcpu->guest_state.tr.limit = 0xffff;
	vcpu->guest_state.tr.ar_bytes = VMX_AR_P_MASK | VMX_AR_TYPE_BUSY_16_TSS;

	vcpu->guest_state.ldtr.selector = 0;
	vcpu->guest_state.ldtr.base = 0;
	vcpu->guest_state.ldtr.limit = 0xffff;
	vcpu->guest_state.ldtr.ar_bytes = VMX_AR_UNUSABLE_MASK;

	vcpu->guest_state.gdtr.base = 0;
	vcpu->guest_state.gdtr.limit = 0xffff;
	vcpu->guest_state.idtr.base = 0;
	vcpu->guest_state.idtr.limit = 0xffff;
	
	vcpu->guest_state.ctrl_regs.cr0 = 0;
	vcpu->guest_state.ctrl_regs.cr2 = 0;
	vcpu->guest_state.ctrl_regs.cr3 = 0;
	vcpu->guest_state.ctrl_regs.cr4 = 0;

	vcpu->guest_state.cr0_read_shadow = vcpu->guest_state.ctrl_regs.cr0;
	vcpu->guest_state.cr4_read_shadow = vcpu->guest_state.ctrl_regs.cr4;

	vcpu->guest_state.pdpte0 = 0;
	vcpu->guest_state.pdpte1 = 0;
	vcpu->guest_state.pdpte2 = 0;
	vcpu->guest_state.pdpte3 = 0;

	vcpu->guest_state.rip = 0x7c00;
	vcpu->guest_state.rflags = BIT1;
	memset(&vcpu->guest_state.gr_regs, 0, sizeof(struct general_regs));

	vcpu->guest_state.ctrl_regs.xcr0 = XCR0_X87;
}

u64 kvm_reg_read(struct vmx_vcpu *vcpu, int index)
{
	switch (index) {
		case KVM_REG_RAX:
			return vcpu->guest_state.gr_regs.rax;
			break;
		case KVM_REG_RCX:
			return vcpu->guest_state.gr_regs.rcx;
			break;
		case KVM_REG_RDX:
			return vcpu->guest_state.gr_regs.rdx;
			break;
		case KVM_REG_RBX:
			return vcpu->guest_state.gr_regs.rbx;
			break;
		case KVM_REG_RSP:
			return vcpu->guest_state.gr_regs.rsp;
			break;
		case KVM_REG_RBP:
			return vcpu->guest_state.gr_regs.rbp;
			break;
		case KVM_REG_RSI:
			return vcpu->guest_state.gr_regs.rsi;
			break;
		case KVM_REG_RDI:
			return vcpu->guest_state.gr_regs.rdi;
			break;
		case KVM_REG_R8:
			return vcpu->guest_state.gr_regs.r8;
			break;
		case KVM_REG_R9:
			return vcpu->guest_state.gr_regs.r9;
			break;
		case KVM_REG_R10:
			return vcpu->guest_state.gr_regs.r10;
			break;
		case KVM_REG_R11:
			return vcpu->guest_state.gr_regs.r11;
			break;
		case KVM_REG_R12:
			return vcpu->guest_state.gr_regs.r12;
			break;
		case KVM_REG_R13:
			return vcpu->guest_state.gr_regs.r13;
			break;
		case KVM_REG_R14:
			return vcpu->guest_state.gr_regs.r14;
			break;
		case KVM_REG_R15:
			return vcpu->guest_state.gr_regs.r15;
			break;
	}
}

void kvm_reg_write(struct vmx_vcpu *vcpu, int index, u64 value)
{
	switch (index) {
		case KVM_REG_RAX:
			vcpu->guest_state.gr_regs.rax = value;
			break;
		case KVM_REG_RCX:
			vcpu->guest_state.gr_regs.rcx = value;
			break;
		case KVM_REG_RDX:
			vcpu->guest_state.gr_regs.rdx = value;
			break;
		case KVM_REG_RBX:
			vcpu->guest_state.gr_regs.rbx = value;
			break;
		case KVM_REG_RSP:
			vcpu->guest_state.gr_regs.rsp = value;
			break;
		case KVM_REG_RBP:
			vcpu->guest_state.gr_regs.rbp = value;
			break;
		case KVM_REG_RSI:
			vcpu->guest_state.gr_regs.rsi = value;
			break;
		case KVM_REG_RDI:
			vcpu->guest_state.gr_regs.rdi = value;
			break;
		case KVM_REG_R8:
			vcpu->guest_state.gr_regs.r8 = value;
			break;
		case KVM_REG_R9:
			vcpu->guest_state.gr_regs.r9 = value;
			break;
		case KVM_REG_R10:
			vcpu->guest_state.gr_regs.r10 = value;
			break;
		case KVM_REG_R11:
			vcpu->guest_state.gr_regs.r11 = value;
			break;
		case KVM_REG_R12:
			vcpu->guest_state.gr_regs.r12 = value;
			break;
		case KVM_REG_R13:
			vcpu->guest_state.gr_regs.r13 = value;
			break;
		case KVM_REG_R14:
			vcpu->guest_state.gr_regs.r14 = value;
			break;
		case KVM_REG_R15:
			vcpu->guest_state.gr_regs.r15 = value;
			break;
	}
}


int vmx_run(struct vmx_vcpu *vcpu)
{	
	int ret0 = 0, ret1 = 0;

	while (1) {
		if (vcpu->state == 0) {
			//printk("VM LAUNCH.\n");
			ret0 = vm_launch(&vcpu->host_state.gr_regs, &vcpu->guest_state.gr_regs);
			if (ret0 == 0) {
				vcpu->state = 1;
			}
		} else {
			//printk("VM RESUME.\n");
			ret0 = vm_resume(&vcpu->host_state.gr_regs, &vcpu->guest_state.gr_regs);
		}
		
		if (ret0 == 0) {
			vmx_save_guest_state(vcpu);
			ret1 = vm_exit_handler(vcpu);
			if (ret1 != 0)
				return -1;
		} else {
			printk("vm entry failed.\n");
			return -1;
		}
	}
}


void vm_init_test()
{
	int ret;
	u8 buf[0x1000];

	void *code_start, *code_end;
	extern u64 test_guest, test_guest_end, test_guest_reset_vector;

	struct vmx_vcpu *vcpu = vmx_preinit();
	if (vcpu == NULL) {
		printk("VM preinit failed.Check BIOS settings for enabling VT-x.\n");
		return;
	}
	vmx_init(vcpu);
	vmx_set_ctrl_state(vcpu);
	vmx_save_host_state(vcpu);
	vmx_set_bist_state(vcpu);
	ret = alloc_guest_memory(vcpu, 0, 0x40000000);
	if (ret == -1) {
		printk("allocate memory for vm failed.\n");
		ret = alloc_guest_memory(vcpu, 0, 0x8000000);
		if (ret == -1) {
			printk("allocate memory for vm failed.\n");
			ret = alloc_guest_memory(vcpu, 0, 0x4000000);
			if (ret == -1)
				printk("allocate memory for vm failed.exiting...\n");
		}
	}

	ret = alloc_guest_memory(vcpu, 0xfee00000, 0x1000);
	if (ret == -1) {
		printk("allocate memory for vm failed.\n");
	}

	code_start = (void *)PHYS2VIRT(0x200000);
	code_end = (void *)PHYS2VIRT(0x400000);
	ret = write_guest_memory_gpa(vcpu, 0x200000, 0x100000, code_start);
	if (ret == -1) {
		printk("writing guest memory failed.\n");
	}

	//alloc_guest_memory(vcpu, 0xff000000, 0x1000000);
	ret = write_guest_memory_gpa(vcpu, 0x7c00, (u64)&test_guest_end - (u64)&test_guest, &test_guest);
	if (ret == -1) {
		printk("writing guest memory failed.\n");
	}

	memset(buf, 0, 0x1000);
	ret = write_guest_memory_gpa(vcpu, 0x10000, 0x4000, buf);
	if (ret == -1) {
		printk("writing guest memory failed.\n");
	}
	//printk("boot_parm->acpi_rsdp = %x\n", boot_parm->acpi_rsdp);

	vmx_set_guest_state(vcpu);
	ret = vmx_run(vcpu);
	printk("vm-exit.ret = %d\n", ret);
}
