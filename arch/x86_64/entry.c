#include <types.h>
#include <kernel.h>
#include <lapic.h>
#include <msr.h>
#include <cr.h>
#include <paging.h>
#include <segment.h>
#include <spinlock.h>
#include <in_out.h>
#include <cpuid.h>
#include <fpu.h>
#include <acpi.h>
#include <string.h>
#include <mm.h>
#include <vmx.h>
#include <cpu.h>
#include <stack.h>
#include <irq.h>
#include <task.h>

void (*jmp_table_percpu[MAX_CPUS])() = {0};
struct bootloader_parm_block *boot_parm = (void *)PHYS2VIRT(0x10000);

struct list_head cpu_list;

u8 cpu_sync_bitmap1[128] = {0};
u8 cpu_sync_bitmap2[128] = {0};
void start_kernel();

int local_apic_eoi()
{
	lapic_reg_write32(APIC_REG_EOI, 1);
}

struct irq_chip irq_apic_chip = {
	.eoi = local_apic_eoi,
};

void intr_handler_common(u64 vector)
{
	struct cpu *cpu = get_cpu();
	cpu->status = CPU_STATUS_IRQ_CONTEXT;
	struct irq_action *action_ptr;
	//printk("interrupt vector:%d on cpu %d\n", vector, cpu->id);

	if (vector >= 32) {
		struct irq_desc *desc = &cpu->intr_desc.irq_desc[vector - 32];
		list_for_each_entry(action_ptr, &desc->irq_action_list, list) {
			action_ptr->irq_handler(vector, action_ptr->data);
		}
		desc->chip->eoi();
	}

	cpu->status = CPU_STATUS_PROCESS_CONTEXT;
}

int unhandled_irq(int irq, void *data)
{
	struct cpu *cpu = get_cpu();
	printk("unhandled irq %d on cpu %d.\n", irq, cpu->id);
	return 0;
}

void setup_irq()
{
	int i;
	struct irq_desc *desc;
	struct irq_action *action;
	struct cpu *cpu = get_cpu();
	cpu->intr_desc.irq_nr = 256 - 32;
	cpu->intr_desc.irq_desc = kmalloc(sizeof(struct irq_desc) * cpu->intr_desc.irq_nr, GFP_KERNEL);
	cpu->intr_desc.irq_bitmap = bitmap_alloc(cpu->intr_desc.irq_nr);
	for (i = 0; i < cpu->intr_desc.irq_nr; i++) {
		desc = &cpu->intr_desc.irq_desc[i];
		desc->chip = &irq_apic_chip;
		INIT_LIST_HEAD(&desc->irq_action_list);
		//request_irq_smp(cpu, i, unhandled_irq, 0, "unhandled irq", NULL);
	}
}

u64 arch_cpuid()
{
	u8 buffer[64] = {0};
	u8 cpu_id = 0;
	cpuid(0x00000001, 0x00000000, (u32 *)&buffer[0]);
	cpu_id = buffer[7];
	return cpu_id;
}

void set_kernel_segment()
{
	struct cpu *cpu = get_cpu();
	struct x86_cpu *x86_cpu = cpu->arch_data;
	struct gdtr gdtr;
	u64 rsp;
	u64 rbp;
	x86_cpu->gdt_base = kmalloc(sizeof(struct segment_desc) * 256, GFP_KERNEL);
	
	set_segment_descriptor(x86_cpu->gdt_base,
		SELECTOR_NULL_INDEX,
		0,
		0,
		0,
		0
	);
	set_segment_descriptor(x86_cpu->gdt_base,
		SELECTOR_KERNEL_CODE_INDEX,
		0,
		0,
		CS_NC | DPL0,
		CS_L
	);
	set_segment_descriptor(x86_cpu->gdt_base,
		SELECTOR_KERNEL_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL0,
		0
	);
	set_segment_descriptor(x86_cpu->gdt_base,
		SELECTOR_USER_CODE_INDEX,
		0,
		0,
		CS_NC | DPL3,
		CS_L
	);
	set_segment_descriptor(x86_cpu->gdt_base,
		SELECTOR_USER_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL3,
		0
	);

	gdtr.base = (u64)x86_cpu->gdt_base;
	gdtr.limit = 256 * sizeof(struct segment_desc) - 1;

	lgdt(&gdtr);

	load_cs(SELECTOR_KERNEL_CODE);
	load_ds(SELECTOR_KERNEL_DATA);
	load_es(SELECTOR_KERNEL_DATA);
	load_fs(SELECTOR_KERNEL_DATA);
	load_gs(SELECTOR_KERNEL_DATA);
	load_ss(SELECTOR_KERNEL_DATA);
}

