#include "8254.h"

UINT64 I8254GetCounter(TIMER_CHIP * TimerChip, UINT64 Channel)
{
	UINT8 CounterL = 0;
	UINT8 CounterH = 0;
	
	if (Channel > 2)
		return 0;
	
	Out8(I8254_MODE_CONTROL_PORT, 
		I8254_CW_SEL_COUNTER(0x3) | I8254_CW_RLM | I8254_CW_MODE2 | I8254_CW_BIN);
	CounterL = In8(Channel + I8254_COUNTER0_PORT);
	CounterH = In8(Channel + I8254_COUNTER0_PORT);

	return (CounterL | (CounterH << 8));
}

RETURN_STATUS I8254SetFrequency(TIMER_CHIP * TimerChip, UINT64 Channel, UINT64 Frequency)
{
	if(Frequency < 18 || Frequency > 1193181)
		return RETURN_UNSUPPORTED;

	UINT16 Counter = 1193181 / Frequency;
	Out8(I8254_MODE_CONTROL_PORT, 
		I8254_CW_SEL_COUNTER(Channel) | I8254_CW_RLM | I8254_CW_MODE2 | I8254_CW_BIN);
	Out8(Channel + I8254_COUNTER0_PORT, Counter & 0xff);
	Out8(Channel + I8254_COUNTER0_PORT, Counter >> 8);

	if (Channel == 0)
		TimerChip->UserSetFrequency = Frequency;

	return RETURN_SUCCESS;
}

UINT64 I8254GetFrequency(TIMER_CHIP * TimerChip, UINT64 Channel)
{
	if (Channel == 0)
		return TimerChip->UserSetFrequency;

	return 0;
}


VOID I8254Init()
{
	/* Do not overlap HPET/TSC/Local APIC. */
	if (TimerChip.SetFrequency != NULL)
		return;

	/* Following setting can be overwriting by HPET/TSC/Local APIC. */
	TimerChip.Attribute = TIMER_ATTR_DECREASING;
	TimerChip.CrystalFrequency = 1193181;
	TimerChip.ClocksPerMicroSeconds = 1;
	TimerChip.GetCounter = I8254GetCounter;
	TimerChip.GetFrequency = I8254GetFrequency;
	TimerChip.SetFrequency = I8254SetFrequency;
	
	TimerChip.SetFrequency(&TimerChip, 0, 18);
	//printk("8254 Frequency:%dHz.\n", I8254GetFrequency(&TimerChip, 0));
	/* BSP handle timer interrupt. */
	PicChip.IrqRemapping(0x2, 0x20, 0xff);
}

ARCH_INIT(I8254Init);

