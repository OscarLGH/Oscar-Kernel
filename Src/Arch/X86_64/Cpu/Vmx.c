#include "Vmx.h"
#include "Vm.h"
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

VOID HostExec();

#include "LongMode.h"
extern GDTR GDT;
extern IDTR IDT;
extern TSS64 TSS[64];


VOID EptIdenticalMapping4G(VMEM *Vmem)
{
	UINT64 *PML4T = Vmem->ShadowPageTable;
	UINT64 *PDPT = (UINT64 *)((UINT64)Vmem->ShadowPageTable + 0x1000);
	PML4T[0] = (VIRT2PHYS(Vmem->ShadowPageTable) + 0x1000) | \
			(EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE | EPT_PML4E_ACCESS_FLAG);
	PDPT[0] = 0x00000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_CACHE_WB | EPT_PDPTE_1GB_PAGE);
	PDPT[1] = 0x40000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_CACHE_WB | EPT_PDPTE_1GB_PAGE);
	PDPT[2] = 0x80000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_CACHE_WB | EPT_PDPTE_1GB_PAGE);
	PDPT[3] = 0xc0000000 | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_UNCACHEABLE | EPT_PDPTE_1GB_PAGE);
}

#include "UnrestrictedGuest/UnrestrictedGuest.h"


#define PML4T_OFFSET 0
#define PDPT_OFFSET  (PML4T_OFFSET + 0x1000)
#define PDT_OFFSET   (PDPT_OFFSET  + 0x1000 * 16)
#define PT_OFFSET    (PDT_OFFSET   + 0x10000)


RETURN_STATUS VmxEptInit(VMEM *Vmem)
{
	Vmem->ShadowPageTable = KMalloc(0x200000);
	memset(Vmem->ShadowPageTable, 0, 0x200000);

	UINT64 *PML4T = Vmem->ShadowPageTable;
	UINT64 *PDPT = (UINT64 *)((UINT64)Vmem->ShadowPageTable + PDPT_OFFSET);

	PML4T[0] = (VIRT2PHYS(Vmem->ShadowPageTable) + PDPT_OFFSET) | \
			(EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE | EPT_PML4E_ACCESS_FLAG);

	UINT64 Index;
	for (Index = 0; Index < 64; Index++)
		PDPT[Index] = (VIRT2PHYS(Vmem->ShadowPageTable) + PDT_OFFSET + Index * 0x1000) | \
			(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_ACCESS_FLAG);

	UINT64 *PDT3_4G = (UINT64 *)((UINT64)Vmem->ShadowPageTable + PDT_OFFSET + 3 * 0x1000);
	UINT64 *PT_APIC = (UINT64 *)((UINT64)Vmem->ShadowPageTable + 0x200000 - 0x1000);

	/*APIC*/
	/*注意：APIC的物理地址EPT表项必须使用4K页表项，否则不会触发APIC虚拟化。见SDM 29.4.5. */
	PDT3_4G[0x1f7] = (VIRT2PHYS(Vmem->ShadowPageTable) + 0x200000 - 0x1000) |\
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE);

	
	PT_APIC[0] = 0xfee00000 |\
		(EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_UNCACHEABLE);

	
	return RETURN_SUCCESS;
}

RETURN_STATUS VmxEptMap(VMEM *Vmem, UINT64 Gpa, UINT64 Hpa, UINT64 Len)
{
	UINT64 Index1, Index2, Index3, Index4, Offset1G, Offset2M, Offset4K;
	
	UINT64 *PML4T = Vmem->ShadowPageTable;
	UINT64 *PDPT = (UINT64 *)((UINT64)Vmem->ShadowPageTable + PDPT_OFFSET);
	UINT64 *PDT = (UINT64 *)((UINT64)Vmem->ShadowPageTable + PDT_OFFSET);

	Index1 = (Gpa >> 39);
	Index2 = (Gpa >> 30) & 0x1FF;
	Index3 = (Gpa >> 21) & 0x1FF;
	Index4 = (Gpa >> 12) & 0x1FF;
	Offset1G = Gpa & ((1 << 30) - 1);
	Offset2M = Gpa & ((1 << 21) - 1);
	Offset4K = Gpa & ((1 << 12) - 1);
	
	Index3 = Gpa / 0x200000;	

	PDT[Index3] = Hpa |\
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_CACHE_WB | EPT_PDE_2MB_PAGE);
	
}

VOID EptRealModeMapping(VMEM *Vmem)
{
	UINT64 Index;
	VOID * GuestMemory = KMalloc(0x1000000);
	VmemAddMemMap(Vmem, 0, (UINT64)GuestMemory, 0x1000000);
	
	memset(GuestMemory, 0, 0x1000000);

	memcpy(((UINT8 *)GuestMemory + 0x200000 - 16), ResetVector, 16);
	memcpy(((UINT8 *)GuestMemory + 0x7c00), GuestStartup16, (UINT64)GuestCodeEnd - (UINT64)GuestStartup16);
	memcpy(((UINT8 *)GuestMemory + 0x200000), (VOID *)PHYS2VIRT(0x200000), 0x100000);
	
	UINT64 *PML4T = Vmem->ShadowPageTable;
	UINT64 *PDPT = (UINT64 *)((UINT64)Vmem->ShadowPageTable + 0x1000);
	PML4T[0] = (VIRT2PHYS(Vmem->ShadowPageTable) + 0x1000) | \
			(EPT_PML4E_READ | EPT_PML4E_WRITE | EPT_PML4E_EXECUTE | EPT_PML4E_ACCESS_FLAG);

	PDPT[0] = (VIRT2PHYS(Vmem->ShadowPageTable) + 0x2000) | \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_ACCESS_FLAG);
	
	PDPT[3] = (VIRT2PHYS(Vmem->ShadowPageTable) + 0x3000) |  \
		(EPT_PDPTE_READ | EPT_PDPTE_WRITE | EPT_PDPTE_EXECUTE | EPT_PDPTE_ACCESS_FLAG);

	UINT64 *PDT0_1G = (UINT64 *)((UINT64)Vmem->ShadowPageTable + 0x2000);
	UINT64 *PDT3_4G = (UINT64 *)((UINT64)Vmem->ShadowPageTable + 0x3000);

	UINT64 *PT_APIC = (UINT64 *)((UINT64)Vmem->ShadowPageTable + 0x4000);

	PDT0_1G[0] = VIRT2PHYS(GuestMemory) | \
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_CACHE_WB | EPT_PDE_2MB_PAGE);

	for (Index = 0; Index < 16 / 2; Index++)
		PDT0_1G[Index] = VIRT2PHYS(GuestMemory) + 0x200000 * Index | \
			(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_CACHE_WB | EPT_PDE_2MB_PAGE);

	/*APIC*/
	/*注意：APIC的物理地址EPT表项必须使用4K页表项，否则不会触发APIC虚拟化。见SDM 29.4.5. */
	PDT3_4G[0x1f7] = (VIRT2PHYS(Vmem->ShadowPageTable) + 0x4000) |\
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE);

	
	PT_APIC[0] = 0xfee00000 |\
		(EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_UNCACHEABLE);
	
	/*Reset Vector*/
	PDT3_4G[511] = VIRT2PHYS(GuestMemory) | \
		(EPT_PDE_READ | EPT_PDE_WRITE | EPT_PDE_EXECUTE | EPT_PDE_CACHE_WB | EPT_PDE_2MB_PAGE);

	printk("Guest Memory:%x\n", GuestMemory);
}

