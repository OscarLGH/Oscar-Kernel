#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "Type.h"
#include "../Arch/X86_64/Cpu/Cpu.h"

typedef struct _SPIN_LOCK
{
	UINT64 SpinLock;
	UINT64 LocalIrqStatus;
}SPIN_LOCK;

VOID SpinLock(SPIN_LOCK * SpinLock);
VOID SpinUnlock(SPIN_LOCK * SpinLock);

VOID SpinLockIrqSave(SPIN_LOCK * SpinLock);
VOID SpinUnlockIrqRestore(SPIN_LOCK * SpinLock);
VOID SpinUnlockIrqEnable(SPIN_LOCK * SpinLock); 

#endif