void set_intr_desc()
{
	struct cpu *cpu = get_cpu();
	struct x86_cpu *x86_cpu = cpu->arch_data;
	x86_cpu->idt_base = kmalloc(sizeof(struct gate_desc) * 256, GFP_KERNEL);
	/* defined in isr.asm */
	extern u64 exception_table[];
	extern u64 irq_table[];

	struct idtr idtr;
	int i;

	for (i = 0; i < 0x20; i++) {
		set_gate_descriptor(x86_cpu->idt_base,
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
		set_gate_descriptor(x86_cpu->idt_base,
			i,
			SELECTOR_KERNEL_CODE,
			irq_table[i - 0x20],
			0,
			INTR_GATE | DPL0
		);
	}
	set_gate_descriptor(x86_cpu->idt_base,
		0x80,
		SELECTOR_KERNEL_CODE,
		irq_table[0x80 - 0x20],
		0,
		TRAP_GATE | DPL3
	);

	idtr.base = (u64)x86_cpu->idt_base;
	idtr.limit = 256 * sizeof(struct gate_desc) - 1;

	lidt(&idtr);
}

void set_tss_desc()
{
	struct cpu *cpu = get_cpu();
	struct x86_cpu *x86_cpu = cpu->arch_data;
	x86_cpu->tss = kmalloc(sizeof(struct tss_desc), GFP_KERNEL);

	memset(x86_cpu->tss, 0, sizeof(struct tss_desc));
	x86_cpu->tss->rsp0 = (u64)kmalloc(0x100000, GFP_KERNEL);
	set_segment_descriptor(x86_cpu->gdt_base,
		SELECTOR_TSS_INDEX,
		(u64)x86_cpu->tss,
		sizeof(struct tss_desc) - 1,
		S_TSS | DPL0,
		0
	);

	ltr(SELECTOR_TSS);
}

u64 kernel_pt_phys = 0;
void map_kernel_memory()
{
	u64 i;
	u64 *pml4t;
	u64 pml4t_phys;
	u64 *pdpt;
	u64 pdpt_phys;
	u64 phy_addr;
	u64 page_attr;

	if (is_bsp()) {
		pml4t = (u64 *)kmalloc(0x1000, GFP_KERNEL);
		pml4t_phys = VIRT2PHYS(pml4t);
		kernel_pt_phys = pml4t_phys;
		pdpt = (u64 *)kmalloc(0x1000, GFP_KERNEL);
		pdpt_phys = VIRT2PHYS(pdpt);
		page_attr = PG_PML4E_PRESENT | PG_PML4E_READ_WRITE | PG_PML4E_SUPERVISOR | PG_PML4E_CACHE_WT;
		pml4t[0] = 0;
		pml4t[0x100] = pdpt_phys | page_attr;

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

	if (kernel_pt_phys != 0)
		write_cr3(kernel_pt_phys);
	else
		printk("kernel_pt_phys = 0!\n");
}

void wakeup_all_processors()
{
	extern u64 ap_startup_vector;
	extern u64 ap_startup_end;
	struct node *node;
	struct cpu *cpu;
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
	
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu, &node->cpu_list, list) {
			if (cpu->id == 0)
				continue;

			//printk("waking up cpu %d...\n", cpu->id);
			mp_init_single(cpu->id, ap_load_addr);
			while (cpu->status != 1);
			//printk(" [%d]", cpu->id);
		}
	}

	printk("Online CPUs: [%d]", get_cpu()->id);
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu, &node->cpu_list, list) {
			if (cpu->id == 0)
				continue;

			while (cpu->status != 1);
			printk(" [%d]", cpu->id);
		}
	}
	printk("\n");
					
}

