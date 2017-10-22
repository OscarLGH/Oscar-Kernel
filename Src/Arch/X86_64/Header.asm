;===========================================================
;Header.asm
;内核入口点
;Oscar 2015.10
;===========================================================
%include	"Cpu/Apic.inc"
%include	"Cpu/lm.inc"
; 导入函数
%define KERNEL_SPACE 0xFFFF800000000000
;%define KERNEL_SPACE 0x0000001000000000
;%define KERNEL_SPACE 0
extern GDT
extern IDT
extern DescriptorSet

extern SysCon;

extern InitMain
extern InitMSG

extern Delay
extern ConsoleInit
extern ClearBSS
extern ArchInit
extern MemoryManagerInit
extern EnableIrq
extern GetIrqStatus

CPUCNT	 	equ 0x10028
CPUIDARRAY	equ 0x10030

%xdefine PHYS2VIRT(x) ((x+KERNEL_SPACE))
%xdefine VIRT2PHYS(x) ((x-KERNEL_SPACE))
%define APStartupVectorAddr 0x20000

%define PML4T_BASE 0x100000
%define PDPT_BASE PML4T_BASE + 0x1000
%define PDT_BASE PDPT_BASE + 0x1000 * 16

%define KERNEL_INIT_STACK 0x600000

[section .text]

global BSPEntry64

bits 64

BSPEntry64:
	mov rsp, KERNEL_INIT_STACK
	mov rax, VIRT2PHYS(SetupKernelPage)
	call rax
	
StackRetAddr:	

	mov rax, VirtMemStart
	jmp rax				;切换到虚拟地址空间

; 内核虚拟地址入口
VirtMemStart:
	
	call 	ClearBSS
	call	DescriptorSet

	mov 	rax,GDT
	LGDT	[rax]
	mov	rax,IDT
	LIDT	[rax]

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
	
	; 保存每个CPU的APIC_ID
	xor rax, rax
	mov rdx, PHYS2VIRT(APIC_BASE + 20H)
	mov eax, [rdx]						; APIC ID寄存器不能读64位，会导致重启
	shr eax, 24
	mov rbx, PHYS2VIRT(CPUIDARRAY)
	mov [rbx],rax
	
	mov rbx, PHYS2VIRT(CPUCNT)
	mov QWORD[rbx], 1
	
	mov rsp, PHYS2VIRT(KERNEL_INIT_STACK)
	
	mov ax, 16 * 64
	ltr ax
	
	mov ecx, 1Bh
	rdmsr
	bts eax, 11							; enable = 1
	wrmsr
	
	
	;mov ecx, 1Bh
	;rdmsr
	;or eax, 0xc00						; bit 10, bit 11 置位
	;wrmsr
	
	;mov rbx,PHYS2VIRT(0xFEE00000)
	;bts DWORD [rbx + 0xF0], 8          ; SVR.enable = 1

	call MemoryManagerInit

	mov rdi, SysCon
	call ConsoleInit
	
	call InitMSG
	
	call APStart

	call ArchInit
	
	call InitMain
	
	jmp $

SyncSpinLock: dq  0
;---------------------------------------------------------------------------------
; 函数名: void SetupKernelPage(void)
;---------------------------------------------------------------------------------
; 运行环境:
;	64位longmode等地址映射模式
; 作用:
;	将内核空间映射到0xFFFF800000000000处
; 
;--------------------------------------------------------------------------------
SetupKernelPage:
	
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
	
	or rax, PG_RWW | PG_USS | PG_P | PG_PCD | PG_S | PG_PWT
	
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
	
IODelay:
	mov ax, 0
	out 0x80,ax
	ret
	
;==================================================================================
;函数名:APStart
;作用:启动所有AP处理器
;参数:无
;==================================================================================
APStart:

	;复制初始化代码到BaseOfAPStartupAddr处供AP读取
	mov rsi, AP_Startup_Vector
	mov rdi, APStartupVectorAddr
	mov rcx, AP_Startup_Vector_End - AP_Startup_Vector;
	rep movsb
	
	;BSP发送IPI-SIPI-SIPI序列激活AP
	mov rbx, PHYS2VIRT(APIC_BASE + 300H)
	
	mov DWORD [rbx],0x000C4500
	mov rcx,10000
	rep call IODelay
	
	mov DWORD [rbx],0x000C4600|(APStartupVectorAddr>>12)
	mov rcx,500
	rep call IODelay
	
	mov DWORD [rbx],0x000C4600|(APStartupVectorAddr>>12)
	mov rcx,500
	rep call IODelay
	
	ret
	
