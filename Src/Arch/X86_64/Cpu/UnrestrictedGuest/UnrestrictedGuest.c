
/*
* UnrestrictedGuest.c
*
* Oscar 2017
*/
#include "../LongMode.h"

/*
Memory map for system descriptor.
|----------------------| 0x2000
|                      |
|                      |
|                      |
|       256 IDTs       |
|                      |
|                      |
|                      |
|----------------------| 0x1000
|       256 GDTs       |
|                      |
|     TSS SEG [127]    |
|          |           |
|          |           |
|          |           |
|     TSS SEG [64]     |
|                      |
|                      |
|   USER DATA SEG [4]  |
|   USER CODE SEG [3]  |
|  KERNEL DATA SEG [2] |
|  KERNEL CODE SEG [1] |
|  NULL DESCRIPTOR [0] |
|----------------------| 0x0000
*/

GDTR GDT_GUEST = {0,0};
IDTR IDT_GUEST = {0,0};
TSS64 TSS_GUEST[64] = {0};

extern int KERNEL_SPACE;

extern void(*IrqTableGuest[])();
extern void(*ExceptionTableGuest[])();

void DescriptorSetGuest();

void DescriptorSetGuest()
{
	UINT64 Index = 0;
	void (*ISPEntryPoint[256])()={0};

	for (Index = 0; Index < 0x20; Index++)
	{
		ISPEntryPoint[Index] = ExceptionTableGuest[Index];
	}
	
	#define HWINT0 0x20

	for (Index = HWINT0; Index < 256; Index++)
	{
		ISPEntryPoint[Index] = IrqTableGuest[Index - HWINT0];
	}
	
	GDT_GUEST.Base = (UINT64)(&KERNEL_SPACE);
	GDT_GUEST.Limit = 256 * sizeof(SEGMENT_DESCRIPTOR) - 1;
	SEGMENT_DESCRIPTOR * GDTPtr = (SEGMENT_DESCRIPTOR *)GDT_GUEST.Base;
	
	//空描述符
	SetSegmentDescriptor(
		GDTPtr,
		SELECTOR_NULL_INDEX,
		0,
		0,
		0,
		0
		);
	//内核代码段
	SetSegmentDescriptor(
		GDTPtr,
		SELECTOR_KERNEL_CODE_INDEX,
		0,
		0,
		CS_NC | DPL0,
		0x20
		);
	//内核数据段
	SetSegmentDescriptor(
		GDTPtr,
		SELECTOR_KERNEL_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL0,
		0
		);
	//用户代码段
	SetSegmentDescriptor(
		GDTPtr,
		SELECTOR_USER_CODE_INDEX,
		0,
		0,
		CS_NC | DPL3,
		0x20
		);
	//用户数据段
	SetSegmentDescriptor(
		GDTPtr,
		SELECTOR_USER_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL3,
		0
		);
	//TSS段
	for(Index = 64; Index < 128; Index++)
	{
		SetSegmentDescriptor(
			GDTPtr,
			Index,
			(VOID *)&TSS_GUEST[Index - 64],
			sizeof(TSS_GUEST) - 1,
			S_TSS | DPL0,
			0
			);
	}
	//初始化所有TSS
	for(Index = 0; Index < 64; Index++)
	{
		TSS_GUEST[Index].Reserved0 = 0;
		TSS_GUEST[Index].Reserved1 = 0;
		TSS_GUEST[Index].Reserved2 = 0;
		TSS_GUEST[Index].Reserved3 = 0;
		TSS_GUEST[Index].RSP0 = 0;		//特权级发生变化时使用
		TSS_GUEST[Index].RSP1 = 0;
		TSS_GUEST[Index].RSP2 = 0;
		/*注意若使用IST机制,IST的地址不能和之前栈地址冲突,否则发生中断后之前的栈被破坏,返回后就会发生异常.*/
		/*使用IST中断无法重入,只在某些特定异常中使用. */
		TSS_GUEST[Index].IST1 = 0x600000 + (UINT64)(&KERNEL_SPACE) - Index * 0x10000;		//中断栈使用
		TSS_GUEST[Index].IST2 = 0x700000 + (UINT64)(&KERNEL_SPACE) - Index * 0x10000;		//异常栈使用;
		TSS_GUEST[Index].IST3 = 0;
		TSS_GUEST[Index].IST4 = 0;
		TSS_GUEST[Index].IST5 = 0;
		TSS_GUEST[Index].IST6 = 0;
		TSS_GUEST[Index].IST7 = 0;
	}
	
	IDT_GUEST.Base = GDT_GUEST.Base + GDT_GUEST.Limit + 1;
	IDT_GUEST.Limit = 256 * sizeof(GATE_DESCRIPTOR_64) - 1;
	
	/* 初始化IDT */
	/* 0 - 31号中断属于异常，使用陷阱门.*/
	GATE_DESCRIPTOR_64  * IDTPtr = (GATE_DESCRIPTOR_64  *)IDT_GUEST.Base;
	for(Index = 0; Index < 0x20; Index++)
	{
		SetGateDescriptor(
			IDTPtr,
			Index,
			SELECTOR_KERNEL_CODE,
			(VOID *)ISPEntryPoint[Index],
			0,
			TRAP_GATE | DPL0
		);
	}

	/* 32 - 127号中断用作IRQ，使用中断门.*/
	for (Index = 0x20; Index < 0x80; Index++)
	{
		SetGateDescriptor(
			IDTPtr,
			Index,
			SELECTOR_KERNEL_CODE,
			(VOID *)ISPEntryPoint[Index],
			0,
			INT_GATE | DPL0
			);
	}

	/*128号中断用作系统调用，用户态陷阱门.*/
	SetGateDescriptor(
		IDTPtr,
		0x80,
		SELECTOR_KERNEL_CODE,
		(VOID *)ISPEntryPoint[Index],
		0,
		TRAP_GATE | DPL3
		);

	/*129 - 255中断用作软中断，使用陷阱门.*/
	for (Index = 0x81; Index < 0x100; Index++)
	{
		SetGateDescriptor(
			IDTPtr,
			Index,
			SELECTOR_KERNEL_CODE,
			(VOID *)ISPEntryPoint[Index],
			0,
			TRAP_GATE | DPL0
			);
	}

	RETURN_STATUS LocalApicEnable();
	LocalApicEnable();
	serial_print("Guest APIC_BASE:%x\n", X64ReadMsr(MSR_IA32_APICBASE));
	serial_print("Guest init done.\n");
}


VOID IrqHandlerGuest(UINT64 Vector)
{
	//if (Vector != 0x20)
	serial_print("Guest Int/Exp:0x%x\n", Vector);
}

VOID GuestLoop()
{
	UINT64 Counter0 = 0;
	UINT64 Counter1 = 0;
	UINT64 CounterT = 0;
	while (1) {
		Counter0 = 0;
		while (Counter0++ < 10000) {
			Counter1 = 0;
			while (Counter1++ < 40000);
		}
		serial_print("Guest running %d.\n", ++CounterT);
	}
	//	serial_print("Guest running...\n");
}
