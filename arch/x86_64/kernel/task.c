#include <task.h>
#include <stack.h>
#include <cpu.h>
#include <cr.h>
#include <x86_cpu.h>
#include <segment.h>
#include <string.h>

void udelay(u64 us)
{
	u64 time1 = rdtscp();
	u64 time2;
	do {
		time2 = rdtscp();
	} while (time2 - time1 < us * 3600);
}

void arch_cpu_halt()
{
	asm("hlt");
}

void arch_init_kstack(struct task_struct *task, void (*fptr)(void), u64 stack, bool kernel)
{
	struct irq_stack_frame *kstack = (void *)task->sp;
	memset(&kstack->g_regs, 0, sizeof(kstack->g_regs));
	kstack->irq_stack.rip = (u64)fptr;
	kstack->irq_stack.rsp = stack;
	kstack->irq_stack.rflags = 0x286;

	if (kernel == 1) {
		kstack->irq_stack.cs = SELECTOR_KERNEL_CODE;
		kstack->irq_stack.ss = SELECTOR_KERNEL_DATA;
	} else {
		kstack->irq_stack.cs = SELECTOR_USER_CODE;
		kstack->irq_stack.ss = SELECTOR_USER_DATA;
	}
}

void __switch_to(struct task_struct *prev, struct task_struct *next)
{
	struct irq_stack_frame *kstack_prev = (void *)prev->sp;
	void *prev_ret_addr = __builtin_return_address(0);
	struct irq_stack_frame *kstack_next = (void *)next->sp;
	
	/* simulate an interrupt */
	asm("mov %ss, %ax");
	asm("push %rax");
	asm("mov %rbp, %rax");
	asm("add $0x8, %rax");
	asm("push %rax");
	asm("pushf");
	asm("mov %cs, %ax");
	asm("push %rax");
	asm("mov %0, %%rax" ::"r"(prev_ret_addr):);
	asm("push %rax");

	asm("pushq %r15 \n\t"
		"pushq %r14 \n\t"
		"pushq %r13 \n\t"
		"pushq %r12 \n\t"
		"pushq %r11 \n\t"
		"pushq %r10 \n\t"
		"pushq %r9 \n\t"
		"pushq %r8 \n\t"
		"pushq %rbp \n\t"
		"pushq %rsp \n\t"
		"pushq %rdi \n\t"
		"pushq %rsi \n\t"
		"pushq %rdx \n\t"
		"pushq %rcx \n\t"
		"pushq %rbx \n\t"
		"pushq %rax \n\t");

	asm("mov %%rsp, %0"::"m"(prev->sp):);
	
	asm("mov %0, %%rsp" ::"m"(next->sp):);
	asm("popq %rax \n\t"
		"popq %rbx \n\t"
		"popq %rcx \n\t"
		"popq %rdx \n\t"
		"popq %rsi \n\t"
		"popq %rdi \n\t"
		"popq %rbp \n\t"
		"popq %rbp \n\t"
		"popq %r8 \n\t"
		"popq %r9 \n\t"
		"popq %r10 \n\t"
		"popq %r11 \n\t"
		"popq %r12 \n\t"
		"popq %r13 \n\t"
		"popq %r14 \n\t"
		"popq %r15 \n\t"
		);
	asm("iretq");
}

void arch_init_mm(struct task_struct *task)
{
	task->mm.pgd = kmalloc(0x1000, GFP_KERNEL);
	memcpy((u64 *)task->mm.pgd, (u64 *)PHYS2VIRT(read_cr3()), 0x1000);
}

/* switch tss->rsp0 so when irq happens next time, irq info can be saved onto the nexttask->stack */ 
void switch_irq_stack(u64 stack)
{
	struct cpu *cpu = get_cpu();
	struct x86_cpu *x86_cpu = cpu->arch_data;
	x86_cpu->tss->rsp0 = stack;
}

struct task_struct *
switch_to(struct task_struct *prev, struct task_struct *next)
{
	struct cpu *cpu = get_cpu();

	switch_irq_stack((u64)next->sp);
	/* new task starts on iret since we want irq handler to finish cleaning work.*/
	/* Otherwise, new task starts immediately */
	if (cpu->status != CPU_STATUS_IRQ_CONTEXT) {
		__switch_to(prev, next);
	}
}

