#include <kernel.h>
#include <task.h>
#include <cpu.h>

struct rq **rq_list = NULL;
struct sq sq;
u64 pid = 0;

void idle_task()
{
	struct cpu *cpu;
	while (1) {
		cpu = get_cpu();
		cpu->status = CPU_STATUS_IDLE;
		printk("idle on cpu %d\n", get_cpu()->index);
		arch_cpu_halt();
	}
}

void rq_init(struct rq * rq)
{
	rq->length = 0;
	INIT_LIST_HEAD(&rq->task_list);
	rq->idle = __create_task(idle_task, -1, 0x1000, 1, 0);
	rq->current = NULL;
	rq->spin_lock = 0;
}

void task_init()
{
	int i;
	rq_list = kmalloc(nr_cpus * sizeof(*rq_list), GFP_KERNEL);
	for (i = 0; i < nr_cpus; i++) {
		rq_list[i] = kmalloc(sizeof(struct rq), GFP_KERNEL);
		rq_init(rq_list[i]);
	}
	sq.length = 0;
	INIT_LIST_HEAD(&sq.task_list);
}

struct task_struct *
__create_task(void (*fun)(void), int prio, int kstack_size, int kernel, int cpu)
{
	struct task_struct *task = kmalloc(sizeof(*task), GFP_KERNEL);
	task->stack = kmalloc(kstack_size, GFP_KERNEL);
	task->sp = (u64)task->stack + kstack_size - 0x200;
	task->prio = prio;
	task->id = pid++;
	task->counter = prio;
	task->cpu = cpu;
	arch_init_kstack(task, fun, (u64)task->stack, kernel);
	arch_init_mm(task);
	return task;
}

static int cpu_min_load()
{
	int i;
	int min_cpu_idx = 0;
	int min_len = 0xffffffff;
	for (i = 0; i < nr_cpus; i++) {
		if (rq_list[i]->length < min_len) {
			min_len = rq_list[i]->length;
			min_cpu_idx = i;
		}
	}

	return min_cpu_idx;
}

struct task_struct *
create_task(void (*fun)(void), int prio, int kstack_size, int kernel, int cpu)
{
	int min_load_cpu;
	int flag;
	struct task_struct *task = __create_task(fun, prio, kstack_size, kernel, cpu);
	if (cpu == -1) {
		min_load_cpu = cpu_min_load();
		task->cpu = min_load_cpu;
		spin_lock_irqsave(&rq_list[min_load_cpu]->spin_lock, flag);
		list_add_tail(&task->list, &rq_list[min_load_cpu]->task_list);
		rq_list[min_load_cpu]->length++;
		spin_unlock_irqrestore(&rq_list[min_load_cpu]->spin_lock, flag);
	} else {
		spin_lock_irqsave(&rq_list[cpu]->spin_lock, flag);
		list_add_tail(&task->list, &rq_list[cpu]->task_list);
		rq_list[cpu]->length++;
		spin_unlock_irqrestore(&rq_list[cpu]->spin_lock, flag);
	}
}


int migrate_task(struct task_struct *task, int cpu)
{
	long irq_flag1 = 0, irq_flag2 = 0, irq_flag3 = 0;
	int ori_cpu = task->cpu;
	if (task->cpu == cpu)
		return 0;

	spin_lock_irqsave(&rq_list[ori_cpu]->spin_lock, irq_flag1);
	spin_lock_irqsave(&task->spin_lock, irq_flag2);
	spin_lock_irqsave(&rq_list[cpu]->spin_lock, irq_flag3);
	list_del(&task->list);
	list_add_tail(&task->list, &rq_list[cpu]->task_list);
	task->cpu = cpu;
	spin_unlock_irqrestore(&rq_list[cpu]->spin_lock, irq_flag3);
	spin_unlock_irqrestore(&task->spin_lock, irq_flag2);
	spin_unlock_irqrestore(&rq_list[ori_cpu]->spin_lock, irq_flag1);

	return 0;
}

struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct task_struct *task_ptr = NULL;
	struct task_struct *cand_task = NULL;
	u64 max_counter;

repick:
	max_counter = 0;
	list_for_each_entry(task_ptr, &rq->task_list, list) {
		if (task_ptr->counter >= max_counter) {
			max_counter = task_ptr->counter;
			cand_task = task_ptr;
		}
	}

	if (!list_empty(&rq->task_list) && cand_task && cand_task->counter == 0) {
		list_for_each_entry(task_ptr, &rq->task_list, list) {
			task_ptr->counter = task_ptr->prio;
		}
		goto repick;
	}

	return cand_task;
}

struct rq *
context_switch(struct rq *rq, struct task_struct *prev,
	       struct task_struct *next, struct rq_flags *rf)
{
	//mm switch
	//switch_mm(&prev->mm, &next->mm, next);

	rq->current = next;
	switch_to(prev, next);
}

void schedule()
{
	struct task_struct *prev, *next;
	struct cpu *cpu = get_cpu();
	struct rq * rq = rq_list[cpu->index];
	prev = rq->current;

	if (cpu->status == CPU_STATUS_PROCESS_CONTEXT) {
	/*schedule() is called directly by process */
		printk("TODO:Process context schedule...\n");
	} else {
	/*schedule() is called in irq handler */

		
		next = pick_next_task(rq, prev, NULL);

		if (next == NULL) {
			printk("idle...\n");
			next = rq->idle;
		}

		//printk("next->id = %d\n", next->id);

		context_switch(rq, prev, next, NULL);
	}
}

struct task_struct *get_current_task()
{
	struct cpu *cpu = get_cpu();
	if (rq_list == NULL)
		return NULL;
	if (rq_list[cpu->index] == NULL)
		return NULL;
	return rq_list[cpu->index]->current;
}

u64 get_current_task_stack()
{
	struct task_struct *task = get_current_task();
	if (task == NULL)
		return 0;
	
	return (u64)task->sp;
}

void set_current_task_stack(u64 sp)
{
	struct task_struct *task = get_current_task();
	if (task == NULL)
		return;
	task->sp = sp;
}

int task_timer_tick(int irq, void *data)
{
	//printk("%s on cpu %d\n", __FUNCTION__, get_cpu()->index);
	struct cpu *cpu = get_cpu();
	struct task_struct *current = rq_list[cpu->index]->current;

	if (current != NULL && current != rq_list[cpu->index]->idle && current->counter != 0) {
		current->counter--;
	} else {
		schedule();
	}
}
