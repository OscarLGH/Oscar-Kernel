#include "Hpet.h"

extern TIMER_CHIP TimerChip;

UINT64 HpetRegRead(UINT32 Index)
{
	volatile UINT64 HPETBase = PHYS2VIRT(HPET_DEFAULT_BASE0);
	return *(UINT64 *)(HPETBase + Index);
}

void HpetRegWrite(UINT32 Index, UINT64 Value)
{
	volatile UINT64 HPETBase = PHYS2VIRT(HPET_DEFAULT_BASE0);
	*(UINT64 *)(HPETBase + Index) = Value;
}

UINT64 HpetGetCounter(TIMER_CHIP * TimerChip, UINT64 Channel)
{
	return HpetRegRead(HPET_COUNTER);
}

RETURN_STATUS HpetSetFrequency(TIMER_CHIP * TimerChip, UINT64 Channel, UINT64 Frequency)
{
	//Disable HPET
	HpetRegWrite(HPET_CONFIGURE, ~HPET_OVERALL_ENABLE);

	//Clear Main Counter
	HpetRegWrite(HPET_COUNTER, 0);
	KDEBUG("HPET Main Counter:%x\n", HpetRegRead(HPET_COUNTER));

	HpetRegWrite(
				HPET_TIMER0_CONFIGURE, 
				HPET_TIMER0_AUTO_INC | HPET_TIMERX_INTERRUPT | HPET_TIMER0_PERIODIC_ON
				);
	
	//Set Timer0 frequency
	HpetRegWrite(HPET_TIMER0_COMPARATOR, COUNTER_PER_SECOND / Frequency);
	
	//Enable HPET
	HpetRegWrite(HPET_CONFIGURE, HPET_OVERALL_ENABLE | HPET_LEGACY_REP_ROUT);
}

UINT64 HpetGetFrequency(TIMER_CHIP * TimerChip, UINT64 Channel)
{
	return COUNTER_PER_SECOND / HpetRegRead(HPET_TIMER0_COMPARATOR);
}

UINT64 TscGetCounter(TIMER_CHIP * TimerChip, UINT64 Channel)
{
	return X64ReadTsc();
}

void HpetInit()
{
	//See Wellsburg Spec P159
	//volatile UINT32 * HPETConfigBase = (UINT32 *)PHYS2VIRT(0xFEC00000 + 0x3404);
	//*HPETConfigBase = 0xFFFFFF7C;
	
	//Initialize Timer
	ACPI_HPET * HpetPtr;
	
	HpetPtr = (ACPI_HPET *)AcpiGetDescriptor("HPET");
	if (HpetPtr == NULL)
	{
		KDEBUG("No 'HPET' found in ACPI.\n");
		KDEBUG("Using 8254.\n");
		//I8254Init();
		TimerChip.Attribute = TIMER_ATTR_INCREASING;
		TimerChip.ClocksPerMicroSeconds = 100;
		TimerChip.GetCounter = TscGetCounter;
	}
	else
	{
		KDEBUG("HPET ID:%x\n", HpetRegRead(HPET_ID)>>32);
		KDEBUG("HPET:Block ID:%08x\n", HpetPtr->EventTimerBlockId);
		KDEBUG("HPET:Base Address:%x\n", HpetPtr->BaseAddress);
		KDEBUG("HPET:Number:%02x\n", HpetPtr->HpetNumber);

		TimerChip.ClocksPerMicroSeconds = 14;
		TimerChip.Attribute = TIMER_ATTR_INCREASING;
		TimerChip.GetCounter = HpetGetCounter;
		TimerChip.SetFrequency = HpetSetFrequency;
		TimerChip.GetFrequency = HpetGetFrequency;

		TimerChip.SetFrequency(&TimerChip, 0, 1);
		KDEBUG("HPET Timer0 Frequency:%dHz.\n", TimerChip.GetFrequency(&TimerChip, 0));
	}

	/* BSP handle timer interrupt. */
	PicChip.IrqRemapping(APIC_HPET_0_IRQ, 0x20, 0xff);
}

ARCH_INIT(HpetInit);




