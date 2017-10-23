#include "Vmx.h"
#include "BaseLib.h"

RETURN_STATUS VmxSupport()
{
	X64_CPUID_BUFFER CpuIdReg = { 0 };
	UINT64 MsrValue = 0;
	
	X64GetCpuId(0x00000001, 0, &CpuIdReg);
	if ((CpuIdReg.ECX & CPUID_FEATURES_ECX_VMX) == 0)
		return RETURN_UNSUPPORTED;
	
	MsrValue = X64ReadMsr(MSR_IA32_FEATURE_CONTROL);
	
	if ((MsrValue & FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX) == 0 || ((MsrValue & FEATURE_CONTROL_LOCKED) == 0))
		return RETURN_ABORTED;

	return RETURN_SUCCESS;

}

RETURN_STATUS VmxStatus()
{
	UINT64 Cr0Reg = 0;
	UINT64 Cr4Reg = 0;
	Cr0Reg = X64ReadCr(0);
	if ((Cr0Reg & (CR0_PG | CR0_NE | CR0_PE)) != (CR0_PG | CR0_NE | CR0_PE))
		return RETURN_NOT_READY;

	Cr4Reg = X64ReadCr(4);
	if ((Cr4Reg & CR4_VMXE) == 0)
		return RETURN_NOT_READY;

	return RETURN_ALREADY_STARTED;
}

RETURN_STATUS VmxEnable()
{
	RETURN_STATUS Status = 0;
	UINT64 Cr0Reg = 0;
	UINT64 Cr4Reg = 0;
	Status = VmxSupport();

	if (Status != RETURN_SUCCESS)
		return Status;

	Status = VmxStatus();
	if (Status == RETURN_ALREADY_STARTED)
		return RETURN_SUCCESS;
	
	Cr0Reg = X64ReadCr(0);
	Cr0Reg |= CR0_NE;
	X64WriteCr(0, Cr0Reg);

	Cr4Reg = X64ReadCr(4);
	Cr4Reg |= CR4_VMXE;
	X64WriteCr(4, Cr4Reg);

	return RETURN_SUCCESS;
}

RETURN_STATUS VmxOnPrepare(VOID * VmxOnRegionPtr)
{
	UINT32 * Ptr = VmxOnRegionPtr;
	Ptr[0] = X64ReadMsr(MSR_IA32_VMX_BASIC);
	return RETURN_SUCCESS;
}

RETURN_STATUS VmOn(UINT64 VmxOnRegionPhys)
{
	UINT64 Flags = 0;
	VmxOn(VmxOnRegionPhys);
	Flags = X64GetFlagsReg();
	if ((Flags & (FLAG_CF | FLAG_ZF)) != 0)
		return RETURN_ABORTED;

	return RETURN_SUCCESS;
}

RETURN_STATUS VmOff(VOID)
{
	UINT64 Flags = 0;
	VmxOff();
	Flags = X64GetFlagsReg();
	if ((Flags & (FLAG_CF | FLAG_ZF)) != 0)
		return RETURN_ABORTED;

	return RETURN_SUCCESS;
}

VOID GuestTestCode();
VOID HostExec();


#include "LongMode.h"
extern GDTR GDT;
extern IDTR IDT;
extern TSS64 TSS[64];
#define ACCESS_RIGHT_UNUSABLE (1 << 16)
#define ACCESS_RIGHT_TR ((1 << 13) | (1 << 12) | (1 << 7) | 0xb)
#define CS_ACCESS_RIGHT ((1 << 13) | (1 << 12) | (1 << 4) | (1 << 7) | 0xf)
#define SS_ACCESS_RIGHT (1 << 16)

VOID EptIdenticalMapping4G(VIRTUAL_MACHINE *Vm)
{
	UINT64 *PML4T = Vm->VmcsVirt.ept_pointer;
	UINT64 *PDPT = (UINT64 *)((UINT8 *)Vm->VmcsVirt.ept_pointer + 0x1000);
	PML4T[0] = (VIRT2PHYS(Vm->VmcsVirt.ept_pointer) + 0x1000) | \
			(EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE | EPT_PML4E_ACCESS_FLAG);
	PDPT[0] = 0x00000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_CACHE_WB | EPT_PDPTE_1GB_PAGE);
	PDPT[1] = 0x40000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_CACHE_WB | EPT_PDPTE_1GB_PAGE);
	PDPT[2] = 0x80000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_CACHE_WB | EPT_PDPTE_1GB_PAGE);
	PDPT[3] = 0xc0000000 |  \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_UNCACHEABLE | EPT_PDPTE_1GB_PAGE);
}

VOID EptRealModeMapping(VIRTUAL_MACHINE *Vm)
{
	VOID * GuestMemory = KMalloc(0x100000);
	UINT64 *PML4T = Vm->VmcsVirt.ept_pointer;
	UINT64 *PDPT = (UINT64 *)((UINT8 *)Vm->VmcsVirt.ept_pointer + 0x1000);
	PML4T[0] = (VIRT2PHYS(Vm->VmcsVirt.ept_pointer) + 0x1000) | \
			(EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE | EPT_PML4E_ACCESS_FLAG);

	PDPT[0] = (VIRT2PHYS(Vm->VmcsVirt.ept_pointer) + 0x2000) | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_ACCESS_FLAG);
	
	PDPT[3] = (VIRT2PHYS(Vm->VmcsVirt.ept_pointer) + 0x3000) |  \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_ACCESS_FLAG);

	UINT64 *PDT0_1G = (UINT64 *)((UINT8 *)Vm->VmcsVirt.ept_pointer + 0x2000);
	UINT64 *PDT3_4G = (UINT64 *)((UINT8 *)Vm->VmcsVirt.ept_pointer + 0x3000);

	PDT0_1G[0] = VIRT2PHYS(GuestMemory) | \
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_CACHE_WB | EPT_PDE_2MB_PAGE);

	PDT3_4G[511] = VIRT2PHYS(GuestMemory) | \
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_CACHE_WB | EPT_PDE_2MB_PAGE);
}

VIRTUAL_MACHINE *VmxNew()
{
	VIRTUAL_MACHINE *VmPtr = KMalloc(sizeof(*VmPtr));
	VmPtr->VmxOnRegionVirtAddr = KMalloc(0x1000);
	VmPtr->VmxOnRegionPhysAddr = VIRT2PHYS(VmPtr->VmxOnRegionVirtAddr);
	
	VmPtr->VmxRegionVirtAddr = KMalloc(0x1000);
	VmPtr->VmxRegionPhysAddr = VIRT2PHYS(VmPtr->VmxRegionVirtAddr);
	
	VmPtr->VmcsVirt.apic_access_addr = KMalloc(0x1000);
	VmPtr->Vmcs.apic_access_addr = VIRT2PHYS(VmPtr->VmcsVirt.apic_access_addr);
	
	VmPtr->VmcsVirt.ept_pointer = KMalloc(0x200000);
	VmPtr->Vmcs.ept_pointer = VIRT2PHYS(VmPtr->VmcsVirt.ept_pointer) | 0x5e;
	//printk("VmPtr->Vmcs.ept_pointer = %016x\n", VmPtr->Vmcs.ept_pointer);
	
	VmPtr->VmcsVirt.io_bitmap_a = KMalloc(0x1000);
	VmPtr->Vmcs.io_bitmap_a = VIRT2PHYS(VmPtr->VmcsVirt.io_bitmap_a);
	memset(VmPtr->VmcsVirt.io_bitmap_a, 0xff, 0x1000);
	
	VmPtr->VmcsVirt.io_bitmap_b = KMalloc(0x1000);
	VmPtr->Vmcs.io_bitmap_b = VIRT2PHYS(VmPtr->VmcsVirt.io_bitmap_b);
	memset(VmPtr->VmcsVirt.io_bitmap_b, 0xff, 0x1000);
	
	VmPtr->VmcsVirt.msr_bitmap = KMalloc(0x1000);
	VmPtr->Vmcs.msr_bitmap = VIRT2PHYS(VmPtr->VmcsVirt.msr_bitmap);
	
	VmPtr->VmcsVirt.pml_address = KMalloc(0x1000);
	VmPtr->Vmcs.pml_address = VIRT2PHYS(VmPtr->VmcsVirt.pml_address);
	
	VmPtr->VmcsVirt.posted_intr_desc_addr = KMalloc(0x1000);
	VmPtr->Vmcs.posted_intr_desc_addr = VIRT2PHYS(VmPtr->VmcsVirt.posted_intr_desc_addr);
	
	VmPtr->VmcsVirt.virtual_apic_page_addr = KMalloc(0x1000);
	VmPtr->Vmcs.virtual_apic_page_addr = VIRT2PHYS(VmPtr->VmcsVirt.virtual_apic_page_addr);
	
	VmPtr->VmcsVirt.vm_entry_msr_load_addr = KMalloc(0x1000);
	VmPtr->Vmcs.vm_entry_msr_load_addr = VIRT2PHYS(VmPtr->VmcsVirt.vm_entry_msr_load_addr);
	
	VmPtr->VmcsVirt.vm_exit_msr_load_addr = KMalloc(0x1000);
	VmPtr->Vmcs.vm_exit_msr_load_addr = VIRT2PHYS(VmPtr->VmcsVirt.vm_exit_msr_load_addr);
	
	VmPtr->VmcsVirt.vm_exit_msr_store_addr = KMalloc(0x1000);
	VmPtr->Vmcs.vm_exit_msr_store_addr = VIRT2PHYS(VmPtr->VmcsVirt.vm_exit_msr_store_addr);
	
	VmPtr->VmcsVirt.xss_exit_bitmap = KMalloc(0x1000);
	VmPtr->Vmcs.xss_exit_bitmap = VIRT2PHYS(VmPtr->VmcsVirt.xss_exit_bitmap);

	return VmPtr;
}

