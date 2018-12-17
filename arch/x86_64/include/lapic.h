#ifndef _APIC_H
#define _APIC_H

#include <types.h>
#include <kernel.h>
#include <msr.h>
#include <cpuid.h>

/* Local APIC Macros: */

#define APIC_REG_BASE						0xFEE00000

#define APIC_REG_ID							0x20
#define APIC_REG_VER						0x30
#define APIC_REG_TPR						0x80
#define APIC_REG_APR						0x90
#define APIC_REG_PPR						0xA0
#define APIC_REG_EOI						0xB0
#define APIC_REG_RRD						0xC0
#define APIC_REG_LDR						0xD0
#define APIC_REG_DFR						0xE0
#define APIC_REG_SVR						0xF0
#define APIC_REG_ISR0						0x100
#define APIC_REG_ISR1						0x110
#define APIC_REG_ISR2						0x120
#define APIC_REG_ISR3						0x130
#define APIC_REG_ISR4						0x140
#define APIC_REG_ISR5						0x150
#define APIC_REG_ISR6						0x160
#define APIC_REG_ISR7						0x170
#define APIC_REG_TMR0						0x180
#define APIC_REG_TMR1						0x190
#define APIC_REG_TMR2						0x1A0
#define APIC_REG_TMR3						0x1B0
#define APIC_REG_TMR4						0x1C0
#define APIC_REG_TMR5						0x1D0
#define APIC_REG_TMR6						0x1E0
#define APIC_REG_TMR7						0x1F0
#define APIC_REG_IRR0						0x200
#define APIC_REG_IRR1						0x210
#define APIC_REG_IRR2						0x220
#define APIC_REG_IRR3						0x230
#define APIC_REG_IRR4						0x240
#define APIC_REG_IRR5						0x250
#define APIC_REG_IRR6						0x260
#define APIC_REG_IRR7						0x270
#define APIC_REG_ESR						0x280
#define APIC_REG_ICR0						0x300
#define APIC_REG_ICR1						0x310
#define APIC_REG_LVT_CMCI					0x2F0
#define APIC_REG_LVT_TIMER					0x320
#define APIC_REG_LVT_THERMAL				0x330
#define APIC_REG_LVT_PERFMON				0x340
#define APIC_REG_LVT_LINT0					0x350
#define APIC_REG_LVT_LINT1					0x360
#define APIC_REG_LVT_ERROR					0x370
#define APIC_REG_TIMER_ICR					0x380
#define APIC_REG_TIMER_CCR					0x390
#define APIC_REG_TIMER_DCR					0x3E0

/* LAPIC Timer */
#define	APIC_TIMER_ONE_SHOT					0x0
#define APIC_TIMER_PERIODIC					0x20000
#define APIC_TIMER_TSC						0x40000

/* Delivery Mode */
#define APIC_ICR_FIXED                       0x0000
#define APIC_ICR_LOWEST                      0x0100
#define APIC_ICR_SMI                         0x0200
#define APIC_ICR_NMI                         0x0400
#define APIC_ICR_INIT                        0x0500
#define APIC_ICR_STARTUP                     0x0600
#define APIC_ICR_EXTINT                      0x0700
#define APIC_ICR_FIXED_DELIVERY				 0x0000
#define APIC_ICR_LOWEST_DELIVERY             0x0100
#define APIC_ICR_SMI_DELIVERY                0x0200
#define APIC_ICR_NMI_DELIVERY                0x0400
#define APIC_ICR_EXTINT_DELIVERY             0x0700
#define APIC_ICR_INIT_DELIVERY               0x0500


#define APIC_ICR_MASKED						 0x10000
#define APIC_ICR_LOGICAL					 0x0800
#define APIC_ICR_PHYSICAL					 0x0000
#define APIC_ICR_EDGE						 0x0000
#define APIC_ICR_ASSERT						 0x4000
#define APIC_ICR_LEVEL						 0x8000
#define APIC_ICR_NOSHORTHAND				 0x00000
#define APIC_ICR_SELF						 0x40000
#define APIC_ICR_ALL_IN_SELF				 0x80000
#define APIC_ICR_ALL_EX_SELF				 0xC0000


/* IPI Method */

#define STARTUP_IPI                             0x000C4600
#define INIT_IPI                                0x000C4500

#define PHYSICAL_ID                             (APIC_ICR_ASSERT | APIC_ICR_EDGE | APIC_ICR_PHYSICAL | APIC_ICR_NOSHORTHAND | APIC_ICR_FIXED)
#define LOGICAL_ID                              (APIC_ICR_ASSERT | APIC_ICR_EDGE | APIC_ICR_LOGICAL | APIC_ICR_NOSHORTHAND | APIC_ICR_FIXED)

/*
 * Constants for Intel APIC based MSI messages.
 */

/*
 * Shifts for MSI data
 */

#define MSI_DATA_VECTOR_SHIFT		0
#define  MSI_DATA_VECTOR_MASK		0x000000ff
#define	 MSI_DATA_VECTOR(v)		(((v) << MSI_DATA_VECTOR_SHIFT) & \
					 MSI_DATA_VECTOR_MASK)

