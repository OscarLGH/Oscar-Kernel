#ifndef _X64_DEBUG_H
#define _X64_DEBUG_H

#include "Cpu.h"
#include "Irq.h"
#include "../Task.h"

//#define IA32_DEBUGCTL 0x1d9
#define LBR BIT0
#define BTF BIT1
#define TR  BIT6
#define BTS BIT7
#define BTINT BIT8
#define BTS_OFF_USR BIT10
#define BTS_OFF_OS  BIT9

//#define MSR_LASTBRANCH_0_FROM_IP 0x680
//#define MSR_LASTBRANCH_0_TO_LIP  0x6c0

#define BRANCH_REGS 16

//#define MSR_LASTBRANCH_TOS 0x1c9

//#define MSR_LBR_SELECT 0x1c8

#define CPL_EQ_0 BIT0
#define CPL_NEQ_0 BIT1
#define JCC BIT2
#define NEAR_REL_CALL BIT3
#define NEAR_IND_CALL BIT4
#define NEAR_RET BIT5
#define NEAR_IND_JMP BIT6
#define NEAR_REL_JMP BIT7
#define FAR_BRANCH BIT8

typedef struct
{
	UINT64 FromIp;
	UINT64 ToIp;
}CPU_BRANCH_DATA;

typedef struct
{
	CPU_BRANCH_DATA BranchData[16];
	UINT64 LbrTop;
}CPU_DEBUG_INFO;

VOID X64SetupBranchRecord();
RETURN_STATUS X64ReadBranchRecord(UINT64 Index, CPU_BRANCH_DATA * Data);
RETURN_STATUS X64GetCpuDebugInfo(CPU_DEBUG_INFO * Info);

#endif