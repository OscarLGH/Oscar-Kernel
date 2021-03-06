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
		printk("transfer gva 0x%x to gpa failed.\n", gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
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

	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva 0x%x to gpa failed.\n", gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);
	vmcs12_phys = *virt;
	//printk("VM-Exit:VMCLEAR.Region:0x%x\n", *virt);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int nested_vmx_run(struct vmx_vcpu *vcpu);

int vmx_handle_vmlaunch(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMLAUNCH.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	nested_vmx_run(vcpu);
	return 0;
}

int vmx_handle_vmresume(struct vmx_vcpu *vcpu)
{
	vcpu->state = 1;
	//printk("VM-Exit:VMRESUME.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
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

	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva 0x%x to gpa failed.\n", gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);

	printk("VM-Exit:VMPTRLD.OPRAND GVA:0x%x GPA:0x%x HPA:0x%x VMCS GPA = 0x%x\n", gva, gpa, hpa, *virt);
	gpa = *virt;

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
		return 0;
	}


	if (vcpu->shadow_vmcs_enabled) {
		vcpu->shadow_vmcs = (void *)PHYS2VIRT(hpa);
		vcpu->shadow_vmcs->revision_id = rdmsr(MSR_IA32_VMX_BASIC) | 0x80000000;
		if (vcpu->vmcs12 == NULL) {
			vcpu->vmcs12 = kmalloc(0x1000, GFP_KERNEL);
			memset(vcpu->vmcs12, 0, 0x1000);
		}
		printk("setting VMCS_LINK_POINTER:%x\n", hpa);
		vmcs_write(VMCS_LINK_POINTER, hpa);
	} else {
		vcpu->vmcs12 = (void *)PHYS2VIRT(hpa);
	}

	//printk("VM-Exit:VMPTRLD.Region:0x%x\n", *virt);
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

	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva 0x%x to gpa failed.\n", gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
		return 0;
	}

	virt = (void *)PHYS2VIRT(hpa);
	printk("VM-Exit:VMPTRST.Region:0x%x\n", *virt);

	*virt = VIRT2PHYS(vcpu->vmcs12);

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

	//printk("VM-Exit:VMREAD.field:0x%x, value:0x%x\n", field, value);
	vmcs12_read_any(vcpu->vmcs12, field, &value);
	kvm_reg_write(vcpu, base_reg, value);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmwrite(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u64 field = kvm_reg_read(vcpu, reg2);
	u64 value = kvm_reg_read(vcpu, base_reg);
	int ret;

	//printk("VM-Exit:VMWRITE.field:0x%x, value:0x%x\n", field, value);
	vmcs12_write_any(vcpu->vmcs12, field, value);
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
	//printk("VM-Exit:INVEPT\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
	decode_mem_address(vcpu, exit_qualification, instruction_info, &gva);
	ret = paging64_gva_to_gpa(vcpu, gva, &gpa);
	if (ret) {
		printk("transfer gva 0x%x to gpa failed.\n", gva);
		return 0;
	}

	ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
		return 0;
	}

	invept_context = (void *)PHYS2VIRT(hpa);

	//printk("VM-Exit:INVEPT.reg2:%d(0x%x) eptp = %x\n",
	//	reg2,
	//	kvm_reg_read(vcpu, reg2),
	//	invept_context->eptp);

	ret = ept_gpa_to_hpa(vcpu, invept_context->eptp, &eptp_hpa);
	if (ret) {
		printk("transfer gpa 0x%x to hpa failed.\n", gpa);
		return 0;
	}
	//invept(kvm_reg_read(vcpu, reg2), VIRT2PHYS(eptp_hpa), 0);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

u64 vmcs_16bit_ctrl_fields[] = {
	VIRTUAL_PROCESSOR_ID,
	POSTED_INTR_NV,
	EPTP_INDEX
};

u64 vmcs_16bit_guest_state_fields[] = {
	GUEST_ES_SELECTOR,
	GUEST_CS_SELECTOR,
	GUEST_SS_SELECTOR,
	GUEST_DS_SELECTOR,
	GUEST_FS_SELECTOR,
	GUEST_GS_SELECTOR,
	GUEST_LDTR_SELECTOR,
	GUEST_TR_SELECTOR,
	GUEST_INTR_STATUS,
	GUEST_PML_INDEX
};

