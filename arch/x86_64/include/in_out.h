#ifndef _IN_OUT_H_
#define _IN_OUT_H_

#include <types.h>

static inline u8 in8(u16 port)
{
	u8 data;
	asm volatile("inb %%dx, %%al\n\t"
				: "=rax"(data) : "rdx"(port)
		);
	return data;
}

static inline u16 in16(u16 port)
{
	u16 data;
	asm volatile("inw %%dx, %%ax\n\t"
				: "=rax"(data) : "dx"(port)
		);
	return data;
}

static inline u32 in32(u16 port)
{
	u32 data;
	asm volatile("inl %%dx, %%eax\n\t"
				: "=rax"(data) : "dx"(port)
		);
	return data;
}

static inline void out8(u16 port, u8 data)
{
	asm volatile("outb %%al, %%dx\n\t"
				:: "al"(data), "dx"(port)
		);
}

static inline void out16(u16 port, u16 data)
{
	asm volatile("outw %%ax, %%dx\n\t"
				:: "ax"(data), "dx"(port)
		);
}

static inline void out32(u16 port, u32 data)
{
	asm volatile("outl %%eax, %%dx\n\t"
				:: "eax"(data), "dx"(port)
		);
}

#endif