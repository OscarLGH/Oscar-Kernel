#include <vmx.h>
#include "vmcs12.h"
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


int decode_mem_address(struct vmx_vcpu *vcpu, unsigned long exit_qualification,
			u32 vmx_instruction_info, gva_t *gva)
{
	gva_t off;
	bool exn;
	int  scaling = vmx_instruction_info & 3;
	int  addr_size = (vmx_instruction_info >> 7) & 7;
	bool is_reg = vmx_instruction_info & (1u << 10);
	int  seg_reg = (vmx_instruction_info >> 15) & 7;
	int  index_reg = (vmx_instruction_info >> 18) & 0xf;
	bool index_is_valid = !(vmx_instruction_info & (1u << 22));
	int  base_reg       = (vmx_instruction_info >> 23) & 0xf;
	bool base_is_valid  = !(vmx_instruction_info & (1u << 27));

	off = exit_qualification; /* holds the displacement */
	if (addr_size == 1)
		off = (gva_t)sign_extend64(off, 31);
	else if (addr_size == 0)
		off = (gva_t)sign_extend64(off, 15);
	if (base_is_valid)
		off += kvm_reg_read(vcpu, base_reg);
	if (index_is_valid)
		off += kvm_reg_read(vcpu, index_reg)<<scaling;

	*gva = off;

	return 0;
}
int vmx_handle_vmxon(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	gva_t gva;
	gpa_t gpa;
	hpa_t hpa;
	u64 *virt;
	int ret;

	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva to gpa failed.\n");
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);
	printk("VM-Exit:VMXON.Region:0x%x\n", *virt);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmxoff(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMXOFF.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}


