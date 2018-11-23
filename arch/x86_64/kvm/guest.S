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

	vmcall

	movw $(desc_data64 - gdt), %ax
	movw %ax, %ds
	movw $(desc_null - gdt), %ax
	lldt %ax

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

	vmcall
	
	movw $(desc_ldt64 - gdt), %ax
	lldt %ax
	

	movl $0x7, %eax
	movl $0, %edx
	movl $0, %ecx
	xsetbv

	mov $0x180000, %rbx
	movabsq $0x55aaaa5512345678, %rax
	movq %rax, 0x0(%rbx)
	movabsq $0x55aaaa5522345678, %rax
	movq %rax, 0x8(%rbx)
	movabsq $0x55aaaa5532345678, %rax
	movq %rax, 0x10(%rbx)
	movabsq $0x55aaaa5542345678, %rax
	movq %rax, 0x18(%rbx)

	vmovapd (%rbx), %ymm0
	vmovapd (%rbx), %ymm1
	vmovapd (%rbx), %ymm2
	vmovapd (%rbx), %ymm3
	vmovapd (%rbx), %ymm4
	vmovapd (%rbx), %ymm5

	//vaddpd %ymm0, %ymm1, %ymm2
	//vaddpd %ymm1, %ymm2, %ymm3

	movabsq $0x55aaaa5500000000, %rax
	movabsq $0x55aaaa5500000001, %rbx
	movabsq $0x55aaaa5500000002, %rcx
	movabsq $0x55aaaa5500000003, %rdx
	movabsq $0x55aaaa5500000004, %rsi
	movabsq $0x55aaaa5500000005, %rdi
	movq %rsp, %rbp
	addq $0x8000, %rbp
	movabsq $0x55aaaa5500000008, %r8
	movabsq $0x55aaaa5500000009, %r9
	movabsq $0x55aaaa5500000010, %r10
	movabsq $0x55aaaa5500000011, %r11
	movabsq $0x55aaaa5500000012, %r12
	movabsq $0x55aaaa5500000013, %r13
	movabsq $0x55aaaa5500000014, %r14
	movabsq $0x55aaaa5500000015, %r15

	vmcall

	hlt	

test_guest_end:
	nop

.code16
reset_vector:
	ljmp $0, $0x7c00	