RETURN_STATUS VmxInit(VIRTUAL_MACHINE *Vm)
{
	RETURN_STATUS Status = 0;
	VmxOnPrepare(Vm->VmxOnRegionVirtAddr);
	VmxOnPrepare(Vm->VmxRegionVirtAddr);

	Status = VmOn(Vm->VmxOnRegionPhysAddr);
	if (Status != RETURN_SUCCESS)
		return Status;

	VmClear(Vm->VmxRegionPhysAddr);
	VmPtrLoad(Vm->VmxRegionPhysAddr);

	return Status;
}


/* CPU State after BIST.See SDM Vol-3 9.1.2 . */
VOID VmxCpuBistState(VIRTUAL_MACHINE *Vm)
{
	EptRealModeMapping(Vm);

	memset(&Vm->GuestRegs, 0, sizeof(Vm->GuestRegs));
	
	Vm->Vmcs.virtual_processor_id = 0x8086;
	Vm->Vmcs.posted_intr_nv = 0;
	Vm->Vmcs.guest_es_selector = 0;
	Vm->Vmcs.guest_cs_selector = 0xf000;
	Vm->Vmcs.guest_ss_selector = 0;
	Vm->Vmcs.guest_ds_selector = 0;
	Vm->Vmcs.guest_fs_selector = 0;
	Vm->Vmcs.guest_gs_selector = 0;
	Vm->Vmcs.guest_ldtr_selector = 0;
	Vm->Vmcs.guest_tr_selector = 0;
	Vm->Vmcs.guest_intr_status= 0;
	Vm->Vmcs.guest_pml_index = 0;
	Vm->Vmcs.host_es_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_cs_selector = SELECTOR_KERNEL_CODE;
	Vm->Vmcs.host_ss_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_ds_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_fs_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_gs_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_tr_selector = SELECTOR_TSS;
	//Vm->Vmcs.io_bitmap_a = 0;
	//Vm->Vmcs.io_bitmap_b = 0;
	//Vm->Vmcs.msr_bitmap = 0;
	//Vm->Vmcs.vm_exit_msr_store_addr = 0;
	//Vm->Vmcs.vm_exit_msr_load_addr = 0;
	//Vm->Vmcs.vm_entry_msr_load_addr = 0;
	//Vm->Vmcs.pml_address = 0;
	Vm->Vmcs.tsc_offset = 0;
	//Vm->Vmcs.virtual_apic_page_addr = 0;
	//Vm->Vmcs.apic_access_addr = 0;
	//Vm->Vmcs.posted_intr_desc_addr = 0;
	//Vm->Vmcs.ept_pointer = 0;
	Vm->Vmcs.eoi_exit_bitmap0 = 0;
	Vm->Vmcs.eoi_exit_bitmap1 = 0;
	Vm->Vmcs.eoi_exit_bitmap2 = 0;
	Vm->Vmcs.eoi_exit_bitmap3 = 0;
	Vm->Vmcs.eoi_exit_bitmap3 = 0;
	Vm->Vmcs.eoi_exit_bitmap3 = 0;
	Vm->Vmcs.xss_exit_bitmap = 0;
	
	Vm->Vmcs.guest_physical_address = 0;
	Vm->Vmcs.vmcs_link_pointer = 0xffffffffffffffff;
	Vm->Vmcs.guest_ia32_debugctl = 0;
	Vm->Vmcs.guest_ia32_pat = 0;
	Vm->Vmcs.guest_ia32_efer = 0;
	
	Vm->Vmcs.guest_ia32_perf_global_ctrl = 0;
	Vm->Vmcs.guest_pdptr0 = 0;
	Vm->Vmcs.guest_pdptr1 = 0;
	Vm->Vmcs.guest_pdptr2 = 0;
	Vm->Vmcs.guest_pdptr3 = 0;
	Vm->Vmcs.guest_bndcfgs = 0;
	Vm->Vmcs.host_ia32_pat = 0;
	Vm->Vmcs.host_ia32_efer = 0;
	Vm->Vmcs.host_ia32_perf_global_ctrl = 0;
	Vm->Vmcs.pin_based_vm_exec_control = PIN_BASED_ALWAYSON_WITHOUT_TRUE_MSR;
	Vm->Vmcs.cpu_based_vm_exec_control = CPU_BASED_FIXED_ONES | CPU_BASED_ACTIVATE_SECONDARY_CONTROLS | CPU_BASED_USE_IO_BITMAPS;
	Vm->Vmcs.exception_bitmap = 0;
	Vm->Vmcs.page_fault_error_code_mask = 0;
	Vm->Vmcs.page_fault_error_code_match = 0;
	Vm->Vmcs.cr3_target_count = 0;
	Vm->Vmcs.vm_exit_controls = VM_EXIT_ALWAYSON_WITHOUT_TRUE_MSR;
	Vm->Vmcs.vm_exit_msr_store_count = 0;
	Vm->Vmcs.vm_exit_msr_load_count = 0;
	Vm->Vmcs.vm_entry_controls = (VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR) ^ 0x4;
	Vm->Vmcs.vm_entry_msr_load_count = 0;
	Vm->Vmcs.vm_entry_intr_info_field = 0;
	Vm->Vmcs.vm_entry_exception_error_code = 0;
	Vm->Vmcs.vm_entry_instruction_len = 0;
	Vm->Vmcs.tpr_threshold = 0;
	Vm->Vmcs.secondary_vm_exec_control = SECONDARY_EXEC_UNRESTRICTED_GUEST | SECONDARY_EXEC_ENABLE_EPT;
	
	Vm->Vmcs.vm_exit_reason = 0;
	Vm->Vmcs.vm_exit_intr_info = 0;
	Vm->Vmcs.vm_exit_intr_error_code = 0;
	Vm->Vmcs.idt_vectoring_info_field = 0;
	Vm->Vmcs.idt_vectoring_error_code = 0;
	Vm->Vmcs.vm_exit_instruction_len = 0;
	Vm->Vmcs.vmx_instruction_info = 0;
	
	Vm->Vmcs.guest_es_limit = 0xffff;
	Vm->Vmcs.guest_cs_limit = 0xffff;
	Vm->Vmcs.guest_ss_limit = 0xffff;
	Vm->Vmcs.guest_ds_limit = 0xffff;
	Vm->Vmcs.guest_fs_limit = 0xffff;
	Vm->Vmcs.guest_gs_limit = 0xffff;
	Vm->Vmcs.guest_ldtr_limit = 0xffff;
	Vm->Vmcs.guest_tr_limit = 0xffff;
	Vm->Vmcs.guest_gdtr_limit = 0xffff;
	Vm->Vmcs.guest_idtr_limit = 0xffff;
	
	Vm->Vmcs.guest_es_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_cs_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_READABLE_MASK | VMX_AR_TYPE_CODE_MASK | VMX_AR_TYPE_WRITEABLE_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK | BIT13;
	Vm->Vmcs.guest_ss_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_ds_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_fs_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_gs_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_ldtr_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_tr_ar_bytes = VMX_AR_P_MASK | VMX_AR_TYPE_BUSY_16_TSS;

	Vm->Vmcs.guest_interruptibility_info = 0;
	Vm->Vmcs.guest_activity_state = GUEST_ACTIVITY_ACTIVE;
	Vm->Vmcs.guest_sysenter_cs = 0;
	Vm->Vmcs.vmx_preemption_timer_value = 0;
	Vm->Vmcs.host_ia32_sysenter_cs = 0;
	Vm->Vmcs.cr0_guest_host_mask = 0;
	Vm->Vmcs.cr4_guest_host_mask = 0;
	Vm->Vmcs.cr0_read_shadow = 0;
	Vm->Vmcs.cr4_read_shadow = 0;
	Vm->Vmcs.cr3_target_value0 = 0;
	Vm->Vmcs.cr3_target_value1 = 0;
	Vm->Vmcs.cr3_target_value2 = 0;
	Vm->Vmcs.cr3_target_value3 = 0;
	Vm->Vmcs.exit_qualification = 0;
	Vm->Vmcs.guest_linear_address = 0;
	Vm->Vmcs.guest_cr0 = 0x60000010 | X64ReadMsr(MSR_IA32_VMX_CR0_FIXED0) & X64ReadMsr(MSR_IA32_VMX_CR0_FIXED1) & 0x7ffffffe;
	Vm->Vmcs.guest_cr3 = 0;
	Vm->Vmcs.guest_cr4 = X64ReadMsr(MSR_IA32_VMX_CR4_FIXED0) & X64ReadMsr(MSR_IA32_VMX_CR4_FIXED1);
	Vm->Vmcs.guest_es_base = 0;
	Vm->Vmcs.guest_cs_base = 0xffff0000;
	Vm->Vmcs.guest_ss_base = 0;
	Vm->Vmcs.guest_ds_base = 0;
	Vm->Vmcs.guest_fs_base = 0;
	Vm->Vmcs.guest_gs_base = 0;
	Vm->Vmcs.guest_ldtr_base = 0;
	Vm->Vmcs.guest_gdtr_base = 0;
	Vm->Vmcs.guest_idtr_base = 0;
	Vm->Vmcs.guest_dr7 = 0;
	Vm->Vmcs.guest_rsp = 0;
	Vm->Vmcs.guest_rip = 0xfff0;
	Vm->Vmcs.guest_rflags = BIT1;
	Vm->Vmcs.guest_pending_dbg_exceptions = 0;
	Vm->Vmcs.guest_sysenter_esp = 0;
	Vm->Vmcs.guest_sysenter_eip = 0;
	Vm->Vmcs.host_cr0 = X64ReadCr(0);
	Vm->Vmcs.host_cr3 = X64ReadCr(3);
	Vm->Vmcs.host_cr4 = X64ReadCr(4);
	Vm->Vmcs.host_fs_base = 0;
	Vm->Vmcs.host_gs_base = 0;
	Vm->Vmcs.host_tr_base = (u64)&TSS[0];
	Vm->Vmcs.host_gdtr_base = GDT.Base;
	Vm->Vmcs.host_idtr_base = IDT.Base;
	Vm->Vmcs.host_ia32_sysenter_esp = 0;
	Vm->Vmcs.host_ia32_sysenter_eip = 0;
	Vm->Vmcs.host_rsp = 0xffff800001000000;	//VmLaunch/VmResume will rewrite this field.
	Vm->Vmcs.host_rip = (u64)VmExit;
}


