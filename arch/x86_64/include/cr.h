#ifndef _CR_H
#define _CR_H

#include <types.h>

/* Control Register bits.*/
#define CR0_PE BIT0
#define CR0_MP BIT1
#define CR0_EM BIT2
#define CR0_TS BIT3
#define CR0_ET BIT4
#define CR0_NE BIT5
#define CR0_WP BIT16
#define CR0_AM BIT18
#define CR0_NW BIT29
#define CR0_CD BIT30
#define CR0_PG BIT31

#define CR3_MASK ~(0xffffULL)
#define CR3_PWT BIT3
#define CR3_PCD BIT4

#define CR4_VME BIT0
#define CR4_PVI BIT1
#define CR4_TSD BIT2
#define CR4_DE BIT3
#define CR4_PSE BIT4
#define CR4_PAE BIT5
#define CR4_MCE BIT6
#define CR4_PGE BIT7
#define CR4_PCE BIT8
#define CR4_OSFXSR BIT9
#define CR4_OSXMMEXCPT BIT10
#define CR4_UMIP BIT11
#define CR4_VMXE BIT13
#define CR4_SMXE BIT14
#define CR4_FSGSBASE BIT16
#define CR4_PCIDE BIT17
#define CR4_OSXSAVE BIT18
#define CR4_SMEP BIT20
#define CR4_SMAP BIT21
#define CR4_PKE BIT22


static inline u64 read_cr8()
{
	u64 cr8;
	asm volatile("movq %%cr8, %0\n\t"
				: "=r"(cr8) :
		);
	return cr8;
}

static inline void write_cr8(u64 cr8)
{
	asm volatile("movq %0, %%cr8\n\t"
				: : "rdi"(cr8)
		);
}

static inline u64 read_cr4()
{
	u64 cr4;
	asm volatile("movq %%cr4, %0\n\t"
				: "=r"(cr4) :
		);
	return cr4;
}

static inline void write_cr4(u64 cr4)
{
	asm volatile("movq %0, %%cr4\n\t"
				: : "rdi"(cr4)
		);
}

static inline void cr4_set_bits(u64 bits)
{
	u64 cr4 = read_cr4();
	cr4 |= bits;
	write_cr4(cr4);
}

static inline void cr4_clear_bits(u64 bits)
{
	u64 cr4 = read_cr4();
	cr4 &= ~bits;
	write_cr4(cr4);
}

static inline u64 read_cr3()
{
	u64 cr3;
	asm volatile("movq %%cr3, %0\n\t"
				: "=r"(cr3) :
		);
	return cr3;
}

static inline void write_cr3(u64 cr3)
{
	asm volatile("movq %0, %%cr3\n\t"
				: : "rdi"(cr3)
		);
}

static inline u64 read_cr2()
{
	u64 cr2;
	asm volatile("movq %%cr2, %0\n\t"
				: "=r"(cr2) :
		);
	return cr2;
}

static inline u64 read_cr0()
{
	u64 cr0;
	asm volatile("movq %%cr0, %0\n\t"
				: "=r"(cr0) :
		);
	return cr0;
}

static inline void write_cr0(u64 cr0)
{
	asm volatile("movq %0, %%cr0\n\t"
				: : "rdi"(cr0)
		);
}

static inline void cr0_set_bits(u64 bits)
{
	u64 cr0 = read_cr0();
	cr0 |= bits;
	write_cr0(cr0);
}

static inline void cr0_clear_bits(u64 bits)
{
	u64 cr0 = read_cr0();
	cr0 &= ~bits;
	write_cr0(cr0);
}



#endif

