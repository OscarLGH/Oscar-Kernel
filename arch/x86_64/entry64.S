/* 64bit long-mode entrance of x86_64 platform.
 * Set C runtime environment (address space switch).
 * Oscar
 * Jul, 2018
 */

KERNEL_OFFSET = 0xffff800000000000
INIT_STACK_PHYS = 0x1000000

INIT_MAPPING_ADDR = setup_init_mapping - KERNEL_OFFSET


PML4T_BASE = 0x100000
PDPT_BASE = PML4T_BASE + 0x1000
PDT_BASE = PDPT_BASE + 0x1000

.data

.global jmp_table

.global bsp_entry_64
.code64
.section .text

setup_init_mapping:

	movq $PML4T_BASE, %rdi

	/* PML4T[0x100] = PDPT_BASE */
	movq $PDPT_BASE, %rax
	orq $0x3, %rax
	movq %rax, 0x800(%rdi)

/* We need to do identical mapping for address space switching,
 * otherwise the instruction prefetch will fail after switching
 * because RIP remains the previous value.
 */
	/* PML4T[0] = PDPT_BASE */
	movq %rax, 0x0(%rdi)

	/* PDPT[0] */
	movabsq $PDPT_BASE, %rdi
	movabsq $0, %rax
	orq $0x83, %rax
	movq %rax, 0x0(%rdi)

	/* PDPT[3] */
	movabsq $PDPT_BASE, %rdi
	movabsq $0xc0000000, %rax
	orq $0x83, %rax
	movq %rax, 0x18(%rdi)

	/* Mapping 16MB booting memory. */
	/* PDT[0] - PDT[7] */
	movabsq $PDT_BASE, %rdi
	movabsq $0, %rax
	orq $0x83, %rax
	movq $0, %rcx

1:
	movq %rax, 0x0(%rdi, %rcx, 8)
	addq $1, %rcx
	addq $0x200000, %rax
	cmp $8, %rcx
	jl 1b

	/* Load PML4T to CR3 */
	movabsq $PML4T_BASE, %rax
	movq %rax, %cr3

	/* We need add KERNEL_OFFSET to RSP to finish address space switching. */
	/* New RIP will be set after return. */
	movabsq $KERNEL_OFFSET, %rbx
	addq %rbx, %rsp
	retq

bsp_entry_64:

	movabsq $INIT_STACK_PHYS, %rsp
	movabsq $INIT_MAPPING_ADDR, %rax

/* Call pushes kernel_space_entry to stack, which is in kernel space and not mapped.
 * We need to set new page table before setup_init_mapping returns.
 */
	call *%rax

kernel_space_entry:

	/* Set init stack for each cpu */
	movl $0x00000001, %eax
	movl $0x00000000, %ecx
	cpuid
	shr $24, %ebx
	/* ebx *= 0x1000 */
	shl $12, %ebx
	movabsq $(INIT_STACK_PHYS + KERNEL_OFFSET), %rsp
	subq %rbx, %rsp

	callq arch_init
	jmp kernel_space_entry


/* Following code should be copied below 1MB space,
 * because APs start with read-mode.
 */

AP_LOAD_ADDR = 0x20000
GDT_ADDR = gdt_desc - ap_startup_vector
AP_ENTRY_32_PHYS = AP_LOAD_ADDR + ap_enrty_32 - ap_startup_vector

.global ap_startup_vector
.global ap_startup_end

ap_startup_vector:

.code16
ap_entry_16:

	/* CS.base = AP_LOAD_ADDR >> 16 after wakeup */
	jmp 1f

	.align 16
gdt:
desc_null:		.octa 0
desc_code32:	.octa 0x004f98000000ffff
desc_code64:	.octa 0x002f98000000ffff
desc_data64:	.octa 0x000f92000000ffff
gdt_desc:		.word 48 - 1
				.quad gdt - ap_startup_vector + AP_LOAD_ADDR

1:
	movw %cs, %ax
	movw %ax, %ds

	/* Load GDT */
	/* CS.base = 0x20000, GDT_ADDR is offset. */
	lgdt GDT_ADDR

	/* Close local interrupt */
	cli

	/* Disable NMI */
	inb $0x70, %al
	orb $0x80, %al
	outb %al, $0x70

	/* Enable #A20 */
	inb $0x92, %al
	orb $0x2, %al
	outb %al, $0x92

	/* Set Protect Enable bit */
	movl %cr0, %eax
	bts $0, %eax
	movl %eax, %cr0

	/* Jump to protected mode, flush prefetch queue. */
	data32 ljmp $(desc_code32 - gdt), $(AP_LOAD_ADDR + ap_enrty_32 - ap_startup_vector)

.code32
ap_enrty_32:

	/* Enable PAE */
	movl %cr4, %eax
	bts $5, %eax
	movl %eax, %cr4

	/* Load PML4T to CR3 */
	movl $PML4T_BASE, %eax
	movl %eax, %cr3

	/* Enable long-mode */
	movl $0xc0000080, %ecx
	rdmsr
	bts $8, %eax
	wrmsr

	/* Enable paging. */
	movl %cr0, %eax
	bts $31, %eax
	movl %eax, %cr0

	/* Jump to long mode, flush prefetch queue. */
	ljmp $(desc_code64 - gdt), $(AP_LOAD_ADDR + ap_enrty_64 - ap_startup_vector)

.code64
ap_enrty_64:
	movabsq $kernel_space_entry, %rax
	jmpq *%rax

ap_startup_end:
	nop