VOID VmxCopyHostState(VIRTUAL_MACHINE *Vm)
{
	EptIdenticalMapping4G(Vm);
	
	Vm->Vmcs.virtual_processor_id = 0x8086;
	Vm->Vmcs.posted_intr_nv = 0;
	Vm->Vmcs.guest_es_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.guest_cs_selector = SELECTOR_KERNEL_CODE;
	Vm->Vmcs.guest_ss_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.guest_ds_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.guest_fs_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.guest_gs_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.guest_ldtr_selector = 0;
	Vm->Vmcs.guest_tr_selector = SELECTOR_TSS;
	Vm->Vmcs.guest_intr_status= 0;
	Vm->Vmcs.guest_pml_index = 0;
	Vm->Vmcs.host_es_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_cs_selector = SELECTOR_KERNEL_CODE;
	Vm->Vmcs.host_ss_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_ds_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_fs_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_gs_selector = SELECTOR_KERNEL_DATA;
	Vm->Vmcs.host_tr_selector = SELECTOR_TSS;
	//Vm->Vmcs.io_bitmap_a = 0;
	//Vm->Vmcs.io_bitmap_b = 0;
	//Vm->Vmcs.msr_bitmap = 0;
	//Vm->Vmcs.vm_exit_msr_store_addr = 0;
	//Vm->Vmcs.vm_exit_msr_load_addr = 0;
	//Vm->Vmcs.vm_entry_msr_load_addr = 0;
	//Vm->Vmcs.pml_address = 0;
	Vm->Vmcs.tsc_offset = 0x80000000;
	//Vm->Vmcs.virtual_apic_page_addr = 0;
	//Vm->Vmcs.apic_access_addr = 0;
	//Vm->Vmcs.posted_intr_desc_addr = 0;
	//Vm->Vmcs.ept_pointer = 0;
	Vm->Vmcs.eoi_exit_bitmap0 = 0;
	Vm->Vmcs.eoi_exit_bitmap1 = 0;
	Vm->Vmcs.eoi_exit_bitmap2 = 0;
	Vm->Vmcs.eoi_exit_bitmap3 = 0;
	Vm->Vmcs.eoi_exit_bitmap3 = 0;
	Vm->Vmcs.eoi_exit_bitmap3 = 0;
	Vm->Vmcs.xss_exit_bitmap = 0;
	
	Vm->Vmcs.guest_physical_address = 0;
	Vm->Vmcs.vmcs_link_pointer = 0xffffffffffffffff;
	Vm->Vmcs.guest_ia32_debugctl = 0;
	Vm->Vmcs.guest_ia32_pat = 0;
	Vm->Vmcs.guest_ia32_efer = 0;
	
	Vm->Vmcs.guest_ia32_perf_global_ctrl = 0;
	Vm->Vmcs.guest_pdptr0 = 0;
	Vm->Vmcs.guest_pdptr1 = 0;
	Vm->Vmcs.guest_pdptr2 = 0;
	Vm->Vmcs.guest_pdptr3 = 0;
	Vm->Vmcs.guest_bndcfgs = 0;
	Vm->Vmcs.host_ia32_pat = 0;
	Vm->Vmcs.host_ia32_efer = 0;
	Vm->Vmcs.host_ia32_perf_global_ctrl = 0;
	Vm->Vmcs.pin_based_vm_exec_control = PIN_BASED_ALWAYSON_WITHOUT_TRUE_MSR;
	Vm->Vmcs.cpu_based_vm_exec_control = CPU_BASED_FIXED_ONES | CPU_BASED_ACTIVATE_SECONDARY_CONTROLS | CPU_BASED_USE_IO_BITMAPS;
	Vm->Vmcs.exception_bitmap = 0;
	Vm->Vmcs.page_fault_error_code_mask = 0;
	Vm->Vmcs.page_fault_error_code_match = 0;
	Vm->Vmcs.cr3_target_count = 0;
	Vm->Vmcs.vm_exit_controls = VM_EXIT_ALWAYSON_WITHOUT_TRUE_MSR;
	Vm->Vmcs.vm_exit_msr_store_count = 0;
	Vm->Vmcs.vm_exit_msr_load_count = 0;
	Vm->Vmcs.vm_entry_controls = (VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR | VM_ENTRY_IA32E_MODE) ^ 0x4;
	Vm->Vmcs.vm_entry_msr_load_count = 0;
	Vm->Vmcs.vm_entry_intr_info_field = 0;
	Vm->Vmcs.vm_entry_exception_error_code = 0;
	Vm->Vmcs.vm_entry_instruction_len = 0;
	Vm->Vmcs.tpr_threshold = 0;
	Vm->Vmcs.secondary_vm_exec_control = SECONDARY_EXEC_UNRESTRICTED_GUEST | SECONDARY_EXEC_ENABLE_EPT;
	
	Vm->Vmcs.vm_exit_reason = 0;
	Vm->Vmcs.vm_exit_intr_info = 0;
	Vm->Vmcs.vm_exit_intr_error_code = 0;
	Vm->Vmcs.idt_vectoring_info_field = 0;
	Vm->Vmcs.idt_vectoring_error_code = 0;
	Vm->Vmcs.vm_exit_instruction_len = 0;
	Vm->Vmcs.vmx_instruction_info = 0;
	
	Vm->Vmcs.guest_es_limit = 0xffff;
	Vm->Vmcs.guest_cs_limit = 0xffff;
	Vm->Vmcs.guest_ss_limit = 0xffff;
	Vm->Vmcs.guest_ds_limit = 0xffff;
	Vm->Vmcs.guest_fs_limit = 0xffff;
	Vm->Vmcs.guest_gs_limit = 0xffff;
	Vm->Vmcs.guest_ldtr_limit = 0xffff;
	Vm->Vmcs.guest_tr_limit = 0xffff;
	Vm->Vmcs.guest_gdtr_limit = GDT.Limit;
	Vm->Vmcs.guest_idtr_limit = IDT.Limit;

	Vm->Vmcs.guest_es_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_cs_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_READABLE_MASK | VMX_AR_TYPE_CODE_MASK | VMX_AR_TYPE_WRITEABLE_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK | BIT13;
	Vm->Vmcs.guest_ss_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_ds_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_fs_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_gs_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_ldtr_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vm->Vmcs.guest_tr_ar_bytes = VMX_AR_P_MASK | VMX_AR_TYPE_BUSY_64_TSS;
	
	Vm->Vmcs.guest_interruptibility_info = 0;
	Vm->Vmcs.guest_activity_state = 0;
	Vm->Vmcs.guest_sysenter_cs = 0;
	Vm->Vmcs.vmx_preemption_timer_value = 0;
	Vm->Vmcs.host_ia32_sysenter_cs = 0;
	Vm->Vmcs.cr0_guest_host_mask = 0;
	Vm->Vmcs.cr4_guest_host_mask = 0;
	Vm->Vmcs.cr0_read_shadow = 0;
	Vm->Vmcs.cr4_read_shadow = 0;
	Vm->Vmcs.cr3_target_value0 = 0;
	Vm->Vmcs.cr3_target_value1 = 0;
	Vm->Vmcs.cr3_target_value2 = 0;
	Vm->Vmcs.cr3_target_value3 = 0;
	Vm->Vmcs.exit_qualification = 0;
	Vm->Vmcs.guest_linear_address = 0;
	Vm->Vmcs.guest_cr0 = X64ReadCr(0);
	Vm->Vmcs.guest_cr3 = X64ReadCr(3);
	Vm->Vmcs.guest_cr4 = X64ReadCr(4);
	Vm->Vmcs.guest_es_base = 0;
	Vm->Vmcs.guest_cs_base = 0;
	Vm->Vmcs.guest_ss_base = 0;
	Vm->Vmcs.guest_ds_base = 0;
	Vm->Vmcs.guest_fs_base = 0;
	Vm->Vmcs.guest_gs_base = 0;
	Vm->Vmcs.guest_ldtr_base = 0;
	Vm->Vmcs.guest_gdtr_base = GDT.Base;
	Vm->Vmcs.guest_idtr_base = IDT.Base;
	Vm->Vmcs.guest_dr7 = 0;
	Vm->Vmcs.guest_rsp = 0xFFFF800010000000;
	Vm->Vmcs.guest_rip = (u64)GuestTestCode;
	Vm->Vmcs.guest_rflags = BIT1 | FLAG_IF;
	Vm->Vmcs.guest_pending_dbg_exceptions = 0;
	Vm->Vmcs.guest_sysenter_esp = 0;
	Vm->Vmcs.guest_sysenter_eip = 0;
	Vm->Vmcs.host_cr0 = X64ReadCr(0);
	Vm->Vmcs.host_cr3 = X64ReadCr(3);
	Vm->Vmcs.host_cr4 = X64ReadCr(4);
	Vm->Vmcs.host_fs_base = 0;
	Vm->Vmcs.host_gs_base = 0;
	Vm->Vmcs.host_tr_base = (u64)&TSS[0];
	Vm->Vmcs.host_gdtr_base = GDT.Base;
	Vm->Vmcs.host_idtr_base = IDT.Base;
	Vm->Vmcs.host_ia32_sysenter_esp = 0;
	Vm->Vmcs.host_ia32_sysenter_eip = 0;
	Vm->Vmcs.host_rsp = 0xffff800001000000;	//VmLaunch/VmResume will rewrite this field.
	Vm->Vmcs.host_rip = (u64)VmExit;
}

