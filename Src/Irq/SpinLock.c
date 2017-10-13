#include "SpinLock.h"
#include "Thread.h"

VOID SpinLock(SPIN_LOCK * SpinLock)
{
	ArchGetSpinLock(&SpinLock->SpinLock);
}

VOID SpinUnlock(SPIN_LOCK * SpinLock)
{
	ArchReleaseSpinLock(&SpinLock->SpinLock);
}

VOID SpinLockIrqSave(SPIN_LOCK * SpinLock)
{
	UINT64 IrqStatus = ArchGetIrqStatus();
	ArchDisableCpuIrq();

	//此处必须先得到锁以后再保存中断的状态，否则保存的可能不是得到锁CPU的中断状态
	ArchGetSpinLock(&SpinLock->SpinLock);	
	SpinLock->LocalIrqStatus = IrqStatus;

	THREAD_CONTROL_BLOCK * Current = GetCurrentThread();
	if (Current)
	{
		Current->PreemptEnable++;
	}
}

VOID SpinUnlockIrqRestore(SPIN_LOCK * SpinLock)
{		
	UINT64 IrqStatus = SpinLock->LocalIrqStatus;
	//要在释放前读取中断的状态，否则可能被其他CPU抢占导致读到错误的状态

	THREAD_CONTROL_BLOCK * Current = GetCurrentThread();
	if (Current)
	{
		Current->PreemptEnable--;
	}

	ArchReleaseSpinLock(&SpinLock->SpinLock);

	if (IrqStatus)
		ArchEnableCpuIrq();
}

VOID SpinUnlockIrqEnable(SPIN_LOCK * SpinLock)
{
	ArchReleaseSpinLock(&SpinLock->SpinLock);
	ArchEnableCpuIrq();
}