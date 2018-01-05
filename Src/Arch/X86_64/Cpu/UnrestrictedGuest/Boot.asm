%include "Cpu/lm.inc"

global GuestStartup16
global ResetVector
global GuestCodeEnd

%define PHYSICAL_ADDRESS 0x7c00

[section .text]


bits 16

GuestStartup16:

	cli
	
	mov ax, 0
	mov ds, ax
	; 加载GDT
	lgdt [(GDTPtr - GuestStartup16) + PHYSICAL_ADDRESS]
	; 为什么只能用CS？？？
	; VMCS中对段寄存器的属性要设置正确，Unusable会导致使用时报#GP异常
	; 数据段Expand-down类型段界限 - 0xffffffff才是正确的范围。超出范围会#GP异常

	; 关外部中断
	cli

	; 关不可屏蔽中断
	in al, 0x70                ; port 0x70
	or al, 0x80                ; disable all NMI source
	out 0x70, al
	
	; 加载IDT
	; lidt [IDT_POINTER]
	
	; 打开A20地址线
	in	al, 92h
	or	al, 00000010b
	out	92h, al
	
	; 开启保护模式位PE
	mov eax, cr0
	bts eax, 0
	mov cr0, eax

	; 跳转进入保护模式
	jmp DWORD Selector32C:(GuestStartup32 - GuestStartup16 + PHYSICAL_ADDRESS)	; 此指令会刷新段寄存器不可见部分和cache

	nop
	
bits 32
GuestStartup32:
	mov ax, Selector32D
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; 开启PAE
	mov eax, cr4
	bts eax, 5
	mov cr4, eax

	; 设置页表(LEGACY PAE)

	;mov ebx, 0x100000
	; PDPTR[0x0]			
	;mov eax, 0x101000
	;or eax, PG_P
	;mov [ebx + 8 * 0], eax
	;mov DWORD[ebx + 8 * 0 + 4], 0

	; PDPT[0x0]
	;mov ebx, 0x101000
	;mov eax, 0
	;or eax, PG_RWW | PG_USS | PG_P | PG_S
	;mov [ebx + 8 * 0], eax
	;mov DWORD[ebx + 8 * 0 + 4], 0


	; 设置页表(Long Mode)
	mov ebx, 0x100000
	; PML4T[0x0]			
	mov eax, 0x101000
	or eax, PG_RWW | PG_USS | PG_P
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; PDPT[0x0]
	mov ebx, 0x101000
	mov eax, 0x102000
	or eax, PG_RWW | PG_USS | PG_P
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; PDT[0x0]
	mov ebx, 0x102000
	mov eax, 0
	or eax, PG_RWW | PG_USS | PG_P | PG_S
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; PDT[0x1]
	mov ebx, 0x102000
	mov eax, 0x200000
	or eax, PG_RWW | PG_USS | PG_P | PG_S
	mov [ebx + 8 * 1], eax
	mov DWORD[ebx + 8 * 1 + 4], 0

	; PDT[0x2]
	mov ebx, 0x102000
	mov eax, 0x400000
	or eax, PG_RWW | PG_USS | PG_P | PG_S
	mov [ebx + 8 * 2], eax
	mov DWORD[ebx + 8 * 2 + 4], 0

	; PDT[0x3]
	mov ebx, 0x102000
	mov eax, 0x600000
	or eax, PG_RWW | PG_USS | PG_P | PG_S
	mov [ebx + 8 * 3], eax
	mov DWORD[ebx + 8 * 3 + 4], 0

	; 加载页表PML4到CR3寄存器
	mov eax, 0x100000
	mov cr3, eax

	; 打开LongMode使能IA32_EFER.LME
	mov ecx, 0C0000080H 
	rdmsr	
	bts eax, 8
	wrmsr

	; 开启分页机制PG
	mov eax, cr0
	bts eax, 31
	bts eax, 0
	mov cr0, eax

	
	; 跳转到64位代码段
	jmp Selector64C:(GuestStartup64 - GuestStartup16 + PHYSICAL_ADDRESS)
	
bits 64
GuestStartup64:

	mov ax, Selector64D
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	;mov ss, ax

	;xor ax, ax
	
	;mov rsi, 0x10000
	;mov rcx, 0x10
	;mov dx, 0x100
	;rep lodsq

	;in al, 21h

	;mov rdi, 0x10000000
	;mov rax, [rdi]
	jmp Header64
	jmp $