X86_VMX_VCPU *VcpuNew()
{
	X86_VMX_VCPU *VcpuPtr = KMalloc(sizeof(*VcpuPtr));
	VcpuPtr->VmxOnRegionVirtAddr = KMalloc(0x1000);
	memset(VcpuPtr->VmxOnRegionVirtAddr, 0, 0x1000);
	VcpuPtr->VmxOnRegionPhysAddr = VIRT2PHYS(VcpuPtr->VmxOnRegionVirtAddr);
	
	VcpuPtr->VmxRegionVirtAddr = KMalloc(0x1000);
	memset(VcpuPtr->VmxRegionVirtAddr, 0, 0x1000);
	VcpuPtr->VmxRegionPhysAddr = VIRT2PHYS(VcpuPtr->VmxRegionVirtAddr);
	
	VcpuPtr->Vmcs.apic_access_addr = 0xfee00000;
	
	//VcpuPtr->VmcsVirt.ept_pointer = KMalloc(0x200000);
	//VcpuPtr->Vmcs.ept_pointer = VIRT2PHYS(VcpuPtr->VmcsVirt.ept_pointer) | 0x5e;
	//printk("VcpuPtr->Vmcs.ept_pointer = %016x\n", VcpuPtr->Vmcs.ept_pointer);
	
	VcpuPtr->VmcsVirt.io_bitmap_a = KMalloc(0x1000);
	VcpuPtr->Vmcs.io_bitmap_a = VIRT2PHYS(VcpuPtr->VmcsVirt.io_bitmap_a);
	memset(VcpuPtr->VmcsVirt.io_bitmap_a, 0xff, 0x1000);
	
	VcpuPtr->VmcsVirt.io_bitmap_b = KMalloc(0x1000);
	VcpuPtr->Vmcs.io_bitmap_b = VIRT2PHYS(VcpuPtr->VmcsVirt.io_bitmap_b);
	memset(VcpuPtr->VmcsVirt.io_bitmap_b, 0xff, 0x1000);
	
	VcpuPtr->VmcsVirt.msr_bitmap = KMalloc(0x1000);
	VcpuPtr->Vmcs.msr_bitmap = VIRT2PHYS(VcpuPtr->VmcsVirt.msr_bitmap);
	memset(VcpuPtr->VmcsVirt.msr_bitmap, 0x00, 0x1000);
	
	VcpuPtr->VmcsVirt.pml_address = KMalloc(0x1000);
	VcpuPtr->Vmcs.pml_address = VIRT2PHYS(VcpuPtr->VmcsVirt.pml_address);
	
	VcpuPtr->VmcsVirt.posted_intr_desc_addr = KMalloc(sizeof(POSTED_INT_DESC));
	VcpuPtr->Vmcs.posted_intr_desc_addr = VIRT2PHYS(VcpuPtr->VmcsVirt.posted_intr_desc_addr);
	memset(VcpuPtr->VmcsVirt.posted_intr_desc_addr, 0x00, sizeof(POSTED_INT_DESC));
	
	VcpuPtr->VmcsVirt.virtual_apic_page_addr = KMalloc(0x1000);
	VcpuPtr->Vmcs.virtual_apic_page_addr = VIRT2PHYS(VcpuPtr->VmcsVirt.virtual_apic_page_addr);
	memset(VcpuPtr->VmcsVirt.virtual_apic_page_addr, 0x00, 0x1000);
	
	VcpuPtr->VmcsVirt.vm_entry_msr_load_addr = KMalloc(0x1000);
	VcpuPtr->Vmcs.vm_entry_msr_load_addr = VIRT2PHYS(VcpuPtr->VmcsVirt.vm_entry_msr_load_addr);
	
	VcpuPtr->VmcsVirt.vm_exit_msr_load_addr = KMalloc(0x1000);
	VcpuPtr->Vmcs.vm_exit_msr_load_addr = VIRT2PHYS(VcpuPtr->VmcsVirt.vm_exit_msr_load_addr);
	
	VcpuPtr->VmcsVirt.vm_exit_msr_store_addr = VcpuPtr->VmcsVirt.vm_entry_msr_load_addr;
	VcpuPtr->Vmcs.vm_exit_msr_store_addr = VIRT2PHYS(VcpuPtr->VmcsVirt.vm_exit_msr_store_addr);
	
	VcpuPtr->VmcsVirt.xss_exit_bitmap = KMalloc(0x1000);
	VcpuPtr->Vmcs.xss_exit_bitmap = VIRT2PHYS(VcpuPtr->VmcsVirt.xss_exit_bitmap);

	return VcpuPtr;
}

RETURN_STATUS VmxInit(X86_VMX_VCPU *Vcpu)
{
	RETURN_STATUS Status = 0;
	VmxOnPrepare(Vcpu->VmxOnRegionVirtAddr);
	VmxOnPrepare(Vcpu->VmxRegionVirtAddr);

	Status = VmOn(Vcpu->VmxOnRegionPhysAddr);
	if (Status != RETURN_SUCCESS)
		return Status;

	VmClear(Vcpu->VmxRegionPhysAddr);
	VmPtrLoad(Vcpu->VmxRegionPhysAddr);

	VmxCpuBistState(Vcpu);

	VmxSetVmcs(&Vcpu->Vmcs);

	return Status;
}


