#include <mm.h>
#include <cpu.h>

LIST_HEAD(node_list);


void mm_node_init(struct node *node)
{
	INIT_LIST_HEAD(&node->cpu_list);
	INIT_LIST_HEAD(&node->zone_list);	
}

void mm_node_register(struct node *node)
{
	list_add_tail(&node->list, &node_list);
}

struct node *mm_get_node_by_id(u64 id)
{
	struct node *node;
	list_for_each_entry(node, &node_list, list) {
		if (node->socket_id == id) {
			return node;
		}
	}
	return NULL;
}

void mm_zone_register(struct node *node, struct zone *zone)
{
	struct zone *zone_ptr;
	list_for_each_entry(zone_ptr, &node->zone_list, list) {
		if (zone_ptr->start_pfn + zone_ptr->pfn_cnt == zone->start_pfn) {
			zone_ptr->pfn_cnt += zone->pfn_cnt;
			return;
		}

		if (zone->start_pfn + zone->pfn_cnt == zone_ptr->start_pfn) {
			zone_ptr->start_pfn = zone->start_pfn;
			zone_ptr->pfn_cnt += zone->pfn_cnt;
			return;
		}
	}
	list_add_tail(&zone->list, &node->zone_list);
}

void mm_enumate()
{
	u64 base = 0;
	u64 length = 0;
	int i;
	struct bootloader_parm_block *boot_parm = (void *)SYSTEM_PARM_BASE;
	for (i = 0; i < boot_parm->ardc_cnt; i++) {
		if (1) {
			struct zone *zone = kmalloc(sizeof(*zone), GFP_KERNEL);
			zone->page_size = 0x1000;
			zone->start_pfn = boot_parm->ardc_array[i].base / zone->page_size;
			zone->pfn_cnt = boot_parm->ardc_array[i].length / zone->page_size;
			//mm_zone_register(zone);
			printk("Memory region:%x, length = %x, type = %d\n", boot_parm->ardc_array[i].base, boot_parm->ardc_array[i].length, boot_parm->ardc_array[i].type);
		}
	}

	//struct zone *zone_ptr;
	//u64 total_ram_size = 0;;
	//list_for_each_entry(zone_ptr, &zone_list, list) {
	//	printk("physical memory zone: base:0x%x, size:0x%x\n", zone_ptr->start_pfn << 12, zone_ptr->pfn_cnt << 12);
	//	total_ram_size += (zone_ptr->pfn_cnt << 12);
	//}
	//printk("Total available memory:%d MB.", total_ram_size >> 20);
}

void mminfo_print()
{
	struct node *node;
	struct cpu *cpu;
	struct zone *zone;
	list_for_each_entry(node, &node_list, list) {
		printk("NUMA %d:\n", node->socket_id);

		printk("    CPUs:");
		list_for_each_entry(cpu, &node->cpu_list, list) {
			printk("[%d] ", cpu->id);
		}

		printk("\n    Memory zone:\n");
		list_for_each_entry(zone, &node->zone_list, list) {
			printk("        [0x%016x - 0x%016x]\n", zone->start_pfn, zone->start_pfn + zone->pfn_cnt - 1);
		}
	}
}

void *kmalloc(unsigned long size, int flags)
{
	bootmem_alloc(size);
}
