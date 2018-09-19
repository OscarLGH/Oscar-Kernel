#ifndef _MM_H
#define _MM_H

#include <types.h>
#include <list.h>

struct node {
	u64 socket;
	struct list_head zone_list;
};

struct zone {
	struct list_head list;
	u64 start_pfn;
	u64 end_pfn;
	
};

struct page {
	
};

#endif