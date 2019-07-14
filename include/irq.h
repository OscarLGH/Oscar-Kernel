#ifndef _IRQ_H
#define _IRQ_H

#include <types.h>

struct irq_chip {
	char *name;
	int (*startup)();
	int (*shutdown)();
	int (*enable)();
	int (*disable)();
	int (*ack)(int irq);
	int (*mask)(int irq);
	int (*eoi)();
};

struct irq_action {
	void *data;
	char *name;
	int (*irq_handler)(int irq, void *data);
	struct list_head list;
};

typedef int (*irq_handler_t)(int irq, void *data);
struct irq_desc {
	int count;
	char *name;
	struct irq_chip *chip;
	struct list_head irq_action_list;
};


int alloc_irqs_cpu(int cpu, int nr_irq);
int alloc_irqs(int *cpu, int nr_irq, int align);


int free_irq(int cpu, int irq);
int alloc_irq_from_smp(int *cpu);

int request_irq(int irq, irq_handler_t handler, unsigned long flags, char *name, void *dev);

struct cpu;
int request_irq_smp(struct cpu *cpu_ptr, int irq, irq_handler_t handler, unsigned long flags, char *name, void *dev);

struct irq_affinity {
	int	pre_vectors;
	int	post_vectors;
};

struct msi_data {
	u64 addr;
	u64 data;
};

#endif
