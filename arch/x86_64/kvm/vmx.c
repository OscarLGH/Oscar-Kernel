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
	u64 msr;
	u64 vmx_msr;
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

struct vmx_vcpu *vmx_preinit()
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
	vmptr_load(vcpu_ptr->vmcs_phys);
	return vcpu_ptr;
}

int vmx_init(struct vmx_vcpu * vcpu)
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
		| PIN_BASED_POSTED_INTR;
	if ((pin_based_vm_exec_ctrl | pin_based_allow1_mask) != pin_based_allow1_mask) {
		printk("Warning:setting pin_based_vm_exec_control:%x unsupported.\n", 
			(pin_based_vm_exec_ctrl & pin_based_allow1_mask) ^ pin_based_vm_exec_ctrl);
		pin_based_vm_exec_ctrl &= pin_based_allow1_mask;
	}
	vmcs_write(PIN_BASED_VM_EXEC_CONTROL, pin_based_vm_exec_ctrl);

	cpu_based_vm_exec_ctrl = CPU_BASED_FIXED_ONES
		| CPU_BASED_ACTIVATE_SECONDARY_CONTROLS
		| CPU_BASED_USE_IO_BITMAPS
		| CPU_BASED_USE_MSR_BITMAPS
		| CPU_BASED_TPR_SHADOW
		//| CPU_BASED_MONITOR_TRAP_FLAG
		;
	if ((cpu_based_vm_exec_ctrl | cpu_based_allow1_mask) != cpu_based_allow1_mask) {
		printk("Warning:setting cpu_based_vm_exec_control:%x unsupported.\n", 
			(cpu_based_vm_exec_ctrl & cpu_based_allow1_mask) ^ cpu_based_vm_exec_ctrl);
		cpu_based_vm_exec_ctrl &= cpu_based_allow1_mask;
	}
	vmcs_write(CPU_BASED_VM_EXEC_CONTROL, cpu_based_vm_exec_ctrl);

	cpu_based_vm_exec_ctrl2 = SECONDARY_EXEC_UNRESTRICTED_GUEST
		| SECONDARY_EXEC_ENABLE_EPT
		| SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES
		| SECONDARY_EXEC_VIRTUAL_INTR_DELIVERY
		| SECONDARY_EXEC_APIC_REGISTER_VIRT
		;
	if ((cpu_based_vm_exec_ctrl2 | cpu_based2_allow1_mask) != cpu_based2_allow1_mask) {
		printk("Warning:setting secondary_vm_exec_control:%x unsupported.\n", 
			(cpu_based_vm_exec_ctrl2 & cpu_based2_allow1_mask) ^ cpu_based_vm_exec_ctrl2);
		cpu_based_vm_exec_ctrl2 &= cpu_based_vm_exec_ctrl2;
	}
	vmcs_write(SECONDARY_VM_EXEC_CONTROL, cpu_based_vm_exec_ctrl);

	vm_entry_ctrl = VM_ENTRY_ALWAYSON_WITHOUT_TRUE_MSR
		| VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL
		| VM_ENTRY_LOAD_IA32_EFER;
	if ((vm_entry_ctrl | vm_entry_allow1_mask) != vm_entry_allow1_mask) {
		printk("Warning:setting vm_entry_controls:%x unsupported.\n", 
			(vm_entry_ctrl & vm_entry_allow1_mask) ^ vm_entry_ctrl);
		vm_entry_ctrl &= vm_entry_allow1_mask;
	}
	vmcs_write(VM_ENTRY_CONTROLS, vm_entry_ctrl);

	vm_exit_ctrl = VM_EXIT_ALWAYSON_WITHOUT_TRUE_MSR
		| VM_EXIT_SAVE_IA32_EFER
		| VM_EXIT_LOAD_IA32_EFER
		| VM_EXIT_ACK_INTR_ON_EXIT
		;
	if ((vm_exit_ctrl | vm_exit_allow1_mask) != vm_exit_allow1_mask) {
		printk("Warning:setting vm_exit_controls:%x unsupported.\n", 
			(vm_exit_ctrl & vm_exit_allow1_mask) ^ vm_exit_ctrl);
		vm_exit_ctrl &= vm_exit_allow1_mask;
	}
	vmcs_write(VM_EXIT_CONTROLS, vm_exit_ctrl);

	return 0;
}

void vm_init_test()
{
	struct vmx_vcpu *vcpu = vmx_preinit();
	vmx_init(vcpu);
}
