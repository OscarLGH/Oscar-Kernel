#ifndef _MM_H
#define _MM_H

#include <types.h>
#include <kernel.h>
#include <list.h>

struct node {
	u64 socket;
	struct list_head list;
};

struct zone {
	struct list_head list;
	u64 page_size;
	u64 start_pfn;
	u64 pfn_cnt;
	
};

struct page {
	
};

void *boot_mem_alloc(u64 size);

#endif