u64 vmcs_16bit_host_state_fields[] = {
	HOST_ES_SELECTOR,
	HOST_CS_SELECTOR,
	HOST_SS_SELECTOR,
	HOST_DS_SELECTOR,
	HOST_FS_SELECTOR,
	HOST_GS_SELECTOR,
	HOST_TR_SELECTOR
};

u64 vmcs_64bit_ctrl_fields[] = {
	IO_BITMAP_A,
	IO_BITMAP_B,
	MSR_BITMAP,
	VM_EXIT_MSR_STORE_ADDR,
	VM_EXIT_MSR_LOAD_ADDR,
	VM_ENTRY_MSR_LOAD_ADDR,
	PML_ADDRESS,
	TSC_OFFSET,
	VIRTUAL_APIC_PAGE_ADDR,
	APIC_ACCESS_ADDR,
	POSTED_INTR_DESC_ADDR,
	VM_FUNCTION_CONTROL,
	EPT_POINTER,
	EOI_EXIT_BITMAP0,
	EOI_EXIT_BITMAP1,
	EOI_EXIT_BITMAP2,
	EOI_EXIT_BITMAP3,
	EPTP_LIST_ADDRESS,
	VMREAD_BITMAP,
	VMWRITE_BITMAP,
	XSS_EXIT_BITMAP,
	ENCLS_EXITING_BITMAP,
	TSC_MULTIPLIER
};

u64 vmcs_64bit_readonly_fields[] = {
	GUEST_PHYSICAL_ADDRESS
};

u64 vmcs_64bit_guest_state_fields[] = {
	VMCS_LINK_POINTER,
	GUEST_IA32_DEBUGCTL,
	GUEST_IA32_PAT,
	GUEST_IA32_EFER,
	GUEST_IA32_PERF_GLOBAL_CTRL,
	GUEST_PDPTR0,
	GUEST_PDPTR1,
	GUEST_PDPTR2,
	GUEST_PDPTR3,
	GUEST_BNDCFGS,
	//GUEST_IA32_RTIT_CTL
};

u64 vmcs_64bit_host_state_fields[] = {
	HOST_IA32_PAT,
	HOST_IA32_PAT_HIGH,
	HOST_IA32_EFER,
	HOST_IA32_EFER_HIGH,
	HOST_IA32_PERF_GLOBAL_CTRL,
	HOST_IA32_PERF_GLOBAL_CTRL_HIGH
};

u64 vmcs_32bit_ctrl_fields[] = {
	PIN_BASED_VM_EXEC_CONTROL,
	CPU_BASED_VM_EXEC_CONTROL,
	EXCEPTION_BITMAP,
	PAGE_FAULT_ERROR_CODE_MASK,
	PAGE_FAULT_ERROR_CODE_MATCH,
	CR3_TARGET_COUNT,
	VM_EXIT_CONTROLS,
	VM_EXIT_MSR_STORE_COUNT,
	VM_EXIT_MSR_LOAD_COUNT,
	VM_ENTRY_CONTROLS,
	VM_ENTRY_MSR_LOAD_COUNT,
	VM_ENTRY_INTR_INFO_FIELD,
	VM_ENTRY_EXCEPTION_ERROR_CODE,
	VM_ENTRY_INSTRUCTION_LEN,
	TPR_THRESHOLD,
	SECONDARY_VM_EXEC_CONTROL,
	PLE_GAP,
	PLE_WINDOW
};

u64 vmcs_32bit_readonly_fields[] = {
	VM_INSTRUCTION_ERROR,
	VM_EXIT_REASON,
	VM_EXIT_INTR_INFO,
	VM_EXIT_INTR_ERROR_CODE,
	IDT_VECTORING_INFO_FIELD,
	IDT_VECTORING_ERROR_CODE,
	VM_EXIT_INSTRUCTION_LEN,
	VMX_INSTRUCTION_INFO
};

