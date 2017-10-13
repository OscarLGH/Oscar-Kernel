#include "Type.h"
#include "Acpi.h"
#include "Cpu/Cpu.h"
#include "Cpu/Debug.h"

#define APIC

void IoInit();

void ArchInit()
{
	X64SetupBranchRecord();

	extern INIT_CALL_FUNC  \
		__initcall1_start, \
		__initcall1_end, \
		__initcall2_start, \
		__initcall2_end, \
		__initcall3_start, \
		__initcall3_end, \
		__initcall4_start, \
		__initcall4_end, \
		__initcall5_start, \
		__initcall5_end, \
		__initcall6_start, \
		__initcall6_end, \
		__initcall7_start, \
		__initcall7_end, \
		__initcall8_start, \
		__initcall8_end;

	struct INIT_CALL_ENRTY
	{
		INIT_CALL_FUNC * Start;
		INIT_CALL_FUNC * End;
	}InitArray[] = {
		{ &__initcall1_start, &__initcall1_end },
		{ &__initcall2_start, &__initcall2_end },
		{ &__initcall3_start, &__initcall3_end },
		{ &__initcall4_start, &__initcall4_end },
		{ &__initcall5_start, &__initcall5_end },
		{ &__initcall6_start, &__initcall6_end },
		{ &__initcall7_start, &__initcall7_end },
		{ &__initcall8_start, &__initcall8_end },
	};
	
	INIT_CALL_FUNC InitFunc;
	INIT_CALL_FUNC * InitArrayPtr;
	
	printk("Initializing X86_64 platform ...\n");
	
	UINT64 Index0;

	for (Index0 = 0; Index0 < 4; Index0++)
	{	
		for (InitArrayPtr = InitArray[Index0].Start; InitArrayPtr < InitArray[Index0].End; InitArrayPtr++)
		{
			InitFunc = *InitArrayPtr;
			if (InitFunc)
			{
				InitFunc();
			}
		}
	}
}