/* CPU State after BIST.See SDM Vol-3 9.1.2 . */
VOID VmxCpuBistState(X86_VMX_VCPU *Vcpu)
{
	memset(&Vcpu->GuestRegs, 0, sizeof(Vcpu->GuestRegs));
	
	Vcpu->Vmcs.virtual_processor_id = 0x8086;
	Vcpu->Vmcs.posted_intr_nv = 0xff;
	Vcpu->Vmcs.guest_es_selector = 0;
	Vcpu->Vmcs.guest_cs_selector = 0xf000;
	Vcpu->Vmcs.guest_ss_selector = 0;
	Vcpu->Vmcs.guest_ds_selector = 0;
	Vcpu->Vmcs.guest_fs_selector = 0;
	Vcpu->Vmcs.guest_gs_selector = 0;
	Vcpu->Vmcs.guest_ldtr_selector = 0;
	Vcpu->Vmcs.guest_tr_selector = 0;
	Vcpu->Vmcs.guest_intr_status= 0;
	Vcpu->Vmcs.guest_pml_index = 0;
	Vcpu->Vmcs.host_es_selector = SELECTOR_KERNEL_DATA;
	Vcpu->Vmcs.host_cs_selector = SELECTOR_KERNEL_CODE;
	Vcpu->Vmcs.host_ss_selector = SELECTOR_KERNEL_DATA;
	Vcpu->Vmcs.host_ds_selector = SELECTOR_KERNEL_DATA;
	Vcpu->Vmcs.host_fs_selector = SELECTOR_KERNEL_DATA;
	Vcpu->Vmcs.host_gs_selector = SELECTOR_KERNEL_DATA;
	Vcpu->Vmcs.host_tr_selector = SELECTOR_TSS;
	//Vcpu->Vmcs.io_bitmap_a = 0;
	//Vcpu->Vmcs.io_bitmap_b = 0;
	//Vcpu->Vmcs.msr_bitmap = 0;
	//Vcpu->Vmcs.vm_exit_msr_store_addr = 0;
	//Vcpu->Vmcs.vm_exit_msr_load_addr = 0;
	//Vcpu->Vmcs.vm_entry_msr_load_addr = 0;
	//Vcpu->Vmcs.pml_address = 0;
	Vcpu->Vmcs.tsc_offset = 0;
	//Vcpu->Vmcs.virtual_apic_page_addr = 0;
	//Vcpu->Vmcs.apic_access_addr = 0;
	//Vcpu->Vmcs.posted_intr_desc_addr = 0;
	//Vcpu->Vmcs.ept_pointer = 0x100000 | 0x5e;
	Vcpu->Vmcs.eoi_exit_bitmap0 = 0;
	Vcpu->Vmcs.eoi_exit_bitmap1 = 0;
	Vcpu->Vmcs.eoi_exit_bitmap2 = 0;
	Vcpu->Vmcs.eoi_exit_bitmap3 = 0;
	Vcpu->Vmcs.xss_exit_bitmap = 0;
	
	Vcpu->Vmcs.guest_physical_address = 0;
	Vcpu->Vmcs.vmcs_link_pointer = 0xffffffffffffffff;
	Vcpu->Vmcs.guest_ia32_debugctl = 0;
	Vcpu->Vmcs.guest_ia32_pat = 0;
	Vcpu->Vmcs.guest_ia32_efer = 0;
	
	Vcpu->Vmcs.guest_ia32_perf_global_ctrl = 0;
	Vcpu->Vmcs.guest_pdptr0 = 0;
	Vcpu->Vmcs.guest_pdptr1 = 0;
	Vcpu->Vmcs.guest_pdptr2 = 0;
	Vcpu->Vmcs.guest_pdptr3 = 0;
	Vcpu->Vmcs.guest_bndcfgs = 0;
	Vcpu->Vmcs.host_ia32_pat = X64ReadMsr(MSR_IA32_CR_PAT);
	Vcpu->Vmcs.host_ia32_efer = X64ReadMsr(MSR_EFER);
	Vcpu->Vmcs.host_ia32_perf_global_ctrl = 0;
	Vcpu->Vmcs.pin_based_vm_exec_control = PIN_BASED_ALWAYSON_WITHOUT_TRUE_MSR
		| PIN_BASED_EXT_INTR_MASK
		| PIN_BASED_POSTED_INTR;
	Vcpu->Vmcs.cpu_based_vm_exec_control = CPU_BASED_FIXED_ONES
		| CPU_BASED_ACTIVATE_SECONDARY_CONTROLS
		| CPU_BASED_USE_IO_BITMAPS
		| CPU_BASED_USE_MSR_BITMAPS
		| CPU_BASED_TPR_SHADOW
		//| CPU_BASED_MONITOR_TRAP_FLAG
		;
	
	Vcpu->Vmcs.exception_bitmap = 0xffffffff;
	Vcpu->Vmcs.page_fault_error_code_mask = 0;
	Vcpu->Vmcs.page_fault_error_code_match = 0;
	Vcpu->Vmcs.cr3_target_count = 0;
	Vcpu->Vmcs.vm_exit_controls = VM_EXIT_ALWAYSON_WITHOUT_TRUE_MSR
		| VM_EXIT_SAVE_IA32_EFER
		| VM_EXIT_LOAD_IA32_EFER
		| VM_EXIT_ACK_INTR_ON_EXIT
		;
	Vcpu->Vmcs.vm_exit_msr_store_count = 0;
	Vcpu->Vmcs.vm_exit_msr_load_count = 0;
	Vcpu->Vmcs.vm_entry_controls = VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR
		| VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL
		| VM_ENTRY_LOAD_IA32_EFER;
	Vcpu->Vmcs.vm_entry_msr_load_count = 0;
	Vcpu->Vmcs.vm_entry_intr_info_field = 0;
	Vcpu->Vmcs.vm_entry_exception_error_code = 0;
	Vcpu->Vmcs.vm_entry_instruction_len = 0;
	Vcpu->Vmcs.tpr_threshold = 0;
	Vcpu->Vmcs.secondary_vm_exec_control = SECONDARY_EXEC_UNRESTRICTED_GUEST
		| SECONDARY_EXEC_ENABLE_EPT
		| SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES
		| SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY
		| SECONDARY_EXEC_APIC_REGISTER_VIRT
		;
		
	
	Vcpu->Vmcs.vm_exit_reason = 0;
	Vcpu->Vmcs.vm_exit_intr_info = 0;
	Vcpu->Vmcs.vm_exit_intr_error_code = 0;
	Vcpu->Vmcs.idt_vectoring_info_field = 0;
	Vcpu->Vmcs.idt_vectoring_error_code = 0;
	Vcpu->Vmcs.vm_exit_instruction_len = 0;
	Vcpu->Vmcs.vmx_instruction_info = 0;
	
	Vcpu->Vmcs.guest_es_limit = 0xffff;
	Vcpu->Vmcs.guest_cs_limit = 0xffff;
	Vcpu->Vmcs.guest_ss_limit = 0xffff;
	Vcpu->Vmcs.guest_ds_limit = 0xffff;
	Vcpu->Vmcs.guest_fs_limit = 0xffff;
	Vcpu->Vmcs.guest_gs_limit = 0xffff;
	Vcpu->Vmcs.guest_ldtr_limit = 0xffff;
	Vcpu->Vmcs.guest_tr_limit = 0xffff;
	Vcpu->Vmcs.guest_gdtr_limit = 0xffff;
	Vcpu->Vmcs.guest_idtr_limit = 0xffff;
	
	Vcpu->Vmcs.guest_es_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_WRITABLE_DATA_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK;
	Vcpu->Vmcs.guest_cs_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_READABLE_CODE_MASK | VMX_AR_TYPE_CODE_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK;
	Vcpu->Vmcs.guest_ss_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_WRITABLE_DATA_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK;
	Vcpu->Vmcs.guest_ds_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_WRITABLE_DATA_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK;
	Vcpu->Vmcs.guest_fs_ar_bytes =
		VMX_AR_P_MASK | VMX_AR_TYPE_WRITABLE_DATA_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK;
	Vcpu->Vmcs.guest_gs_ar_bytes = 
		VMX_AR_P_MASK | VMX_AR_TYPE_WRITABLE_DATA_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK;
	Vcpu->Vmcs.guest_ldtr_ar_bytes = VMX_AR_UNUSABLE_MASK;
	Vcpu->Vmcs.guest_tr_ar_bytes = VMX_AR_P_MASK | VMX_AR_TYPE_BUSY_16_TSS;

	Vcpu->Vmcs.guest_interruptibility_info = 0;
	Vcpu->Vmcs.guest_activity_state = GUEST_ACTIVITY_ACTIVE;
	Vcpu->Vmcs.guest_sysenter_cs = 0;
	Vcpu->Vmcs.vmx_preemption_timer_value = 0;
	Vcpu->Vmcs.host_ia32_sysenter_cs = 0;
	Vcpu->Vmcs.cr0_guest_host_mask = X64ReadMsr(MSR_IA32_VMX_CR0_FIXED0) & X64ReadMsr(MSR_IA32_VMX_CR0_FIXED1) & 0xfffffffe;
	Vcpu->Vmcs.cr4_guest_host_mask = X64ReadMsr(MSR_IA32_VMX_CR4_FIXED0) & X64ReadMsr(MSR_IA32_VMX_CR4_FIXED1);
	Vcpu->Vmcs.cr0_read_shadow = 0;
	Vcpu->Vmcs.cr4_read_shadow = 0;
	Vcpu->Vmcs.cr3_target_value0 = 0;
	Vcpu->Vmcs.cr3_target_value1 = 0;
	Vcpu->Vmcs.cr3_target_value2 = 0;
	Vcpu->Vmcs.cr3_target_value3 = 0;
	Vcpu->Vmcs.exit_qualification = 0;
	Vcpu->Vmcs.guest_linear_address = 0;
	Vcpu->Vmcs.guest_cr0 = X64ReadMsr(MSR_IA32_VMX_CR0_FIXED0) & X64ReadMsr(MSR_IA32_VMX_CR0_FIXED1) & 0x7ffffffe;
	printk("VMCS.GUEST_CR0:%08x\n", Vcpu->Vmcs.guest_cr0);
	Vcpu->Vmcs.guest_cr3 = 0;
	Vcpu->Vmcs.guest_cr4 = X64ReadMsr(MSR_IA32_VMX_CR4_FIXED0) & X64ReadMsr(MSR_IA32_VMX_CR4_FIXED1);
	printk("VMCS.GUEST_CR4:%08x\n", Vcpu->Vmcs.guest_cr4);
	Vcpu->Vmcs.guest_es_base = 0;
	Vcpu->Vmcs.guest_cs_base = 0xffff0000;
	Vcpu->Vmcs.guest_ss_base = 0;
	Vcpu->Vmcs.guest_ds_base = 0;
	Vcpu->Vmcs.guest_fs_base = 0;
	Vcpu->Vmcs.guest_gs_base = 0;
	Vcpu->Vmcs.guest_ldtr_base = 0;
	Vcpu->Vmcs.guest_gdtr_base = 0;
	Vcpu->Vmcs.guest_idtr_base = 0;
	Vcpu->Vmcs.guest_dr7 = 0;
	Vcpu->Vmcs.guest_rsp = 0;
	Vcpu->Vmcs.guest_rip = 0xfff0;
	Vcpu->Vmcs.guest_rflags = BIT1;
	Vcpu->Vmcs.guest_pending_dbg_exceptions = 0;
	Vcpu->Vmcs.guest_sysenter_esp = 0;
	Vcpu->Vmcs.guest_sysenter_eip = 0;
	Vcpu->Vmcs.host_cr0 = X64ReadCr(0);
	Vcpu->Vmcs.host_cr3 = X64ReadCr(3);
	Vcpu->Vmcs.host_cr4 = X64ReadCr(4);
	Vcpu->Vmcs.host_fs_base = 0;
	Vcpu->Vmcs.host_gs_base = 0;
	Vcpu->Vmcs.host_tr_base = (u64)&TSS[0];
	Vcpu->Vmcs.host_gdtr_base = GDT.Base;
	Vcpu->Vmcs.host_idtr_base = IDT.Base;
	Vcpu->Vmcs.host_ia32_sysenter_esp = 0;
	Vcpu->Vmcs.host_ia32_sysenter_eip = 0;
	Vcpu->Vmcs.host_rsp = 0;					//VmLaunch/VmResume will rewrite this field.
	Vcpu->Vmcs.host_rip = (u64)VmExit;
}