u64 vmcs_32bit_guest_state_fields[] = {
	GUEST_ES_LIMIT,
	GUEST_CS_LIMIT,
	GUEST_SS_LIMIT,
	GUEST_DS_LIMIT,
	GUEST_FS_LIMIT,
	GUEST_GS_LIMIT,
	GUEST_LDTR_LIMIT,
	GUEST_TR_LIMIT,
	GUEST_GDTR_LIMIT,
	GUEST_IDTR_LIMIT,
	GUEST_ES_AR_BYTES,
	GUEST_CS_AR_BYTES,
	GUEST_SS_AR_BYTES,
	GUEST_DS_AR_BYTES,
	GUEST_FS_AR_BYTES,
	GUEST_GS_AR_BYTES,
	GUEST_LDTR_AR_BYTES,
	GUEST_TR_AR_BYTES,
	GUEST_INTERRUPTIBILITY_INFO,
	GUEST_ACTIVITY_STATE,
	GUEST_SYSENTER_CS,
	VMX_PREEMPTION_TIMER_VALUE
};

u64 vmcs_32bit_host_state_field[] = {
	HOST_IA32_SYSENTER_CS
};

u64 vmcs_natural_width_ctrl_fields[] = {
	CR0_GUEST_HOST_MASK,
	CR4_GUEST_HOST_MASK,
	CR0_READ_SHADOW,
	CR4_READ_SHADOW,
	CR3_TARGET_VALUE0,
	CR3_TARGET_VALUE1,
	CR3_TARGET_VALUE2,
	CR3_TARGET_VALUE3
};

u64 vmcs_natural_width_readonly_fields[] = {
	EXIT_QUALIFICATION,
	IO_RCX,
	IO_RSI,
	IO_RDI,
	IO_RIP,
	GUEST_LINEAR_ADDRESS
};

u64 vmcs_natural_width_guest_state_fields[] = {
	GUEST_CR0,
	GUEST_CR3,
	GUEST_CR4,
	GUEST_ES_BASE,
	GUEST_CS_BASE,
	GUEST_SS_BASE,
	GUEST_DS_BASE,
	GUEST_FS_BASE,
	GUEST_GS_BASE,
	GUEST_LDTR_BASE,
	GUEST_TR_BASE,
	GUEST_GDTR_BASE,
	GUEST_IDTR_BASE,
	GUEST_DR7,
	GUEST_RSP,
	GUEST_RIP,
	GUEST_RFLAGS,
	GUEST_PENDING_DBG_EXCEPTIONS,
	GUEST_SYSENTER_ESP,
	GUEST_SYSENTER_EIP,
};

u64 vmcs_natural_width_host_state_fields[] = {
	HOST_CR0,
	HOST_CR3,
	HOST_CR4,
	HOST_FS_BASE,
	HOST_GS_BASE,
	HOST_TR_BASE,
	HOST_GDTR_BASE,
	HOST_IDTR_BASE,
	HOST_IA32_SYSENTER_ESP,
	HOST_IA32_SYSENTER_EIP,
	HOST_RSP,
	HOST_RIP	
};

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

	if (vmcs12->pin_based_vm_exec_control & PIN_BASED_POSTED_INTR) {
		gpa = vmcs12->posted_intr_desc_addr;
		ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
		if (ret) {
			printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
			return -1;
		}
		vmcs_write(POSTED_INTR_DESC_ADDR, hpa);
	}

	//vmcs12->secondary_vm_exec_control &= (~SECONDARY_EXEC_SHADOW_VMCS);
	vmcs_write(VMCS_LINK_POINTER, -1);
	if (vmcs12->secondary_vm_exec_control & SECONDARY_EXEC_SHADOW_VMCS) {

		if (vmcs12->vmcs_link_pointer != 0xffffffffffffffff) {
			gpa = vmcs12->vmcs_link_pointer;
			ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
			if (ret) {
				printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
				return -1;
			}
			((struct vmcs_hdr *)PHYS2VIRT(hpa))->revision_id = 0x80000000 | rdmsr(MSR_IA32_VMX_BASIC);
			vmcs_write(VMCS_LINK_POINTER, hpa);
		}

		gpa = vmcs12->vmread_bitmap;
		ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
		if (ret) {
			printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
			return -1;
		}
		vmcs_write(VMREAD_BITMAP, hpa);

		gpa = vmcs12->vmwrite_bitmap;
		ret = ept_gpa_to_hpa(vcpu, gpa, &hpa);
		if (ret) {
			printk("transfer gpa to hpa failed.gpa = %x LINE = %d\n", gpa, __LINE__);
			return -1;
		}
		vmcs_write(VMWRITE_BITMAP, hpa);
	}

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
	int i;
	u64 field, value;

	for (i = 0; i < ARRAY_SIZE(vmcs_16bit_guest_state_fields); i++) {
		field = vmcs_16bit_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_guest_state_fields); i++) {
		field = vmcs_32bit_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_guest_state_fields); i++) {
		field = vmcs_64bit_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_guest_state_fields); i++) {
		field = vmcs_natural_width_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	vmcs_write(CR0_READ_SHADOW, vmcs12->cr0_read_shadow);
	vmcs_write(CR4_READ_SHADOW, vmcs12->cr4_read_shadow);

	vmcs_write(APIC_ACCESS_ADDR, vmcs12->apic_access_addr);
	
	//xsetbv(0, vcpu->guest_state.ctrl_regs.xcr0);
	//xrstor(vcpu->guest_state.fp_regs, vcpu->guest_state.ctrl_regs.xcr0);
	/* Use host XCRO features to zero out all host fpu regs. */
	//xrstor(vcpu->guest_state.fp_regs, vcpu->host_state.ctrl_regs.xcr0);
	//xrstor(vcpu->guest_state.fp_regs, XCR0_X87 | XCR0_SSE | XCR0_AVX);

	return 0;
}


