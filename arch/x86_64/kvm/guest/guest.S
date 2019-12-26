.global test_guest
.global test_guest_end
.global test_guest_reset_vector

.text
.code16
test_guest:
guest_entry_16:

	movw $0, %ax
	movw %ax, %ds
	jmp 1f

.align 16
gdt:
desc_null:		.octa 0
desc_code32:	.octa 0x004f98000000ffff
desc_data32:	.octa 0x00cf92000000ffff
desc_code64:	.octa 0x002f98000000ffff
desc_data64:	.octa 0x000f92000000ffff
desc_ldt64:		.octa 0x008782000000ffff
gdt_desc:		.word gdt_desc - gdt - 1
				.quad gdt - guest_entry_16 + 0x7c00

1:
	/* Load GDT */
	lgdt 0x7c00 + gdt_desc - guest_entry_16

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
	data32 ljmp $(desc_code32 - gdt), $(0x7c00 + guest_entry_32 - guest_entry_16)

.code32
guest_entry_32:

	mov $(desc_data32 - gdt), %ax
	mov %ax, %ds

	/* Enable PAE */
	movl %cr4, %eax
	bts $5, %eax
	movl %eax, %cr4

	movl $0x100000, %eax
	movl $0x101003, (%eax)
	movl $0, 4(%eax)

	movl $0x101000, %eax
	movl $0x102003, (%eax)
	movl $0, 4(%eax)

	movl $0x102000, %eax
	movl $0x83, (%eax)
	movl $0, 4(%eax)

	/* Load PML4T to CR3 */
	movl $0x100000, %eax
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
	ljmp $(desc_code64 - gdt), $(0x7c00 + guest_entry_64 - guest_entry_16)

.code64
guest_entry_64:

	movabsq $0xffff800001000000, %rsp

	movq %cr4, %rax
	bts $9, %rax
	bts $18, %rax
	movq %rax, %cr4

	movq %cr0, %rax
	bts $1, %rax
	bts $5, %rax
	btr $2, %rax
	movq %rax, %cr0

	movq %cr4, %rax
	bts $18, %rax
	movq %rax, %cr4
	
	movl $0x7, %eax
	movl $0, %edx
	movl $0, %ecx
	xsetbv

	vmcall

	hlt	

test_guest_end:
	nop

.code16
reset_vector:
	ljmp $0, $0x7c00	
