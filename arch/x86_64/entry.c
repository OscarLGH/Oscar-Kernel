#include <types.h>
#include <kernel.h>
#include <arch.h>
#include <lapic.h>
#include <msr.h>
#include <cr.h>
#include <paging.h>
#include <segment.h>
#include <spinlock.h>
#include <in_out.h>
#include <cpuid.h>
#include <acpi.h>
#include <string.h>

void (*jmp_table_percpu[MAX_CPUS])() = {0};
struct bootloader_parm_block *boot_parm = (void *)PHYS2VIRT(0x10000);

u8 cpu_sync_bitmap1[128] = {0};
u8 cpu_sync_bitmap2[128] = {0};
void start_kernel();

void irq_handler(u64 vector)
{
	u8 cpu_id = 0;
	u8 buffer[64] = {0};
	cpuid(0x00000001, 0x00000000, (u32 *)&buffer[0]);
	cpu_id = buffer[7];
	printk("interrupt vector:%d on cpu %d\n", vector, cpu_id);
	if (vector >= 0x20) {
		lapic_reg_write32(APIC_REG_EOI, 1);
	}
}

void set_kernel_segment()
{
	u8 *percpu_base = (u8 *)PHYS2VIRT(get_percpu_area_base());
	struct gdtr gdtr_percpu;
	struct segment_desc *gdt_base = (void *)(percpu_base + 0);

	set_segment_descriptor(gdt_base,
		SELECTOR_NULL_INDEX,
		0,
		0,
		0,
		0
	);
	set_segment_descriptor(gdt_base,
		SELECTOR_KERNEL_CODE_INDEX,
		0,
		0,
		CS_NC | DPL0,
		CS_L
	);
	set_segment_descriptor(gdt_base,
		SELECTOR_KERNEL_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL0,
		0
	);
	set_segment_descriptor(gdt_base,
		SELECTOR_USER_CODE_INDEX,
		0,
		0,
		CS_NC | DPL3,
		CS_L
	);
	set_segment_descriptor(gdt_base,
		SELECTOR_USER_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL3,
		0
	);

	gdtr_percpu.base = (u64)gdt_base;
	gdtr_percpu.limit = 256 * sizeof(struct segment_desc) - 1;

	lgdt(&gdtr_percpu);

	load_cs(SELECTOR_KERNEL_CODE);
	load_ds(SELECTOR_KERNEL_DATA);
	load_es(SELECTOR_KERNEL_DATA);
	load_fs(SELECTOR_KERNEL_DATA);
	load_gs(SELECTOR_KERNEL_DATA);
	load_ss(SELECTOR_KERNEL_DATA);
}

void set_intr_desc()
{
	/* defined in isr.asm */
	extern u64 exception_table[];
	extern u64 irq_table[];

	u8 *percpu_base = (u8 *)PHYS2VIRT(get_percpu_area_base());
	struct idtr idtr_percpu;
	struct gate_desc *idt_base = (void *)(percpu_base + 0x1000);
	int i;

	for (i = 0; i < 0x20; i++) {
		set_gate_descriptor(idt_base,
			i,
			SELECTOR_KERNEL_CODE,
			(u64)exception_table[i],
			0,
			TRAP_GATE | DPL0
		);
	}
	for (i = 0x20; i < 0x100; i++) {
		if (i == 0x80)
			continue;
		set_gate_descriptor(idt_base,
			i,
			SELECTOR_KERNEL_CODE,
			irq_table[i - 0x20],
			0,
			INTR_GATE | DPL0
		);
	}
	set_gate_descriptor(idt_base,
		0x80,
		SELECTOR_KERNEL_CODE,
		irq_table[0x80 - 0x20],
		0,
		TRAP_GATE | DPL3
	);

	idtr_percpu.base = (u64)idt_base;
	idtr_percpu.limit = 256 * sizeof(struct gate_desc) - 1;

	lidt(&idtr_percpu);
}

void set_tss_desc()
{
	u8 *percpu_base = (u8 *)PHYS2VIRT(get_percpu_area_base());
	struct segment_desc *gdt_base = (void *)(percpu_base + 0);
	struct tss_desc *tss = (void *)(percpu_base + 0x2000);

	memset(tss, 0, sizeof(*tss));
	tss->rsp0 = (u64)(percpu_base + PERCPU_AREA_SIZE);
	set_segment_descriptor(gdt_base,
		SELECTOR_TSS_INDEX,
		(u64)tss,
		sizeof(*tss) - 1,
		S_TSS | DPL0,
		0
	);

	ltr(SELECTOR_TSS);
}

