#ifndef _STACK_H
#define _STACK_H

#include <types.h>
#include <x86_cpu.h>

struct exception_stack {
	u64 error_code;
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
};

struct irq_stack {
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
};

#pragma pack(1)
struct exception_stack_frame {
	struct general_regs g_regs;
	struct exception_stack excep_stack;
};
#pragma pack(0)

struct irq_stack_frame {
	struct general_regs g_regs;
	struct irq_stack irq_stack;
};

static inline void load_sp(u64 rsp)
{
	asm volatile("mov %rdi, %rsp\n\t");
	asm volatile("mov %rdi, %rbp\n\t");
}

static inline u64 save_sp()
{
	asm volatile("mov %rsp, %rax\n\t");
}

static inline u64 get_sp_in_func()
{
	asm volatile("mov %rbp, %rax\n\t");
}

#endif