;==================================================================================
;AP启动代码
%define APVIRT2PHYS(x) (x - AP_Startup_Vector + APStartupVectorAddr)
%define APOFFSET(x) (x - AP_Startup_Vector)
bits 16
AP_Startup_Vector:
	
	;以下是AP启动代码
APEntry16:
	;此处CS应该被IPI机制自动设置为正确的地址，无需重复设置
	;但DS的地址由于全局变量的存在应该设置为全局地址
	;之前没有设置DS导致AP写入到错误的地址
	; 加载Loader基地址到ds,es,ss
	; AP收到广播的IPI-SIPI-SIPI信号后，CS = 2000H 数据段需要重新加载才能正确找到锁地址
	mov ax, cs
	mov ds, ax
	
	; 加载GDT
	lgdt [APOFFSET(GDTPtr)]
	
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
	jmp DWORD Selector32C:(APVIRT2PHYS(APEntry32))	; 此指令会刷新段寄存器不可见部分和cache

	nop
	
bits 32
APEntry32:
	; 开启PAE
	mov eax, cr4
	bts eax, 5
	mov cr4, eax
	
	; 加载页表PML4到CR3寄存器
	mov eax, PML4T_BASE
	mov cr3, eax
	
	; 打开LongMode使能
	mov ecx, 0C0000080H 
	rdmsr
	bts eax, 8
	wrmsr
	
	; 开启分页机制PG
	mov eax, cr0
	bts eax, 31
	mov cr0, eax
	
	; 跳转到64位代码段
	jmp Selector64C:(VIRT2PHYS(APEntry64))
	
bits 64
APEntry64:
	; 使用BSP设置好的GDT IDT
	mov 	rax,GDT
	LGDT	[rax]
	mov	rax,IDT
	LIDT	[rax]
	
	mov rax, APLongAddr64
	jmp QWORD far[rax]
	
; CS段选择子
APLongAddr64:
	dq	APStartAddr
	dw	16

APStartAddr:
	mov ax, 32
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
SpinLock_AP:
	mov rax,APSpinLock
	lock bts DWORD[rax],0
	jc SpinLock_AP
	
	; 保存每个CPU的APIC_ID
	mov rbx, PHYS2VIRT(CPUCNT)
	mov rcx, [rbx]
	xor rax, rax
	mov rdx, PHYS2VIRT(APIC_BASE + 20H)
	mov eax, [rdx]						; APIC ID寄存器不能读64位，会导致重启
	shr eax, 24
	mov rbx, PHYS2VIRT(CPUIDARRAY)
	mov [rbx + rcx * 8],rax
	
	mov rbx, PHYS2VIRT(CPUCNT)
	inc QWORD[rbx]
	mov rax, [rbx]
	
	mov r8, rax
	add rax, 64
	shl rax, 4
	
	;mov ax,80
	ltr ax
	
	mov rsp, PHYS2VIRT(KERNEL_INIT_STACK)
	mov rax, r8
	dec rax
	mov rbx, 0x10000
	mul rbx
	sub rsp, rax
	
	mov ecx, 1Bh
	rdmsr
	bts eax, 11							; enable = 1
	wrmsr
	
	
	;mov ecx, 1Bh
	;rdmsr
	;or eax, 0xc00						; bit 10, bit 11 置位
	;wrmsr
	
	mov rbx,PHYS2VIRT(0xFEE00000)
	bts DWORD [rbx + 0xF0], 8          ; SVR.enable = 1
	
	call InitMSG

	mov rax,APSpinLock
	lock btr DWORD[rax],0

extern ApStartSig
WaitForBSPDone:
	mov rax, ApStartSig
	test QWORD[rax], 1
	jz WaitForBSPDone
	
	sti
	
HaltAp:
	hlt
	jmp HaltAp
	
	jmp $
;                            	段基址   段界限, 属性
LABEL_GDT:
LABEL_DESC_NULL:	Descriptor 	0,            0, 0            			  ; 空描述符
LABEL_DESC_32_C:  	Descriptor 	0,      0fffffh, ATTR_32|ATTR_C 
LABEL_DESC_64_C:  	Descriptor 	0,      0fffffh, ATTR_64|ATTR_C 


GDTLen			equ	$ - LABEL_GDT

GDTPtr:			
GDTLimit		dw	GDTLen - 1							; 段界限
GDTAdd			dd	APVIRT2PHYS(LABEL_GDT)				; 基地址

; GDT 选择子
Selector32C		equ	LABEL_DESC_32_C	- LABEL_GDT
Selector64C		equ	LABEL_DESC_64_C	- LABEL_GDT

APSpinLock		dq	0

	
AP_Startup_Vector_End:
