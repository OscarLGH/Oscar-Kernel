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
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rbp
	pushq %rsp
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax

.endm

.macro RESTORE_CREGS
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %rbp
	popq %rbp
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15
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

.macro EXCEPTION_COMMON handler = 0 has_error_code = 0
.if \has_error_code == 0
	pushq $0xffffffffffffffff
.endif
	SAVE_CREGS
	movabsq $\handler, %rax
	mov %rsp, %rdi
	call *%rax
	RESTORE_CREGS
	addq $8, %rsp
	iretq
.endm

divide_error_exception:
	EXCEPTION_COMMON handler=do_divide_error_exception, has_error_code = 0

debug_exception:
	EXCEPTION_COMMON handler=do_debug_exception, has_error_code = 0

nmi_interrupt:
	EXCEPTION_COMMON handler=do_nmi_interrupt, has_error_code = 0

breakpoint_exception:
	EXCEPTION_COMMON handler=do_breakpoint_exception, has_error_code = 0

overflow_exception:
	EXCEPTION_COMMON handler=do_overflow_exception, has_error_code = 0

bound_range_exceeded_exception:
	EXCEPTION_COMMON handler=do_bound_range_exceeded_exception, has_error_code = 0

invalid_opcode_exception:
	EXCEPTION_COMMON handler=do_invalid_opcode_exception, has_error_code = 0

device_not_available_exception:
	EXCEPTION_COMMON handler=do_device_not_available_exception, has_error_code = 0

double_fault_exception:
	EXCEPTION_COMMON handler=do_double_fault_exception, has_error_code = 1

coprocessor_segment_overrun:
	EXCEPTION_COMMON handler=do_coprocessor_segment_overrun, has_error_code = 0

invalid_tss_exception:
	EXCEPTION_COMMON handler=do_invalid_tss_exception, has_error_code = 1

segment_not_present:
	EXCEPTION_COMMON handler=do_segment_not_present, has_error_code = 1

stack_fault_exception:
	EXCEPTION_COMMON handler=do_stack_fault_exception, has_error_code = 1

general_protection_exception:
	EXCEPTION_COMMON handler=do_general_protection_exception, has_error_code = 1

page_fault_exception:
	EXCEPTION_COMMON handler=do_page_fault_exception, has_error_code = 1

exception_handler_15:
	EXCEPTION_COMMON handler=do_reserved_exception, has_error_code = 0

x87_fpu_floating_point_error:
	EXCEPTION_COMMON handler=do_x87_fpu_floating_point_error, has_error_code = 0

alignment_check_exception:
	EXCEPTION_COMMON handler=do_alignment_check_exception, has_error_code = 1

machine_check_exception:
	EXCEPTION_COMMON handler=do_machine_check_exception, has_error_code = 0

simd_floatingn_point_exception:
	EXCEPTION_COMMON handler=do_simd_floatingn_point_exception, has_error_code = 0

virtualization_exception:
	EXCEPTION_COMMON handler=do_virtualization_exception, has_error_code = 0

reserved_exception:
	EXCEPTION_COMMON handler=do_reserved_exception, has_error_code = 0


.macro IRQ_COMMON vector = 0
	SAVE_CREGS

	movabsq $save_tmp_stack, %rax
	mov %rsp, %rdi
	call *%rax

	movabsq $set_current_task_stack, %rax
	call *%rax

	movabsq $get_irq_stack, %rax
	call *%rax
	mov %rax, %rsp
	movq $\vector, %rdi
	call intr_handler_common

	movabsq $get_current_task_stack, %rax
	call *%rax
	test %rax, %rax
	jz 1f
	mov %rax, %rsp
	jmp 2f
1:
	movabsq $get_tmp_stack, %rax
	call *%rax
	mov %rax, %rsp
2:
	RESTORE_CREGS
	iretq
.endm


irq_vector:
index = 0
.rept 0x100 - 0x20
	IRQ_COMMON vector = index + 0x20
	index = index + 1
.endr
irq_vector_end:
	nop


.data
PER_IRQ_SIZE = ((irq_vector_end - irq_vector) / 224)
irq_table:
index = 0
.rept 0x100 - 0x20
	.quad irq_vector + index * PER_IRQ_SIZE
	index = index + 1
.endr

