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
	printk("VM-Exit:VMXON.\n");
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
	printk("VM-Exit:VMPTRLD.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmptrst(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMPTRST.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmread(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMREAD.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}

int vmx_handle_vmwrite(struct vmx_vcpu *vcpu)
{
	printk("VM-Exit:VMWRITE.\n");
	vcpu->guest_state.rip += vmcs_read(VM_EXIT_INSTRUCTION_LEN);
	return 0;
}