VOID VmxSetVmcs(struct vmcs12 *vmcs_ptr)
{
	VmWrite(VIRTUAL_PROCESSOR_ID, vmcs_ptr->virtual_processor_id);
	VmWrite(POSTED_INTR_NV, vmcs_ptr->posted_intr_nv);
	VmWrite(GUEST_ES_SELECTOR, vmcs_ptr->guest_es_selector);
	VmWrite(GUEST_CS_SELECTOR, vmcs_ptr->guest_cs_selector);
	VmWrite(GUEST_SS_SELECTOR, vmcs_ptr->guest_ss_selector);
	VmWrite(GUEST_DS_SELECTOR, vmcs_ptr->guest_ds_selector);
	VmWrite(GUEST_FS_SELECTOR, vmcs_ptr->guest_fs_selector);
	VmWrite(GUEST_GS_SELECTOR, vmcs_ptr->guest_gs_selector);
	VmWrite(GUEST_LDTR_SELECTOR, vmcs_ptr->guest_ldtr_selector);
	VmWrite(GUEST_TR_SELECTOR, vmcs_ptr->guest_tr_selector);
	VmWrite(GUEST_INTR_STATUS, vmcs_ptr->guest_intr_status);
	VmWrite(GUEST_PML_INDEX, vmcs_ptr->guest_pml_index);
	VmWrite(HOST_ES_SELECTOR, vmcs_ptr->host_es_selector);
	VmWrite(HOST_CS_SELECTOR, vmcs_ptr->host_cs_selector);
	VmWrite(HOST_SS_SELECTOR, vmcs_ptr->host_ss_selector);
	VmWrite(HOST_DS_SELECTOR, vmcs_ptr->host_ds_selector);
	VmWrite(HOST_FS_SELECTOR, vmcs_ptr->host_fs_selector);
	VmWrite(HOST_GS_SELECTOR, vmcs_ptr->host_gs_selector);
	VmWrite(HOST_TR_SELECTOR, vmcs_ptr->host_tr_selector);
	VmWrite(IO_BITMAP_A, vmcs_ptr->io_bitmap_a);
	VmWrite(IO_BITMAP_B, vmcs_ptr->io_bitmap_b);
	VmWrite(MSR_BITMAP, vmcs_ptr->msr_bitmap);
	VmWrite(VM_EXIT_MSR_STORE_ADDR, vmcs_ptr->vm_exit_msr_store_addr);
	VmWrite(VM_EXIT_MSR_LOAD_ADDR, vmcs_ptr->vm_exit_msr_load_addr);
	VmWrite(VM_ENTRY_MSR_LOAD_ADDR, vmcs_ptr->vm_entry_msr_load_addr);
	VmWrite(PML_ADDRESS, vmcs_ptr->pml_address);
	VmWrite(TSC_OFFSET, vmcs_ptr->tsc_offset);
	VmWrite(VIRTUAL_APIC_PAGE_ADDR, vmcs_ptr->virtual_apic_page_addr);
	VmWrite(APIC_ACCESS_ADDR, vmcs_ptr->apic_access_addr);
	VmWrite(POSTED_INTR_DESC_ADDR, vmcs_ptr->posted_intr_desc_addr);
	VmWrite(EPT_POINTER, vmcs_ptr->ept_pointer);
	VmWrite(EOI_EXIT_BITMAP0, vmcs_ptr->eoi_exit_bitmap0);
	VmWrite(EOI_EXIT_BITMAP1, vmcs_ptr->eoi_exit_bitmap1);
	VmWrite(EOI_EXIT_BITMAP2, vmcs_ptr->eoi_exit_bitmap2);
	VmWrite(EOI_EXIT_BITMAP3, vmcs_ptr->eoi_exit_bitmap3);
	VmWrite(VMREAD_BITMAP, vmcs_ptr->eoi_exit_bitmap3);
	VmWrite(VMWRITE_BITMAP, vmcs_ptr->eoi_exit_bitmap3);
	VmWrite(XSS_EXIT_BITMAP, vmcs_ptr->xss_exit_bitmap);
	VmWrite(TSC_MULTIPLIER, vmcs_ptr->tsc_offset);
	//VmWrite(GUEST_PHYSICAL_ADDRESS, vmcs_ptr->virtual_processor_id);
	VmWrite(VMCS_LINK_POINTER, vmcs_ptr->vmcs_link_pointer);
	VmWrite(GUEST_IA32_DEBUGCTL, vmcs_ptr->guest_ia32_debugctl);
	VmWrite(GUEST_IA32_PAT, vmcs_ptr->guest_ia32_pat);
	VmWrite(GUEST_IA32_EFER, vmcs_ptr->guest_ia32_efer);
	
	VmWrite(GUEST_IA32_PERF_GLOBAL_CTRL, vmcs_ptr->guest_ia32_perf_global_ctrl);
	VmWrite(GUEST_PDPTR0, vmcs_ptr->guest_pdptr0);
	VmWrite(GUEST_PDPTR1, vmcs_ptr->guest_pdptr1);
	VmWrite(GUEST_PDPTR2, vmcs_ptr->guest_pdptr2);
	VmWrite(GUEST_PDPTR3, vmcs_ptr->guest_pdptr3);
	VmWrite(GUEST_BNDCFGS, vmcs_ptr->guest_bndcfgs);
	VmWrite(HOST_IA32_PAT, vmcs_ptr->host_ia32_pat);
	VmWrite(HOST_IA32_EFER, vmcs_ptr->host_ia32_efer);
	VmWrite(HOST_IA32_PERF_GLOBAL_CTRL, vmcs_ptr->host_ia32_perf_global_ctrl);
	VmWrite(PIN_BASED_VM_EXEC_CONTROL, vmcs_ptr->pin_based_vm_exec_control);
	VmWrite(CPU_BASED_VM_EXEC_CONTROL, vmcs_ptr->cpu_based_vm_exec_control);
	VmWrite(EXCEPTION_BITMAP, vmcs_ptr->exception_bitmap);
	VmWrite(PAGE_FAULT_ERROR_CODE_MASK, vmcs_ptr->page_fault_error_code_mask);
	VmWrite(PAGE_FAULT_ERROR_CODE_MATCH, vmcs_ptr->page_fault_error_code_match);
	VmWrite(CR3_TARGET_COUNT, vmcs_ptr->cr3_target_count);
	VmWrite(VM_EXIT_CONTROLS, vmcs_ptr->vm_exit_controls);
	VmWrite(VM_EXIT_MSR_STORE_COUNT, vmcs_ptr->vm_exit_msr_store_count);
	VmWrite(VM_EXIT_MSR_LOAD_COUNT, vmcs_ptr->vm_exit_msr_load_count);
	VmWrite(VM_ENTRY_CONTROLS, vmcs_ptr->vm_entry_controls);
	VmWrite(VM_ENTRY_MSR_LOAD_COUNT, vmcs_ptr->vm_entry_msr_load_count);
	VmWrite(VM_ENTRY_INTR_INFO_FIELD, vmcs_ptr->vm_entry_intr_info_field);
	//VmWrite(VM_ENTRY_EXCEPTION_ERROR_CODE, vmcs_ptr->virtual_processor_id);
	VmWrite(VM_ENTRY_INSTRUCTION_LEN, vmcs_ptr->vm_entry_instruction_len);
	VmWrite(TPR_THRESHOLD, vmcs_ptr->tpr_threshold);
	VmWrite(SECONDARY_VM_EXEC_CONTROL, vmcs_ptr->secondary_vm_exec_control);
	
	//VmWrite(PLE_GAP, vmcs_ptr->);
	//VmWrite(PLE_WINDOW, vmcs_ptr->);
	
	//VmWrite(VM_EXIT_REASON, vmcs_ptr->virtual_processor_id);
	//VmWrite(VM_EXIT_INTR_INFO, vmcs_ptr->virtual_processor_id);
	//VmWrite(VM_EXIT_INTR_ERROR_CODE, vmcs_ptr->virtual_processor_id);
	//VmWrite(IDT_VECTORING_INFO_FIELD, vmcs_ptr->virtual_processor_id);
	//VmWrite(IDT_VECTORING_ERROR_CODE, vmcs_ptr->virtual_processor_id);
	//VmWrite(VM_EXIT_INSTRUCTION_LEN, vmcs_ptr->virtual_processor_id);
	//VmWrite(VMX_INSTRUCTION_INFO, vmcs_ptr->virtual_processor_id);
	VmWrite(GUEST_ES_LIMIT, vmcs_ptr->guest_es_limit);
	VmWrite(GUEST_CS_LIMIT, vmcs_ptr->guest_cs_limit);
	VmWrite(GUEST_SS_LIMIT, vmcs_ptr->guest_ss_limit);
	VmWrite(GUEST_DS_LIMIT, vmcs_ptr->guest_ds_limit);
	VmWrite(GUEST_FS_LIMIT, vmcs_ptr->guest_fs_limit);
	VmWrite(GUEST_GS_LIMIT, vmcs_ptr->guest_gs_limit);
	VmWrite(GUEST_LDTR_LIMIT, vmcs_ptr->guest_ldtr_limit);
	VmWrite(GUEST_TR_LIMIT, vmcs_ptr->guest_tr_limit);
	VmWrite(GUEST_GDTR_LIMIT, vmcs_ptr->guest_gdtr_limit);
	VmWrite(GUEST_IDTR_LIMIT, vmcs_ptr->guest_idtr_limit);
	VmWrite(GUEST_ES_AR_BYTES, vmcs_ptr->guest_es_ar_bytes);
	VmWrite(GUEST_CS_AR_BYTES, vmcs_ptr->guest_cs_ar_bytes);
	VmWrite(GUEST_SS_AR_BYTES, vmcs_ptr->guest_ss_ar_bytes);
	VmWrite(GUEST_DS_AR_BYTES, vmcs_ptr->guest_ds_ar_bytes);
	VmWrite(GUEST_FS_AR_BYTES, vmcs_ptr->guest_fs_ar_bytes);
	VmWrite(GUEST_GS_AR_BYTES, vmcs_ptr->guest_gs_ar_bytes);
	VmWrite(GUEST_LDTR_AR_BYTES, vmcs_ptr->guest_ldtr_ar_bytes);
	VmWrite(GUEST_TR_AR_BYTES, vmcs_ptr->guest_tr_ar_bytes);
	VmWrite(GUEST_INTERRUPTIBILITY_INFO, vmcs_ptr->guest_interruptibility_info);
	VmWrite(GUEST_ACTIVITY_STATE, vmcs_ptr->guest_activity_state);
	VmWrite(GUEST_SYSENTER_CS, vmcs_ptr->guest_sysenter_cs);
	VmWrite(VMX_PREEMPTION_TIMER_VALUE, vmcs_ptr->vmx_preemption_timer_value);
	VmWrite(HOST_IA32_SYSENTER_CS, vmcs_ptr->host_ia32_sysenter_cs);
	VmWrite(CR0_GUEST_HOST_MASK, vmcs_ptr->cr0_guest_host_mask);
	VmWrite(CR4_GUEST_HOST_MASK, vmcs_ptr->cr4_guest_host_mask);
	VmWrite(CR0_READ_SHADOW, vmcs_ptr->cr0_read_shadow);
	VmWrite(CR4_READ_SHADOW, vmcs_ptr->cr4_read_shadow);
	VmWrite(CR3_TARGET_VALUE0, vmcs_ptr->cr3_target_value0);
	VmWrite(CR3_TARGET_VALUE1, vmcs_ptr->cr3_target_value1);
	VmWrite(CR3_TARGET_VALUE2, vmcs_ptr->cr3_target_value2);
	VmWrite(CR3_TARGET_VALUE3, vmcs_ptr->cr3_target_value3);
	//VmWrite(EXIT_QUALIFICATION, vmcs_ptr->virtual_processor_id);
	//VmWrite(GUEST_LINEAR_ADDRESS, vmcs_ptr->virtual_processor_id);
	VmWrite(GUEST_CR0, vmcs_ptr->guest_cr0);
	VmWrite(GUEST_CR3, vmcs_ptr->guest_cr3);
	VmWrite(GUEST_CR4, vmcs_ptr->guest_cr4);
	VmWrite(GUEST_ES_BASE, vmcs_ptr->guest_es_base);
	VmWrite(GUEST_CS_BASE, vmcs_ptr->guest_cs_base);
	VmWrite(GUEST_SS_BASE, vmcs_ptr->guest_ss_base);
	VmWrite(GUEST_DS_BASE, vmcs_ptr->guest_ds_base);
	VmWrite(GUEST_FS_BASE, vmcs_ptr->guest_fs_base);
	VmWrite(GUEST_GS_BASE, vmcs_ptr->guest_gs_base);
	VmWrite(GUEST_LDTR_BASE, vmcs_ptr->guest_ldtr_base);
	VmWrite(GUEST_GDTR_BASE, vmcs_ptr->guest_gdtr_base);
	VmWrite(GUEST_IDTR_BASE, vmcs_ptr->guest_idtr_base);
	VmWrite(GUEST_DR7, vmcs_ptr->guest_dr7);
	VmWrite(GUEST_RSP, vmcs_ptr->guest_rsp);
	VmWrite(GUEST_RIP, vmcs_ptr->guest_rip);
	VmWrite(GUEST_RFLAGS, vmcs_ptr->guest_rflags);
	VmWrite(GUEST_PENDING_DBG_EXCEPTIONS, vmcs_ptr->guest_pending_dbg_exceptions);
	VmWrite(GUEST_SYSENTER_ESP, vmcs_ptr->guest_sysenter_esp);
	VmWrite(GUEST_SYSENTER_EIP, vmcs_ptr->guest_sysenter_eip);
	VmWrite(HOST_CR0, vmcs_ptr->host_cr0);
	VmWrite(HOST_CR3, vmcs_ptr->host_cr3);
	VmWrite(HOST_CR4, vmcs_ptr->host_cr4);
	VmWrite(HOST_FS_BASE, vmcs_ptr->host_fs_base);
	VmWrite(HOST_GS_BASE, vmcs_ptr->host_gs_base);
	VmWrite(HOST_TR_BASE, vmcs_ptr->host_tr_base);
	VmWrite(HOST_GDTR_BASE, vmcs_ptr->host_gdtr_base);
	VmWrite(HOST_IDTR_BASE, vmcs_ptr->host_idtr_base);
	VmWrite(HOST_IA32_SYSENTER_ESP, vmcs_ptr->host_ia32_sysenter_esp);
	VmWrite(HOST_IA32_SYSENTER_EIP, vmcs_ptr->host_ia32_sysenter_eip);
	VmWrite(HOST_RSP, vmcs_ptr->host_rsp);
	VmWrite(HOST_RIP, vmcs_ptr->host_rip);
}

