#ifndef _THREAD_H
#define _THREAD_H

#include "Type.h"
#include "BaseLib.h"
#include "Irq.h"
//#include "Process.h"
#include "../Arch/X86_64/Task.h"

//#define DEBUG
#include "Debug.h"

#include "Timer.h"

//=====================================================================
//Thread.h
//线程所需要的数据结构TCB
//Oscar 2015.5
//=====================================================================

//Thread Attribute
#define KERNEL_THREAD  0x1
#define USER_THREAD    0x0
#define CREATE_PROCESS 0x2
#define CREATE_THREAD  0x0
#define THREAD_WAIT	   0x4

//Thread Status
#define THREAD_RUNNING		0x0
#define THREAD_READY		0x1
#define THREAD_SLEEPING		0x2
#define THREAD_STOPPED		0x3
#define THREAD_KILLED		0x4

#pragma pack(8)
typedef struct _PROCESS_CONTROL_BLOCK
{
	VOID * PhysicalAddress;
	VOID * PageTableBase;
	UINT64 PID;
	UINT64 ThreadCnt;					//创建线程的数量
	
	MEMORY_REGION * CodeMemorySpace;
	MEMORY_REGION * DataMemorySpace;
	MEMORY_REGION * StackMemorySpace;

	struct _THREAD_CONTROL_BLOCK * MainThread;		//指向第一个线程

	struct _PROCESS_CONTROL_BLOCK * ParentProcess;	//指向父进程的指针
	struct _PROCESS_CONTROL_BLOCK * ChildProcess;	//指向长子进程的指针(最新创建的子进程)
	struct _PROCESS_CONTROL_BLOCK * NextProcess;	//指向下一个进程的指针
	struct _PROCESS_CONTROL_BLOCK * PrevProcess;	//指向上一个进程的指针
}PROCESS_CONTROL_BLOCK;
#pragma pack(0)

#define MAX_STACK_FRAME_SIZE 0x10000
#pragma pack(16)
typedef struct _THREAD_CONTROL_BLOCK
{
	UINT8 StackFrame[MAX_STACK_FRAME_SIZE];			//保存CPU现场
	UINT64 StackPointer;
	UINT64 ReSchedule;					//重新调度标志
	UINT64 PreemptEnable;				//抢占允许
	
	UINT64 State;						//0-就绪，1-正在被调度执行，2-挂起，3-停止，4-可被回收
	UINT64 Kernel;						//是否为内核线程
	UINT64 Counter;						//运行时钟数
	UINT64 Priority;					//优先级
	UINT64 TID;							//线程ID
	UINT64 ThreadRetValue;				//线程返回值
	
	SPIN_LOCK SpinLock;

	VOID * PageTableBase;
	MEMORY_REGION * CodeMemorySpace;
	MEMORY_REGION * DataMemorySpace;
	MEMORY_REGION * StackMemorySpace;
	//struct _PROCESS_CONTROL_BLOCK * Process;		//属于哪个进程

	struct _THREAD_CONTROL_BLOCK * ParentThread;	//指向父线程的指针
	struct _THREAD_CONTROL_BLOCK * ChildThread;		//指向长子线程的指针(最新创建的子线程)
	struct _THREAD_CONTROL_BLOCK * NextThread;		//指向下一个线程的指针
	struct _THREAD_CONTROL_BLOCK * PrevThread;		//指向上一个线程的指针
	
	LIST_HEAD ThreadNode;
	LIST_HEAD WakeupListHead;
	
	UINT64 WhichCpu;
	UINT64 PaddingArea[16];
}THREAD_CONTROL_BLOCK;
#pragma pack()

typedef struct
{
	THREAD_CONTROL_BLOCK * Thread;
	LIST_HEAD WakeupList;
	SPIN_LOCK SpinLock;
}WAKEUP_NODE;

typedef struct
{
	SPIN_LOCK QueueSpinLock;
	UINT64 QueueLength;
	THREAD_CONTROL_BLOCK * CurrentThread;
	THREAD_CONTROL_BLOCK * PerCpuIdleThread;
	LIST_HEAD ThreadList;
}RUNNING_QUENE;

typedef struct
{
	SPIN_LOCK QueueSpinLock;
	UINT64 QueueLength;
	LIST_HEAD ThreadList;
}SLEEPING_QUENE;

UINT64 RunningQueueSchedule();
RETURN_STATUS RunningQueueInsert(THREAD_CONTROL_BLOCK * Thread, UINT64 RunQueueIndex);
RETURN_STATUS RunningQueueRemove(THREAD_CONTROL_BLOCK * Thread, UINT64 RunQueueIndex);

RETURN_STATUS SleepingQueueInsert(THREAD_CONTROL_BLOCK * Thread);
RETURN_STATUS SleepingQueueRemove(THREAD_CONTROL_BLOCK * Thread);

THREAD_CONTROL_BLOCK * GetCurrentThread();

void TaskManagerInit();
typedef void *(Func)(void *);
UINT64 GetCpuLogicNum();
void ThreadLoader(THREAD_CONTROL_BLOCK * Thread, Func *fun, void *arg);
UINT64 ThreadCreate(THREAD_CONTROL_BLOCK ** ThreadPtr, Func *EntryPoint, VOID * Arg, UINT64 Flag, UINT64 Priority);
VOID ThreadJoin(THREAD_CONTROL_BLOCK * Thread, void ** ThreadReturn);
void ThreadSleep(THREAD_CONTROL_BLOCK * Thread);
void ThreadWakeUp(THREAD_CONTROL_BLOCK * Thread);
void ThreadExit(THREAD_CONTROL_BLOCK * Thread);
UINT64 ThreadAddWakeup(THREAD_CONTROL_BLOCK * Thread, THREAD_CONTROL_BLOCK * ThreadToWakeup);
VOID ThreadTimer(VOID * Dev);
THREAD_CONTROL_BLOCK * PickNextThread();

#endif
