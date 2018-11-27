#ifndef _ARCH_H_
#define _ARCH_H_

#include <types.h>
#include <cpuid.h>
#include <paging.h>
#include <segment.h>

#define PERCPU_AREA_BASE 0x2000000
#define PERCPU_AREA_SIZE 0x8000

struct x86_cpu {
	struct segment_desc *gdt_base;
	struct gate_desc *idt_base;
	struct tss_desc *tss;
	u64 *pml4t_base;
};


inline static u64 get_percpu_area_base()
{
	u8 regs[16];
	u8 cpu_id;
	cpuid(0x00000001, 0x00000000, (u32 *)&regs[0]);
	cpu_id = regs[7];
	return (PERCPU_AREA_BASE + cpu_id * PERCPU_AREA_SIZE);
}


#endif