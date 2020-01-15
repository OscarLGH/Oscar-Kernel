#include <types.h>
#include <x86_cpu.h>
#include <cr.h>
#include <msr.h>
#include <stack.h>
#include <kernel.h>
#include <cpu.h>

void dump_regs(struct exception_stack_frame *stack)
{
	printk("RAX:%016x RBX:%016x RCX:%016x RDX:%016x RSI:%016x RDI:%016x RSP:%016x RBP:%016x\n",
		stack->g_regs.rax,
		stack->g_regs.rbx,
		stack->g_regs.rcx,
		stack->g_regs.rdx,
		stack->g_regs.rsi,
		stack->g_regs.rdi,
		stack->excep_stack.rsp,
		stack->g_regs.rbp
	);
	printk("R08:%016x R09:%016x R10:%016x R11:%016x R12:%016x R13:%016x R14:%016x R15:%016x\n",
		stack->g_regs.r8,
		stack->g_regs.r9,
		stack->g_regs.r10,
		stack->g_regs.r11,
		stack->g_regs.r12,
		stack->g_regs.r13,
		stack->g_regs.r14,
		stack->g_regs.r15
	);
	printk("RIP:%016x RFLAGS:%016x error code:%x\n",
		stack->excep_stack.rip,
		stack->excep_stack.rflags,
		stack->excep_stack.error_code
	);
	printk("CS:%04x DS:%04x ES:%04x FS:%04x GS:%04x SS:%04x\n",
		stack->excep_stack.cs,
		save_ds(),
		save_es(),
		save_fs(),
		save_gs(),
		stack->excep_stack.ss);
	printk("CR0:%016x CR2:%016x CR3:%016x CR4:%016x CR8:%016x\n", 
		read_cr0(),
		read_cr2(),
		read_cr3(),
		read_cr4(),
		read_cr8());
}

void back_trace(struct exception_stack_frame *stack)
{
	u64 rbp = 0;
	u64 ret_addr = 0;
	int i = 0;

	printk("back trace:\n");
	rbp = stack->g_regs.rbp;
	while (rbp != -1) {
		ret_addr = *((u64 *)(rbp + 8));
		rbp = *((u64 *)rbp);
		printk("(%d)[0x%016x]\n", i++, ret_addr);
	}
}

void do_divide_error_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#DE on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_debug_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#DB on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_nmi_interrupt(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:NMI on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_breakpoint_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#BP on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_overflow_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#OF on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_bound_range_exceeded_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#BR on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_invalid_opcode_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#UD on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_device_not_available_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#NM on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_double_fault_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#DF on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_coprocessor_segment_overrun(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:Coprocessor Segment Overrun on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_invalid_tss_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#TS on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_segment_not_present(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#NP on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_stack_fault_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#SS on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_general_protection_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#GP on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_page_fault_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;
	int ret;

	ret = page_fault_mm(read_cr2(), stack->excep_stack.error_code, stack->excep_stack.cs & 0x3);
	if ((stack->excep_stack.cs & 0x3) == 0) {
		if (ret != 0)
			goto err_exit;
		else
			goto exit;
	} else {
		/* TODO: send signal to process. */
	}

err_exit:
	printk("Exception:#PF on cpu %d.\n", cpu->id);
	dump_regs(stack);
	back_trace(stack);
	asm("hlt");
exit:
	write_cr2(0);
	cpu->status = cpu_status;
}

void do_x87_fpu_floating_point_error(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#MF on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}
void do_alignment_check_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#AC on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}
void do_machine_check_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#MC on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}
void do_simd_floatingn_point_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#XM on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}
void do_virtualization_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#VE on cpu %d.\n", cpu->id);
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

void do_reserved_exception(struct exception_stack_frame *stack)
{
	struct cpu *cpu = get_cpu();
	u64 cpu_status = cpu->status;
	cpu->status = CPU_STATUS_EXCEPTION_CONTEXT;

	if ((stack->excep_stack.cs & 0x3) == 0) {
		printk("Exception:#reserved.\n");
		dump_regs(stack);
		back_trace(stack);
		asm("hlt");
	} else {
		/* TODO: send signal to process. */
	}

	cpu->status = cpu_status;
}

