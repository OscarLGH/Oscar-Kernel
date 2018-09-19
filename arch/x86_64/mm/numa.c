#include <types.h>
#include <kernel.h>
#include <arch.h>
#include <cr.h>
#include <spinlock.h>
#include <cpuid.h>
#include <acpi.h>
#include <string.h>
#include <mm.h>

void numa_init()
{
	int table_len;

	struct acpi_srat *srat_ptr = acpi_get_desc("SRAT");

	if (srat_ptr == NULL)
		return;
	
	table_len = srat_ptr->header.length - sizeof(*srat_ptr);
	struct acpi_subtable_header *table_ptr = (struct acpi_subtable_header *)&srat_ptr[1];
	struct acpi_srat_cpu_affinity *srat_cpu_affinity_ptr;
	struct acpi_srat_mem_affinity *srat_mem_affinity_ptr;

	while (table_len > 0) {
		switch(table_ptr->type) {
			case ACPI_SRAT_TYPE_CPU_AFFINITY:
				srat_cpu_affinity_ptr = (struct acpi_srat_cpu_affinity *)table_ptr;
				printk("CPU domain %d: APIC ID = %x\n",
					srat_cpu_affinity_ptr->proximity_domain_lo,
					srat_cpu_affinity_ptr->apic_id
				);
				break;
			case ACPI_SRAT_TYPE_MEMORY_AFFINITY:
				srat_mem_affinity_ptr = (struct acpi_srat_mem_affinity *)table_ptr;
				printk("NUMA %d:Base address:0x%016x, length = 0x%x\n",
					srat_mem_affinity_ptr->proximity_domain,
					srat_mem_affinity_ptr->base_address,
					srat_mem_affinity_ptr->length
				);
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


