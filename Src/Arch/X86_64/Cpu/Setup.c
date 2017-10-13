
/*
* Setup.c
*
* Oscar 2016
*/
#include "LongMode.h"

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

GDTR GDT = {0,0};
IDTR IDT = {0,0};
TSS64 TSS[64] = {0};

extern int KERNEL_SPACE;

extern void(*IrqTable[])();
extern void(*ExceptionTable[])();

void DescriptorSet();

void DescriptorSet()
{
	UINT64 Index = 0;
	void (*ISPEntryPoint[256])()={0};

	for (Index = 0; Index < 0x20; Index++)
	{
		ISPEntryPoint[Index] = ExceptionTable[Index];
	}
	
	#define HWINT0 0x20

	for (Index = HWINT0; Index < 256; Index++)
	{
		ISPEntryPoint[Index] = IrqTable[Index - HWINT0];
	}
	
	GDT.Base = (UINT64)(&KERNEL_SPACE);
	GDT.Limit = 256 * sizeof(SEGMENT_DESCRIPTOR) - 1;
	SEGMENT_DESCRIPTOR * GDTPtr = (SEGMENT_DESCRIPTOR *)GDT.Base;
	
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
		SELETOR_KERNEL_CODE_INDEX,
		0,
		0,
		CS_NC | DPL0,
		0x20
		);
	//内核数据段
	SetSegmentDescriptor(
		GDTPtr,
		SELETOR_KERNEL_DATA_INDEX,
		0,
		0,
		DS_S_W | DPL0,
		0
		);
	//用户代码段
	SetSegmentDescriptor(
		GDTPtr,
		SELETOR_USER_CODE_INDEX,
		0,
		0,
		CS_NC | DPL3,
		0x20
		);
	//用户数据段
	SetSegmentDescriptor(
		GDTPtr,
		SELETOR_USER_DATA_INDEX,
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
			(VOID *)&TSS[Index - 64],
			sizeof(TSS) - 1,
			S_TSS | DPL0,
			0
			);
	}
	//初始化所有TSS
	for(Index = 0; Index < 64; Index++)
	{
		TSS[Index].Reserved0 = 0;
		TSS[Index].Reserved1 = 0;
		TSS[Index].Reserved2 = 0;
		TSS[Index].Reserved3 = 0;
		TSS[Index].RSP0 = 0;		//特权级发生变化时使用
		TSS[Index].RSP1 = 0;
		TSS[Index].RSP2 = 0;
		/*注意若使用IST机制,IST的地址不能和之前栈地址冲突,否则发生中断后之前的栈被破坏,返回后就会发生异常.*/
		/*使用IST中断无法重入,只在某些特定异常中使用. */
		TSS[Index].IST1 = 0x600000 + (UINT64)(&KERNEL_SPACE) - Index * 0x10000;		//中断栈使用
		TSS[Index].IST2 = 0x700000 + (UINT64)(&KERNEL_SPACE) - Index * 0x10000;		//异常栈使用;
		TSS[Index].IST3 = 0;
		TSS[Index].IST4 = 0;
		TSS[Index].IST5 = 0;
		TSS[Index].IST6 = 0;
		TSS[Index].IST7 = 0;
	}
	
	IDT.Base = GDT.Base + GDT.Limit + 1;
	IDT.Limit = 256 * sizeof(GATE_DESCRIPTOR_64) - 1;
	
	/* 初始化IDT */
	/* 0 - 31号中断属于异常，使用陷阱门.*/
	GATE_DESCRIPTOR_64  * IDTPtr = (GATE_DESCRIPTOR_64  *)IDT.Base;
	for(Index = 0; Index < 0x20; Index++)
	{
		SetGateDescriptor(
			IDTPtr,
			Index,
			SELETOR_KERNEL_CODE,
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
			SELETOR_KERNEL_CODE,
			(VOID *)ISPEntryPoint[Index],
			0,
			INT_GATE | DPL0
			);
	}

	/*128号中断用作系统调用，用户态陷阱门.*/
	SetGateDescriptor(
		IDTPtr,
		0x80,
		SELETOR_KERNEL_CODE,
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
			SELETOR_KERNEL_CODE,
			(VOID *)ISPEntryPoint[Index],
			0,
			TRAP_GATE | DPL0
			);
	}
}