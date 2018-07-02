#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <types.h>

#pragma pack(1)

struct segment_desc {
	u16	limit_0_15;
	u16 base_0_15;
	u8  base16_23;
	u8  attr1;
	u8	limit16_19_attr2;
	u8	base24_31;
	u32 base32_63;
	u32 reserved;
};

/* Segment descriptor attributes1: */

/* Non-system segment:	*/                                               
#define CS_C    0x9c	/* Conforming Code Segment */
#define CS_NC   0x98	/* Non conforming Code Segment */
#define DS_S_W  0x92	/* Data Segment, Stack Segment */
#define	DS_S_NW 0x90 	/* Data Segment, not available for Stack */
/* System segment */
#define S_TSS	0x89  	/*TSS Descriptor*/
#define S_LDT	0x82  	/*LDT Descriptor*/

/* Descriptor Privilege Level */
#define DPL0	0x0
#define DPL1	0x20
#define DPL2	0x40
#define DPL3	0x60

/* Segment descriptor attributes2: */

#define CS_L	0x20　/* LongMode */
#define CS_C_32	0x40　/* Compatibility mode，32-bit oprend */
#define CS_C_16 0x00　/* Compatibility mode，16-bit oprend */

struct gate_desc {
	u16	offset0_15;
	u16 selector;
	u8	ist;
	u8	attr;
	u16 offset16_31;
	u32 offset32_63;
	u32 reserved;
};

struct tss_desc {
	u32 reserved0;
	u64 rsp0;
	u64 rsp1;
	u64 rsp2;
	u64 reserved1;
	u64 ist1;
	u64 ist2;
	u64 ist3;
	u64 ist4;
	u64 ist5;
	u64 ist6;
	u64 ist7;
	u64 reserved2;
	u16 reserved3;
	u16 iomap_base;
};

struct gdtr {
	u16 limit;
	u64 base;
};

struct idtr {
	u16 limit;
	u64 base;
};

#pragma pack()


//Descriptor index used by kernel
#define SELECTOR_NULL_INDEX			0
#define SELECTOR_KERNEL_CODE_INDEX	1
#define SELECTOR_KERNEL_DATA_INDEX	2
#define SELECTOR_USER_CODE_INDEX	3
#define SELECTOR_USER_DATA_INDEX	4
#define SELECTOR_TSS_INDEX			64

#define SELECTOR_NULL			(SELECTOR_NULL_INDEX*16)
#define SELECTOR_KERNEL_CODE	(SELECTOR_KERNEL_CODE_INDEX*16)
#define SELECTOR_KERNEL_DATA	(SELECTOR_KERNEL_DATA_INDEX*16)
#define SELECTOR_USER_CODE		(SELECTOR_USER_CODE_INDEX*16)
#define SELECTOR_USER_DATA		(SELECTOR_USER_DATA_INDEX*16)
#define SELECTOR_TSS			(SELECTOR_TSS_INDEX*16)

static inline void set_gate_descriptor(
	struct gate_desc *idt,
	u8	vector,
	u16 selector,
	u64 offset,
	u8 ist,
	u8 attr
)
{
	idt[vector].selector = selector;
	idt[vector].offset0_15 = offset;
	idt[vector].offset16_31 = offset >> 16;
	idt[vector].offset32_63 = offset >> 32;
	idt[vector].ist = ist;
	idt[vector].attr = attr;
	idt[vector].reserved = 0;
}

static inline void set_segment_descriptor(
	struct segment_desc *gdt,
	u16 index,
	u64 base,
	u32 limit,
	u8 attr1,
	u8 attr2
)
{
	gdt[index].base_0_15 = base;
	gdt[index].base16_23 = base >> 16;
	gdt[index].base24_31 = base >>24;
	gdt[index].base32_63 = base >> 32;
	gdt[index].limit_0_15 = limit;
	gdt[index].limit16_19_attr2 = ((limit >> 16) & 0xf) | (attr2 & 0xf0);
	gdt[index].attr1 = attr1;
	gdt[index].reserved = 0;
}

#endif
