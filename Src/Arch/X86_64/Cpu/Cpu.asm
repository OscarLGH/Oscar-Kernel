;*
;* X86_64 ISA related implementation
;* Oscar
;* 2017.4
;*

global X64GetCpuId
global X64ReadMsr
global X64WriteMsr
global X64ReadTsc
global X64ReadCr
global X64WriteCr
global X64GetFlagsReg
global X64EnableLocalIrq
global X64DisableLocalIrq
global X64GetSpinLock
global X64ReleaseSpinLock
global X64HaltCpu
global X64RefreshTlb



; @ProtoType:	VOID X64GetCpuId(UINT32 EAX, UINT32 ECX, UINT8 * Buffer);
; @Function:	Get CPU information.
; @Param:		IN eax
; @Param:		IN ecx
; @Param:		IN Buffer Address, OUT edx:ecx:ebx:eax->Buffer
X64GetCpuId:
	push rbx
	mov eax, edi
	mov ecx, esi
	mov r8, rdx
	cpuid
	mov [r8], eax
	mov [r8 + 4], ebx
	mov [r8 + 8], ecx
	mov [r8 + 12], edx
	pop rbx
	ret
		
; @ProtoType:	UINT64 X64ReadMsr(UINT64 Index);
; @Function:	Read Model-Specific Register.
; @Param:		IN	MSR Index
; @Ret:			OUT	64-bit MSR Value,edx:eax -> rax
X64ReadMsr:
	mov rcx, rdi
	rdmsr
	shl rdx, 32
	or  rax, rdx
	ret

; @ProtoType:	UINT64 X64WriteMsr(UINT64 Index, UINT64 Value);
; @Function:	Write Model-Specific Register.
; @Param:		IN	MSR Index
; @Param:		IN	64-bit MSR Value
X64WriteMsr:
	mov rcx, rdi
	mov eax, esi
	shr rsi, 32
	mov edx, esi
	wrmsr
	ret
	
; @ProtoType:	UINT64 X64ReadTsc(VOID);
; @Function:	Read Time-Stamp Counter register.
; @Ret:  OUT	64-bit TSC Value, edx:eax -> rax
X64ReadTsc:
	rdtsc
	shl rdx, 32
	or  rax, rdx
	ret

; @ProtoType:	UINT64 X64ReadCr(UINT64 Index);
; @Function:	Read Control Register.
; @Param:		IN	Index of control register,0-15
; @Ret:			OUT	Value of control register,crn->rax
X64ReadCr:

RdCr0:
	cmp rdi, 0
	jne RdCr1
	mov rax, cr0
	ret

RdCr1:
	cmp rdi, 1
	jne RdCr2
	mov rax, cr1
	ret

RdCr2:
	cmp rdi, 2
	jne RdCr3
	mov rax, cr2
	ret

RdCr3:
	cmp rdi, 3
	jne RdCr4
	mov rax, cr3
	ret

RdCr4:
	cmp rdi, 4
	jne RdCr8
	mov rax, cr4
	ret

RdCr8:
	cmp rdi, 8
	jne RdCr15
	mov rax, cr8
	ret

RdCr15:
	ret

; @ProtoType:	UINT64 X64WriteCr(UINT64 Index, UINT64 Value);
; @Function:	Write Control Register.
; @Param: Index of control register,0-15
; @Param: Value write to register.
X64WriteCr:

WrCr0:
	cmp rdi, 0
	jne WrCr1
	mov cr0, rsi
	ret

WrCr1:
	cmp rdi, 1
	jne WrCr2
	mov cr1, rsi
	ret

WrCr2:
	cmp rdi, 2
	jne WrCr3
	mov cr2, rsi
	ret

WrCr3:
	cmp rdi, 3
	jne WrCr4
	mov cr3, rsi
	ret

WrCr4:
	cmp rdi, 4
	jne WrCr8
	mov cr4, rsi
	ret

WrCr8:
	cmp rdi, 8
	jne WrCr15
	mov cr8, rsi
	ret

WrCr15:
	ret


; @ProtoType:	UINT64 X64GetFlagsReg(VOID);
; @Function:	read RFLAGS register.
; @Ret:			OUT	RFLAGS register
X64GetFlagsReg:
	pushfq
	pop rax
	ret

; @ProtoType:	VOID X64EnableLocalIrq(VOID);
; @Function:	Enable IRQ on local CPU.
X64EnableLocalIrq:
	sti
	ret
	
; @ProtoType:	VOID X64DisableLocalIrq(VOID);
; @Function:	Disable IRQ on local CPU.
X64DisableLocalIrq:
	cli
	ret

; @ProtoType:	VOID X64GetSpinLock(UINT64 * SpinLock);
; @Function:	Aquire spinlock.
; @Param:		IN	Pointer of SpinLock
X64GetSpinLock:
	lock bts QWORD[rdi], 0
	pause
	jc X64GetSpinLock
	ret
		
; @ProtoType:	VOID X64ReleaseSpinLock(UINT64 * SpinLock);
; @Function:	Release spinlock.
; @Param:		IN	Pointer of SpinLock
X64ReleaseSpinLock:
	lock btr QWORD[rdi], 0
	ret

; @ProtoType:	VOID X64HaltCpu(VOID);
; @Function:	Put CPU to idle state.
X64HaltCpu:
	hlt
	ret

; @ProtoType:	VOID X64InvalidateTlb(UINT64 LinearAddress);
; @Function:	Invalidate TLB entries for page containing Param1.
; @Param:		IN	Linear address of page frame to invalidate
X64InvalidateTlb:
	invlpg [rdi]
	ret

