;extern KERNEL_BASE
%define KERNEL_BASE						0xFFFF800000000000
%define APIC_BASE						(0xFEE00000 + KERNEL_BASE)
%define LVT_LINT0						0x350
%define IOAPIC_INDEX					(0xFEC00000 + KERNEL_BASE)
%define IOAPIC_DATA						(0xFEC00010 + KERNEL_BASE)
%define EOI								0xB0
;%include	"APIC.inc"
[section .text]
[bits 64]
; 导出函数

global IrqTable

IrqTable:
%assign i 0
%rep	0x100-0x20
dq HWIntVector%+i
%assign i i+1
%endrep

global ExceptionTable
ExceptionTable:
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


global TraceHandler

extern IrqHandler
extern TSS
extern Schedule
extern RegisterDump

extern Delay
extern DefaultExceptionHandle

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
		mov r8, TSS
		lea rbx, [r8 + rdx]	
		mov [rbx + 44], rsp
		mov rdi, %1
		mov rax, IrqHandler
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


extern GetCurrentThread
extern ArchTaskSchedule

%macro  ISR    1
		SaveContext

		mov rbp, rsp						; rbp寄存器用来传递寄存器现场的栈地址

		xor rcx, rcx
		str	cx
		shr cx, 4
		sub cx, 64
		imul rdx, rcx, 104
		mov r8, TSS
		lea rbx, [r8 + rdx]	
		;mov rsp, [rbx + 36]				; 切换到中断栈

		;APIC EOI
		mov rbx, APIC_BASE
		mov DWORD[rbx + EOI], 0
		;8259A EOI
		mov	al, 0x20		; EOI
		out	0x20, al		;主片EOI
		out	0xa0, al		;从片EOI
		
		mov rdi, %1
		mov rax, IrqHandler
		call rax

		mov rax, ArchTaskSchedule
		call rax
		; if schedule occurs, this point can not be reached.

		;mov rsp, rbp
		RestoreContext
		iretq
		
%endmacro

%assign i 0
%rep	0x100-0x20
%assign vec i+0x20
ALIGN 16
HWIntVector%+i:                
		ISR    vec
%assign i i+1
%endrep

TraceHandler:

		mov rsi, rdi
		xor rcx, rcx
		str	cx
		shr cx, 4
		sub cx, 64
		imul rdx, rcx, 104
		mov r8, TSS
		lea rbx, [r8 + rdx]	
		mov rdi, [rbx + 44]				; 得到压栈后的RSP
		
		call RegisterDump
		ret