;                            	      段基址        段界限, 属性
LABEL_GDT:
LABEL_DESC_NULL:	Descriptor 	0,            0, 0            			  ; 空描述符
LABEL_DESC_32_C:  	Descriptor 	0,      0fffffh, ATTR_32|ATTR_C 
LABEL_DESC_32_D:  	Descriptor 	0,      0fffffh, ATTR_32|ATTR_DW|ATTR_LIMIT_4K 
LABEL_DESC_64_C:  	Descriptor 	0,      0fffffh, ATTR_64|ATTR_C 
LABEL_DESC_64_D:  	Descriptor 	0,      0fffffh, ATTR_64|ATTR_D 


GDTLen			equ	$ - LABEL_GDT

GDTPtr:			
GDTLimit		dw	GDTLen - 1						; 段界限
GDTAdd			dd	LABEL_GDT - GuestStartup16 + PHYSICAL_ADDRESS		; 基地址

; GDT 选择子
Selector32C		equ	LABEL_DESC_32_C	- LABEL_GDT
Selector32D		equ	LABEL_DESC_32_D	- LABEL_GDT
Selector64C		equ	LABEL_DESC_64_C	- LABEL_GDT
Selector64D		equ	LABEL_DESC_64_D	- LABEL_GDT

bits 16
ResetVector:
	jmp 0:0x7c00

size   equ $ - GuestStartup16
%if size+2 >512
%error "code is too large for boot sector"
%endif
times      (512 - size - 2) db 0
 
db   0x55, 0xAA          ;2 byte boot signature



; 导入函数
%define KERNEL_SPACE 0xFFFF800000000000
extern GDT_GUEST
extern IDT_GUEST
extern DescriptorSetGuest


%xdefine PHYS2VIRT(x) ((x+KERNEL_SPACE))
%xdefine VIRT2PHYS(x) ((x-KERNEL_SPACE))

%define PML4T_BASE 0x180000
%define PDPT_BASE PML4T_BASE + 0x1000
%define PDT_BASE PDPT_BASE + 0x1000 * 16

%define KERNEL_INIT_STACK 0x600000

extern GuestLoop

bits 64

Header64:
	
	;mov rax, 0x1122334455667788
	;mov rbx, 0x1234567887654321
	;mov rcx, 0x8765432112345678
	;mov rdx, 0x2345678998765432
	;mov rsi, 0x3456789009876543
	;mov rdi, 0x1111222233334444
	;mov rbp, 0x4433221111223344
	;mov r8,  0x9988776666778899
	;mov r9,  0x8877665555667788
	;mov r10, 0x6655443333445566
	;mov r11, 0x6789987623455432
	;mov r12, 0xaabbccddddccbbaa
	;mov r13, 0xccddeeffffeeddcc
	;mov r14, 0xabcddcbaabcddcba
	;mov r15, 0xaaaabbbbccccdddd

	mov rsp, KERNEL_INIT_STACK
	mov rax, VIRT2PHYS(SetupKernelPageGuest)
	call rax

StackRetAddr:	

	mov rax, VirtMemStart
	jmp rax				;切换到虚拟地址空间

; 内核虚拟地址入口
VirtMemStart:
	
	in al, 0x86

	in al, 0x87
	call	DescriptorSetGuest

	in al, 0x88

	mov 	rax,GDT_GUEST
	LGDT	[rax]
	mov	rax,IDT_GUEST
	LIDT	[rax]

	in al, 0x89

	mov rax, LongAddr64
	jmp QWORD far[rax]

	
; CS段选择子
LongAddr64:
	dq	StartAddr
	dw	16

StartAddr:
	mov ax, 32
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov ecx, 1Bh
	rdmsr
	bts eax, 11							; enable = 1
	wrmsr

	;mov rdi, 0xffff8000fee000b0
	;mov rax, [rdi]

	;ud2
	
	sti

	mov rax, GuestLoop
	call rax
	
	jmp $
;---------------------------------------------------------------------------------
; 函数名: void SetupKernelPage(void)
;---------------------------------------------------------------------------------
; 运行环境:
;	64位longmode等地址映射模式
; 作用:
;	将内核空间映射到0xFFFF800000000000处
; 
;--------------------------------------------------------------------------------
SetupKernelPageGuest:
	
	mov rdi, PML4T_BASE
	mov rdx, rdi
	
