#ifndef _MM_H
#define _MM_H

#include <types.h>
#include <kernel.h>
#include <list.h>
#include <spinlock.h>

#define MIGRATE_UNMOVABLE 0
#define MIGRATE_RECLAIMABLE 1
#define MIGRATE_MOVABLE 2
#define MIGRATE_RESERVE 3
#define MIGRAGE_ISOLATE 4
#define MIGRATE_TYPES 5

#define MAX_ORDER 18

struct node {
	u64 socket_id;
	struct list_head list;
	struct list_head zone_list;
	struct list_head cpu_list;
};

struct free_area {
	struct list_head free_list[MIGRATE_TYPES];
	u64 nr_free;
};

struct zone {
	struct list_head list;
	u64 page_size;
	u64 start_pfn;
	u64 pfn_cnt;
	struct free_area free_areas[MAX_ORDER];
};

struct page {
	struct list_head list;
	u64 pfn;
	u64 attribute;
};

struct bootmem {
	u8 *bitmap;
	u64 last_alloc_offset;
	u64 last_alloc_pfn;
	spin_lock_t spin_lock;
};

struct mm_struct {
	u64 *pgd;
};

extern struct list_head node_list;

#define GFP_KERNEL 0

struct page *allocate_page(u64 area);
struct page *allocate_pages();
void bootmem_free(void *vaddr);
void bootmem_init();
void *bootmem_alloc(u64 size);
void *kmalloc(unsigned long size, int flags);
void kfree(void *addr);
void mm_node_init(struct node *node);
void mm_node_register(struct node *node);
struct node *mm_get_node_by_id(u64 id);
void mm_zone_register(struct node *node, struct zone *zone);
void *ioremap(u64 phys_addr, u64 size);
void *ioremap_nocache(u64 phys_addr, u64 size);
void iounmap(void *virt_addr);

struct task_struct;
void switch_mm(struct mm_struct *prev, struct mm_struct *next,
			struct task_struct *tsk);

int page_fault_mm(u64 addr, u64 err_code, int cpl, u64 ip);

#endif
