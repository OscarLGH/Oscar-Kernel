#ifndef _IRQ_H
#define _IRQ_H

#include <types.h>
#include <cpu.h>

#define MAX_NR_IRQ 512

struct irq_desc {
	u64 nr_irqs;
	u512 irq_bitmap;
	u64 (*irq_handler[MAX_NR_IRQ])();
};

#endif