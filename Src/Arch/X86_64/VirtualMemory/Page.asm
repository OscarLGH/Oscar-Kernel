;============================================
; Page.asm
; 内存分页
;============================================
global GetPageTableBase
global SetPageTableBase
global GetPageFaultAddr
global GetMaxVirtAddr
global GetMaxPhysAddr
global RefreshTLB

GetPageTableBase:
	mov rax, cr3
	and rax, 0xFFFFFFFFFFFFF000
	ret
	
SetPageTableBase:
	mov rax, rdi
	and rax, 0xFFFFFFFFFFFFF000
	mov cr3, rax
	ret
	
GetPageFaultAddr:
	mov rax, cr2
	ret
	
GetMaxVirtAddr:
	push rbx
	mov rax, 0x80000008
	cpuid
	shr rax, 8
	pop rbx
	ret
	
GetMaxPhysAddr:
	push rbx
	mov rax, 0x80000008
	cpuid
	and rax, 0xFF
	pop rbx
	ret
	
RefreshTLB:
	mov rax, rdi
	cmp rax, 0xFFFFFFFFFFFFFFFF
	je RefreshAll
	invlpg [rax]
	jmp Return
RefreshAll:
	mov rax, cr3
	mov cr3, rax
Return:
	ret
	
	
	