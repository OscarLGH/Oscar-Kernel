#include <types.h>
#include <kernel.h>
#include <lapic.h>
#include <msr.h>
#include <paging.h>
#include <segment.h>
#include <spinlock.h>
#include <in_out.h>
#include <cpuid.h>
#include <acpi.h>

void (*jmp_table_percpu[MAX_CPUS])() = {0};
struct bootloader_parm_block *boot_parm = (void *)0x10000;

u8 cpu_bitmap[128] = {0};

void set_kernel_segment()
{

}

void map_kernel_memory()
{

}

void wakeup_all_processors()
{
	extern u64 ap_startup_vector;
	extern u64 ap_startup_end;
	u64 ap_load_addr = 0x20000;
	u8 *dest = (u8 *)PHYS2VIRT(ap_load_addr);
	u8 *src = (u8 *)&ap_startup_vector;
	int i;

	/* copy ap startup code to AP_START_ADDR */
	for (i = 0; i < (u64)&ap_startup_end - (u64)&ap_startup_vector; i++)
		dest[i] = src[i];

	struct acpi_madt *madt_ptr = acpi_get_desc("APIC");

	struct processor_lapic_structure *lapic_ptr;
	u8 *ptr = (u8 *)madt_ptr + sizeof(*madt_ptr);

	while (ptr[1] != 0 && (u64)ptr < (u64)madt_ptr + madt_ptr->header.length) {
		switch(ptr[0]) {
			case PROCESSOR_LOCAL_APIC:
				lapic_ptr = (struct processor_lapic_structure *)ptr;
				if (lapic_ptr->apic_id != 0) {
					//printk("waking up CPU:APIC ID = %d\n", lapic_ptr->apic_id);
					cpu_bitmap[lapic_ptr->apic_id] = 0;
					mp_init_single(lapic_ptr->apic_id, ap_load_addr);
					while(!cpu_bitmap[lapic_ptr->apic_id]);
				}
				break;
		}
		ptr += ptr[1];
	}
}

void arch_init()
{
	u8 buffer[64] = {0};
	bool bsp;
	u8 cpu_id = 0;
	cpuid(0x00000001, 0x00000000, (u32 *)&buffer[0]);
	cpu_id = buffer[7];

	cpuid(0x80000002, 0x00000000, (u32 *)&buffer[0]);
	cpuid(0x80000003, 0x00000000, (u32 *)&buffer[16]);
	cpuid(0x80000004, 0x00000000, (u32 *)&buffer[32]);
	cpu_bitmap[cpu_id] = 1;
	printk("[CPU %02d] %s\n", cpu_id, buffer);

	bsp = is_bsp();
	if (bsp) {
		set_kernel_segment();
		map_kernel_memory();
		wakeup_all_processors();
	} else {

	}

	asm("hlt");
}
