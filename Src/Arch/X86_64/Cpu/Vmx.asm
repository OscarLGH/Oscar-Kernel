; Intel VMX implementation.
; Oscar 
; 2017.10


global VmxOn
global VmxOff
global VmLaunch
global VmResume
global VmExit
global VmPtrLoad
global VmPtrStore
global VmClear
global VmRead
global VmWrite
global VmCall
global VmFunc
global InvalidateEpt
global InvalidateVpid


; @ProtoType:	VOID VmxOn(UINT64 VmxOnRegionPhysAddr);
; @Function:	Enter VMX operation
; @Para1:	IN	Physical address of VMXON Region,4k aligned.
VmxOn:
	push rdi
	vmxon [rsp]
	pop rdi
	ret

; @ProtoType:	VOID VmxOff(VOID);
; @Function:	Exit VMX operation
VmxOff:
	vmxoff
	ret

%define HOST_RSP 			0x00006c14
%define HOST_ES_SELECTOR                0x00000c00
%define HOST_CS_SELECTOR                0x00000c02
%define HOST_SS_SELECTOR                0x00000c04
%define HOST_DS_SELECTOR                0x00000c06
%define HOST_FS_SELECTOR                0x00000c08
%define HOST_GS_SELECTOR                0x00000c0a
%define HOST_TR_SELECTOR                0x00000c0c
%define HOST_CR0                        0x00006c00
%define HOST_CR3                        0x00006c02
%define HOST_CR4                        0x00006c04
%define HOST_FS_BASE                    0x00006c06
%define HOST_GS_BASE                    0x00006c08
%define HOST_TR_BASE                    0x00006c0a
%define HOST_GDTR_BASE                  0x00006c0c
%define HOST_IDTR_BASE                  0x00006c0e
%define HOST_IA32_SYSENTER_ESP          0x00006c10
%define HOST_IA32_SYSENTER_EIP          0x00006c12
%define HOST_RSP                        0x00006c14
%define HOST_RIP                        0x00006c16

; @ProtoType:	VOID VmLaunch(REG_STATE *HostReg, REG_STATE *GuestReg);
; @Function:	Save host registers and launch virtual machine managed by current VMCS
; Caution:	This "function" returns when vmexit.Then host will run 'VmExit' defined below.
VmLaunch:
	push rdi		; Saving host regs pointer.
	push rsi		; Saving guest regs pointer.

	; Saving host regs status.
	mov [rdi + 0 * 8], r15
	mov [rdi + 1 * 8], r14
	mov [rdi + 2 * 8], r13
	mov [rdi + 3 * 8], r12
	mov [rdi + 4 * 8], r11
	mov [rdi + 5 * 8], r10
	mov [rdi + 6 * 8], r9
	mov [rdi + 7 * 8], r8
	mov [rdi + 8 * 8], rdi
	mov [rdi + 9 * 8], rsi
	mov [rdi + 10 * 8], rbp
	mov [rdi + 11 * 8], rsp
	mov [rdi + 12 * 8], rdx
	mov [rdi + 13 * 8], rcx
	mov [rdi + 14 * 8], rbx
	mov [rdi + 15 * 8], rax

	; restoring guest regs status.
	mov r15, [rsi + 0 * 8]
	mov r14, [rsi + 1 * 8]
	mov r13, [rsi + 2 * 8]
	mov r12, [rsi + 3 * 8]
	mov r11, [rsi + 4 * 8]
	mov r10, [rsi + 5 * 8]
	mov r9, [rsi + 6 * 8]
	mov r8, [rsi + 7 * 8]
	mov rdi, [rsi + 8 * 8]
	mov rsi, [rsi + 9 * 8]
	mov rbp, [rsi + 10 * 8]
	;mov rsp, [rsi + 11 * 8]
	mov rdx, [rsi + 12 * 8]
	mov rcx, [rsi + 13 * 8]
	mov rbx, [rsi + 14 * 8]
	mov rax, [rsi + 15 * 8]

	mov rdi, HOST_TR_SELECTOR
	str si
	call VmWrite
	
	mov rdi, HOST_RSP
	mov rsi, rsp
	call VmWrite
	
	vmlaunch

; @ProtoType:	VOID VmExit(VOID);
; @Function:	vmexit handler,Save guest registers and restore host registers.
; Caution: 	This is NOT a function,means it can not be called directly.
;		VmExit should be filled in Vmcs.HostRip
VmExit:
	pop rsi			; restoring guest regs pointer.
	pop rdi			; restoring host regs pointer.
	; saving guest regs status.
	mov [rsi + 0 * 8], r15
	mov [rsi + 1 * 8], r14
	mov [rsi + 2 * 8], r13
	mov [rsi + 3 * 8], r12
	mov [rsi + 4 * 8], r11
	mov [rsi + 5 * 8], r10
	mov [rsi + 6 * 8], r9
	mov [rsi + 7 * 8], r8
	mov [rsi + 8 * 8], rdi
	mov [rsi + 9 * 8], rsi
	mov [rsi + 10 * 8], rbp
	mov [rsi + 11 * 8], rsp
	mov [rsi + 12 * 8], rdx
	mov [rsi + 13 * 8], rcx
	mov [rsi + 14 * 8], rbx
	mov [rsi + 15 * 8], rax

	;restoring host regs status.
	mov r15, [rdi + 0 * 8]
	mov r14, [rdi + 1 * 8]
	mov r13, [rdi + 2 * 8]
	mov r12, [rdi + 3 * 8]
	mov r11, [rdi + 4 * 8]
	mov r10, [rdi + 5 * 8]
	mov r9, [rdi + 6 * 8]
	mov r8, [rdi + 7 * 8]
	mov rdi, [rdi + 8 * 8]
	mov rsi, [rdi + 9 * 8]
	mov rbp, [rdi + 10 * 8]
	;mov rsp, [rdi + 11 * 8]
	mov rdx, [rdi + 12 * 8]
	mov rcx, [rdi + 13 * 8]
	mov rbx, [rdi + 14 * 8]
	mov rax, [rdi + 15 * 8]

	ret