VOID GetVmxState(struct vmcs12 *vmcs_ptr)
{
	vmcs_ptr->virtual_processor_id = VmRead(VIRTUAL_PROCESSOR_ID);
	vmcs_ptr->posted_intr_nv = VmRead(POSTED_INTR_NV);
	vmcs_ptr->guest_es_selector = VmRead(GUEST_ES_SELECTOR);
	vmcs_ptr->guest_cs_selector = VmRead(GUEST_CS_SELECTOR);
	vmcs_ptr->guest_ss_selector = VmRead(GUEST_SS_SELECTOR);
	vmcs_ptr->guest_ds_selector = VmRead(GUEST_DS_SELECTOR);
	vmcs_ptr->guest_fs_selector = VmRead(GUEST_FS_SELECTOR);
	vmcs_ptr->guest_gs_selector = VmRead(GUEST_GS_SELECTOR);
	vmcs_ptr->guest_ldtr_selector = VmRead(GUEST_LDTR_SELECTOR);
	vmcs_ptr->guest_tr_selector = VmRead(GUEST_TR_SELECTOR);
	vmcs_ptr->guest_intr_status= VmRead(GUEST_INTR_STATUS);
	vmcs_ptr->guest_pml_index = VmRead(GUEST_PML_INDEX);
	vmcs_ptr->host_es_selector = VmRead(HOST_ES_SELECTOR);
	vmcs_ptr->host_cs_selector = VmRead(HOST_CS_SELECTOR);
	vmcs_ptr->host_ss_selector = VmRead(HOST_SS_SELECTOR);
	vmcs_ptr->host_ds_selector = VmRead(HOST_DS_SELECTOR);
	vmcs_ptr->host_fs_selector = VmRead(HOST_FS_SELECTOR);
	vmcs_ptr->host_gs_selector = VmRead(HOST_GS_SELECTOR);
	vmcs_ptr->host_tr_selector = VmRead(HOST_TR_SELECTOR);
	vmcs_ptr->io_bitmap_a = VmRead(IO_BITMAP_A);
	vmcs_ptr->io_bitmap_b = VmRead(IO_BITMAP_B);
	vmcs_ptr->msr_bitmap = VmRead(MSR_BITMAP);
	vmcs_ptr->vm_exit_msr_store_addr = VmRead(VM_EXIT_MSR_STORE_ADDR);
	vmcs_ptr->vm_exit_msr_load_addr = VmRead(VM_EXIT_MSR_LOAD_ADDR);
	vmcs_ptr->vm_entry_msr_load_addr = VmRead(VM_ENTRY_MSR_LOAD_ADDR);
	vmcs_ptr->pml_address = VmRead(PML_ADDRESS);
	vmcs_ptr->tsc_offset = VmRead(TSC_OFFSET);
	vmcs_ptr->virtual_apic_page_addr = VmRead(VIRTUAL_APIC_PAGE_ADDR);
	vmcs_ptr->apic_access_addr = VmRead(APIC_ACCESS_ADDR);
	vmcs_ptr->posted_intr_desc_addr = VmRead(POSTED_INTR_DESC_ADDR);
	vmcs_ptr->ept_pointer = VmRead(EPT_POINTER);
	vmcs_ptr->eoi_exit_bitmap0 = VmRead(EOI_EXIT_BITMAP0);
	vmcs_ptr->eoi_exit_bitmap1 = VmRead(EOI_EXIT_BITMAP1);
	vmcs_ptr->eoi_exit_bitmap2 = VmRead(EOI_EXIT_BITMAP2);
	vmcs_ptr->eoi_exit_bitmap3 = VmRead(EOI_EXIT_BITMAP3);
	vmcs_ptr->eoi_exit_bitmap3 = VmRead(VMREAD_BITMAP);
	vmcs_ptr->eoi_exit_bitmap3 = VmRead(VMWRITE_BITMAP);
	vmcs_ptr->xss_exit_bitmap = VmRead(XSS_EXIT_BITMAP);
	vmcs_ptr->tsc_offset = VmRead(TSC_MULTIPLIER);
	vmcs_ptr->guest_physical_address = VmRead(GUEST_PHYSICAL_ADDRESS);
	vmcs_ptr->vmcs_link_pointer = VmRead(VMCS_LINK_POINTER);
	vmcs_ptr->guest_ia32_debugctl = VmRead(GUEST_IA32_DEBUGCTL);
	vmcs_ptr->guest_ia32_pat = VmRead(GUEST_IA32_PAT);
	vmcs_ptr->guest_ia32_efer = VmRead(GUEST_IA32_EFER);
	
	vmcs_ptr->guest_ia32_perf_global_ctrl = VmRead(GUEST_IA32_PERF_GLOBAL_CTRL);
	vmcs_ptr->guest_pdptr0 = VmRead(GUEST_PDPTR0);
	vmcs_ptr->guest_pdptr1 = VmRead(GUEST_PDPTR1);
	vmcs_ptr->guest_pdptr2 = VmRead(GUEST_PDPTR2);
	vmcs_ptr->guest_pdptr3 = VmRead(GUEST_PDPTR3);
	vmcs_ptr->guest_bndcfgs = VmRead(GUEST_BNDCFGS);
	vmcs_ptr->host_ia32_pat = VmRead(HOST_IA32_PAT);
	vmcs_ptr->host_ia32_efer = VmRead(HOST_IA32_EFER);
	vmcs_ptr->host_ia32_perf_global_ctrl = VmRead(HOST_IA32_PERF_GLOBAL_CTRL);
	vmcs_ptr->pin_based_vm_exec_control = VmRead(PIN_BASED_VM_EXEC_CONTROL);
	vmcs_ptr->cpu_based_vm_exec_control = VmRead(CPU_BASED_VM_EXEC_CONTROL);
	vmcs_ptr->exception_bitmap = VmRead(EXCEPTION_BITMAP);
	vmcs_ptr->page_fault_error_code_mask = VmRead(PAGE_FAULT_ERROR_CODE_MASK);
	vmcs_ptr->page_fault_error_code_match = VmRead(PAGE_FAULT_ERROR_CODE_MATCH);
	vmcs_ptr->cr3_target_count = VmRead(CR3_TARGET_COUNT);
	vmcs_ptr->vm_exit_controls = VmRead(VM_EXIT_CONTROLS);
	vmcs_ptr->vm_exit_msr_store_count = VmRead(VM_EXIT_MSR_STORE_COUNT);
	vmcs_ptr->vm_exit_msr_load_count = VmRead(VM_EXIT_MSR_LOAD_COUNT);
	vmcs_ptr->vm_entry_controls = VmRead(VM_ENTRY_CONTROLS);
	vmcs_ptr->vm_entry_msr_load_count = VmRead(VM_ENTRY_MSR_LOAD_COUNT);
	vmcs_ptr->vm_entry_intr_info_field = VmRead(VM_ENTRY_INTR_INFO_FIELD);
	vmcs_ptr->vm_entry_exception_error_code = VmRead(VM_ENTRY_EXCEPTION_ERROR_CODE);
	vmcs_ptr->vm_entry_instruction_len = VmRead(VM_ENTRY_INSTRUCTION_LEN);
	vmcs_ptr->tpr_threshold = VmRead(TPR_THRESHOLD);
	vmcs_ptr->secondary_vm_exec_control = VmRead(SECONDARY_VM_EXEC_CONTROL);
	
	// = VmRead(PLE_GAP, vmcs_ptr->);
	// = VmRead(PLE_WINDOW, vmcs_ptr->);
	
	vmcs_ptr->vm_exit_reason = VmRead(VM_EXIT_REASON);
	vmcs_ptr->vm_exit_intr_info = VmRead(VM_EXIT_INTR_INFO);
	vmcs_ptr->vm_exit_intr_error_code = VmRead(VM_EXIT_INTR_ERROR_CODE);
	vmcs_ptr->idt_vectoring_info_field = VmRead(IDT_VECTORING_INFO_FIELD);
	vmcs_ptr->idt_vectoring_error_code = VmRead(IDT_VECTORING_ERROR_CODE);
	vmcs_ptr->vm_exit_instruction_len = VmRead(VM_EXIT_INSTRUCTION_LEN);
	vmcs_ptr->vmx_instruction_info = VmRead(VMX_INSTRUCTION_INFO);
	
	vmcs_ptr->guest_es_limit = VmRead(GUEST_ES_LIMIT);
	vmcs_ptr->guest_cs_limit = VmRead(GUEST_CS_LIMIT);
	vmcs_ptr->guest_ss_limit = VmRead(GUEST_SS_LIMIT);
	vmcs_ptr->guest_ds_limit = VmRead(GUEST_DS_LIMIT);
	vmcs_ptr->guest_fs_limit = VmRead(GUEST_FS_LIMIT);
	vmcs_ptr->guest_gs_limit = VmRead(GUEST_GS_LIMIT);
	vmcs_ptr->guest_ldtr_limit = VmRead(GUEST_LDTR_LIMIT);
	vmcs_ptr->guest_tr_limit = VmRead(GUEST_TR_LIMIT);
	vmcs_ptr->guest_gdtr_limit = VmRead(GUEST_GDTR_LIMIT);
	vmcs_ptr->guest_idtr_limit = VmRead(GUEST_IDTR_LIMIT);
	vmcs_ptr->guest_es_ar_bytes = VmRead(GUEST_ES_AR_BYTES);
	vmcs_ptr->guest_cs_ar_bytes = VmRead(GUEST_CS_AR_BYTES);
	vmcs_ptr->guest_ss_ar_bytes = VmRead(GUEST_SS_AR_BYTES);
	vmcs_ptr->guest_ds_ar_bytes = VmRead(GUEST_DS_AR_BYTES);
	vmcs_ptr->guest_fs_ar_bytes = VmRead(GUEST_FS_AR_BYTES);
	vmcs_ptr->guest_gs_ar_bytes = VmRead(GUEST_GS_AR_BYTES);
	vmcs_ptr->guest_ldtr_ar_bytes = VmRead(GUEST_LDTR_AR_BYTES);
	vmcs_ptr->guest_tr_ar_bytes = VmRead(GUEST_TR_AR_BYTES);
	vmcs_ptr->guest_interruptibility_info = VmRead(GUEST_INTERRUPTIBILITY_INFO);
	vmcs_ptr->guest_activity_state = VmRead(GUEST_ACTIVITY_STATE);
	vmcs_ptr->guest_sysenter_cs = VmRead(GUEST_SYSENTER_CS);
	vmcs_ptr->vmx_preemption_timer_value = VmRead(VMX_PREEMPTION_TIMER_VALUE);
	vmcs_ptr->host_ia32_sysenter_cs = VmRead(HOST_IA32_SYSENTER_CS);
	vmcs_ptr->cr0_guest_host_mask = VmRead(CR0_GUEST_HOST_MASK);
	vmcs_ptr->cr4_guest_host_mask = VmRead(CR4_GUEST_HOST_MASK);
	vmcs_ptr->cr0_read_shadow = VmRead(CR0_READ_SHADOW);
	vmcs_ptr->cr4_read_shadow = VmRead(CR4_READ_SHADOW);
	vmcs_ptr->cr3_target_value0 = VmRead(CR3_TARGET_VALUE0);
	vmcs_ptr->cr3_target_value1 = VmRead(CR3_TARGET_VALUE1);
	vmcs_ptr->cr3_target_value2 = VmRead(CR3_TARGET_VALUE2);
	vmcs_ptr->cr3_target_value3 = VmRead(CR3_TARGET_VALUE3);
	vmcs_ptr->exit_qualification = VmRead(EXIT_QUALIFICATION);
	vmcs_ptr->guest_linear_address = VmRead(GUEST_LINEAR_ADDRESS);
	vmcs_ptr->guest_cr0 = VmRead(GUEST_CR0);
	vmcs_ptr->guest_cr3 = VmRead(GUEST_CR3);
	vmcs_ptr->guest_cr4 = VmRead(GUEST_CR4);
	vmcs_ptr->guest_es_base = VmRead(GUEST_ES_BASE);
	vmcs_ptr->guest_cs_base = VmRead(GUEST_CS_BASE);
	vmcs_ptr->guest_ss_base = VmRead(GUEST_SS_BASE);
	vmcs_ptr->guest_ds_base = VmRead(GUEST_DS_BASE);
	vmcs_ptr->guest_fs_base = VmRead(GUEST_FS_BASE);
	vmcs_ptr->guest_gs_base = VmRead(GUEST_GS_BASE);
	vmcs_ptr->guest_ldtr_base = VmRead(GUEST_LDTR_BASE);
	vmcs_ptr->guest_gdtr_base = VmRead(GUEST_GDTR_BASE);
	vmcs_ptr->guest_idtr_base = VmRead(GUEST_IDTR_BASE);
	vmcs_ptr->guest_dr7 = VmRead(GUEST_DR7);
	vmcs_ptr->guest_rsp = VmRead(GUEST_RSP);
	vmcs_ptr->guest_rip = VmRead(GUEST_RIP);
	vmcs_ptr->guest_rflags = VmRead(GUEST_RFLAGS);
	vmcs_ptr->guest_pending_dbg_exceptions = VmRead(GUEST_PENDING_DBG_EXCEPTIONS);
	vmcs_ptr->guest_sysenter_esp = VmRead(GUEST_SYSENTER_ESP);
	vmcs_ptr->guest_sysenter_eip = VmRead(GUEST_SYSENTER_EIP);
	vmcs_ptr->host_cr0 = VmRead(HOST_CR0);
	vmcs_ptr->host_cr3 = VmRead(HOST_CR3);
	vmcs_ptr->host_cr4 = VmRead(HOST_CR4);
	vmcs_ptr->host_fs_base = VmRead(HOST_FS_BASE);
	vmcs_ptr->host_gs_base = VmRead(HOST_GS_BASE);
	vmcs_ptr->host_tr_base = VmRead(HOST_TR_BASE);
	vmcs_ptr->host_gdtr_base = VmRead(HOST_GDTR_BASE);
	vmcs_ptr->host_idtr_base = VmRead(HOST_IDTR_BASE);
	vmcs_ptr->host_ia32_sysenter_esp = VmRead(HOST_IA32_SYSENTER_ESP);
	vmcs_ptr->host_ia32_sysenter_eip = VmRead(HOST_IA32_SYSENTER_EIP);
	vmcs_ptr->host_rsp = VmRead(HOST_RSP);
	vmcs_ptr->host_rip = VmRead(HOST_RIP);
}

