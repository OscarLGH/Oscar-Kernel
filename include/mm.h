#ifndef _MM_H
#define _MM_H

#include <types.h>

struct node {
	u64 socket;
	//list_head *zone_list;
};

struct zone {
	//list_head list;
	u64 start_pfn;
	u64 end_pfn;
	
};

struct page {
	
};

#endif