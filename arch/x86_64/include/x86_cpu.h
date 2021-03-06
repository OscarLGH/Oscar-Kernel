#ifndef _X86_CPU_H
#define _X86_CPU_H

#include <segment.h>
#include <fpu.h>
#include <list.h>
#include <lapic.h>

struct general_regs {
	u64 rax;
	u64 rbx;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;
	u64 rsp;
	u64 rbp;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;
}; 

struct x86_cpu {
	char name[64];
	u64 apic_id;
	u64 online;
	u64 features[8];
	u64 *page_table;
	u64 *kernel_stack;
	struct segment_desc *gdt_base;
	struct gate_desc *idt_base;
	struct tss_desc *tss;
	struct lapic *lapic_ops;
	struct list_head list;
	u64 spin_lock;
};

void soft_irq_call(u8 vector);
#endif