; @ProtoType:	VOID VmResume(REG_STATE *HostReg, REG_STATE *GuestReg);
; @Function:	Save host registers and restore guest registers,then 
;		resume to guest mode.
; Caution:	This "function" returns when vmexit.Then host will run 'VmExit' defined above.
VmResume:
	push rdi		; Saving host regs pointer.
	push rsi		; Saving guest regs pointer.
	; Saving host regs status.
	mov [rdi + 0 * 8], r15
	mov [rdi + 1 * 8], r14
	mov [rdi + 2 * 8], r13
	mov [rdi + 3 * 8], r12
	mov [rdi + 4 * 8], r11
	mov [rdi + 5 * 8], r10
	mov [rdi + 6 * 8], r9
	mov [rdi + 7 * 8], r8
	mov [rdi + 8 * 8], rdi
	mov [rdi + 9 * 8], rsi
	mov [rdi + 10 * 8], rbp
	mov [rdi + 11 * 8], rsp
	mov [rdi + 12 * 8], rdx
	mov [rdi + 13 * 8], rcx
	mov [rdi + 14 * 8], rbx
	mov [rdi + 15 * 8], rax

	; restoring guest regs status.
	mov r15, [rsi + 0 * 8]
	mov r14, [rsi + 1 * 8]
	mov r13, [rsi + 2 * 8]
	mov r12, [rsi + 3 * 8]
	mov r11, [rsi + 4 * 8]
	mov r10, [rsi + 5 * 8]
	mov r9, [rsi + 6 * 8]
	mov r8, [rsi + 7 * 8]
	mov rdi, [rsi + 8 * 8]
	mov rsi, [rsi + 9 * 8]
	mov rbp, [rsi + 10 * 8]
	;mov rsp, [rsi + 11 * 8]
	mov rdx, [rsi + 12 * 8]
	mov rcx, [rsi + 13 * 8]
	mov rbx, [rsi + 14 * 8]
	mov rax, [rsi + 15 * 8]

	mov rdi, HOST_TR_SELECTOR
	str si
	call VmWrite
	
	mov rdi, HOST_RSP
	mov rsi, rsp
	call VmWrite

	vmresume

; @ProtoType:	VOID VmPtrLoad(UINT64 VmcsPhysAddr);
; @Function:	Loads the current VMCS pointer from memory
; @Param:		IN	64-bit physical address of VMCS pointer
VmPtrLoad:
	push rdi
	vmptrld [rsp]
	pop rdi
	ret

; @ProtoType:	VOID VmPtrStore(UINT64 * VmcsPhysAddr);
; @Function:	Stores the current VMCS pointer into memory
; @Param:		OUT	64-bit physical address of VMCS pointer
VmPtrStore:
	vmptrst [rdi]
	ret

; @ProtoType:	VOID VmClear(UINT64 VmcsPhysAddr);
; @Function:	Copy VMCS data to VMCS region in memroy
; @Param:		IN	64-bit physical address of VMCS pointer
VmClear:
	push rdi
	vmclear [rsp]
	pop rdi
	ret

; @ProtoType:	UINT64 VmRead(UINT64 VmcsID);
; @Function:	Reads a specified VMCS field.
; @Param:		IN	VMCS ID
; @Ret:			Specific 64-bit VMCS field
VmRead:
	vmread rax, rdi
	ret

; @ProtoType:	VOID VmWrite(UINT64 VmcsID, UINT64 Value);
; @Function:	Writes a specified VMCS field.
; @Param:		IN	VMCS ID
; @Param:		IN	64-bit value write to VMCS
VmWrite:
	vmwrite rdi, rsi
	ret

; @ProtoType:	VOID VmCall(VOID);
; @Function:	Call to VM monitor by causing VM exit.
VmCall:
	vmcall
	ret

; @ProtoType:	VOID VmFunc(UINT32 VmFun);
; @Function:	Invoke VM function specified by Param1.
; @Param:		IN	VM function.
VmFunc:
	mov eax, edi
	vmfunc
	ret

; @ProtoType:	VOID InvalidateEpt(UINT64 Type, INVVEPT_DESCRIPTOR * Ptr);
; @Function:	Invalidates EPT-derived entries in the TLBs and paging-structure caches.
; @Param:		IN	INVEPT type
; @Param:		IN	Pointer to the INVVEPT Descriptor
InvalidateEpt:
	invept rdi, [rsi]
	ret

; @ProtoType:	VOID InvalidateVpid(UINT64 Type, INVVPID_DESCRIPTOR * Ptr);
; @Function:	Invalidates entries in the TLBs and paging-structure caches based on VPID.
; @Param:		IN	INVVPID type
; @Param:		IN	Pointer to the INVVPID Descriptor
InvalidateVpid:
	invvpid rdi, [rsi]
	ret