void enable_cpu_features()
{
	u32 regs[4] = {0};
	u8 cpu_id = 0;
	u32 features1, features2, xcr0_mask;
	u8 cpu_string[64] = {0};

	cpuid(0x80000002, 0x00000000, (u32 *)&cpu_string[0]);
	cpuid(0x80000003, 0x00000000, (u32 *)&cpu_string[16]);
	cpuid(0x80000004, 0x00000000, (u32 *)&cpu_string[32]);

	//u32 max_linear_bits, max_physical_bits;
	cpuid(0x00000001, 0x00000000, &regs[0]);
	cpu_id = ((u8 *)regs)[7];
	features1 = regs[2];
	features2 = regs[3];

	cpuid(0x0000000d, 0x00000000, &regs[0]);
	xcr0_mask = regs[0];
	//cpuid(0x00000000, 0x00000000, &buffer[0]);
	//printk("Max leaf:%x\n", buffer[0]);

	//cpuid(0x80000000, 0x00000000, &buffer[0]);
	//printk("Max ext leaf:%x\n", buffer[0]);

	//cpuid(0x00000001, 0x00000000, &buffer[0]);
	//printk("Model:%08x\n", buffer[0]);
	//printk("supported features:%08x, %08x\n", buffer[2], buffer[3]);

	//printk("[CPU %02d] %s\n", cpu_id, cpu_string);
	//printk("enabled features:");

	/* Lazy FPU ? */
	//u64 cr0 = read_cr0();
	//cr0 |= (CR0_MP | CR0_TS);
	//write_cr0(cr0);
	cr0_set_bits(CR0_MP | CR0_NE);
	cr0_clear_bits(CR0_EM | CR0_CD | CR0_NW);

	u32 xcr0 = 0;
	u64 cr4 = read_cr4();

	if (features2 & BIT25) {
		//printk("SSE ");
		xcr0 |= XCR0_SSE;
	}

	if (features2 & BIT26) {
		//printk("SSE2 ");
	}

	if (features1 & BIT0) {
		//printk("SSE3 ");
	}

	if (features1 & BIT9) {
		//printk("SSSE3 ");
	}

	if (features1 & BIT26) {
		//printk("OSXSAVE ");
		cr4 |= CR4_OSXSAVE;
		xcr0 |= XCR0_X87;
	}

	if (features2 & BIT24 != 0) {
		cr4 |= (CR4_OSFXSR | CR4_OSXMMEXCPT);
		//printk("FXSR ");
	}

	if ((features1 & BIT28) && (xcr0_mask & XCR0_AVX)) {
		//printk("AVX ");
		xcr0 |= XCR0_AVX;
	}

	if ((xcr0_mask & XCR0_AVX) && (xcr0_mask & XCR0_OPMASK)
		&& (xcr0_mask & XCR0_H16_ZMM) && (xcr0_mask & XCR0_ZMM_H256)) {
		//printk("AVX512 ");
		xcr0 |= (XCR0_OPMASK | XCR0_H16_ZMM | XCR0_ZMM_H256);
	}

	if (features1 & BIT5 != 0) {
		cr4 |= CR4_VMXE;
		//printk("VMX ");
	}

	if (features1 & BIT6 != 0) {
		//cr4 |= CR4_SMXE;		//Vmware BUG ? Cause #GP here.
		//printk("SMXE ");
	}

	if (features1 & BIT17 != 0) {
		cr4 |= CR4_PCIDE;
		//printk("PCIDE ");
	}

	if (features2 & BIT13) {
		cr4 |= CR4_PGE;
		//printk("PDE ");
	}

	//printk("\n");
	/* Not fully supported on core i7 5960x ? */
	//cr4 |= (CR4_SMAP | CR4_SMEP);
	//printk("CR4:%x\n", cr4);
	write_cr4(cr4);
	if (xcr0 & XCR0_X87) {
		xsetbv(0, xcr0);
	}

	cpuid(0x00000002, 0x00000000, &regs[0]);
	/* TODO:Add more print here */

	cpuid(0x80000008, 0x00000000, &regs[0]);
	//max_linear_bits = (regs[0] >> 8) & 0xff;
	//max_physical_bits = regs[0] & 0xff;
}

extern void numa_init();

