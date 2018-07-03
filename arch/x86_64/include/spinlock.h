#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <types.h>

static inline void spin_lock(unsigned long long *lock)
{
	__asm__ __volatile__("1:lock bts $0, (%0)\n\t"
				"pause\n\t"
				"jc 1b\n\t"
				: : "rdi"(lock)
		);
}

static inline void spin_unlock(unsigned long long  *lock)
{
	__asm__ __volatile__("lock btr $0, (%0)\n\t"
				: : "rdi"(lock)
		);
}

#endif