VOID VmxSetVmcs(struct vmcs12 *vmcs_ptr)
{

	UINT64 PinBasedVmExecCtrl = X64ReadMsr(MSR_IA32_VMX_PINBASED_CTLS);
	UINT64 CpuBasedVmExecCtrl = X64ReadMsr(MSR_IA32_VMX_PROCBASED_CTLS);
	UINT64 CpuBasedVmExecCtrl2 = X64ReadMsr(MSR_IA32_VMX_PROCBASED_CTLS2);
	UINT64 VmEntryCtrl = X64ReadMsr(MSR_IA32_VMX_ENTRY_CTLS);
	UINT64 VmExitCtrl = X64ReadMsr(MSR_IA32_VMX_EXIT_CTLS);

	UINT64 PinBasedVmExecAllow1 = PinBasedVmExecCtrl >> 32;
	UINT64 PinBasedVmExecAllow0 = PinBasedVmExecCtrl & 0xffffffff;

	UINT64 CpuBasedVmExecCtrlAllow1 = CpuBasedVmExecCtrl >> 32;
	UINT64 CpuBasedVmExecCtrlAllow0 = CpuBasedVmExecCtrl & 0xffffffff;

	UINT64 CpuBasedVmExecCtrl2Allow1 = CpuBasedVmExecCtrl2 >> 32;
	UINT64 CpuBasedVmExecCtrl2Allow0 = CpuBasedVmExecCtrl2 & 0xffffffff;

	UINT64 VmEntryCtrlAllow1 = VmEntryCtrl >> 32;
	UINT64 VmEntryCtrlAllow0 = VmEntryCtrl & 0xffffffff;

	UINT64 VmExitCtrlAllow1 = VmExitCtrl >> 32;
	UINT64 VmExitCtrlAllow0 = VmExitCtrl & 0xffffffff;

	if (CpuBasedVmExecCtrlAllow1 & SECONDARY_EXEC_ENABLE_VPID)
		VmWrite(VIRTUAL_PROCESSOR_ID, vmcs_ptr->virtual_processor_id);

	if (PinBasedVmExecAllow1 & PIN_BASED_POSTED_INTR)
		VmWrite(POSTED_INTR_NV, vmcs_ptr->posted_intr_nv);
	
	VmWrite(GUEST_ES_SELECTOR, vmcs_ptr->guest_es_selector);
	VmWrite(GUEST_CS_SELECTOR, vmcs_ptr->guest_cs_selector);
	VmWrite(GUEST_SS_SELECTOR, vmcs_ptr->guest_ss_selector);
	VmWrite(GUEST_DS_SELECTOR, vmcs_ptr->guest_ds_selector);
	VmWrite(GUEST_FS_SELECTOR, vmcs_ptr->guest_fs_selector);
	VmWrite(GUEST_GS_SELECTOR, vmcs_ptr->guest_gs_selector);
	VmWrite(GUEST_LDTR_SELECTOR, vmcs_ptr->guest_ldtr_selector);
	VmWrite(GUEST_TR_SELECTOR, vmcs_ptr->guest_tr_selector);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY)
		VmWrite(GUEST_INTR_STATUS, vmcs_ptr->guest_intr_status);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_ENABLE_PML)
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

	if (CpuBasedVmExecCtrlAllow1 & CPU_BASED_USE_MSR_BITMAPS)
		VmWrite(MSR_BITMAP, vmcs_ptr->msr_bitmap);
	
	VmWrite(VM_EXIT_MSR_STORE_ADDR, vmcs_ptr->vm_exit_msr_store_addr);
	VmWrite(VM_EXIT_MSR_LOAD_ADDR, vmcs_ptr->vm_exit_msr_load_addr);
	VmWrite(VM_ENTRY_MSR_LOAD_ADDR, vmcs_ptr->vm_entry_msr_load_addr);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_ENABLE_PML)
		VmWrite(PML_ADDRESS, vmcs_ptr->pml_address);
	
	VmWrite(TSC_OFFSET, vmcs_ptr->tsc_offset);

	if (CpuBasedVmExecCtrlAllow1 & CPU_BASED_TPR_SHADOW)
		VmWrite(VIRTUAL_APIC_PAGE_ADDR, vmcs_ptr->virtual_apic_page_addr);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES)
		VmWrite(APIC_ACCESS_ADDR, vmcs_ptr->apic_access_addr);

	if (PinBasedVmExecAllow1 & PIN_BASED_POSTED_INTR)
		VmWrite(POSTED_INTR_DESC_ADDR, vmcs_ptr->posted_intr_desc_addr);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_ENABLE_EPT)
		VmWrite(EPT_POINTER, vmcs_ptr->ept_pointer);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY) {
		VmWrite(EOI_EXIT_BITMAP0, vmcs_ptr->eoi_exit_bitmap0);
		VmWrite(EOI_EXIT_BITMAP1, vmcs_ptr->eoi_exit_bitmap1);
		VmWrite(EOI_EXIT_BITMAP2, vmcs_ptr->eoi_exit_bitmap2);
		VmWrite(EOI_EXIT_BITMAP3, vmcs_ptr->eoi_exit_bitmap3);
	}

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_SHADOW_VMCS) {
		VmWrite(VMREAD_BITMAP, vmcs_ptr->eoi_exit_bitmap3);
		VmWrite(VMWRITE_BITMAP, vmcs_ptr->eoi_exit_bitmap3);
	}

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_XSAVES)
		VmWrite(XSS_EXIT_BITMAP, vmcs_ptr->xss_exit_bitmap);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_TSC_SCALING)
		VmWrite(TSC_MULTIPLIER, vmcs_ptr->tsc_offset);

	//Read Only
	//if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_ENABLE_EPT)
	//	VmWrite(GUEST_PHYSICAL_ADDRESS, vmcs_ptr->virtual_processor_id);
	
	VmWrite(VMCS_LINK_POINTER, vmcs_ptr->vmcs_link_pointer);
	VmWrite(GUEST_IA32_DEBUGCTL, vmcs_ptr->guest_ia32_debugctl);

	if (VmEntryCtrlAllow1 & VM_ENTRY_LOAD_IA32_PAT)
		VmWrite(GUEST_IA32_PAT, vmcs_ptr->guest_ia32_pat);

	if (VmExitCtrlAllow1 & VM_EXIT_SAVE_IA32_EFER)
		VmWrite(GUEST_IA32_EFER, vmcs_ptr->guest_ia32_efer);

	if (VmEntryCtrlAllow1 & VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL)
		VmWrite(GUEST_IA32_PERF_GLOBAL_CTRL, vmcs_ptr->guest_ia32_perf_global_ctrl);

	if (CpuBasedVmExecCtrl2Allow1 & SECONDARY_EXEC_ENABLE_EPT) {
		VmWrite(GUEST_PDPTR0, vmcs_ptr->guest_pdptr0);
		VmWrite(GUEST_PDPTR1, vmcs_ptr->guest_pdptr1);
		VmWrite(GUEST_PDPTR2, vmcs_ptr->guest_pdptr2);
		VmWrite(GUEST_PDPTR3, vmcs_ptr->guest_pdptr3);
	}

	if (VmEntryCtrlAllow1 & VM_ENTRY_LOAD_BNDCFGS)
		VmWrite(GUEST_BNDCFGS, vmcs_ptr->guest_bndcfgs);

	if (VmExitCtrlAllow1 & VM_EXIT_LOAD_IA32_PAT)
		VmWrite(HOST_IA32_PAT, vmcs_ptr->host_ia32_pat);
	
	if (VmExitCtrlAllow1 & VM_EXIT_LOAD_IA32_EFER)
		VmWrite(HOST_IA32_EFER, vmcs_ptr->host_ia32_efer);
	
	if (VmExitCtrlAllow1 & VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL)
		VmWrite(HOST_IA32_PERF_GLOBAL_CTRL, vmcs_ptr->host_ia32_perf_global_ctrl);

	if ((PinBasedVmExecAllow1 | vmcs_ptr->pin_based_vm_exec_control) != PinBasedVmExecAllow1)
		printk("Warning:setting pin_based_vm_exec_control:%x unsupported.\n", 
			(PinBasedVmExecAllow1 & vmcs_ptr->pin_based_vm_exec_control) ^ vmcs_ptr->pin_based_vm_exec_control);
	VmWrite(PIN_BASED_VM_EXEC_CONTROL, vmcs_ptr->pin_based_vm_exec_control & PinBasedVmExecAllow1);

	if ((CpuBasedVmExecCtrlAllow1 | vmcs_ptr->cpu_based_vm_exec_control) != CpuBasedVmExecCtrlAllow1)
		printk("Warning:setting cpu_based_vm_exec_control:%x unsupported.\n", 
			(CpuBasedVmExecCtrlAllow1 & vmcs_ptr->cpu_based_vm_exec_control) ^ vmcs_ptr->cpu_based_vm_exec_control);
	VmWrite(CPU_BASED_VM_EXEC_CONTROL, vmcs_ptr->cpu_based_vm_exec_control & CpuBasedVmExecCtrlAllow1);
	
	VmWrite(EXCEPTION_BITMAP, vmcs_ptr->exception_bitmap);
	VmWrite(PAGE_FAULT_ERROR_CODE_MASK, vmcs_ptr->page_fault_error_code_mask);
	VmWrite(PAGE_FAULT_ERROR_CODE_MATCH, vmcs_ptr->page_fault_error_code_match);
	VmWrite(CR3_TARGET_COUNT, vmcs_ptr->cr3_target_count);

	if ((VmExitCtrlAllow1 | vmcs_ptr->vm_exit_controls) != VmExitCtrlAllow1)
		printk("Warning:setting vm_exit_controls:%x unsupported.\n", 
			(VmExitCtrlAllow1 & vmcs_ptr->vm_exit_controls) ^ vmcs_ptr->vm_exit_controls);
	
	VmWrite(VM_EXIT_CONTROLS, vmcs_ptr->vm_exit_controls & VmExitCtrlAllow1);
	VmWrite(VM_EXIT_MSR_STORE_COUNT, vmcs_ptr->vm_exit_msr_store_count);
	VmWrite(VM_EXIT_MSR_LOAD_COUNT, vmcs_ptr->vm_exit_msr_load_count);

	if ((VmEntryCtrlAllow1 | vmcs_ptr->vm_entry_controls) != VmEntryCtrlAllow1)
		printk("Warning:setting vm_entry_controls:%x unsupported.\n", (VmEntryCtrlAllow1 & vmcs_ptr->vm_entry_controls) ^ vmcs_ptr->vm_entry_controls);
	VmWrite(VM_ENTRY_CONTROLS, vmcs_ptr->vm_entry_controls & VmEntryCtrlAllow1);
	VmWrite(VM_ENTRY_MSR_LOAD_COUNT, vmcs_ptr->vm_entry_msr_load_count);
	VmWrite(VM_ENTRY_INTR_INFO_FIELD, vmcs_ptr->vm_entry_intr_info_field);
	//VmWrite(VM_ENTRY_EXCEPTION_ERROR_CODE, vmcs_ptr->virtual_processor_id);
	VmWrite(VM_ENTRY_INSTRUCTION_LEN, vmcs_ptr->vm_entry_instruction_len);

	if (CpuBasedVmExecCtrlAllow1 & CPU_BASED_TPR_SHADOW)
		VmWrite(TPR_THRESHOLD, vmcs_ptr->tpr_threshold);

	if (CpuBasedVmExecCtrlAllow1 & CPU_BASED_ACTIVATE_SECONDARY_CONTROLS) {
		if ((CpuBasedVmExecCtrl2Allow1 | vmcs_ptr->secondary_vm_exec_control) != CpuBasedVmExecCtrl2Allow1)
			printk("Warning:setting secondary_vm_exec_control:%x unsupported.\n", (CpuBasedVmExecCtrl2Allow1 & vmcs_ptr->secondary_vm_exec_control) ^ vmcs_ptr->secondary_vm_exec_control);
		VmWrite(SECONDARY_VM_EXEC_CONTROL, vmcs_ptr->secondary_vm_exec_control & CpuBasedVmExecCtrl2Allow1);
	}

	if (CpuBasedVmExecCtrlAllow1 & CPU_BASED_PAUSE_EXITING) {
		VmWrite(PLE_GAP, 0);
		VmWrite(PLE_WINDOW, 0);
	}
	
	
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

	if (PinBasedVmExecAllow1 & PIN_BASED_VMX_PREEMPTION_TIMER)
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
	//VmWrite(HOST_TR_BASE, vmcs_ptr->host_tr_base);
	VmWrite(HOST_GDTR_BASE, vmcs_ptr->host_gdtr_base);
	VmWrite(HOST_IDTR_BASE, vmcs_ptr->host_idtr_base);
	VmWrite(HOST_IA32_SYSENTER_ESP, vmcs_ptr->host_ia32_sysenter_esp);
	VmWrite(HOST_IA32_SYSENTER_EIP, vmcs_ptr->host_ia32_sysenter_eip);
	//VmWrite(HOST_RSP, vmcs_ptr->host_rsp);
	VmWrite(HOST_RIP, vmcs_ptr->host_rip);

	printk("VM Instruction Error:%d\n", VmRead(VM_INSTRUCTION_ERROR));
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