int vmx_handle_vmclear(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	gva_t gva;
	gpa_t gpa;
	hpa_t hpa;
	u64 *virt;
	u64 vmcs12_phys;
	int ret;

	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;
	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva to gpa failed.\n");
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);
	vmcs12_phys = *virt;
	printk("VM-Exit:VMCLEAR.Region:0x%x\n", *virt);
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int nested_vmx_run(struct vmx_vcpu *vcpu);
int vmx_handle_vmlaunch(struct vmx_vcpu *vcpu)
{
	vcpu->state = 0;
	printk("VM-Exit:VMLAUNCH.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	vmclear(vcpu->vmcs01_phys);
	nested_vmx_run(vcpu);
	return 0;
}

int vmx_handle_vmresume(struct vmx_vcpu *vcpu)
{
	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;
	printk("VM-Exit:VMRESUME.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	vmclear(vcpu->vmcs01_phys);
	nested_vmx_run(vcpu);
	return 0;
}

int vmx_handle_vmptrld(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	gva_t gva;
	gpa_t gpa;
	hpa_t hpa;
	u64 *virt;
	int ret;

	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;

	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva to gpa failed.\n");
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);
	vcpu->vmcs12 = (void *)PHYS2VIRT(*virt);

	printk("VM-Exit:VMPTRLD.Region:0x%x\n", *virt);
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmptrst(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	gva_t gva;
	gpa_t gpa;
	hpa_t hpa;
	u64 *virt;
	int ret;

	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;

	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva to gpa failed.\n");
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);
	printk("VM-Exit:VMPTRST.Region:0x%x\n", *virt);

	*virt = VIRT2PHYS(vcpu->vmcs12);

	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmread(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u64 field = kvm_reg_read(vcpu, base_reg);
	int ret;
	u64 value;

	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;
	vmcs12_read_any(vcpu->vmcs12, field, &value);
	printk("VM-Exit:VMREAD.field:0x%x, value:0x%x\n", field, value);
	kvm_reg_write(vcpu, base_reg, value);
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}
void delay(u64);
int vmx_handle_vmwrite(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u64 field = kvm_reg_read(vcpu, reg2);
	u64 value = kvm_reg_read(vcpu, base_reg);
	int ret;

	vmcs12_write_any(vcpu->vmcs12, field, value);
	if (field == GUEST_CR0) {
		printk("vmwrite guest cr0:value:%x\n", value);
	}
	printk("VM-Exit:VMWRITE.field:0x%x, value:0x%x\n", field, value);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_invept(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int reg2 = (instruction_info >> 28) & 0xf;
	gva_t gva;
	gpa_t gpa;
	hpa_t hpa;
	hpa_t eptp_hpa;
	u64 *virt;
	int ret;
	struct {
		u64 eptp, gpa;
	} *invept_context = NULL;

	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;
	vmptr_load(vcpu->vmcs02_phys);
	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva to gpa failed.\n");
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}

	invept_context = (void *)PHYS2VIRT(hpa);

	//printk("VM-Exit:INVEPT.reg2:%d(0x%x) eptp = %x\n",
	//	reg2,
	//	kvm_reg_read(vcpu, reg2),
	//	invept_context->eptp);

	ret = ept_gpa_to_hpa(vcpu, invept_context->eptp, &eptp_hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}
	//invept(kvm_reg_read(vcpu, reg2), VIRT2PHYS(eptp_hpa), 0);

	vmclear(vcpu->vmcs02_phys);
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int nested_vmx_set_ctrl_state(struct vmx_vcpu *vcpu, struct vmcs12 *vmcs12)
{
	u64 gpa, hpa;
	int ret;
	u64 *ept_pointer;

	vmcs_write(VIRTUAL_PROCESSOR_ID, vmcs12->virtual_processor_id);

	gpa = vmcs12->io_bitmap_a;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(IO_BITMAP_A, hpa);

	gpa = vmcs12->io_bitmap_b;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(IO_BITMAP_B, hpa);
	gpa = vmcs12->msr_bitmap;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(MSR_BITMAP, hpa);
	gpa = vmcs12->vm_entry_msr_load_addr;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(VM_ENTRY_MSR_LOAD_ADDR, hpa);

	gpa = vmcs12->vm_exit_msr_store_addr;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(VM_EXIT_MSR_STORE_ADDR, hpa);
	gpa = vmcs12->vm_exit_msr_load_addr;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(VM_EXIT_MSR_LOAD_ADDR, hpa);

	gpa = vmcs12->virtual_apic_page_addr;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	vmcs_write(VIRTUAL_APIC_PAGE_ADDR, hpa);

	if (PT_ENTRY_ADDR(vmcs_read(EPT_POINTER)) == 0) {
		ept_pointer = kmalloc(0x1000, GFP_KERNEL);
		memset(ept_pointer, 0, 0x1000);
		vmcs_write(EPT_POINTER, VIRT2PHYS(ept_pointer) | 0x5e);
	}

	gpa = vmcs12->posted_intr_desc_addr;
	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
		return -1;
	}
	//vmcs_write(POSTED_INTR_DESC_ADDR, hpa);

	vmcs_write(VMX_PREEMPTION_TIMER_VALUE, vmcs12->vmx_preemption_timer_value);
	vmcs_write(PIN_BASED_VM_EXEC_CONTROL, vmcs12->pin_based_vm_exec_control);
	vmcs_write(CPU_BASED_VM_EXEC_CONTROL, vmcs12->cpu_based_vm_exec_control);
	vmcs_write(SECONDARY_VM_EXEC_CONTROL, vmcs12->secondary_vm_exec_control);
	vmcs_write(VM_ENTRY_CONTROLS, vmcs12->vm_entry_controls);
	vmcs_write(VM_EXIT_CONTROLS, vmcs12->vm_exit_controls);

	vmcs_write(CR3_TARGET_COUNT, vmcs12->cr3_target_count);
	vmcs_write(CR0_GUEST_HOST_MASK, vmcs12->cr0_guest_host_mask);
	vmcs_write(CR4_GUEST_HOST_MASK, vmcs12->cr4_guest_host_mask);
	vmcs_write(EXCEPTION_BITMAP, vmcs12->exception_bitmap);

	return 0;
}

int nested_vmx_save_host_state(struct vmx_vcpu *vcpu)
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


int nested_vmx_set_guest_state(struct vmx_vcpu *vcpu, struct vmcs12 *vmcs12)
{
	vmcs_write(GUEST_CR0, vmcs12->guest_cr0);
	vmcs_write(CR0_READ_SHADOW, vmcs12->cr0_read_shadow);
	vmcs_write(GUEST_CR3, vmcs12->guest_cr3);
	vmcs_write(GUEST_CR4, vmcs12->guest_cr4);
	vmcs_write(CR4_READ_SHADOW, vmcs12->cr4_read_shadow);

	vmcs_write(GUEST_RIP, vmcs12->guest_rip);
	vmcs_write(GUEST_RFLAGS, vmcs12->guest_rflags);
	
	vmcs_write(GUEST_RSP, vmcs12->guest_rsp);
	vmcs_write(GUEST_DR7, vmcs12->guest_dr7);
	vmcs_write(GUEST_IA32_DEBUGCTL, vmcs12->guest_ia32_debugctl);

	vmcs_write(GUEST_CS_SELECTOR, vmcs12->guest_cs_selector);
	vmcs_write(GUEST_CS_BASE, vmcs12->guest_cs_base);
	vmcs_write(GUEST_CS_LIMIT, vmcs12->guest_cs_limit);
	vmcs_write(GUEST_CS_AR_BYTES, vmcs12->guest_cs_ar_bytes);

	vmcs_write(GUEST_DS_SELECTOR, vmcs12->guest_ds_selector);
	vmcs_write(GUEST_DS_BASE, vmcs12->guest_ds_base);
	vmcs_write(GUEST_DS_LIMIT, vmcs12->guest_ds_limit);
	vmcs_write(GUEST_DS_AR_BYTES, vmcs12->guest_ds_ar_bytes);

	vmcs_write(GUEST_ES_SELECTOR, vmcs12->guest_es_selector);
	vmcs_write(GUEST_ES_BASE, vmcs12->guest_es_base);
	vmcs_write(GUEST_ES_LIMIT, vmcs12->guest_es_limit);
	vmcs_write(GUEST_ES_AR_BYTES, vmcs12->guest_es_ar_bytes);

	vmcs_write(GUEST_FS_SELECTOR, vmcs12->guest_fs_selector);
	vmcs_write(GUEST_FS_BASE, vmcs12->guest_fs_base);
	vmcs_write(GUEST_FS_LIMIT, vmcs12->guest_fs_limit);
	vmcs_write(GUEST_FS_AR_BYTES, vmcs12->guest_fs_ar_bytes);

	vmcs_write(GUEST_GS_SELECTOR, vmcs12->guest_gs_selector);
	vmcs_write(GUEST_GS_BASE, vmcs12->guest_gs_base);
	vmcs_write(GUEST_GS_LIMIT, vmcs12->guest_gs_limit);
	vmcs_write(GUEST_GS_AR_BYTES, vmcs12->guest_gs_ar_bytes);

	vmcs_write(GUEST_SS_SELECTOR, vmcs12->guest_ss_selector);
	vmcs_write(GUEST_SS_BASE, vmcs12->guest_ss_base);
	vmcs_write(GUEST_SS_LIMIT, vmcs12->guest_ss_limit);
	vmcs_write(GUEST_SS_AR_BYTES, vmcs12->guest_ss_ar_bytes);	

	vmcs_write(GUEST_TR_SELECTOR, vmcs12->guest_tr_selector);
	vmcs_write(GUEST_TR_BASE, vmcs12->guest_tr_base);
	vmcs_write(GUEST_TR_LIMIT, vmcs12->guest_tr_limit);
	vmcs_write(GUEST_TR_AR_BYTES, vmcs12->guest_tr_ar_bytes);
	
	vmcs_write(GUEST_LDTR_SELECTOR, vmcs12->guest_ldtr_selector);
	vmcs_write(GUEST_LDTR_BASE, vmcs12->guest_ldtr_base);
	vmcs_write(GUEST_LDTR_LIMIT, vmcs12->guest_ldtr_limit);
	vmcs_write(GUEST_LDTR_AR_BYTES, vmcs12->guest_ldtr_ar_bytes);

	vmcs_write(GUEST_GDTR_BASE, vmcs12->guest_gdtr_base);
	vmcs_write(GUEST_GDTR_LIMIT, vmcs12->guest_gdtr_limit);
	vmcs_write(GUEST_IDTR_BASE, vmcs12->guest_idtr_base);
	vmcs_write(GUEST_IDTR_LIMIT, vmcs12->guest_idtr_limit);

	vmcs_write(GUEST_SYSENTER_ESP, vmcs12->guest_sysenter_esp);
	vmcs_write(GUEST_SYSENTER_EIP, vmcs12->guest_sysenter_eip);
	vmcs_write(GUEST_SYSENTER_CS, vmcs12->guest_sysenter_cs);
	vmcs_write(GUEST_IA32_DEBUGCTL, vmcs12->guest_ia32_debugctl);
	vmcs_write(GUEST_IA32_PERF_GLOBAL_CTRL, vmcs12->guest_ia32_perf_global_ctrl);
	vmcs_write(GUEST_IA32_EFER, vmcs12->guest_ia32_efer);
	vmcs_write(GUEST_IA32_PAT, vmcs12->guest_ia32_pat);
	
	vmcs_write(GUEST_ACTIVITY_STATE, vmcs12->guest_activity_state);

	vmcs_write(GUEST_PDPTR0, vmcs12->guest_pdptr0);
	vmcs_write(GUEST_PDPTR1, vmcs12->guest_pdptr1);
	vmcs_write(GUEST_PDPTR2, vmcs12->guest_pdptr2);
	vmcs_write(GUEST_PDPTR3, vmcs12->guest_pdptr3);

	vmcs_write(GUEST_INTERRUPTIBILITY_INFO, vmcs12->guest_interruptibility_info);
	vmcs_write(VM_ENTRY_INTR_INFO_FIELD, vmcs12->vm_entry_intr_info_field);
	vmcs_write(GUEST_PENDING_DBG_EXCEPTIONS, vmcs12->guest_pending_dbg_exceptions);
	vmcs_write(GUEST_BNDCFGS, vmcs12->guest_bndcfgs);

	vmcs_write(VMCS_LINK_POINTER, vmcs12->vmcs_link_pointer);

	vmcs_write(APIC_ACCESS_ADDR, vmcs12->apic_access_addr);

	vmcs_write(VM_ENTRY_CONTROLS, vmcs12->vm_entry_controls);

	//xsetbv(0, vcpu->guest_state.ctrl_regs.xcr0);
	//xrstor(vcpu->guest_state.fp_regs, vcpu->guest_state.ctrl_regs.xcr0);
	/* Use host XCRO features to zero out all host fpu regs. */
	//xrstor(vcpu->guest_state.fp_regs, vcpu->host_state.ctrl_regs.xcr0);
	//xrstor(vcpu->guest_state.fp_regs, XCR0_X87 | XCR0_SSE | XCR0_AVX);

	return 0;
}


int prepare_vmcs02(struct vmx_vcpu *vcpu, struct vmcs12 *vmcs12)
{
	vmclear(vcpu->vmcs01_phys);
	vmptr_load(vcpu->vmcs02_phys);
	nested_vmx_set_ctrl_state(vcpu, vmcs12);
	nested_vmx_save_host_state(vcpu);
	memset(&vcpu->l2_guest_state, 0, sizeof(vcpu->l2_guest_state));
	nested_vmx_set_guest_state(vcpu, vmcs12);
}

int sync_vmcs12(struct vmx_vcpu *vcpu, struct vmcs12 *vmcs12)
{
	vmcs12->guest_rflags = vmcs_read(GUEST_RFLAGS);
	vmcs12->guest_rip = vmcs_read(GUEST_RIP);
	vmcs12->guest_rsp = vmcs_read(GUEST_RSP);
	vmcs12->guest_es_selector = vmcs_read(GUEST_ES_SELECTOR);
	vmcs12->guest_cs_selector = vmcs_read(GUEST_CS_SELECTOR);
	vmcs12->guest_ss_selector = vmcs_read(GUEST_SS_SELECTOR);
	vmcs12->guest_ds_selector = vmcs_read(GUEST_DS_SELECTOR);
	vmcs12->guest_fs_selector = vmcs_read(GUEST_FS_SELECTOR);
	vmcs12->guest_gs_selector = vmcs_read(GUEST_GS_SELECTOR);
	vmcs12->guest_ldtr_selector = vmcs_read(GUEST_LDTR_SELECTOR);
	vmcs12->guest_tr_selector = vmcs_read(GUEST_TR_SELECTOR);
	vmcs12->guest_es_limit = vmcs_read(GUEST_ES_LIMIT);
	vmcs12->guest_cs_limit = vmcs_read(GUEST_CS_LIMIT);
	vmcs12->guest_ss_limit = vmcs_read(GUEST_SS_LIMIT);
	vmcs12->guest_ds_limit = vmcs_read(GUEST_DS_LIMIT);
	vmcs12->guest_fs_limit = vmcs_read(GUEST_FS_LIMIT);
	vmcs12->guest_gs_limit = vmcs_read(GUEST_GS_LIMIT);
	vmcs12->guest_ldtr_limit = vmcs_read(GUEST_LDTR_LIMIT);
	vmcs12->guest_tr_limit = vmcs_read(GUEST_TR_LIMIT);
	vmcs12->guest_gdtr_limit = vmcs_read(GUEST_GDTR_LIMIT);
	vmcs12->guest_idtr_limit = vmcs_read(GUEST_IDTR_LIMIT);
	vmcs12->guest_es_ar_bytes = vmcs_read(GUEST_ES_AR_BYTES);
	vmcs12->guest_cs_ar_bytes = vmcs_read(GUEST_CS_AR_BYTES);
	vmcs12->guest_ss_ar_bytes = vmcs_read(GUEST_SS_AR_BYTES);
	vmcs12->guest_ds_ar_bytes = vmcs_read(GUEST_DS_AR_BYTES);
	vmcs12->guest_fs_ar_bytes = vmcs_read(GUEST_FS_AR_BYTES);
	vmcs12->guest_gs_ar_bytes = vmcs_read(GUEST_GS_AR_BYTES);
	vmcs12->guest_ldtr_ar_bytes = vmcs_read(GUEST_LDTR_AR_BYTES);
	vmcs12->guest_tr_ar_bytes = vmcs_read(GUEST_TR_AR_BYTES);
	vmcs12->guest_es_base = vmcs_read(GUEST_ES_BASE);
	vmcs12->guest_cs_base = vmcs_read(GUEST_CS_BASE);
	vmcs12->guest_ss_base = vmcs_read(GUEST_SS_BASE);
	vmcs12->guest_ds_base = vmcs_read(GUEST_DS_BASE);
	vmcs12->guest_fs_base = vmcs_read(GUEST_FS_BASE);
	vmcs12->guest_gs_base = vmcs_read(GUEST_GS_BASE);
	vmcs12->guest_ldtr_base = vmcs_read(GUEST_LDTR_BASE);
	vmcs12->guest_tr_base = vmcs_read(GUEST_TR_BASE);
	vmcs12->guest_gdtr_base = vmcs_read(GUEST_GDTR_BASE);
	vmcs12->guest_idtr_base = vmcs_read(GUEST_IDTR_BASE);

	vmcs12->guest_interruptibility_info =
		vmcs_read(GUEST_INTERRUPTIBILITY_INFO);
	vmcs12->guest_pending_dbg_exceptions =
		vmcs_read(GUEST_PENDING_DBG_EXCEPTIONS);

	vmcs12->guest_cr0 = vmcs_read(GUEST_CR0);
	vmcs12->guest_cr4 = vmcs_read(GUEST_CR4);
	vmcs12->cr0_read_shadow = vmcs_read(CR0_READ_SHADOW);
	vmcs12->cr4_read_shadow = vmcs_read(CR4_READ_SHADOW);
	vmcs12->vm_exit_reason = vmcs_read(VM_EXIT_REASON);
	vmcs12->exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	vmcs12->guest_ia32_efer = vmcs_read(GUEST_IA32_EFER);
}

int nested_handle_vm_entry_failed(struct vmx_vcpu *vcpu)
{
	u32 instruction_error = vmcs_read(VM_INSTRUCTION_ERROR);
	u32 exit_reason = vmcs_read(VM_EXIT_REASON);
	printk("VM-Entry failed.\n");
	printk("vm instruction error:%d\n", instruction_error);
	printk("exit reason:%d\n", exit_reason & 0x7fffffff);
	return 0;
}

int nested_vmx_handle_ept_volation(struct vmx_vcpu *vcpu)
{
	printk("nested VM-Exit:EPT Volation.\n");
	gpa_t l2gpa = vmcs_read(GUEST_PHYSICAL_ADDRESS);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	gpa_t l1gpa;
	hpa_t hpa;
	u64 *ept_pointer = (u64 *)PHYS2VIRT(PT_ENTRY_ADDR(vmcs_read(EPT_POINTER)));
	int ret;
	printk("l2gpa = 0x%x\n", l2gpa);
	printk("exit qualification:0x%x\n", exit_qualification);
	nested_ept_l2gpa_to_l1gpa(vcpu, vcpu->vmcs12, l2gpa, &l1gpa);
	printk("l1gpa = 0x%x\n", l1gpa);
	ret = ept_gpa_to_hpa(vcpu, l1gpa, &hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}
	printk("hpa = 0x%x\n", hpa);
	ept_map_page(ept_pointer, l2gpa, hpa, 0x1000, EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE | EPT_PTE_CACHE_WB);
	//vcpu->vmcs12->guest_rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_run(struct vmx_vcpu *vcpu);
int nested_vm_exit_to_l1(struct vmx_vcpu *vcpu)
{
	vmclear(vcpu->vmcs02_phys);
	vcpu->state = 0;
	vcpu->guest_state.rip = vcpu->vmcs12->host_rip;
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.gr_regs.rsp = vcpu->vmcs12->host_rsp;
	vmcs_write(GUEST_RIP, vcpu->guest_state.rip);
	vmcs_write(GUEST_RSP, vcpu->guest_state.gr_regs.rsp);
	vmcs_write(VM_EXIT_REASON, vcpu->vmcs12->vm_exit_reason);
	vmcs_write(EXIT_QUALIFICATION, vcpu->vmcs12->exit_qualification);
	vmcs_write(VM_EXIT_INSTRUCTION_LEN, vcpu->vmcs12->vm_exit_instruction_len);
	printk("vmx:return L1 from L0\n");
	memcpy(&vcpu->guest_state.gr_regs, &vcpu->l2_guest_state.gr_regs, sizeof(vcpu->guest_state.gr_regs));
	vmx_run(vcpu);
}

int nested_vm_exit_handler(struct vmx_vcpu *vcpu)
{
	int ret = 1;
	u32 exit_reason = vmcs_read(VM_EXIT_REASON);
	u32 instruction_error = vmcs_read(VM_INSTRUCTION_ERROR);
	//printk("vm_exit reason:%d\n", exit_reason);
	//printk("Guest RIP:%x\n", vcpu->guest_state.rip);
	vcpu->vmcs12->vm_exit_reason = exit_reason;
	vcpu->vmcs12->exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	vcpu->vmcs12->vm_exit_instruction_len = vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	if (exit_reason & 0x80000000) {
		nested_handle_vm_entry_failed(vcpu);
		while (1);
	} else {
		printk("nested vm-exit.reason = %d rip = 0x%x\n", exit_reason, vmcs_read(GUEST_RIP));
		switch (exit_reason & 0xff) {
		case EXIT_REASON_EXCEPTION_NMI:
			break;
		case EXIT_REASON_EXTERNAL_INTERRUPT:
			break;
		case EXIT_REASON_TRIPLE_FAULT:
			while (1);
			break;
		case EXIT_REASON_PENDING_INTERRUPT:
			break;
		case EXIT_REASON_NMI_WINDOW:
			break;
		case EXIT_REASON_TASK_SWITCH:
			break;
		case EXIT_REASON_CPUID:
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
			break;
		case EXIT_REASON_DR_ACCESS:
			break;
		case EXIT_REASON_IO_INSTRUCTION:
			break;
		case EXIT_REASON_MSR_READ:
			break;
		case EXIT_REASON_MSR_WRITE:
			break;
		case EXIT_REASON_INVALID_STATE:
			printk("VM Exit:Invalid Guest State.\n");
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
			ret = nested_vmx_handle_ept_volation(vcpu);
			break;
		case EXIT_REASON_EPT_MISCONFIG:
			while(1);
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
		default:
			printk("VM-Exit:Unhandled exit reason:%d\n", exit_reason & 0xff);
			break;
		}
	}
	//printk("Next RIP:%x\n", vcpu->guest_state.rip);
	if (ret == 0) {
		vmcs_write(GUEST_RIP, vcpu->vmcs12->guest_rip);
	} else {
		nested_vm_exit_to_l1(vcpu);
	}
	return ret;
}

int nested_vmx_run(struct vmx_vcpu *vcpu)
{
	int ret0 = 0, ret1 = 0;
	printk("vmx:enter L2 from L1\n");
	while (1) {
		if (vcpu->state == 0) {
			prepare_vmcs02(vcpu, vcpu->vmcs12);	
			//printk("VM LAUNCH.\n");
			ret0 = vm_launch(&vcpu->host_state.gr_regs, &vcpu->l2_guest_state.gr_regs);
			if (ret0 == 0) {
				vcpu->state = 1;
				vcpu->guest_mode = 1;
			}
		} else {
			//printk("VM RESUME.\n");
			nested_vmx_set_guest_state(vcpu, vcpu->vmcs12);
			ret0 = vm_resume(&vcpu->host_state.gr_regs, &vcpu->l2_guest_state.gr_regs);
		}
		sync_vmcs12(vcpu, vcpu->vmcs12);
		if (ret0 == 0) {
			nested_vm_exit_handler(vcpu);
		} else {
			printk("nested vmx:fail invalid.\n");
			return -1;
		}
	}
}

