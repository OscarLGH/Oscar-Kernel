#ifndef _TIMER_H
#define _TIMER_H

#include "Type.h"
#include "SpinLock.h"

typedef struct _TIMER_CHIP
{
	VOID *Device;
	UINT64 CrystalFrequency;
	UINT64 Attribute;
	UINT64 ClocksPerMicroSeconds;
	UINT64 UserSetFrequency;
	UINT64 Jiffies;
	SPIN_LOCK SpinLock;
	UINT64 (*GetFrequency)(struct _TIMER_CHIP *TimerChip, UINT64 Channel);
	RETURN_STATUS (*SetFrequency)(struct _TIMER_CHIP *TimerChip, UINT64 Channel, UINT64 Frequency);
	UINT64 (*GetCounter)(struct _TIMER_CHIP *TimerChip, UINT64 Channel);
}TIMER_CHIP;

#define TIMER_ATTR_INCREASING 0
#define TIMER_ATTR_DECREASING 1

extern TIMER_CHIP TimerChip;

#endif