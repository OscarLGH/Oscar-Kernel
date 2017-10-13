#include "Timer.h"
#include "BaseLib.h"

void MicroSecondDelay(UINT64 Microsecond)
{
	return;
	UINT64 StartCounter, EndCounter;
	
	SpinLockIrqSave(&TimerChip.SpinLock);
	UINT64 MicrosecondPerTicks = 1000000 / TimerChip.GetFrequency(&TimerChip, 0);
	SpinUnlockIrqRestore(&TimerChip.SpinLock);
	//UINT64 MicrosecondPerTicks = 1000000 / 1000;
	StartCounter = TimerChip.Jiffies;
	//printk("TimerChip.GetFrequency:%d\n", TimerChip.GetFrequency(&TimerChip, 0));
	//printk("StartCountere = %016x\n", StartCounter);
	while(1)
	{
		EndCounter = TimerChip.Jiffies;
		//printk("EndCounter = %016x\n", EndCounter);
		if ((EndCounter - StartCounter) * MicrosecondPerTicks >= Microsecond)
			break;
	}
}

void MilliSecondDelay(UINT64 counter)
{
	MicroSecondDelay(counter * 1000);
}

