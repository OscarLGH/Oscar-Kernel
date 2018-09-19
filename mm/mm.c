#include <mm.h>

LIST_HEAD(zone_list);

void mm_zone_register(struct zone *zone)
{
	struct zone *zone_ptr;
	list_for_each_entry(zone_ptr, &zone_list, list) {
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
	list_add_tail(&zone->list, &zone_list);
}

void mm_enumate()
{
	u64 base = 0;
	u64 length = 0;
	struct bootloader_parm_block *boot_parm = (void *)SYSTEM_PARM_BASE;
	for (int i = 0; i < boot_parm->ardc_cnt; i++) {
		if (boot_parm->ardc_array[i].type == 1) {
			struct zone *zone = boot_mem_alloc(sizeof(*zone));
			zone->page_size = 0x1000;
			zone->start_pfn = boot_parm->ardc_array[i].base / zone->page_size;
			zone->pfn_cnt = boot_parm->ardc_array[i].length / zone->page_size;
			mm_zone_register(zone);
			//printk("Memory region:%x, length = %x\n", boot_parm->ardc_array[i].base, boot_parm->ardc_array[i].length);
		}
	}

	struct zone *zone_ptr;
	list_for_each_entry(zone_ptr, &zone_list, list) {
		printk("physical memory zone: base:0x%x, size:0x%x\n", zone_ptr->start_pfn << 12, zone_ptr->pfn_cnt << 12);
	}
}
