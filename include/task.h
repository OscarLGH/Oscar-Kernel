#ifndef TASK_H
#define TASK_H
#include <types.h>
#include <mm.h>
#include <spinlock.h>

struct mm_struct;
struct task_struct {
	void *stack;
	u64 sp;
	int state;
	int id;
	int cpu;
	int prio;
	int counter;
	int reschedule;
	int signal;

	spin_lock_t spin_lock;
	struct mm_struct mm;

	struct task_struct *parent;
	struct task_struct *sibling;
	struct list_head list;
};

#define TASK_STATE_RUNNING 0
#define TASK_STATE_SLEEPING 1
#define TASK_STATE_KILLED 2
#define TASK_STATE_ZOMBIE 3

struct rq {
	spin_lock_t spin_lock;
	u64 length;
	struct task_struct *current;
	struct task_struct *idle;
	struct list_head task_list;
};

struct sq {
	spin_lock_t spin_lock;
	u64 length;
	struct list_head task_list;
};

struct rq_flags {
};

struct task_struct *
switch_to(struct task_struct *prev, struct task_struct *next);

void task_init();
struct task_struct *get_current_task();
u64 get_current_task_stack();
void set_current_task_stack(u64 sp);
void arch_init_kstack(struct task_struct *task, void (*fptr)(void), u64 stack, bool kernel);
void arch_init_mm(struct task_struct *task);

struct task_struct *
__create_task(void (*fun)(void), int prio, int kstack_size, int kernel, int cpu);

struct task_struct *
create_task(void (*fun)(void), int prio, int kstack_size, int kernel, int cpu);
int task_timer_tick(int irq, void *data);





#endif