ClearPageArea:
	mov QWORD[rdx], 0
	add rdx, 8
	cmp rdx, 0x200000
	jl ClearPageArea;
	
	; PML4T[0x100]			; 0xFFFF800000000000 - 0xFFFF807FFFFFFFFF
	mov rax, PDPT_BASE
	or rax, PG_RWW | PG_USS | PG_P
	mov [rdi + 8 * 0x100], rax	; PML4T[0x100] = PDPT_BASE

	; PMT4T[0x170]			; Map for 64bit PCI BAR.
					; 0xFFFFB83F00000000 - 0xFFFFB83FFFFFFFFF
	mov rax, PDPT_BASE + 0x1000 * 2
	or rax, PG_RWW | PG_USS | PG_P
	mov [rdi + 8 * 0x170], rax	; PMT4T[0x170] = PDPT_BASE + 0x1000 * 2

	; PML4T[0]			; 0x0000000000000000 - 0x0000007FFFFFFFFF
	mov rax, PDPT_BASE + 0x1000 * 1
	or rax, PG_RWW | PG_USS | PG_P
    	mov [rdi], rax			; PMT4T[  0x0] = PDPT_BASE + 0x1000 * 1

	; PDPT[0] -> PDPT[127]
	
	mov rcx, 0
	mov rdi, PDPT_BASE
	mov rax, PDT_BASE
PStart0:

	or  rax, PG_RWW | PG_USU | PG_P
	
	mov [rdi + 8 * rcx], rax
	
	add rax, 0x1000
	inc rcx
	cmp rcx, 128
	
	jne	PStart0

	; PDPT15[0x7e] -> PDPT15[0x81] 4GB 1GB Page mapping
	mov rdi, PDPT_BASE + 0x1000 * 2
	mov rax, 0x383F00000000
	or rax, PG_RWW | PG_USS | PG_P | PG_PCD | PG_S | PG_PWT
	mov [rdi + 8 * 0xfc], rax
	add rax, 0x40000000
	mov [rdi + 8 * 0xfd], rax
	add rax, 0x40000000
	mov [rdi + 8 * 0xfe], rax
	add rax, 0x40000000
	mov [rdi + 8 * 0xff], rax

	; PDPT
	
	; PDT
	; 映射64GB物理内存空间
	mov rax, 0x0000000000000000				; 要映射的物理地址
	mov r15, 0x4000						; 映射大小32GB/2MB
	mov rdx, PDT_BASE
	mov rcx, 0

	; 0xE0000000 -> 0xFFFFFFFF 为设备占用区域，禁用Cache，使用WT
	mov r8, 0xE0000000
	mov r9, 0x100000000
	
PStart1:
	
	cmp rax, r8
	jl	PageCache
	cmp rax, r9
	jg	PageCache
	
	or rax, PG_RWW | PG_USS | PG_P | PG_PCD | PG_S
	
	jmp SetPage
	
	; 普通页面缓存策略使用WB
PageCache:
	
	or  rax, PG_RWW | PG_USS | PG_P | PG_S
	
SetPage:
	
	mov [rdx + rcx * 8], rax
	
	add rax, 0x200000

	inc rcx
	cmp rcx, r15
	jl PStart1

SetupEqualPage:
	; 等地址映射0-7M物理地址空间，不映射此地址将导致切换页表后取指令失败
	mov rbx, PDPT_BASE + 0x1000 * 1
	mov rdx, PDT_BASE + 0x1000 * 128
	mov rax, rdx
	or  rax, PG_RWW | PG_USU | PG_P

	mov QWORD[rbx + 0*8], rax
	
	mov QWORD[rdx + 0*8], 0x000000 | PG_RWW | PG_USS | PG_P | PG_S
	mov QWORD[rdx + 1*8], 0x200000 | PG_RWW | PG_USS | PG_P | PG_S
	mov QWORD[rdx + 2*8], 0x400000 | PG_RWW | PG_USS | PG_P | PG_S
	mov QWORD[rdx + 3*8], 0x600000 | PG_RWW | PG_USS | PG_P | PG_S
	
	;mov cr3, r14
	mov rax, PML4T_BASE
	mov cr3, rax

	mov rax, KERNEL_SPACE
	add rsp, rax
	
	ret




;#################################################################
; Exception/IRQ Vectors:
;#################################################################

global IrqTableGuest

IrqTableGuest:
%assign i 0
%rep	0x100-0x20
dq HWIntVectorGuest%+i
%assign i i+1
%endrep

global ExceptionTableGuest
ExceptionTableGuest:
dq ExceptionVector00 			;#DE
dq ExceptionVector01       		;#DB
dq ExceptionVector02			;NMI		
dq ExceptionVector03			;#BP
dq ExceptionVector04			;#OF
dq ExceptionVector05			;#BR
dq ExceptionVector06			;#UD
dq ExceptionVector07			;#NM
dq ExceptionVector08			;#DF
dq ExceptionVector09			;Not Used
dq ExceptionVector0A			;#TS
dq ExceptionVector0B			;#NP
dq ExceptionVector0C			;#SS
dq ExceptionVector0D			;#GP
dq ExceptionVector0E			;#PF
dq ExceptionVector0F			;Not Used
dq ExceptionVector10			;#MF
dq ExceptionVector11			;#AC
dq ExceptionVector12			;#MC	
dq ExceptionVector13			;#XM
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0
dq 0

