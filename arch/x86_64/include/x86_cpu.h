#ifndef _X86_CPU_H
#define _X86_CPU_H

#include <segment.h>
#include <fpu.h>
#include <list.h>

struct X86_cpu {
	char name[64];
	u64 apic_id;
	u64 online;
	u64 features[8];
	u64 *page_table;
	u64 *kernel_stack;
	struct gdtr *gdtr_base;
	struct idtr *idtr_base;
	struct tss_desc *tss_base;

	struct list_head list;
	u64 spin_lock;
};

#endif