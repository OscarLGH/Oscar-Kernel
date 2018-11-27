#ifndef _CPU_H_
#define _CPU_H_

#include <types.h>
#include <list.h>
#include <mm.h>

struct cpu {
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
	struct irq_desc *irq_desc;
	struct list_head list;
};

int register_cpu_local(struct node *node);
int register_cpu_remote(u64 id, struct node *node);
struct cpu *get_cpu();

#endif