RETURN_STATUS HandleCpuId(X86_VMX_VCPU *Vcpu)
{
	UINT8 Buffer[50];
	//ArchReadCpuId(Buffer);
	//printk("Host CPU String:%s\n", Buffer);
	strcpy(Buffer, "Intel(R) Core(TM) i7-5960X CPU @ 8.00GHz");
	if (Vcpu->GuestRegs.RAX == 0x80000002 && Vcpu->GuestRegs.RCX == 0)
	{
		Vcpu->GuestRegs.RAX = *(UINT32 *)&Buffer[0];
		Vcpu->GuestRegs.RBX = *(UINT32 *)&Buffer[4];
		Vcpu->GuestRegs.RCX = *(UINT32 *)&Buffer[8];
		Vcpu->GuestRegs.RDX = *(UINT32 *)&Buffer[12];
	}

	if (Vcpu->GuestRegs.RAX == 0x80000003 && Vcpu->GuestRegs.RCX == 0)
	{
		Vcpu->GuestRegs.RAX = *(UINT32 *)&Buffer[16];
		Vcpu->GuestRegs.RBX = *(UINT32 *)&Buffer[20];
		Vcpu->GuestRegs.RCX = *(UINT32 *)&Buffer[24];
		Vcpu->GuestRegs.RDX = *(UINT32 *)&Buffer[28];
	}

	if (Vcpu->GuestRegs.RAX == 0x80000004 && Vcpu->GuestRegs.RCX == 0)
	{
		Vcpu->GuestRegs.RAX = *(UINT32 *)&Buffer[32];
		Vcpu->GuestRegs.RBX = *(UINT32 *)&Buffer[36];
		Vcpu->GuestRegs.RCX = *(UINT32 *)&Buffer[40];
		Vcpu->GuestRegs.RDX = *(UINT32 *)&Buffer[44];
	}

	if (Vcpu->GuestRegs.RAX == 0x80000001 && Vcpu->GuestRegs.RCX == 0)
	{
		Vcpu->GuestRegs.RAX = 0;
		Vcpu->GuestRegs.RBX = 0;
		Vcpu->GuestRegs.RCX = 0;
		Vcpu->GuestRegs.RDX = 0;
	}

	Vcpu->Vmcs.guest_rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
	return RETURN_SUCCESS;
}

