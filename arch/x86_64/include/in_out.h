#ifndef _IN_OUT_H_
#define _IN_OUT_H_

#include <types.h>

static inline u8 in8(u64 port)
{
	u8 data;
	asm volatile("inb %%dx, %%al\n\t"
				: "=a"(data) : "d"(port)
		);
	return data;
}

static inline u16 in16(u64 port)
{
	u16 data;
	asm volatile("inw %%dx, %%ax\n\t"
				: "=a"(data) : "d"(port)
		);
	return data;
}

static inline u32 in32(u64 port)
{
	u32 data;
	asm volatile("inl %%dx, %%eax\n\t"
				: "=a"(data) : "d"(port)
		);
	return data;
}

static inline void out8(u64 port, u8 data)
{
	asm volatile("outb %%al, %%dx\n\t"
				:: "a"(data), "d"(port)
		);
}

static inline void out16(u64 port, u16 data)
{
	asm volatile("outw %%ax, %%dx\n\t"
				:: "a"(data), "d"(port)
		);
}

static inline void out32(u64 port, u32 data)
{
	asm volatile("outl %%eax, %%dx\n\t"
				:: "a"(data), "d"(port)
		);
}

#endif