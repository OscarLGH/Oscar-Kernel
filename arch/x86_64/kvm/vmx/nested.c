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
	//vmclear(vmcs12_phys);
	printk("VM-Exit:VMCLEAR.Region:0x%x\n", *virt);
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}


int vmx_handle_vmlaunch(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMLAUNCH.\n");
	while (1);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmresume(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMRESUME.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
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
	u32 field = kvm_reg_read(vcpu, base_reg);
	int ret;
	u64 value;

	vmclear(vcpu->vmcs01_phys);
	vcpu->state = 0;
	vmcs12_read_any(vcpu->vmcs12, field, &value);
	printk("VM-Exit:VMREAD.field:0x%x, value:0x%x\n", field, value);
	kvm_reg_write(vcpu, reg2, value);

	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmwrite(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u32 field = kvm_reg_read(vcpu, reg2);
	u32 value = kvm_reg_read(vcpu, base_reg);
	int ret;

	printk("VM-Exit:VMWRITE.field:0x%x, value:0x%x\n", field, value);
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

	printk("VM-Exit:INVEPT.reg2:%d(0x%x) eptp = %x\n",
		reg2,
		kvm_reg_read(vcpu, reg2),
		invept_context->eptp);
	ret = ept_gpa_to_hpa(vcpu, invept_context->eptp, &eptp_hpa);
	if (ret) {
		printk("transfer gpa to hpa failed.\n");
		return 0;
	}
	invept(kvm_reg_read(vcpu, reg2), eptp_hpa, 0);
	vmclear(vcpu->vmcs02_phys);
	vmptr_load(vcpu->vmcs01_phys);
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int nested_vmx_run(struct vmx_vcpu *vcpu)
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
	}
}