extern VIRTUAL_MACHINE *VmG;
RETURN_STATUS HandleException(X86_VMX_VCPU *Vcpu)
{
	UINT32 ExitQualification = VmRead(EXIT_QUALIFICATION);
	printk("Instruction Error = %x\n", VmRead(VM_INSTRUCTION_ERROR));
	printk("Guest RIP:%x\n", VmRead(GUEST_RIP));
	printk("Exception Vector = %x\n", VmRead(VM_EXIT_INTR_INFO));
	printk("Guest Physical Address = %x\n", VmRead(GUEST_PHYSICAL_ADDRESS));
	printk("ExitQualification = %x\n", ExitQualification);
	printk("VM Exit interruption error code:%x\n", VmRead(VM_EXIT_INTR_ERROR_CODE));
	printk("Guest CR0:%x\n", VmRead(GUEST_CR0));
	printk("Guest linear address:%x\n", VmRead(GUEST_LINEAR_ADDRESS));
	printk("Guest CR3:%x\n", VmRead(GUEST_CR3));
	printk("Guest CR4:%x\n", VmRead(GUEST_CR4));

	while(1);
	return RETURN_SUCCESS;
}
RETURN_STATUS VmxEventInject(VCPU *Vcpu, UINT64 Type, UINT64 Vector);
VOID LocalApicSendIpiSelf(UINT64 Vector);

UINT64 Counter_Inj = 0;
RETURN_STATUS HandleExternInterrupt(X86_VMX_VCPU *Vcpu)
{
	VCPU *VcpuPtr = ContainerOf(Vcpu, VCPU, ArchSpecificData);
	//printk("CPU:%d\n", ArchGetCpuIdentifier());
	volatile UINT32 * LocalApciEoi = (UINT32 *)PHYS2VIRT(0xfee000b0);
	UINT64 VmExitIntrInfo = VmRead(VM_EXIT_INTR_INFO);
	//printk("Vm Exit.External Interrupt,Vector = %d.\n", VmExitIntrInfo & 0xff);
	EnableLocalIrq();
	
	if ((VmExitIntrInfo & 0x80000000) != 0) {
		/* Resend a vector to HOST CPU.*/
		//CallIrqDest(VmExitIntrInfo & 0xff, 1); //Calling this causes frequent Vm-Exit. ???
		/* Inject interrupt to GUEST CPU.*/
		//printk("Injecting vector %d\n", VmExitIntrInfo & 0xff);
		//IrqHandler(VmExitIntrInfo >> 8);

		//*LocalApciEoi = 0;
		/* EOI handling is already included in HOST ISR.*/
		/* Use software interrupt to call host IRQ.*/
		X64IntN(VmExitIntrInfo & 0xff);

		/* Following code cannot work. Calling this causes frequent Vm-Exit,
		* SELF-IPI maybe delays somewhile, CPU get interrupted and
		* vm-exit after entering the guest.
		* Nested interrupts occur here.
		*/
		
		//LocalApicSendIpiSelf(VmExitIntrInfo & 0xff);
		
		VmxEventInject(NULL, (VmExitIntrInfo >> 8) & 0x7, VmExitIntrInfo & 0xff);
		
		/* Conner case: If guest IF is not enabled,guest never do IRQ handling.
		*  VMM must send an EOI to local APIC.
		*  Otherwise following interrupts are blocked.
		*/
		
	}

	return RETURN_SUCCESS;
}

RETURN_STATUS HandleIo(X86_VMX_VCPU *Vcpu)
{
	UINT32 ExitQualification = VmRead(EXIT_QUALIFICATION);
	UINT16 PortNum = ExitQualification >> 16;
	UINT8 Direction = ExitQualification & BIT3;
	UINT8 IsString = ExitQualification & BIT4;
	UINT8 IsRep = ExitQualification & BIT5;
	UINT8 IsImme = ExitQualification & BIT6;
	UINT8 String[2] = {0};
	
	//printk("VM Exit:IO\n");
	//if (Direction)
	//	printk("IN:Port = %04x\n", PortNum);
	//else
	//	printk("OUT:Port = %04x\n", PortNum);
	//Guest serial port print
	String[0] = Vcpu->GuestRegs.RAX;
	if (PortNum == 0x3f8)
		printk(String);

	if (!IsRep) {
		Vcpu->Vmcs.guest_rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
	}
	else {
		Vcpu->GuestRegs.RCX--;
		if (Vcpu->GuestRegs.RCX == 0)
			Vcpu->Vmcs.guest_rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
	}

	return RETURN_SUCCESS;
}

