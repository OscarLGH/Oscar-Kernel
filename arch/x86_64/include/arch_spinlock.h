#ifndef _SPINLOCK_X86_H_
#define _SPINLOCK_X86_H_

#include <types.h>

typedef unsigned long long spin_lock_t;
static inline void spin_lock(spin_lock_t *lock)
{
	__asm__ __volatile__("1:lock bts $0, (%0)\n\t"
				"pause\n\t"
				"jc 1b\n\t"
				: : "rdi"(lock)
		);
}

static inline void spin_unlock(spin_lock_t *lock)
{
	__asm__ __volatile__("lock btr $0, (%0)\n\t"
				: : "rdi"(lock)
		);
}

static inline long arch_local_irq_save()
{
	u64 irq_state;
	__asm__ __volatile__("pushf \n\t"
				"movq (%%rsp), %0 \n\t"
				"andq $0x200, %0 \n\t"
				"andq $~0x200, (%%rsp) \n\t"
				"popf \n\t"
				: "=r"(irq_state): 
		);
	return irq_state;
}

static inline void arch_local_irq_restore(long irq_state)
{
	__asm__ __volatile__("pushf \n\t"
				"orq %0, (%%rsp) \n\t"
				"popf \n\t"
				: : "r"(irq_state)
		);
}

#endif