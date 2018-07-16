/* Interrupt Service Routine.
 * Oscar
 * Jul, 2018
 */
.global exception_table
.global irq_table

.data
exception_table:
.quad divide_error_exception
.quad debug_exception
.quad nmi_interrupt
.quad breakpoint_exception
.quad overflow_exception
.quad bound_range_exceeded_exception
.quad invalid_opcode_exception
.quad device_not_available_exception
.quad double_fault_exception
.quad coprocessor_segment_overrun
.quad invalid_tss_exception
.quad segment_not_present
.quad stack_fault_exception
.quad general_protection_exception
.quad page_fault_exception
.quad exception_handler_15
.quad x87_fpu_floating_point_error
.quad alignment_check_exception
.quad machine_check_exception
.quad simd_floatingn_point_exception
.quad virtualization_exception
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0

.code64
.section .text

.macro SAVE_CREGS
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rbp
	pushq %rsi
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
.endm

.macro RESTORE_CREGS
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdi
	popq %rsi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
.endm

.macro SAVE_FREGS
	subq $512, %rsp
	fxsave (%rsp)
	subq $512, %rsp
	xsave (%rsp)
.endm

.macro RESTORE_FREGS
	xrstors
	addq &512, %rsp
	fxrestor
	addq &512, %rsp
.endm

.macro LAZY_FPU_SET
	movq %cr0, %rax
	bts $1, %rax
	bts $3, %rax
	movq %rax, %cr0
.endm

.macro LAZY_FPU_CLR
	movq %cr0, %rax
	btr $1, %rax
	btr $3, %rax
	movq %rax, %cr0
.endm

.macro EXCEPTION_COMMON vector=0 has_error_code=0
.if \has_error_code == 0
	pushq $0xffffffffffffffff
.endif
	SAVE_CREGS
	movq $\vector, %rdi
	movq 90(%rsp), %rsi
	call irq_handler
	RESTORE_CREGS
	addq $8, %rsp
	iretq
.endm

divide_error_exception:
	EXCEPTION_COMMON vector=0, has_error_code=0

debug_exception:
	EXCEPTION_COMMON vector=1, has_error_code=0

nmi_interrupt:
	EXCEPTION_COMMON vector=2, has_error_code=0

breakpoint_exception:
	EXCEPTION_COMMON vector=3, has_error_code=0

overflow_exception:
	EXCEPTION_COMMON vector=4, has_error_code=0

bound_range_exceeded_exception:
	EXCEPTION_COMMON vector=5, has_error_code=0

invalid_opcode_exception:
	EXCEPTION_COMMON vector=6, has_error_code=0

device_not_available_exception:
	EXCEPTION_COMMON vector=7, has_error_code=0

double_fault_exception:
	EXCEPTION_COMMON vector=8, has_error_code=1

coprocessor_segment_overrun:
	EXCEPTION_COMMON vector=9, has_error_code=0

invalid_tss_exception:
	EXCEPTION_COMMON vector=10, has_error_code=1

segment_not_present:
	EXCEPTION_COMMON vector=11, has_error_code=1

stack_fault_exception:
	EXCEPTION_COMMON vector=12, has_error_code=1

general_protection_exception:
	EXCEPTION_COMMON vector=13, has_error_code=1

page_fault_exception:
	EXCEPTION_COMMON vector=14, has_error_code=1

exception_handler_15:
	EXCEPTION_COMMON vector=15, has_error_code=0

x87_fpu_floating_point_error:
	EXCEPTION_COMMON vector=16, has_error_code=0

alignment_check_exception:
	EXCEPTION_COMMON vector=17, has_error_code=1

machine_check_exception:
	EXCEPTION_COMMON vector=18, has_error_code=0

simd_floatingn_point_exception:
	EXCEPTION_COMMON vector=19, has_error_code=0

virtualization_exception:
	EXCEPTION_COMMON vector=20, has_error_code=0

reserved_exception:
	EXCEPTION_COMMON vector=21, has_error_code=0


.macro IRQ_COMMON vector=0
.align 8
	SAVE_CREGS
	movq $\vector, %rdi
	call irq_handler
	RESTORE_CREGS
	iretq
.endm


irq_vector:
index=0
.rept 0x100 - 0x20
	IRQ_COMMON vector = index + 0x20
	index=index+1
.endr
irq_vector_end:
	nop


.data
PER_IRQ_SIZE = ((irq_vector_end - irq_vector) / 224)
irq_table:
index=0
.rept 0x100 - 0x20
	.quad irq_vector + index * PER_IRQ_SIZE
	index=index+1
.endr

