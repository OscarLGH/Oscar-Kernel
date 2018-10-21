#include <types.h>
#include <kernel.h>
#include <arch.h>
#include <cr.h>
#include <spinlock.h>
#include <cpuid.h>
#include <acpi.h>
#include <string.h>
#include <mm.h>
#include <cpu.h>
#include <list.h>

void arch_numa_init()
{
	int table_len;
	int node_bitmap[64] = {0};
	struct node *node;
	struct cpu *cpu;
	struct zone *zone;

	struct acpi_srat *srat_ptr = acpi_get_desc("SRAT");

	if (srat_ptr == NULL) {
		node = boot_mem_alloc(sizeof(*node));
		mm_node_init(node);

		struct acpi_madt *madt_ptr = acpi_get_desc("APIC");

		node = (struct node *)boot_mem_alloc(sizeof(*node));
		INIT_LIST_HEAD(&node->cpu_list);
		INIT_LIST_HEAD(&node->zone_list);
		node->socket_id = 0;
		mm_node_register(node);

		if (madt_ptr == NULL) {
			cpu = boot_mem_alloc(sizeof(*cpu));
			cpu->id = 0;
			list_add_tail(&cpu->list, &node->cpu_list);
		} else {
		
			struct processor_lapic_structure *lapic_ptr;
			u8 *ptr = (u8 *)madt_ptr + sizeof(*madt_ptr);

			while ((u64)ptr < (u64)madt_ptr + madt_ptr->header.length - sizeof(*madt_ptr)) {
				switch(ptr[0]) {
					case PROCESSOR_LOCAL_APIC:
						lapic_ptr = (struct processor_lapic_structure *)ptr;
						if (lapic_ptr->flags & 0x1 != 0) {
							cpu = boot_mem_alloc(sizeof(*cpu));
							cpu->id = lapic_ptr->apic_id;
							list_add_tail(&cpu->list, &node->cpu_list);
						}
						break;
				}
				ptr += ptr[1];
			}
		}

		struct bootloader_parm_block *boot_parm = (void *)SYSTEM_PARM_BASE;
		for (int i = 0; i < boot_parm->ardc_cnt; i++) {
			if (boot_parm->ardc_array[i].type == 1 || boot_parm->ardc_array[i].type == 2) {
				struct zone *zone = boot_mem_alloc(sizeof(*zone));
				zone->page_size = 0x1000;
				zone->start_pfn = boot_parm->ardc_array[i].base;
				zone->pfn_cnt = boot_parm->ardc_array[i].length;
				mm_zone_register(node, zone);
			}
		}
		return;
	}
	
	table_len = srat_ptr->header.length - sizeof(*srat_ptr);
	struct acpi_subtable_header *table_ptr = (struct acpi_subtable_header *)&srat_ptr[1];
	struct acpi_srat_cpu_affinity *srat_cpu_affinity_ptr;
	struct acpi_srat_mem_affinity *srat_mem_affinity_ptr;

	while (table_len > 0) {
		switch(table_ptr->type) {
			case ACPI_SRAT_TYPE_CPU_AFFINITY:
				srat_cpu_affinity_ptr = (struct acpi_srat_cpu_affinity *)table_ptr;
				if (srat_cpu_affinity_ptr->flags & 0x1 != 0) {
					if (node_bitmap[srat_cpu_affinity_ptr->proximity_domain_lo] == 0) {
						node = (struct node *)boot_mem_alloc(sizeof(*node));
						mm_node_init(node);
						node->socket_id = srat_cpu_affinity_ptr->proximity_domain_lo;
						mm_node_register(node);

						cpu = boot_mem_alloc(sizeof(*cpu));
						cpu->id = srat_cpu_affinity_ptr->apic_id;
						list_add_tail(&cpu->list, &node->cpu_list);
						
						node_bitmap[srat_cpu_affinity_ptr->proximity_domain_lo] = 1;
					} else {
						node = mm_get_node_by_id(srat_cpu_affinity_ptr->proximity_domain_lo);
						if (node != NULL) {
							cpu = boot_mem_alloc(sizeof(*cpu));
							cpu->id = srat_cpu_affinity_ptr->apic_id;
							list_add_tail(&cpu->list, &node->cpu_list);
						}
					}
				}
				break;
			case ACPI_SRAT_TYPE_MEMORY_AFFINITY:
				srat_mem_affinity_ptr = (struct acpi_srat_mem_affinity *)table_ptr;
				if (srat_mem_affinity_ptr->length != 0) {
					if (node_bitmap[srat_mem_affinity_ptr->proximity_domain] == 0) {
						node = (struct node *)boot_mem_alloc(sizeof(*node));
						mm_node_init(node);
						node->socket_id = srat_mem_affinity_ptr->proximity_domain;
						mm_node_register(node);

						zone = boot_mem_alloc(sizeof(*cpu));
						zone->start_pfn = srat_mem_affinity_ptr->base_address;
						zone->pfn_cnt = srat_mem_affinity_ptr->length;
						mm_zone_register(node, zone);
						node_bitmap[srat_mem_affinity_ptr->proximity_domain] = 1;
					} else {
						node = mm_get_node_by_id(srat_mem_affinity_ptr->proximity_domain);
						if (node != NULL) {
							zone = boot_mem_alloc(sizeof(*cpu));
							zone->start_pfn = srat_mem_affinity_ptr->base_address;
							zone->pfn_cnt = srat_mem_affinity_ptr->length;
							mm_zone_register(node, zone);
						}
					}
				}
				break;
			case ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY:
				break;
			default:
				break;
		}
		table_len -= table_ptr->length;
		table_ptr = (struct acpi_subtable_header *)((u64)table_ptr + table_ptr->length);
	}
}