void map_kernel_memory()
{
	struct ards *ardc_ptr = boot_parm->ardc_array;
	u64 i;
	u64 offset = 0x1000;
	u64 pml4t_phys = 0x1000000;
	u64 phy_addr = 0;
	u64 *pml4t = (u64 *)PHYS2VIRT(pml4t_phys);
	u64 *pdpt = (u64 *)PHYS2VIRT(pml4t_phys + offset);
	u64 page_attr;

	if (is_bsp()) {
		page_attr = PG_PML4E_PRESENT | PG_PML4E_READ_WRITE | PG_PML4E_SUPERVISOR | PG_PML4E_CACHE_WT;
		pml4t[0] = 0;
		pml4t[0x100] = (pml4t_phys + offset) | page_attr;

		page_attr = PG_PDPTE_PRESENT |
				PG_PDPTE_READ_WRITE |
				PG_PDPTE_SUPERVISOR |
				PG_PDPTE_1GB_PAGE |
				PG_PDPTE_1GB_PAT |
				PG_PDPTE_1GB_GLOBAL;

		for (i = 0; i < 512; i++) {
			/* Use uncached memory type for MMIO. */
			/* TODO:PCI BAR mapped to high address above 4GB. */
			if (i == 3) {
				pdpt[i] = i * SIZE_1GB | (page_attr | PG_PDPTE_CACHE_DISABLE);
			} else {
				pdpt[i] = i * SIZE_1GB | (page_attr | PG_PDPTE_CACHE_WT);
			}
		}
	}

	write_cr3(pml4t_phys);
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
		mp_init_all(ap_load_addr);
		return;
	}
	
	struct processor_lapic_structure *lapic_ptr;
	u8 *ptr = (u8 *)madt_ptr + sizeof(*madt_ptr);

	while ((u64)ptr < (u64)madt_ptr + madt_ptr->header.length - sizeof(*lapic_ptr)) {
		switch(ptr[0]) {
			case PROCESSOR_LOCAL_APIC:
				lapic_ptr = (struct processor_lapic_structure *)ptr;
				if (lapic_ptr->apic_id != 0) {
					//printk("waking up CPU:APIC ID = %d\n", lapic_ptr->apic_id);
					cpu_sync_bitmap1[lapic_ptr->apic_id] = 0;
					mp_init_single(lapic_ptr->apic_id, ap_load_addr);
					while(!cpu_sync_bitmap1[lapic_ptr->apic_id]);
				}
				break;
		}
		ptr += ptr[1];
	}
}

void ap_work()
{
	
}

void check_point()
{
	u8 buffer[64] = {0};
	u8 cpu_id = 0;
	cpuid(0x00000001, 0x00000000, (u32 *)&buffer[0]);
	cpu_id = buffer[7];
	//printk("CPU %d readched the checkpoint.\n", cpu_id);
}

void enable_cpu_features()
{
	u32 regs[4] = {0};
	u8 cpu_id = 0;
	u32 features1, features2;
	u8 cpu_string[64] = {0};
	//u32 max_linear_bits, max_physical_bits;
	cpuid(0x00000001, 0x00000000, &regs[0]);
	cpu_id = ((u8 *)regs)[7];
	features1 = regs[2];
	features2 = regs[3];
	//cpuid(0x00000000, 0x00000000, &buffer[0]);
	//printk("Max leaf:%x\n", buffer[0]);

	//cpuid(0x80000000, 0x00000000, &buffer[0]);
	//printk("Max ext leaf:%x\n", buffer[0]);

	//cpuid(0x00000001, 0x00000000, &buffer[0]);
	//printk("Model:%08x\n", buffer[0]);
	//printk("supported features:%08x, %08x\n", buffer[2], buffer[3]);

	/* Lazy FPU ? */
	//u64 cr0 = read_cr0();
	//cr0 |= (CR0_MP | CR0_TS);
	//write_cr0(cr0);

	u64 cr4 = read_cr4();
	if (features2 & BIT24 != 0) {
		cr4 |= CR4_OSFXSR;
	}

	if (features1 & BIT5 != 0) {
		cr4 |= CR4_VMXE;
	}

	if (features1 & BIT6 != 0) {
		//cr4 |= CR4_SMXE;		//Vmware BUG ? Cause #GP here.
	}

	if (features1 & BIT17 != 0) {
		cr4 |= CR4_PCIDE;
	}

	if (features2 & BIT13) {
		cr4 |= CR4_PGE;
	}

	cr4 |= (CR4_SMAP | CR4_SMEP);

	write_cr4(cr4);

	cpuid(0x00000002, 0x00000000, &regs[0]);
	/* TODO:Add more print here */

	cpuid(0x80000008, 0x00000000, &regs[0]);
	//max_linear_bits = (regs[0] >> 8) & 0xff;
	//max_physical_bits = regs[0] & 0xff;

	cpuid(0x80000002, 0x00000000, (u32 *)&cpu_string[0]);
	cpuid(0x80000003, 0x00000000, (u32 *)&cpu_string[16]);
	cpuid(0x80000004, 0x00000000, (u32 *)&cpu_string[32]);

	printk("[CPU %02d] %s\n", cpu_id, cpu_string);
}

extern void numa_init();

void arch_init()
{
	u8 buffer[64] = {0};
	bool bsp;
	u8 cpu_id = 0;

	if (is_bsp()) {
		numa_init();
	}
	cpuid(0x00000001, 0x00000000, (u32 *)&buffer[0]);
	cpu_id = buffer[7];

	enable_cpu_features();

	cpu_sync_bitmap1[cpu_id] = 1;

	map_kernel_memory();

	if (is_bsp()) {
		wakeup_all_processors();
	}

	set_kernel_segment();
	set_tss_desc();
	set_intr_desc();
	lapic_enable();

	asm("sti");

#if CONFIG_AMP
	jmp_table_percpu[cpu_id]();
#endif

	cpu_sync_bitmap2[cpu_id] = 1;
	check_point();

	if (is_bsp()) {
		start_kernel();
		//lapic_send_ipi(1, 0xff, APIC_ICR_ASSERT);
		//lapic_send_ipi(2, 0xff, APIC_ICR_ASSERT);
		//lapic_send_ipi(3, 0xff, APIC_ICR_ASSERT);
	} else {
		//lapic_send_ipi(1, 0xff, APIC_ICR_ASSERT);
		//lapic_send_ipi(0, 0xfe, APIC_ICR_ASSERT);
	}

	asm("hlt");
}
