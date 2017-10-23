#ifndef _LONG_MODE_H
#define _LONG_MODE_H

#include "Type.h"

//#define DEBUG
#include "Debug.h"

//=====================================================================
//LongMode.h
//保护模式所需要的数据结构
//Oscar 2015.4
//=====================================================================

//LongMode下代码段,数据段,LDT段,TSS段描述符,每个描述符16字节
#pragma pack(1)
typedef struct
{
	UINT16	Limit0_15;
	UINT16	Base0_15;
	UINT8	Base16_23;
	UINT8	Attr1;
	UINT8	Limit16_19_Attr2;
	UINT8	Base24_31;
	UINT32	Base32_63;
	UINT32	Reserved;//Must be 0
}SEGMENT_DESCRIPTOR;
//段描述符本身是64位的，为了与门描述符保持一致这里添加了64位的0
#pragma pack()
//段描述符属性1============================================== 
//类型40-43,44
//非系统段                                                 
#define CS_C    0x9c	//一致性代码段
#define CS_NC   0x98	//非一致性代码段
#define DS_S_W  0x92	//数据段(SS段可写,其他段寄存器忽略该值)
#define	DS_S_NW 0x90 	//数据段(SS段不可写,加载到SS将导致异常)
//系统段
#define S_TSS	0x89  	//TSS Descriptor
#define S_LDT	0x82  	//LDT Descriptor

//权限45-46
#define DPL0	0x0
#define DPL1	0x20
#define DPL2	0x40
#define DPL3	0x60
//============================================================


//段描述符属性2===============================================
#define CS_L	0x20　//LongMode
#define CS_C_32	0x40　//Compatibility模式，３２位操作数
#define CS_C_16 0x00　//Compatibility模式，１６位操作数

#define DS_0	0x00  //数据段忽略这一属性
//============================================================

//门描述符,调用门，中断门，陷阱门
#pragma pack(1)
typedef struct
{
	UINT16  Offset0_15;
	UINT16  Selector;
	UINT8	IST;	//Interrupt Gate/Trap Gate,低三位有效
	UINT8	Attr;
	UINT16	Offset16_31;
	UINT32	Offset32_63;
	UINT32	Reserved;//Must be 0
}GATE_DESCRIPTOR_64;
#pragma pack()
//门描述符属性================================================
//类型40-43,44
#define INT_GATE  0x8E
#define TRAP_GATE 0x8F


//权限45-46
#define DPL0	0x0
#define DPL1	0x20
#define DPL2	0x40
#define DPL3	0x60
//============================================================

//GDTR,IDTR寄存器结构
#pragma pack(1)
typedef struct
{
	UINT16 Limit;
	UINT64 Base;
}GDTR,IDTR;
#pragma pack()
//TSS段
#pragma pack(1)
typedef struct
{
	UINT32 Reserved0;
	UINT64 RSP0;
	UINT64 RSP1;
	UINT64 RSP2;
	UINT64 Reserved1;
	UINT64 IST1;
	UINT64 IST2;
	UINT64 IST3;
	UINT64 IST4;
	UINT64 IST5;
	UINT64 IST6;
	UINT64 IST7;
	UINT64 Reserved2;
	UINT16 Reserved3;
	UINT16 IOMapBaseAddress;
}TSS64;
#pragma pack()

void SetGateDescriptor(
		GATE_DESCRIPTOR_64 *IDT,
		UINT8 Vector,
		UINT16 Selector,
		VOID * Offset,
		UINT8 IST,
		UINT8 Attr
		);
void SetSegmentDescriptor(
		SEGMENT_DESCRIPTOR *GDT,
		UINT16 Index,
		VOID * Base,
		UINT32 Limit,
		UINT8 Attr1,
		UINT8 Attr2
		);


//Descriptor index used by kernel
#define SELECTOR_NULL_INDEX			0
#define SELECTOR_KERNEL_CODE_INDEX	1
#define SELECTOR_KERNEL_DATA_INDEX	2
#define SELECTOR_USER_CODE_INDEX		3
#define SELECTOR_USER_DATA_INDEX		4
#define SELECTOR_TSS_INDEX			64

#define SELECTOR_NULL		(SELECTOR_NULL_INDEX*16)
#define SELECTOR_KERNEL_CODE	(SELECTOR_KERNEL_CODE_INDEX*16)
#define SELECTOR_KERNEL_DATA	(SELECTOR_KERNEL_DATA_INDEX*16)
#define SELECTOR_USER_CODE	(SELECTOR_USER_CODE_INDEX*16)
#define SELECTOR_USER_DATA	(SELECTOR_USER_DATA_INDEX*16)
#define SELECTOR_TSS		(SELECTOR_TSS_INDEX*16)

#endif
