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

int vmx_handle_vmxon(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	u64 gva;
	u64 gpa;
	u64 hpa;
	u64 *virt;
	int ret;

	gva = kvm_reg_read(vcpu, base_reg);
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
	printk("VM-Exit:VMCLEAR.\n");
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
	int base_reg = (instruction_info >> 23) & 0xf;
	u64 gva;
	u64 gpa;
	u64 hpa;
	u64 *virt;
	int ret;

	gva = kvm_reg_read(vcpu, base_reg);
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
	printk("VM-Exit:VMPTRLD.Region:0x%x\n", *virt);

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmptrst(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	u64 gva;
	u64 gpa;
	u64 hpa;
	u64 *virt;
	int ret;

	gva = kvm_reg_read(vcpu, base_reg);
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

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmread(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u64 gva;
	u64 gpa;
	u64 hpa;
	u64 *virt;
	int ret;
	printk("VM-Exit:VMREAD.base reg:%d(0x%x), reg2:%d(0x%x)\n",
		base_reg,
		kvm_reg_read(vcpu, base_reg),
		reg2,
		kvm_reg_read(vcpu, reg2));

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmwrite(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u64 gva;
	u64 gpa;
	u64 hpa;
	u64 *virt;
	int ret;
	printk("VM-Exit:VMWRITE.base reg:%d(0x%x), reg2:%d(0x%x)\n",
		base_reg,
		kvm_reg_read(vcpu, base_reg),
		reg2,
		kvm_reg_read(vcpu, reg2));

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_invept(struct vmx_vcpu *vcpu)
{
	u32 instruction_info = vmcs_read(VMX_INSTRUCTION_INFO);
	u64 exit_qualification = vmcs_read(EXIT_QUALIFICATION);
	int base_reg = (instruction_info >> 23) & 0xf;
	int reg2 = (instruction_info >> 28) & 0xf;
	u64 gva;
	u64 gpa;
	u64 hpa;
	u64 *virt;
	int ret;
	printk("VM-Exit:INVEPT.base reg:%d(0x%x), reg2:%d(0x%x)\n",
		base_reg,
		kvm_reg_read(vcpu, base_reg),
		reg2,
		kvm_reg_read(vcpu, reg2));

	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}
