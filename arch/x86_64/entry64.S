/* 64bit long-mode entrance of x86_64 platform.
 * Set C runtime environment (address space switch).
 * Oscar
 * Jul, 2018
 */

KERNEL_OFFSET = 0xffff800000000000
INIT_STACK_PHYS = 0x600000

INIT_MAPPING_ADDR = setup_init_mapping - KERNEL_OFFSET

PML4T_BASE = 0x100000
PDPT_BASE = PML4T_BASE + 0x1000
PDT_BASE = PDPT_BASE + 0x1000

KERNEL_SPACE_ADDR = kernel_space_entry

.global bsp_entry_64
.code64
.section .text

setup_init_mapping:

	movq $PML4T_BASE, %rdi

	// PML4T[0x100] = PDPT_BASE
	movq $PDPT_BASE, %rax
	orq $0x3, %rax
	movq %rax, 0x800(%rdi)

	//Identical mapping for address space switching.
	// PML4T[0] = PDPT_BASE
	movq %rax, 0x0(%rdi)

	// PDPT[0]
	movq $PDPT_BASE, %rdi
	movq $PDT_BASE, %rax
	orq $0x3, %rax
	movq %rax, 0x0(%rdi)

	// Mapping 16MB booting memory.
	// PDT[0] - PDT[7]
	movq $PDT_BASE, %rdi
	movq $0, %rax
	orq $0x83, %rax
	movq $0, %rcx

1:
	movq %rax, 0x0(%rdi, %rcx, 8)
	addq $1, %rcx
	addq $0x200000, %rax
	cmp $8, %rcx
	jl 1b

	movq $PML4T_BASE, %rax
	movq %rax, %cr3
	movq $KERNEL_OFFSET, %rbx
	addq %rbx, %rsp
	retq

bsp_entry_64:
	movw $0x3f8, %dx
	movb $'A', %al
	outb %al, %dx

	movq $INIT_STACK_PHYS, %rsp
	movq $INIT_MAPPING_ADDR, %rax
	call *%rax

kernel_space_entry:
	callq arch_init
	jmp kernel_space_entry
