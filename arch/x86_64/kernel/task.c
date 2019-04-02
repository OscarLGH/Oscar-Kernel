#include <task.h>
#include <stack.h>
#include <cpu.h>
#include <cr.h>
#include <x86_cpu.h>
#include <segment.h>
#include <string.h>


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

void arch_start_task(struct task_struct *task)
{
	struct irq_stack_frame *kstack = (void *)task->sp;
	asm("mov %0, %%rax" ::"r"(kstack->irq_stack.ss):);
	asm("push %rax");
	asm("mov %0, %%rax" ::"r"(kstack->irq_stack.rsp):);
	asm("push %rax");
	asm("mov %0, %%rax" ::"r"(kstack->irq_stack.rflags):);
	asm("push %rax");
	asm("mov %0, %%rax" ::"r"(kstack->irq_stack.cs):);
	asm("push %rax");
	asm("mov %0, %%rax" ::"r"(kstack->irq_stack.rip):);
	asm("push %rax");
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
		arch_start_task(next);
	}
}