void instruction_test()
{
	u64 zmm0[8];
	u64 zmm1[8];
	u32 reg[4];
	cpuid(0xd, 2, reg);
	printk("AVX XSAVE size %d, OFFSET:%d\n", reg[0], reg[1]);
	cpuid(0xd, 3, reg);
	printk("AVX MPX BNDREGS size %d, OFFSET:%d\n", reg[0], reg[1]);
	cpuid(0xd, 4, reg);
	printk("AVX MPX BNDCSR size %d, OFFSET:%d\n", reg[0], reg[1]);
	cpuid(0xd, 5, reg);
	printk("AVX512 OPMASK XSAVE size %d, OFFSET:%d\n", reg[0], reg[1]);
	cpuid(0xd, 6, reg);
	printk("AVX512 ZMM_Hi256 XSAVE size %d, OFFSET:%d\n", reg[0], reg[1]);
	cpuid(0xd, 7, reg);
	printk("AVX512 Hi16_ZMM XSAVE size %d, OFFSET:%d\n", reg[0], reg[1]);
	struct xsave_area *xsave_area = kmalloc(0x1000, GFP_KERNEL);
	memset(xsave_area, 0, 0x1000);
	//printk("SSE2 test:\n");
	//asm("addpd %xmm1, %xmm2");

	//printk("AVX test:\n");
	//asm("vaddpd %ymm1, %ymm2, %ymm3");
	
	memset(&zmm0[4], 0x33, 32);
	memset(&zmm0[2], 0x22, 16);
	memset(&zmm0[0], 0x11, 16);

	memset(&zmm1[4], 0x66, 32);
	memset(&zmm1[2], 0x55, 16);
	memset(&zmm1[0], 0x44, 16);

	/*
	asm volatile(
		"vmovapd (%0), %%zmm0\n\t"
		"vmovapd (%1), %%zmm1\n\t"
		"vmovapd (%0), %%zmm16\n\t"
		"vmovapd (%1), %%zmm17\n\t"
		::"r"(&zmm0), "r"(&zmm1)
	);
	*/
	asm volatile(
		"vmovapd (%0), %%ymm0\n\t"
		"vmovapd (%1), %%ymm1\n\t"
		::"r"(&zmm0), "r"(&zmm1)
	);

	xsave(xsave_area, 0xe7);

	printk("XSTATE_BV:\n");
	long_int_print((u8 *)&xsave_area->xsave_header.xstate_bv, 8);
	printk("XCOMP_BV:\n");
	long_int_print((u8 *)&xsave_area->xsave_header.xcomp_bv, 8);
	
	printk("ZMM0 lower 128bit:\n");
	long_int_print((u8 *)&xsave_area->legacy_region.xmm_reg[0], 16);
	printk("ZMM0 middle 128bit:\n");
	long_int_print((u8 *)&xsave_area->avx_state.ymm[0], 16);
	printk("ZMM0_Hi256:\n");
	long_int_print((u8 *)&xsave_area->avx512_state.zmm_hi256[0], 16);

	printk("ZMM1 lower 128bit:\n");
	long_int_print((u8 *)&xsave_area->legacy_region.xmm_reg[1], 16);
	printk("ZMM1 middle 128bit:\n");
	long_int_print((u8 *)&xsave_area->avx_state.ymm[1], 16);
	printk("ZMM1_Hi256:\n");
	long_int_print((u8 *)&xsave_area->avx512_state.zmm_hi256[1], 16);

	printk("ZMM16:\n");
	long_int_print((u8 *)&xsave_area->avx512_state.hi16_zmm[0], 64);

	printk("ZMM17:\n");
	long_int_print((u8 *)&xsave_area->avx512_state.hi16_zmm[1], 64);
	
	//printk("AVX512 test:\n");
	//asm("vaddpd %zmm1, %zmm2, %zmm3");
	
}

extern void bootmem_init();

extern void vm_init_test();
extern void x86_pci_hostbridge_init();

void x86_cpu_init()
{
	u64 rbp = 0;
	struct cpu *cpu = get_cpu();
	cpu->arch_data = kmalloc(sizeof(struct x86_cpu), GFP_KERNEL);
	cpu->kernel_stack = kmalloc(0x200000, GFP_KERNEL) + 0x200000;
	cpu->irq_stack = kmalloc(0x200000, GFP_KERNEL) + 0x200000;

	enable_cpu_features();
	map_kernel_memory();

	set_kernel_segment();
	set_tss_desc();
	set_intr_desc();
	setup_irq();
	lapic_enable();

	asm("sti");
	cpu->status = 1;
}

void arch_numa_init();

void test_task()
{
	struct task_struct *task = get_current_task();
	struct cpu *cpu = get_cpu();
	int i,j = 0;
	printk("task %d on cpu %x begins...\n", task->id, cpu->id);
	while (1) {
		printk("cpu %d iteration:%d\n",cpu->id, j++);
		for (i = 0; i < 0x8000000; i++) {};
	}
}

void arch_init()
{
	u8 buffer[64] = {0};
	bool bsp;
	u8 cpu_id = 0;

	if (is_bsp()) {
		bootmem_init();
		arch_numa_init();
	}

	x86_cpu_init();

#if CONFIG_AMP
	jmp_table_percpu[cpu_id]();
#endif

	if (is_bsp()) {
		start_kernel();
		task_init();
		//vm_init_test();
		//x86_pci_hostbridge_init();
		//instruction_test();
	} else {

	}
	
	if (is_bsp()) {
		wakeup_all_processors();
		//lapic_send_ipi(0xff, 0xfe, APIC_ICR_ASSERT);
		//lapic_send_ipi(0, 0xfc, APIC_ICR_ASSERT);
		//lapic_send_ipi(2, 0xfc, APIC_ICR_ASSERT);
		//lapic_send_ipi(4, 0xfc, APIC_ICR_ASSERT);
		//lapic_send_ipi(6, 0xfc, APIC_ICR_ASSERT);
		//asm("ud2");
		//int irq = alloc_irqs_cpu(0, 1);
		//request_irq_smp(get_cpu(), irq, timer_handler, 0, NULL, NULL);
		//printk("irq = %d\n", irq);
		//asm("int $0x20");
		//asm("int $0x21");
		//create_task(test_task, 3, 0x10000, 1, -1);
	}
	request_irq_smp(get_cpu(), 0x5, task_timer_tick, 0, "lapic-timer", NULL);
	lapic_set_timer(1, 0x25);

	//if (is_bsp()) {
	create_task(test_task, 3, 0x10000, 1, -1);
	//}

	while(1) {
		asm("hlt");
	};
}