int prepare_vmcs02(struct vmx_vcpu *vcpu, struct vmcs12 *vmcs12)
{
	vmclear(VIRT2PHYS(vcpu->vmcs01));
	vcpu->state = 0;
	vmptr_load(VIRT2PHYS(vcpu->vmcs02));
	nested_vmx_set_ctrl_state(vcpu, vmcs12);
	nested_vmx_save_host_state(vcpu);
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
	vmcs12->guest_cr3 = vmcs_read(GUEST_CR3);
	vmcs12->guest_cr4 = vmcs_read(GUEST_CR4);
	vmcs12->cr0_read_shadow = vmcs_read(CR0_READ_SHADOW);
	vmcs12->cr4_read_shadow = vmcs_read(CR4_READ_SHADOW);
	vmcs12->vm_exit_reason = vmcs_read(VM_EXIT_REASON);
	vmcs12->exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	vmcs12->vm_exit_intr_info = vmcs_read(VM_EXIT_INTR_INFO);
	vmcs12->guest_ia32_efer = vmcs_read(GUEST_IA32_EFER);
	vmcs12->vm_exit_instruction_len = vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	vmcs12->vm_exit_intr_error_code = vmcs_read(VM_EXIT_INTR_ERROR_CODE);
	vmcs12->guest_physical_address = vmcs_read(GUEST_PHYSICAL_ADDRESS);
	vmcs12->guest_linear_address = vmcs_read(GUEST_LINEAR_ADDRESS);
	vmcs12->vmx_instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
}

int copy_vmcs12_to_shadow(struct vmx_vcpu *vcpu)
{
	int i;
	u64 field, value;
	struct vmcs12 *vmcs12 = vcpu->vmcs12;
	u64 vmptr_phys;
	vmptr_store(&vmptr_phys);
	vmclear(vmptr_phys);
	vmptr_load(VIRT2PHYS(vcpu->shadow_vmcs));

	for (i = 0; i < ARRAY_SIZE(vmcs_16bit_guest_state_fields); i++) {
		field = vmcs_16bit_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_guest_state_fields); i++) {
		field = vmcs_32bit_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_guest_state_fields); i++) {
		field = vmcs_64bit_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_guest_state_fields); i++) {
		field = vmcs_natural_width_guest_state_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}


	for (i = 0; i < ARRAY_SIZE(vmcs_16bit_ctrl_fields); i++) {
		field = vmcs_16bit_ctrl_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_ctrl_fields); i++) {
		field = vmcs_32bit_ctrl_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_ctrl_fields); i++) {
		field = vmcs_64bit_ctrl_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_ctrl_fields); i++) {
		field = vmcs_natural_width_ctrl_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_readonly_fields); i++) {
		field = vmcs_32bit_readonly_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_readonly_fields); i++) {
		field = vmcs_64bit_readonly_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_readonly_fields); i++) {
		field = vmcs_natural_width_readonly_fields[i];
		vmcs12_read_any(vcpu->vmcs12, field, &value);
		vmcs_write(field, value);
	}

	vmptr_load(vmptr_phys);
}

