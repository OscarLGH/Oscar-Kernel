#include <lapic.h>

u64 lapic_reg_read32_mmio(u32 offset)
{
        volatile u32 *reg = (u32 *)PHYS2VIRT(APIC_REG_BASE + offset);
        return *reg;
}

void lapic_reg_write32_mmio(u32 offset, u64 value)
{
        volatile u32 *reg = (u32 *)PHYS2VIRT(APIC_REG_BASE + offset);
        *reg = value;
}

u64 lapic_reg_read64_msr(u32 offset)
{
        u64 value = rdmsr(MSR_APIC_BASE + (offset >> 4));
        return value;
}

void lapic_reg_write64_msr(u32 offset, u64 value)
{
        wrmsr(MSR_APIC_BASE + (offset >> 4), value);
}

void lapic_enable(struct lapic *lapic)
{
        /* Enable global APIC. */
        u64 value = rdmsr(MSR_IA32_APICBASE);
        value |= BIT11;
        lapic->mode = 1;
        if (x2_apic_supported() == true) {
		lapic->mode = 2;
                value |= BIT10;
	}
        wrmsr(MSR_IA32_APICBASE, value);
        if (lapic->mode == 2) {
		lapic->reg_read = lapic_reg_read64_msr;
		lapic->reg_write = lapic_reg_write64_msr;
	} else {
		lapic->reg_read = lapic_reg_read32_mmio;
		lapic->reg_write = lapic_reg_write32_mmio;
	}

        /* Software enable APIC. */
        /* some old version kvm (3.10.0) fail to emulate APIC_REG_SVR. */
        value = lapic_reg_read(APIC_REG_SVR);
        value |= BIT8;
        lapic_reg_write(APIC_REG_SVR, value);
}

u64 lapic_reg_read(u32 offset)
{
	struct x86_cpu *x86_cpu_ptr = get_cpu()->arch_data;
	return x86_cpu_ptr->lapic_ops->reg_read(offset);
}

void lapic_reg_write(u32 offset, u64 value)
{
	struct x86_cpu *x86_cpu_ptr = get_cpu()->arch_data;
	x86_cpu_ptr->lapic_ops->reg_write(offset, value);
}

void lapic_send_ipi(u8 apic_id, u8 vector, u32 attr)
{
	struct x86_cpu *x86_cpu_ptr = get_cpu()->arch_data;
	u64 dest = (apic_id << 24);

	if (x86_cpu_ptr->lapic_ops->mode == 2) {
		lapic_reg_write(APIC_REG_ICR0, ((u64)apic_id << 32) | vector | attr);
	} else {
		lapic_reg_write(APIC_REG_ICR1, dest);
		lapic_reg_write(APIC_REG_ICR0, vector | attr);
	}
}

/* ap_startup_addr should below 1MB. */
void mp_init_single(u8 apic_id, u64 ap_startup_addr)
{
	lapic_send_ipi(apic_id, 0, APIC_ICR_ASSERT | APIC_ICR_INIT);
	lapic_send_ipi(apic_id, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP);
	lapic_send_ipi(apic_id, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP);
}

void mp_init_all(u64 ap_startup_addr)
{
	lapic_send_ipi(0, 0, APIC_ICR_ASSERT | APIC_ICR_INIT | APIC_ICR_ALL_EX_SELF);
	lapic_send_ipi(0, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP | APIC_ICR_ALL_EX_SELF);
	lapic_send_ipi(0, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP | APIC_ICR_ALL_EX_SELF);
}

int lapic_set_timer(u64 freq, u8 vector)
{
	u64 tsc_freq, crystal_freq;
	u32 buffer1[4] = {0};
	//u32 buffer2[4] = {0};
	write_cr8(0);
	cpuid(0x15, 0, buffer1);
	//printk("cpuid 0x15:eax = %x ebx = %x ecx = %x\n", buffer1[0], buffer1[1], buffer1[2]);
	//default_freq = buffer[1] / buffer[0];
	//printk("default freq:%dHz\n", buffer[1] / buffer[0]);
	//cpuid(0x16, 0, buffer2);
	//printk("cpuid 0x16:eax = %x ebx = %x ecx = %x\n", buffer2[0], buffer2[1], buffer2[2]);
	//printk("bus freq:%dMHz\n", buffer[2]);

	/* some cpu doesn't report crystal freq by cpuid */
	if (buffer1[2] == 0)
		buffer1[2] = 24000000;
	if (buffer1[0] == 0)
		buffer1[0] = 1;

	if (buffer1[0] == 0)
		buffer1[0] = 1;

	crystal_freq = buffer1[2];
	tsc_freq = buffer1[2] * buffer1[1] / buffer1[0];
	printk("tsc freq = %d\n", tsc_freq);
	lapic_reg_write(APIC_REG_TIMER_DCR, 0xb);

	lapic_reg_write(APIC_REG_LVT_TIMER, 0x20000 | vector);
	lapic_reg_write(APIC_REG_TIMER_ICR, crystal_freq / freq);

	return 0;
}

