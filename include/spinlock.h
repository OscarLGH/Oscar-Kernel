#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <arch_spinlock.h>

extern int printk(const char *fmt, ...);

static inline int __raw_spin_lock_irqsave(spin_lock_t *lock)
{
	long irq_flags;
	irq_flags = arch_local_irq_save();
	spin_lock(lock);
	return irq_flags;
}

static inline void __raw_spin_unlock_irqrestore(spin_lock_t *lock, long irq_flags)
{
	spin_unlock(lock);
	arch_local_irq_restore(irq_flags);
}

#define spin_lock_irqsave(lock, flags)	\
	do {										\
		flags = __raw_spin_lock_irqsave(lock);	\
	} while (0);

static inline void spin_unlock_irqrestore(spin_lock_t *lock, long irq_flags)
{
	__raw_spin_unlock_irqrestore(lock, irq_flags);
}
#endif