int copy_shadow_to_vmcs12(struct vmx_vcpu *vcpu)
{
	int i;
	u64 field, value;
	struct vmcs12 *vmcs12 = vcpu->vmcs12;
	u64 vmptr_phys;
	vmptr_store(&vmptr_phys);
	vmclear(vmptr_phys);
	vmclear(VIRT2PHYS(vcpu->shadow_vmcs));
	vmptr_load(VIRT2PHYS(vcpu->shadow_vmcs));
	
	for (i = 0; i < ARRAY_SIZE(vmcs_16bit_guest_state_fields); i++) {
		field = vmcs_16bit_guest_state_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_guest_state_fields); i++) {
		field = vmcs_32bit_guest_state_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_guest_state_fields); i++) {
		field = vmcs_64bit_guest_state_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_guest_state_fields); i++) {
		field = vmcs_natural_width_guest_state_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}


	for (i = 0; i < ARRAY_SIZE(vmcs_16bit_ctrl_fields); i++) {
		field = vmcs_16bit_ctrl_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_ctrl_fields); i++) {
		field = vmcs_32bit_ctrl_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_ctrl_fields); i++) {
		field = vmcs_64bit_ctrl_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_ctrl_fields); i++) {
		field = vmcs_natural_width_ctrl_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_32bit_readonly_fields); i++) {
		field = vmcs_32bit_readonly_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_64bit_readonly_fields); i++) {
		field = vmcs_64bit_readonly_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	for (i = 0; i < ARRAY_SIZE(vmcs_natural_width_readonly_fields); i++) {
		field = vmcs_64bit_readonly_fields[i];
		value = vmcs_read(field);
		vmcs12_write_any(vcpu->vmcs12, field, value);
	}

	vmptr_load(vmptr_phys);
}

int copy_shadow_to_vmcs12_host(struct vmx_vcpu *vcpu)
{
	struct vmcs12 *vmcs12 = vcpu->vmcs12;
	u64 vmptr_phys;
	vmptr_store(&vmptr_phys);
	vmclear(vmptr_phys);
	vmclear(VIRT2PHYS(vcpu->shadow_vmcs));
	vmptr_load(VIRT2PHYS(vcpu->shadow_vmcs));

	vmcs12->host_rip = vmcs_read(HOST_RIP);
	vmcs12->host_rsp = vmcs_read(HOST_RSP);
	vmcs12->host_cr0 = vmcs_read(HOST_CR0);
	vmcs12->host_cr3 = vmcs_read(HOST_CR3);
	vmcs12->host_cr4 = vmcs_read(HOST_CR4);
	vmcs12->host_gdtr_base = vmcs_read(HOST_GDTR_BASE);
	vmcs12->host_idtr_base = vmcs_read(HOST_IDTR_BASE);

/*

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
*/
	vmptr_load(vmptr_phys);
}