#define MSI_DATA_DELIVERY_MODE_SHIFT	8
#define  MSI_DATA_DELIVERY_FIXED	(0 << MSI_DATA_DELIVERY_MODE_SHIFT)
#define  MSI_DATA_DELIVERY_LOWPRI	(1 << MSI_DATA_DELIVERY_MODE_SHIFT)

#define MSI_DATA_LEVEL_SHIFT		14
#define	 MSI_DATA_LEVEL_DEASSERT	(0 << MSI_DATA_LEVEL_SHIFT)
#define	 MSI_DATA_LEVEL_ASSERT		(1 << MSI_DATA_LEVEL_SHIFT)

#define MSI_DATA_TRIGGER_SHIFT		15
#define  MSI_DATA_TRIGGER_EDGE		(0 << MSI_DATA_TRIGGER_SHIFT)
#define  MSI_DATA_TRIGGER_LEVEL		(1 << MSI_DATA_TRIGGER_SHIFT)

/*
 * Shift/mask fields for msi address
 */

#define MSI_ADDR_BASE_HI		0
#define MSI_ADDR_BASE_LO		0xfee00000

#define MSI_ADDR_DEST_MODE_SHIFT	2
#define  MSI_ADDR_DEST_MODE_PHYSICAL	(0 << MSI_ADDR_DEST_MODE_SHIFT)
#define	 MSI_ADDR_DEST_MODE_LOGICAL	(1 << MSI_ADDR_DEST_MODE_SHIFT)

#define MSI_ADDR_REDIRECTION_SHIFT	3
#define  MSI_ADDR_REDIRECTION_CPU	(0 << MSI_ADDR_REDIRECTION_SHIFT)
					/* dedicated cpu */
#define  MSI_ADDR_REDIRECTION_LOWPRI	(1 << MSI_ADDR_REDIRECTION_SHIFT)
					/* lowest priority */

#define MSI_ADDR_DEST_ID_SHIFT		12
#define	 MSI_ADDR_DEST_ID_MASK		0x00ffff0
#define  MSI_ADDR_DEST_ID(dest)		(((dest) << MSI_ADDR_DEST_ID_SHIFT) & \
					 MSI_ADDR_DEST_ID_MASK)
#define MSI_ADDR_EXT_DEST_ID(dest)	((dest) & 0xffffff00)

#define MSI_ADDR_IR_EXT_INT		(1 << 4)
#define MSI_ADDR_IR_SHV			(1 << 3)
#define MSI_ADDR_IR_INDEX1(index)	((index & 0x8000) >> 13)
#define MSI_ADDR_IR_INDEX2(index)	((index & 0x7fff) << 5)

inline static u32 lapic_reg_read32(u32 offset)
{
	volatile u32 *reg = (u32 *)PHYS2VIRT(APIC_REG_BASE + offset);
	return *reg;
}

inline static u32 lapic_reg_write32(u32 offset, u32 value)
{
	volatile u32 *reg = (u32 *)PHYS2VIRT(APIC_REG_BASE + offset);
	*reg = value;
}

inline static bool is_bsp()
{
	u64 value = rdmsr(MSR_IA32_APICBASE);
	return (value >> 8) & 0x1;
}

inline static bool x2_apic_supported()
{
	u32 regs[4] = {0};
	cpuid(0x00000001, 0x00000000, regs);
	if (regs[2] & (BIT21) == 1)
		return true;
	return false;
}

inline static void lapic_enable()
{
	/* Enable global APIC. */
	u64 value = rdmsr(MSR_IA32_APICBASE);
	value |= BIT11;
	if (x2_apic_supported() == true)
		value |= BIT10;
	wrmsr(MSR_IA32_APICBASE, value);

	/* Software enable APIC. */
	value = lapic_reg_read32(APIC_REG_SVR);
	value |= BIT8;
	lapic_reg_write32(APIC_REG_SVR, value);
}

inline static void apic_base_set(u32 base)
{
	u64 value = rdmsr(MSR_IA32_APICBASE);

	value &= (BIT8 | BIT10 | BIT11);
	value |= (base & (SIZE_4KB - 1));

	wrmsr(MSR_IA32_APICBASE, value);
}

inline static void lapic_send_ipi(u8 apic_id, u8 vector, u32 attr)
{
	u32 dest = (apic_id << 24);
	lapic_reg_write32(APIC_REG_ICR1, dest);
	lapic_reg_write32(APIC_REG_ICR0, vector | attr);
}

/* ap_startup_addr should below 1MB. */
inline static void mp_init_single(u8 apic_id, u64 ap_startup_addr)
{
	lapic_send_ipi(apic_id, 0, APIC_ICR_ASSERT | APIC_ICR_INIT);
	lapic_send_ipi(apic_id, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP);
	lapic_send_ipi(apic_id, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP);
}

inline static void mp_init_all(u64 ap_startup_addr)
{
	lapic_send_ipi(0, 0, APIC_ICR_ASSERT | APIC_ICR_INIT | APIC_ICR_ALL_EX_SELF);
	lapic_send_ipi(0, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP | APIC_ICR_ALL_EX_SELF);
	lapic_send_ipi(0, (ap_startup_addr >> 12), APIC_ICR_ASSERT | APIC_ICR_STARTUP | APIC_ICR_ALL_EX_SELF);
}


#endif
