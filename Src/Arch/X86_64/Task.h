#ifndef _TASK_H
#define _TASK_H

#include "Cpu/LongMode.h"
#include "Thread.h"

typedef struct
{
	UINT64 GS;
	UINT64 FS;
	UINT64 ES;
	UINT64 DS;
	
	UINT64 R15;
	UINT64 R14;
	UINT64 R13;
	UINT64 R12;
	UINT64 R11;
	UINT64 R10;
	UINT64 R9;
	UINT64 R8;
	UINT64 RDI;
	UINT64 RSI;
	UINT64 RBP;
	UINT64 RDX;
	UINT64 RCX;
	UINT64 RBX;
	UINT64 RAX;

	UINT64 RIP;
	UINT64 CS;
	UINT64 RFLAGS;
	UINT64 RSP;
	UINT64 SS;
}X86_64_STACK_FRAME;

RETURN_STATUS ArchFrameIpSet(VOID * StackFrame, UINT64 Ip);
RETURN_STATUS ArchFrameSpSet(VOID * StackFrame, UINT64 Sp);
RETURN_STATUS ArchFrameKernelModeSet(VOID * StackFrame);
RETURN_STATUS ArchFrameUserModeSet(VOID * StackFrame);
RETURN_STATUS ArchFrameParaRegSet(
	VOID * StackFrame,
	UINT64 Para1,
	UINT64 Para2,
	UINT64 Para3,
	UINT64 Para4,
	UINT64 Para5,
	UINT64 Para6);

VOID ArchTaskSchedule();
VOID ArchThreadStart();

#endif