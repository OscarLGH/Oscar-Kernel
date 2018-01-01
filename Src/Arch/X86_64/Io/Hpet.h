#ifndef _HPET_H
#define _HPET_H

#include "Timer.h"
#include "Irq.h"

#include "../Acpi.h"
#include "BaseLib.h"

#define DEBUG
#include "Debug.h"

#define HPET_DEFAULT_BASE0 0xFED00000
#define HPET_DEFAULT_BASE1 0xFED01000
#define HPET_DEFAULT_BASE2 0xFED02000
#define HPET_DEFAULT_BASE3 0xFED03000

#define HPET_ID						0x0
#define HPET_CONFIGURE 				0x10
	#define HPET_OVERALL_ENABLE 		BIT0
	#define HPET_LEGACY_REP_ROUT		BIT1
#define HPET_INT_STATUS				0x20
#define HPET_COUNTER				0xF0

#define HPET_TIMERX_LEVEL_TRIGGER	BIT1
#define HPET_TIMERX_INTERRUPT		BIT2
	
#define HPET_TIMER0_CONFIGURE		0x100
	#define HPET_TIMER0_32BIT		BIT8
	#define HPET_TIMER0_AUTO_INC	BIT6
	#define HPET_TIMER0_PERIODIC_ON	BIT3
#define HPET_TIMER0_COMPARATOR		0x108
	
#define HPET_TIMER1_CONFIGURE		0x120
#define HPET_TIMER1_COMPARATOR		0x128

#define HPET_TIMER2_CONFIGURE		0x140
#define HPET_TIMER2_COMPARATOR		0x148

#define HPET_TIMER3_CONFIGURE		0x160
#define HPET_TIMER3_COMPARATOR		0x168

#define HPET_TIMER4_CONFIGURE		0x180
#define HPET_TIMER4_COMPARATOR		0x188

#define HPET_TIMER5_CONFIGURE		0x1A0
#define HPET_TIMER5_COMPARATOR		0x1A8

#define HPET_TIMER6_CONFIGURE		0x1C0
#define HPET_TIMER6_COMPARATOR		0x1C8

#define HPET_TIMER7_CONFIGURE		0x1E0
#define HPET_TIMER7_COMPARATOR		0x1E8

#define COUNTER_PER_SECOND 			14318179

#define APIC_HPET_0_IRQ 2
#define APIC_8254_0_IRQ 2


UINT64 HpetRegRead(UINT32 Index);
void HpetRegWrite(UINT32 Index, UINT64 Value);
void HpetDelay(UINT64 microsecond);

void HpetInit();
UINT64 HpetGetCounter(TIMER_CHIP * TimerChip, UINT64 Channel);
UINT64 TscGetCounter(TIMER_CHIP * TimerChip, UINT64 Channel);
RETURN_STATUS HpetSetFrequency(TIMER_CHIP * TimerChip, UINT64 Channel, UINT64 Frequency);
UINT64 HpetGetFrequency(TIMER_CHIP * TimerChip, UINT64 Channel);



#endif