extern IrqHandlerGuest
extern TSS_GUEST

%macro SaveContext 0
	push rax
	push rbx
	push rcx
	push rdx
	;push rsp
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	xor rax, rax
	mov ax, ds
	push rax
	mov ax, es
	push rax
	mov ax, fs
	push rax
	mov ax, gs
	push rax
%endmacro

%macro RestoreContext 0
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
%endmacro

SpinLock	dq	0

%macro  ESR    1
		SaveContext
		
		mov rax, cr8
		push rax
		mov rax, cr4
		push rax
		mov rax, cr3
		push rax
		mov rax, cr2
		push rax
		mov rax, cr0
		push rax
		
		xor rcx, rcx
		str	cx
		shr cx, 4
		sub cx, 64
		imul rdx, rcx, 104
		mov r8, TSS_GUEST
		lea rbx, [r8 + rdx]	
		mov [rbx + 44], rsp
		mov rdi, %1
		mov rax, IrqHandlerGuest
		call rax
		
		add rsp, 72
		RestoreContext
		add rsp, 8
		
		iretq
		
SpinLock%1	dq	0
%endmacro

ExceptionRoutineTable:
ALIGN 16
ExceptionVector00:
		push 0xFFFFFFFFFFFFFFFF
		ESR    0
		
ALIGN 16
ExceptionVector01: 
		push 0xFFFFFFFFFFFFFFFF
		ESR    1
		
ALIGN 16
ExceptionVector02:
		push 0xFFFFFFFFFFFFFFFF
		ESR    2
		
ALIGN 16
ExceptionVector03: 
		push 0xFFFFFFFFFFFFFFFF
		ESR    3
		
ALIGN 16
ExceptionVector04:
		push 0xFFFFFFFFFFFFFFFF
		ESR    4
		
ALIGN 16
ExceptionVector05:
		push 0xFFFFFFFFFFFFFFFF
		ESR    5
		
ALIGN 16
ExceptionVector06:
		push 0xFFFFFFFFFFFFFFFF
		ESR    6
		
ALIGN 16
ExceptionVector07: 
		push 0xFFFFFFFFFFFFFFFF
		ESR    7
		
ALIGN 16
ExceptionVector08:                
		ESR    8
		
ALIGN 16
ExceptionVector09:                
		ESR    9
		
ALIGN 16
ExceptionVector0A:                
		ESR    0xA
		
ALIGN 16
ExceptionVector0B:                
		ESR    0xB
		
ALIGN 16
ExceptionVector0C:                
		ESR    0xC
		
ALIGN 16
ExceptionVector0D:                
		ESR    0xD
		
ALIGN 16
ExceptionVector0E:                
		ESR    0xE
		
ALIGN 16
ExceptionVector0F:                
		ESR    0xF
		
ALIGN 16
ExceptionVector10:
		push 0xFFFFFFFFFFFFFFFF
		ESR    0x10
		
ALIGN 16
ExceptionVector11:                
		ESR    0x11
		
ALIGN 16
ExceptionVector12: 
		push 0xFFFFFFFFFFFFFFFF
		ESR    0x12
		
ALIGN 16
ExceptionVector13:
		push 0xFFFFFFFFFFFFFFFF
		ESR    0x13



%macro  ISR    1
		SaveContext

		mov rbp, rsp						; rbp寄存器用来传递寄存器现场的栈地址

		xor rcx, rcx
		str	cx
		shr cx, 4
		sub cx, 64
		imul rdx, rcx, 104
		mov r8, TSS_GUEST
		lea rbx, [r8 + rdx]	
		;mov rsp, [rbx + 36]				; 切换到中断栈

		;APIC EOI
		mov rbx, 0xffff8000fee00000
		mov DWORD[rbx + 0xb0], 0
		;8259A EOI
		mov	al, 0x20		; EOI
		out	0x20, al		;主片EOI
		out	0xa0, al		;从片EOI
		
		mov rdi, %1
		mov rax, IrqHandlerGuest
		call rax

		;mov rsp, rbp
		RestoreContext
		iretq
		
%endmacro

%assign i 0
%rep	0x100-0x20
%assign vec i+0x20
ALIGN 16
HWIntVectorGuest%+i:                
		ISR    vec
%assign i i+1
%endrep


GuestCodeEnd:
	db 0