RETURN_STATUS HandleMonitorTrap(X86_VMX_VCPU *Vcpu)
{
	//printk("VM Exit:Monitor Trap Flag.\n");
	UINT64 GuestRip = VmRead(GUEST_RIP);
	UINT64 InstLen = VmRead(VM_EXIT_INSTRUCTION_LEN);
	UINT8 Binary[32];
	VmemGuestMemoryRead(VmG->VmNodeArray[0]->Vmem, GuestRip - InstLen, Binary, InstLen);
	
	printk("RIP:%016x Instruction Code:",GuestRip - InstLen);
	UINT64 Index;
	for (Index = 0; Index < InstLen; Index++)
		printk("%02x ", Binary[Index]);
	printk("\n");
	
	printk("RAX:%016x RBX:%016x RCX:%016x RDX:%016x RSI:%016x RDI:%016x RSP:%016x RBP:%016x\n",
		Vcpu->GuestRegs.RAX,
		Vcpu->GuestRegs.RBX,
		Vcpu->GuestRegs.RCX,
		Vcpu->GuestRegs.RDX,
		Vcpu->GuestRegs.RSI,
		Vcpu->GuestRegs.RDI,
		VmRead(GUEST_RSP),
		Vcpu->GuestRegs.RBP);
	printk("R08:%016x R09:%016x R10:%016x R11:%016x R12:%016x R13:%016x R14:%016x R15:%016x\n",
		Vcpu->GuestRegs.R8,
		Vcpu->GuestRegs.R9,
		Vcpu->GuestRegs.R10,
		Vcpu->GuestRegs.R11,
		Vcpu->GuestRegs.R12,
		Vcpu->GuestRegs.R13,
		Vcpu->GuestRegs.R14,
		Vcpu->GuestRegs.R15);

	printk("RFLAGS:%016x\n",
		VmRead(GUEST_RFLAGS));
	/*
	printk("CR0:%016x CR2:%016x CR3:%016x CR4:%016x CR8:%016x\n",
		VmRead(GUEST_CR0),
		VmRead(GUEST_LINEAR_ADDRESS),
		VmRead(GUEST_CR3),
		VmRead(GUEST_CR4),
		0
		);
	*/
	return RETURN_SUCCESS;
}

RETURN_STATUS HandleVmcall(X86_VMX_VCPU *Vcpu)
{
	UINT64 Rax = Vcpu->GuestRegs.RAX;
	UINT64 CpuBasedCtrl = VmRead(CPU_BASED_VM_EXEC_CONTROL);
	printk("VM Exit:VMCALL.Function Num = %d\n", Rax);

	switch (Rax) {
		case 0x101:
			CpuBasedCtrl |= CPU_BASED_MONITOR_TRAP_FLAG;
			VmWrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedCtrl);
			printk("VMCALL:MTF enabled.\n");
			break;
		case 0x102:
			CpuBasedCtrl &= (~CPU_BASED_MONITOR_TRAP_FLAG);
			VmWrite(CPU_BASED_VM_EXEC_CONTROL, CpuBasedCtrl);
			printk("VMCALL:MTF disabled.\n");
			break;
		default:
			break;
	}
	Vcpu->Vmcs.guest_rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
	return RETURN_SUCCESS;
}


