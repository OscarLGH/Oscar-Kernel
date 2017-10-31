%include "Cpu/lm.inc"

global GuestStartup16
global ResetVector

%define PHYSICAL_ADDRESS 0x7c00

[section .text]


bits 16

GuestStartup16:

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

	mov ebx, 0x100000
	; PDPTR[0x0]			
	mov eax, 0x101000
	or eax, PG_P
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; PDPT[0x0]
	mov ebx, 0x101000
	mov eax, 0
	or eax, PG_RWW | PG_USS | PG_P | PG_S
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0


	; 设置页表(Long Mode)
	mov ebx, 0x200000
	; PML4T[0x0]			
	mov eax, 0x201000
	or eax, PG_RWW | PG_USS | PG_P
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; PDPT[0x0]
	mov ebx, 0x201000
	mov eax, 0x202000
	or eax, PG_RWW | PG_USS | PG_P
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; PDT[0x0]
	mov ebx, 0x202000
	mov eax, 0
	or eax, PG_RWW | PG_USS | PG_P | PG_S
	mov [ebx + 8 * 0], eax
	mov DWORD[ebx + 8 * 0 + 4], 0

	; 加载页表PML4到CR3寄存器
	mov eax, 0x200000
	mov cr3, eax

	; 打开LongMode使能IA32_EFER.LME
	mov ecx, 0C0000080H 
	rdmsr	
	bts eax, 8
	wrmsr

	in	al, 17h

	; 开启分页机制PG
	mov eax, cr0
	bts eax, 31
	bts eax, 0
	mov cr0, eax

	in	al, 18h

	;in al, 0x13
	
	;jmp $
	
	; 跳转到64位代码段
	jmp Selector64C:(GuestStartup64 - GuestStartup16 + PHYSICAL_ADDRESS)
	
bits 64
GuestStartup64:

	in al, 0x19
	mov ax, Selector64D
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	;mov ss, ax
	
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