int printk(const char *fmt, ...);

VOID VcpuRun(VIRTUAL_MACHINE *Vm)
{
	if (Vm->VmStatus == VM_STATUS_CLEAR)
	{
		Vm->VmStatus = VM_STATUS_LAUNCHED;
		VmLaunch(&Vm->HostRegs, &Vm->GuestRegs);
	}
	else
	{
		VmResume(&Vm->HostRegs, &Vm->GuestRegs);
	}
}
VOID VmHandleCpuId(VIRTUAL_MACHINE *Vm)
{
	UINT8 Buffer[50];
	//ArchReadCpuId(Buffer);
	//printk("Host CPU String:%s\n", Buffer);
	strcpy(Buffer, "Intel(R) Core(TM) i7-5960X CPU @ 8.00GHz");
	if (Vm->GuestRegs.RAX == 0x80000002 && Vm->GuestRegs.RCX == 0)
	{
		Vm->GuestRegs.RAX = *(UINT32 *)&Buffer[0];
		Vm->GuestRegs.RBX = *(UINT32 *)&Buffer[4];
		Vm->GuestRegs.RCX = *(UINT32 *)&Buffer[8];
		Vm->GuestRegs.RDX = *(UINT32 *)&Buffer[12];
	}

	if (Vm->GuestRegs.RAX == 0x80000003 && Vm->GuestRegs.RCX == 0)
	{
		Vm->GuestRegs.RAX = *(UINT32 *)&Buffer[16];
		Vm->GuestRegs.RBX = *(UINT32 *)&Buffer[20];
		Vm->GuestRegs.RCX = *(UINT32 *)&Buffer[24];
		Vm->GuestRegs.RDX = *(UINT32 *)&Buffer[28];
	}

	if (Vm->GuestRegs.RAX == 0x80000004 && Vm->GuestRegs.RCX == 0)
	{
		Vm->GuestRegs.RAX = *(UINT32 *)&Buffer[32];
		Vm->GuestRegs.RBX = *(UINT32 *)&Buffer[36];
		Vm->GuestRegs.RCX = *(UINT32 *)&Buffer[40];
		Vm->GuestRegs.RDX = *(UINT32 *)&Buffer[44];
	}

	if (Vm->GuestRegs.RAX == 0x80000001 && Vm->GuestRegs.RCX == 0)
	{
		Vm->GuestRegs.RAX = 0;
		Vm->GuestRegs.RBX = 0;
		Vm->GuestRegs.RCX = 0;
		Vm->GuestRegs.RDX = 0;
	}
}

