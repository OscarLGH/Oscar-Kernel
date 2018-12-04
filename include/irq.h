#ifndef _IRQ_H
#define _IRQ_H

#include <types.h>
#include <cpu.h>

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

#endif