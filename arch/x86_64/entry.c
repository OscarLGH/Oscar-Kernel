#include <types.h>
#include <kernel.h>
#include <arch.h>
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
	u8 *percpu_base = (u8 *)PHYS2VIRT(get_percpu_area_base());
	struct gdtr gdtr_percpu;
	struct segment_desc *gdt_base = (void *)(percpu_base + 0);

	set_segment_descriptor(gdt_base, SELECTOR_NULL_INDEX, 0, 0, 0, 0);
	set_segment_descriptor(gdt_base, SELECTOR_KERNEL_CODE_INDEX, 0, 0, CS_NC | DPL0, CS_L);
	set_segment_descriptor(gdt_base, SELECTOR_KERNEL_DATA_INDEX, 0, 0, DS_S_W | DPL0, 0);
	set_segment_descriptor(gdt_base, SELECTOR_USER_CODE_INDEX, 0, 0, CS_NC | DPL3, CS_L);
	set_segment_descriptor(gdt_base, SELECTOR_USER_DATA_INDEX, 0, 0, DS_S_W | DPL3, 0);
	set_segment_descriptor(gdt_base, SELECTOR_TSS_INDEX, 0, 0, S_TSS | DPL0, 0);
	
	gdtr_percpu.base = (u64)percpu_base;
	gdtr_percpu.limit = 256 * sizeof(struct segment_desc) - 1;
	
	lgdt(&gdtr_percpu);

	load_cs(SELECTOR_KERNEL_CODE);
	load_ds(SELECTOR_KERNEL_DATA);
	load_es(SELECTOR_KERNEL_DATA);
	load_fs(SELECTOR_KERNEL_DATA);
	load_gs(SELECTOR_KERNEL_DATA);
	load_ss(SELECTOR_KERNEL_DATA);
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
	if (madt_ptr == NULL) {
		//mp_init_all(ap_load_addr);
		return;
	}
	
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
		map_kernel_memory();
		wakeup_all_processors();
	} else {

	}
	set_kernel_segment();
#if CONFIG_AMP
	jmp_table_percpu[cpu_id]();
#endif

	asm("hlt");
}
