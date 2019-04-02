#ifndef _CPU_H_
#define _CPU_H_

#include <types.h>
#include <list.h>
#include <mm.h>
#include <irq.h>
#include <bitmap.h>

#define MAX_EXCEPTIONS 512
extern u64 nr_cpus;

struct intr_desc {
	int irq_nr;
	struct bitmap *irq_bitmap;
	struct irq_desc *irq_desc;
};

struct cpu {
	u64 index;
	u64 id;
	u64 domain;
	u64 status;
	u64 features;
	u64 default_freq;
	u64 phys_addr_bits;
	u64 virt_addr_bits;
	u64 page_level;
	u64 nr_irq;
	void *arch_data;
	void *kernel_stack;
	void *irq_stack;
	u64 tmp_stack;
	struct intr_desc intr_desc;
	struct list_head list;
};

#define CPU_STATUS_OFFLINE 0
#define CPU_STATUS_IDLE 1
#define CPU_STATUS_PROCESS_CONTEXT 2
#define CPU_STATUS_IRQ_CONTEXT 3
#define CPU_STATUS_EXCEPTION_CONTEXT 4


int register_cpu_local(struct node *node);
int register_cpu_remote(u64 id, struct node *node);
struct cpu *get_cpu();
u64 get_irq_stack();
void save_tmp_stack(u64 sp);
u64 get_tmp_stack();

void arch_cpu_halt();

#endif