RETURN_STATUS EnterLongMode(X86_VMX_VCPU *Vcpu)
{
	UINT32 Efer = VmRead(GUEST_IA32_EFER);
	UINT32 OriCr0 = VmRead(GUEST_CR0);
	UINT32 OriCr4 = VmRead(GUEST_CR4);
	VmWrite(GUEST_CR0, OriCr0 | 0x80000001);
	VmWrite(GUEST_CR4, OriCr4 | 0x00000200);
	VmWrite(GUEST_TR_AR_BYTES, VMX_AR_P_MASK | VMX_AR_TYPE_BUSY_64_TSS);
	/*VmWrite(GUEST_CS_AR_BYTES, VMX_AR_P_MASK | VMX_AR_TYPE_READABLE_CODE_MASK | VMX_AR_TYPE_CODE_MASK | VMX_AR_TYPE_ACCESSES_MASK | VMX_AR_S_MASK | BIT13);
	VmWrite(GUEST_DS_AR_BYTES, VMX_AR_UNUSABLE_MASK);
	VmWrite(GUEST_ES_AR_BYTES, VMX_AR_UNUSABLE_MASK);
	VmWrite(GUEST_FS_AR_BYTES, VMX_AR_UNUSABLE_MASK);
	VmWrite(GUEST_GS_AR_BYTES, VMX_AR_UNUSABLE_MASK);
	VmWrite(GUEST_SS_AR_BYTES, VMX_AR_UNUSABLE_MASK);
	VmWrite(GUEST_LDTR_AR_BYTES, VMX_AR_UNUSABLE_MASK);*/
	VmWrite(VM_ENTRY_CONTROLS, 
		VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR | 
		VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL | 
		VM_ENTRY_LOAD_IA32_EFER |
		VM_ENTRY_IA32E_MODE);
	VmWrite(GUEST_IA32_EFER, Efer | 0x500);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS HandleCrAccess(X86_VMX_VCPU *Vcpu)
{
	UINT32 ExitQualification = VmRead(EXIT_QUALIFICATION);
	UINT8 AccessType = (ExitQualification >> 4) & 0x3;
	UINT16 CrNum = ExitQualification & 0xf;
	UINT8 Reg = (ExitQualification >> 8) & 0xf;
	printk("Exit:%s CR%d\n",AccessType == 0 ? "Write":"Read" ,CrNum);
	EnterLongMode(Vcpu);
	Vcpu->Vmcs.guest_rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
	return RETURN_SUCCESS;
}

RETURN_STATUS HandleEptViolation(X86_VMX_VCPU *Vcpu)
{
	UINT32 ExitQualification = VmRead(EXIT_QUALIFICATION);
	UINT64 GuestPhysicalAddress = VmRead(GUEST_PHYSICAL_ADDRESS);
	printk("VM Exit:EPT Violation.\n");
	printk("Guest Physical Address = %x\n", GuestPhysicalAddress);
	printk("ExitQualification = %x\n", ExitQualification);
	return RETURN_SUCCESS;
}

RETURN_STATUS HandleEptMisconfig(X86_VMX_VCPU *Vcpu)
{
	UINT32 ExitQualification = VmRead(EXIT_QUALIFICATION);
	UINT64 GuestPhysicalAddress = VmRead(GUEST_PHYSICAL_ADDRESS);
	printk("VM Exit:EPT Misconfiguration.\n");
	printk("Guest Physical Address = %x\n", GuestPhysicalAddress);
	printk("ExitQualification = %x\n", ExitQualification);
	return RETURN_SUCCESS;
}


RETURN_STATUS HandleApicAccess(X86_VMX_VCPU *Vcpu)
{
	UINT32 ExitQualification = VmRead(EXIT_QUALIFICATION);
	UINT64 Offset = ExitQualification & (0xfff);
	printk("VM Exit:APIC Access.Offset = %x\n", Offset);
	UINT8 Type = (ExitQualification >> 12);
	UINT32 * LocalEoi = (UINT32 *)PHYS2VIRT(0xfee000b0);
	//if (Offset == 0xb0)
	//	*LocalEoi = 0;

	UINT64 ExitType = 0;

	/* Trap like.*/
	if (ExitType == 0)
		Vcpu->Vmcs.guest_rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS HandleApicWrite(X86_VMX_VCPU *Vcpu)
{
	printk("VM Exit:APIC Write.\n");
	return RETURN_SUCCESS;
}


RETURN_STATUS VmxVcpuExitHandler(VCPU *Vcpu)
{
	X86_VMX_VCPU *VcpuPtr = Vcpu->ArchSpecificData;
	RETURN_STATUS Status = 0;
	UINT64 ExitReason = VcpuPtr->Vmcs.vm_exit_reason = VmRead(VM_EXIT_REASON);
	VcpuPtr->Vmcs.exit_qualification = VmRead(EXIT_QUALIFICATION);
	VcpuPtr->Vmcs.guest_rip = VmRead(GUEST_RIP);
	
	UINT64 Rflags = VmRead(GUEST_RFLAGS);
	//printk("Vm Exit.Reason = %d\n", ExitReason);
	switch (ExitReason & 0xff)
	{
		case EXIT_REASON_EXCEPTION_NMI:
			Status = HandleException(VcpuPtr);
			break;
		case EXIT_REASON_EXTERNAL_INTERRUPT:
			Status = HandleExternInterrupt(VcpuPtr);
			break;
		case EXIT_REASON_TRIPLE_FAULT:
			Status = RETURN_ABORTED;
			break;
		case EXIT_REASON_PENDING_INTERRUPT:
			break;
		case EXIT_REASON_NMI_WINDOW:
			break;
		case EXIT_REASON_TASK_SWITCH:
			break;
		case EXIT_REASON_CPUID:
			Status = HandleCpuId(VcpuPtr);
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
			Status = HandleVmcall(VcpuPtr);
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
			Status = HandleCrAccess(VcpuPtr);
			break;
		case EXIT_REASON_DR_ACCESS:
			break;
		case EXIT_REASON_IO_INSTRUCTION:
			Status = HandleIo(VcpuPtr);
			break;
		case EXIT_REASON_MSR_READ:
			printk("VM Exit:MSR READ.\n");
			break;
		case EXIT_REASON_MSR_WRITE:
			printk("VM Exit:MSR WRITE.\n");
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
			Status = HandleMonitorTrap(VcpuPtr);
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
			Status = HandleApicAccess(VcpuPtr);
			break;
		case EXIT_REASON_EOI_INDUCED:
			break;
		case EXIT_REASON_GDTR_IDTR:
			break;
		case EXIT_REASON_LDTR_TR:
			break;
		case EXIT_REASON_EPT_VIOLATION:
			Status = HandleEptViolation(VcpuPtr);
			break;
		case EXIT_REASON_EPT_MISCONFIG:
			Status = HandleEptMisconfig(VcpuPtr);
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
			Status = HandleApicWrite(VcpuPtr);
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

	VmWrite(GUEST_RIP, VcpuPtr->Vmcs.guest_rip);

	return Status;
}


RETURN_STATUS VmxVcpuInit(VCPU *Vcpu)
{
	Vcpu->ArchSpecificData = VcpuNew();
	return RETURN_SUCCESS;
}

RETURN_STATUS VmxVcpuPinEpt(VCPU *Vcpu, UINT64 EptPhys)
{
	X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;
	Ptr->Vmcs.ept_pointer = EptPhys | 0x5e;
	return RETURN_SUCCESS;
}

RETURN_STATUS VmxVcpuPreapare(VCPU *Vcpu)
{
	RETURN_STATUS Status;

	Status = VmxEnable();
	if (Status)
		return Status;

	X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;

	Status = VmxInit(Ptr);

	Vcpu->VmStatus = VM_STATUS_CLEAR;

	return Status;
}

RETURN_STATUS VmxVcpuRun(VCPU *Vcpu)
{
	RETURN_STATUS Status = RETURN_SUCCESS;
	X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;
	//printk("VMX VCPU RUN.\n");
	if (Vcpu->VmStatus == VM_STATUS_CLEAR)
	{
		Vcpu->VmStatus = VM_STATUS_LAUNCHED;
		Status = VmLaunch(&Ptr->HostRegs, &Ptr->GuestRegs);
		//printk("Status = %x\n", Status);
	}
	else
	{
		Status = VmResume(&Ptr->HostRegs, &Ptr->GuestRegs);
	}

	if (Status)
		printk("VMX failed.\n");
	return Status;
}


RETURN_STATUS VmxVcpuStop(VCPU *Vcpu)
{
	RETURN_STATUS Status;

	X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;
	
	VmClear(Ptr->VmxRegionPhysAddr);

	Vcpu->VmStatus = VM_STATUS_CLEAR;
	
	VmxOff();
	
	return Status;
}

RETURN_STATUS VmxEventInject(VCPU *Vcpu, UINT64 Type, UINT64 Vector)
{
	UINT64 Rflags = VmRead(GUEST_RFLAGS);
	if ((Rflags & FLAG_IF) == 0) {
		return RETURN_ABORTED;
	}
	//X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;
	//printk("Injecting vector %d\n", Vector);
	
	UINT64 Reg = 0x80000000 | (Vector & 0xff) | ((Type & 0x7) << 8);
	VmWrite(VM_ENTRY_INTR_INFO_FIELD, Reg);
	
	return RETURN_SUCCESS;
}


RETURN_STATUS VmxPostedInterruptVector(VCPU *Vcpu, UINT64 Vector)
{
	X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;
	Ptr->Vmcs.posted_intr_nv = Vector & 0xff;
	return RETURN_SUCCESS;
}

RETURN_STATUS VmxPostedInterruptDescSet(VCPU *Vcpu, UINT64 Vector)
{
	X86_VMX_VCPU *Ptr = Vcpu->ArchSpecificData;
	POSTED_INT_DESC *Desc = Ptr->VmcsVirt.posted_intr_desc_addr;

	Desc->OutStanding = 1;
	Desc->Vector[Vector / 8] = (1 << (Vector % 8));
	return RETURN_SUCCESS;
}


VCPU *VmxVcpuCreate(VIRTUAL_MACHINE *Vm)
{
	VCPU *Ptr = KMalloc(sizeof(*Ptr));
	Ptr->ArchSpecificData = KMalloc(sizeof(X86_VMX_VCPU));

	Ptr->VcpuInit = VmxVcpuInit;
	Ptr->VcpuPinShadowPage = VmxVcpuPinEpt;
	Ptr->VcpuPrepare = VmxVcpuPreapare;
	Ptr->VcpuRun = VmxVcpuRun;
	Ptr->ExitHandler = VmxVcpuExitHandler;
	Ptr->VcpuStop = VmxVcpuStop;
	Ptr->EventInject = VmxEventInject;
	Ptr->PostedInterruptVectorSet = VmxPostedInterruptVector;
	Ptr->PostedInterruptSet = VmxPostedInterruptDescSet;

	return Ptr;
}

VMEM *VmxVmemCreate()
{
	VMEM *Ptr = KMalloc(sizeof(*Ptr));
	Ptr->VmemInit = VmxEptInit;
	Ptr->ShadowPageMap = VmxEptMap;

	return Ptr;
}

VM_TEMPLATE VmTemplate = 
{
	.VcpuCreate = VmxVcpuCreate,
	.VmemCreate = VmxVmemCreate,
};



