#ifndef _CPU_H_
#define _CPU_H_

#include <types.h>
#include <list.h>

struct cpu {
	u64 id;
	u64 features;
	u64 default_freq;
	u64 phys_addr_bits;
	u64 virt_addr_bits;
	u64 page_level;
	u64 nr_irq;
	u64 reserved;
	struct list_head list;
};

#endif