int nested_vmx_handle_cpuid(struct vmx_vcpu *vcpu)
{
	u32 buffer[4];
	u32 eax, ecx;
	//printk("VM-Exit:cpuid.rip = 0x%x\n", vcpu->guest_state.rip);
	eax = vcpu->guest_state.gr_regs.rax;
	ecx = vcpu->guest_state.gr_regs.rcx;

	if (eax == 0x1 && ecx == 0x0) {
		/* Passthrough host cpuid. */
		cpuid(vcpu->guest_state.gr_regs.rax, vcpu->guest_state.gr_regs.rcx, buffer);
		vcpu->guest_state.gr_regs.rax = buffer[0];
		vcpu->guest_state.gr_regs.rbx = buffer[1];
		vcpu->guest_state.gr_regs.rcx = buffer[2];
		vcpu->guest_state.gr_regs.rdx = buffer[3];
		vcpu->guest_state.gr_regs.rcx = 0;
		printk("nested:eax = 0x%x, ebx = 0x%x ecx = 0x%x edx = 0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		vcpu->vmcs12->guest_rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
		return 0;
	}
	//vcpu->vmcs12->guest_rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 1;
}

int nested_vmx_handle_ept_volation(struct vmx_vcpu *vcpu)
{
	//printk("nested VM-Exit:EPT Volation.\n");
	gpa_t l2gpa = vmcs_read(GUEST_PHYSICAL_ADDRESS);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	gpa_t l1gpa;
	hpa_t hpa;
	u64 *ept_pointer = (u64 *)PHYS2VIRT(PT_ENTRY_ADDR(vmcs_read(EPT_POINTER)));
	int ret;
	//printk("exit qualification:0x%x\n", exit_qualification);
	ret = nested_ept_l2gpa_to_l1gpa(vcpu, vcpu->vmcs12, l2gpa, &l1gpa);
	if (ret) {
		printk("transfer l2gpa 0x%x to l1gpa failed.\n", l2gpa);
		printk("rip = 0x%x\n", vmcs_read(GUEST_RIP));
		return 1;
	}
	ret = ept_gpa_to_hpa(vcpu, l1gpa, &hpa);
	if (ret) {
		printk("transfer l1gpa 0x%x to hpa failed.\n", l1gpa);
		return 1;
	}

	//printk("shadow EPT:0x%x -> 0x%x -> 0x%x ===>>> 0x%x -> 0x%x rip = 0x%x\n", l2gpa, l1gpa, hpa, l2gpa, hpa, vmcs_read(GUEST_RIP));
	ept_map_page(ept_pointer, l2gpa, PT_ENTRY_ADDR(hpa), 0x1000, EPT_PTE_READ | EPT_PTE_WRITE | EPT_PTE_EXECUTE | EPT_PTE_CACHE_WB);
	return 0;
}

int nested_vmx_vmexit(struct vmx_vcpu *vcpu, u32 exit_reason, u32 exit_intr_info, u32 exit_qualification)
{
	//printk("enter L1 from L2. exit reason:%x \n", exit_reason);
	if (vcpu->shadow_vmcs_enabled) {
		copy_shadow_to_vmcs12_host(vcpu);
	}
	sync_vmcs12(vcpu, vcpu->vmcs12);

	vcpu->vmcs12->vm_exit_reason = exit_reason;
	vcpu->vmcs12->vm_exit_intr_info = exit_intr_info;
	vcpu->vmcs12->exit_qualification = exit_qualification;
	vmclear(VIRT2PHYS(vcpu->vmcs02));

	if (vcpu->shadow_vmcs_enabled) {
		copy_vmcs12_to_shadow(vcpu);
	}

	vcpu->state = 0;
	vcpu->guest_mode = 0;
	vcpu->guest_state.rip = vcpu->vmcs12->host_rip;
	vmptr_load(VIRT2PHYS(vcpu->vmcs01));
	vcpu->guest_state.gr_regs.rsp = vcpu->vmcs12->host_rsp;
	vmcs_write(GUEST_RIP, vcpu->guest_state.rip);
	vmcs_write(GUEST_RSP, vcpu->guest_state.gr_regs.rsp);

	return 0;
}


int nested_vm_exit_reflected(struct vmx_vcpu *vcpu, u32 exit_reason)
{
	switch(exit_reason) {
		case EXIT_REASON_EXTERNAL_INTERRUPT:
			return 0;
		case EXIT_REASON_EPT_VIOLATION:
			return 0;
		case EXIT_REASON_VMCALL:
		case EXIT_REASON_VMCLEAR:
		case EXIT_REASON_VMLAUNCH:
		case EXIT_REASON_VMPTRLD:
		case EXIT_REASON_VMPTRST:
		case EXIT_REASON_VMREAD:
		case EXIT_REASON_VMRESUME:
		case EXIT_REASON_VMWRITE:
		case EXIT_REASON_VMOFF:
		case EXIT_REASON_VMON:
		case EXIT_REASON_INVEPT:
		case EXIT_REASON_INVVPID:
			if (vcpu->guest_mode)
				return 1;
			else
				return 0;
			break;
		default:
			return 1;
	}
}

int nested_vmx_run(struct vmx_vcpu *vcpu)
{
	//printk("vmx:enter L2.\n");
	if (vcpu->shadow_vmcs_enabled)
		copy_shadow_to_vmcs12(vcpu);

	prepare_vmcs02(vcpu, vcpu->vmcs12);
	vcpu->guest_state.rip = vcpu->vmcs12->guest_rip;
	vcpu->guest_state.gr_regs.rsp = vcpu->vmcs12->guest_rsp;
	vcpu->guest_mode = 1;
	return 0;
}

int nested_vmx_reflect_vmexit(struct vmx_vcpu *vcpu, u32 exit_reason)
{
	//printk("reflect vmexit:reason = %x\n", exit_reason);
	nested_vmx_vmexit(vcpu, exit_reason, vmcs_read(VM_EXIT_INTR_INFO), vmcs_read(EXIT_QUALIFICATION));
}

