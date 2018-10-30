.global vm_launch
.global vm_exit
.global vm_resume
.text
VMCS_HOST_RSP = 0x00006c14
//int vm_launch(void *host_reg, void *guest_reg);

vm_launch:
	push %rdi
	push %rsi

	movq %rax, 0x0(%rdi)
	movq %rbx, 0x8(%rdi)
	movq %rcx, 0x10(%rdi)
	movq %rdx, 0x18(%rdi)
	movq %rsi, 0x20(%rdi)
	movq %rdi, 0x28(%rdi)
	movq %rsp, 0x30(%rdi)
	movq %rbp, 0x38(%rdi)
	movq %r8, 0x40(%rdi)
	movq %r9, 0x48(%rdi)
	movq %r10, 0x50(%rdi)
	movq %r11, 0x58(%rdi)
	movq %r12, 0x60(%rdi)
	movq %r13, 0x68(%rdi)
	movq %r14, 0x70(%rdi)
	movq %r15, 0x78(%rdi)

	mov %rsp, %rax
	mov $VMCS_HOST_RSP, %rbx
	vmwrite %rax, %rbx

	movq 0x0(%rsi), %rax
	movq 0x8(%rsi), %rbx
	movq 0x10(%rsi), %rcx
	movq 0x18(%rsi), %rdx
	//movq 0x20(%rsi), %rsi
	movq 0x28(%rsi), %rdi
	//movq 0x30(%rsi), %rsp
	movq 0x38(%rsi), %rbp
	movq 0x40(%rsi), %r8
	movq 0x48(%rsi), %r9
	movq 0x50(%rsi), %r10
	movq 0x58(%rsi), %r11
	movq 0x60(%rsi), %r12
	movq 0x68(%rsi), %r13
	movq 0x70(%rsi), %r14
	movq 0x78(%rsi), %r15

	vmlaunch

	popq %rsi
	popq %rdi

	movq 0x0(%rdi), %rax
	movq 0x8(%rdi), %rbx
	movq 0x10(%rdi), %rcx
	movq 0x18(%rdi), %rdx
	//movq 0x20(%rdi), %rsi
	//movq 0x28(%rdi), %rdi
	//movq 0x30(%rdi), %rsp
	movq 0x38(%rdi), %rbp
	movq 0x40(%rdi), %r8
	movq 0x48(%rdi), %r9
	movq 0x50(%rdi), %r10
	movq 0x58(%rdi), %r11
	movq 0x60(%rdi), %r12
	movq 0x68(%rdi), %r13
	movq 0x70(%rdi), %r14
	movq 0x78(%rdi), %r15

	movabsq $-1, %rax
	ret

//int vm_resume(void *host_reg, void *guest_reg);

vm_resume:
	push %rdi
	push %rsi

	movq %rax, 0x0(%rdi)
	movq %rbx, 0x8(%rdi)
	movq %rcx, 0x10(%rdi)
	movq %rdx, 0x18(%rdi)
	movq %rsi, 0x20(%rdi)
	movq %rdi, 0x28(%rdi)
	movq %rsp, 0x30(%rdi)
	movq %rbp, 0x38(%rdi)
	movq %r8, 0x40(%rdi)
	movq %r9, 0x48(%rdi)
	movq %r10, 0x50(%rdi)
	movq %r11, 0x58(%rdi)
	movq %r12, 0x60(%rdi)
	movq %r13, 0x68(%rdi)
	movq %r14, 0x70(%rdi)
	movq %r15, 0x78(%rdi)

	mov %rsp, %rax
	mov $VMCS_HOST_RSP, %rbx
	vmwrite %rax, %rbx

	movq 0x0(%rsi), %rax
	movq 0x8(%rsi), %rbx
	movq 0x10(%rsi), %rcx
	movq 0x18(%rsi), %rdx
	//movq 0x20(%rsi), %rsi
	movq 0x28(%rsi), %rdi
	//movq 0x30(%rsi), %rsp
	movq 0x38(%rsi), %rbp
	movq 0x40(%rsi), %r8
	movq 0x48(%rsi), %r9
	movq 0x50(%rsi), %r10
	movq 0x58(%rsi), %r11
	movq 0x60(%rsi), %r12
	movq 0x68(%rsi), %r13
	movq 0x70(%rsi), %r14
	movq 0x78(%rsi), %r15

	vmresume

	popq %rsi
	popq %rdi

	movq 0x0(%rdi), %rax
	movq 0x8(%rdi), %rbx
	movq 0x10(%rdi), %rcx
	movq 0x18(%rdi), %rdx
	//movq 0x20(%rdi), %rsi
	//movq 0x28(%rdi), %rdi
	//movq 0x30(%rdi), %rsp
	movq 0x38(%rdi), %rbp
	movq 0x40(%rdi), %r8
	movq 0x48(%rdi), %r9
	movq 0x50(%rdi), %r10
	movq 0x58(%rdi), %r11
	movq 0x60(%rdi), %r12
	movq 0x68(%rdi), %r13
	movq 0x70(%rdi), %r14
	movq 0x78(%rdi), %r15

	movq $-1, %rax
	ret

vm_exit:

	pushq %rdi
	pushq %rsi

	movq 0x10(%rsp), %rsi
	movq 0x18(%rsp), %rdi

	movq %rax, 0x0(%rsi)
	movq %rbx, 0x8(%rsi)
	movq %rcx, 0x10(%rsi)
	movq %rdx, 0x18(%rsi)
	movq %rsi, 0x20(%rsi)
	movq %rdi, 0x28(%rsi)
	movq %rsp, 0x30(%rsi)
	movq %rbp, 0x38(%rsi)

	popq %rax
	popq %rbx
	
	movq %rax, 0x40(%rsi)
	movq %rbx, 0x48(%rsi)
	movq %r10, 0x50(%rsi)
	movq %r11, 0x58(%rsi)
	movq %r12, 0x60(%rsi)
	movq %r13, 0x68(%rsi)
	movq %r14, 0x70(%rsi)
	movq %r15, 0x78(%rsi)

	movq 0x0(%rdi), %rax
	movq 0x8(%rdi), %rbx
	movq 0x10(%rdi), %rcx
	movq 0x18(%rdi), %rdx
	//movq 0x20(%rdi), %rsi
	//movq 0x28(%rdi), %rdi
	//movq 0x30(%rdi), %rsp
	movq 0x38(%rdi), %rbp
	movq 0x40(%rdi), %r8
	movq 0x48(%rdi), %r9
	movq 0x50(%rdi), %r10
	movq 0x58(%rdi), %r11
	movq 0x60(%rdi), %r12
	movq 0x68(%rdi), %r13
	movq 0x70(%rdi), %r14
	movq 0x78(%rdi), %r15

	popq %rsi
	popq %rdi

	xorq %rax, %rax

	ret
