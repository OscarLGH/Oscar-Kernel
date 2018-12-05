#ifndef _STACK_H
#define _STACK_H

#include <types.h>

static inline void load_sp(u64 rsp)
{
	asm volatile("mov %rdi, %rsp\n\t");
}

static inline u64 save_sp()
{
	asm volatile("mov %rsp, %rax\n\t");
}

#endif