RETURN_STATUS VmExitHandler(VIRTUAL_MACHINE *Vm)
{
	RETURN_STATUS Status = 0;
	UINT64 ExitReason = VmRead(VM_EXIT_REASON);
	UINT64 ExitQualification = VmRead(EXIT_QUALIFICATION);
	UINT64 ExitInstructionLen = VmRead(VM_EXIT_INSTRUCTION_LEN);
	UINT64 Rip = VmRead(GUEST_RIP);
	extern SPIN_LOCK ConsoleSpinLock;
	ConsoleSpinLock.SpinLock = 0;

	UINT64 Rflags = VmRead(GUEST_RFLAGS);

	switch (ExitReason & 0xff)
	{
		case EXIT_REASON_EXCEPTION_NMI:
			break;
		case EXIT_REASON_EXTERNAL_INTERRUPT:
			//printk("EXT INT.\n");
			//if (Rflags & FLAG_IF)
			//	VmWrite(GUEST_INTERRUPTIBILITY_INFO, 0x800000ff);
			break;
		case EXIT_REASON_TRIPLE_FAULT:
			printk("VM Exit.Triple Fault.\n");
			printk("Guest RIP:%x\n", Rip);
			Status = RETURN_ABORTED;
			break;
		case EXIT_REASON_PENDING_INTERRUPT:
			break;
		case EXIT_REASON_NMI_WINDOW:
			break;
		case EXIT_REASON_TASK_SWITCH:
			break;
		case EXIT_REASON_CPUID:
			VmHandleCpuId(Vm);
			break;
		case EXIT_REASON_HLT:
			break;
		case EXIT_REASON_INVD:
			break;
		case EXIT_REASON_INVLPG:
			break;
		case EXIT_REASON_RDPMC:
			break;
		case EXIT_REASON_RDTSC:
			break;
		case EXIT_REASON_VMCALL:
			break;
		case EXIT_REASON_VMCLEAR:
			break;
		case EXIT_REASON_VMLAUNCH:
			break;
		case EXIT_REASON_VMPTRLD:
			break;
		case EXIT_REASON_VMPTRST:
			break;
		case EXIT_REASON_VMREAD:
			break;
		case EXIT_REASON_VMRESUME:
			break;
		case EXIT_REASON_VMWRITE:
			break;
		case EXIT_REASON_VMOFF:
			break;
		case EXIT_REASON_VMON:
			break;
		case EXIT_REASON_CR_ACCESS:
			printk("Exit:Read CR%d\n", ExitQualification & 15);
			break;
		case EXIT_REASON_DR_ACCESS:
			break;
		case EXIT_REASON_IO_INSTRUCTION:
			//printk("VM Exit:IO\n");
			//if (ExitQualification & BIT3)
			//	printk("IN:Port = %04x\n", ExitQualification >> 16);
			//else
			//	printk("OUT:Port = %04x\n", ExitQualification >> 16);
			break;
		case EXIT_REASON_MSR_READ:
			break;
		case EXIT_REASON_MSR_WRITE:
			break;
		case EXIT_REASON_INVALID_STATE:
			printk("VM Exit:Invalid Guest State.\n");
			Status = RETURN_ABORTED;
			break;
		case EXIT_REASON_MSR_LOAD_FAIL:
			break;
		case EXIT_REASON_MWAIT_INSTRUCTION:
			break;
		case EXIT_REASON_MONITOR_TRAP_FLAG:
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
			break;
		case EXIT_REASON_EOI_INDUCED:
			break;
		case EXIT_REASON_GDTR_IDTR:
			break;
		case EXIT_REASON_LDTR_TR:
			break;
		case EXIT_REASON_EPT_VIOLATION:
			printk("VM Exit:EPT Violation.\n");
			printk("Guest Physical Address = %x\n", VmRead(GUEST_PHYSICAL_ADDRESS));
			printk("ExitQualification = %x\n", ExitQualification);
			break;
		case EXIT_REASON_EPT_MISCONFIG:
			break;
		case EXIT_REASON_INVEPT:
			break;
		case EXIT_REASON_RDTSCP:
			break;
		case EXIT_REASON_PREEMPTION_TIMER:
			break;
		case EXIT_REASON_INVVPID:
			break;
		case EXIT_REASON_WBINVD:
			break;
		case EXIT_REASON_XSETBV:
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
			break;
		case EXIT_REASON_XRSTORS:
			break;
	}

	if (ExitReason != EXIT_REASON_CPUID)
		printk("VM Exit.Reason:%d\n", ExitReason & 0xff);

	if (ExitReason != EXIT_REASON_EXTERNAL_INTERRUPT && ExitReason != EXIT_REASON_EPT_VIOLATION)
		VmWrite(GUEST_RIP, Rip + ExitInstructionLen);

	return Status;
}


VOID ArchReadCpuId(UINT8 * Buffer);
VOID GuestTestCode()
{
	extern CONSOLE SysCon;
	SysCon.CursorX = SysCon.Width / 2;
	SysCon.CursorY = 0;

	SetColor(&SysCon,0xFF0f00ff);

	
	//asm("hlt");
	//asm("movl %eax, %cr0");
	
	UINT8 Buffer[64] = {0};
	
	//asm("movq %rax, %cr3");

	//UINT32 Counter = 0;
	//while(Counter++ < 20)
	//	printk("=================%d=================\n", Counter);

	//printk("================GUEST EXITING...===============\n");

	void VmPerTest();
	asm("cli");
	printk("Guest:\n");
	asm("cli");
	//VmPerTest();

	
	printk("================GUEST RUNNING!!!===============\n");
	
	ArchReadCpuId(&Buffer[0]);
		
	printk("Injected CPUID: %s\n", Buffer);

	asm("movq %cr0, %rax");
	
	asm("movq %cr2, %rax");

	asm("movq %cr3, %rax");
	
	asm("movq %cr4, %rax");	

	asm("movq %cr8, %rax");	
	
	while(1);
}
