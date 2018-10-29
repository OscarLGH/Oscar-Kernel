#include <vmx.h>
#include <cpuid.h>
#include <paging.h>
#include <msr.h>
#include <cr.h>
#include <kernel.h>
#include <string.h>
#include <mm.h>

int vmx_enable()
{
	u32 regs[4] = {0};
	u32 msr;
	u32 vmx_msr;
	cpuid(0x00000001, 0x00000000, &regs[0]);
	if (regs[2] & CPUID_FEATURES_ECX_VMX == 0) {
		printk("VMX is unsupported on this CPU!.\n");
		return -1;
	}

	msr = rdmsr(MSR_IA32_FEATURE_CONTROL);
	vmx_msr = FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX | FEATURE_CONTROL_LOCKED;
	if (msr & vmx_msr != vmx_msr) {
		printk("VMX is supported, but BIOS disabled VMX.\n");
		return -2;
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

struct vmx_vcpu *vmx_init()
{
	int ret;
	struct vmx_vcpu *vcpu_ptr = bootmem_alloc(sizeof(*vcpu_ptr));
	vcpu_ptr->vmxon_region = bootmem_alloc(0x1000);
	vcpu_ptr->vmxon_region_phys = VIRT2PHYS(vcpu_ptr->vmxon_region);
	vcpu_ptr->vmcs = bootmem_alloc(0x1000);
	vcpu_ptr->vmcs_phys = VIRT2PHYS(vcpu_ptr->vmcs);

	ret = vmx_enable();
	if (ret) {
		return NULL;
	}

	vcpu_ptr->vmxon_region->revision_id = rdmsr(MSR_IA32_VMX_BASIC);
	vcpu_ptr->vmcs->revision_id = rdmsr(MSR_IA32_VMX_BASIC);
	vmx_on(vcpu_ptr->vmxon_region_phys);
	return vcpu_ptr;
}
