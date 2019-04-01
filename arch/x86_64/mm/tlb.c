#include <mm.h>
#include <task.h>
#include <cr.h>

void switch_mm(struct mm_struct *prev, struct mm_struct *next,
			struct task_struct *tsk)
{
	write_cr3(VIRT2PHYS(next->pgd) | (tsk->id & CR3_PCID_MASK));
}

