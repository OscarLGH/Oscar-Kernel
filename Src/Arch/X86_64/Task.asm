extern TSS

global ArchTaskSchedule
global ArchThreadStart
extern PickNextThread
extern GetCurrentThread


ArchThreadStart:

	xor rcx, rcx
	str	cx							; 保存当前TSS描述符到cx
	shr cx, 4
	sub cx, 64						; TSS描述符从GDT第65项开始
	imul rdx, rcx, 104
	mov r8, TSS
	lea rbx, [r8 + rdx]
	mov rax, [rdi + 0x10000]		; 加载目标线程内核栈指针
	mov rsp, rax
	add rax, 192
	mov [rbx + 4], rax				; 改变TSS.RSP0使下次中断发生时使用目标线程内核栈

	pop rax
	mov gs, ax
	pop rax
	mov fs, ax
	pop rax
	mov es, ax
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	;pop rsp
	pop rdx
	pop rcx
	pop rbx
	pop rax

	iretq

ArchTaskSchedule:
		
		mov rax, GetCurrentThread
		call rax
		test rax, rax
		jz PickThread

		mov rbx, rax					;rbx为当前线程指针
		
		mov rax, [rbx + 0x10008]		;检查重新调度标记
		test rax, rax
		jz NoSchedule

		mov rax, [rbx + 0x10010]		;检查抢占计数
		test rax, rax
		jnz NoSchedule
		
		mov [rbx + 0x10000], rbp		;保存之前栈指针到线程描述符中，rsp->rbp在中断处理函数中被保存

PickThread:
		
		mov rax, PickNextThread
		call rax
		
		mov rdi, rax
		mov rax, ArchThreadStart
		call rax

		; Cannot reach here
		jmp $

NoSchedule:
		ret