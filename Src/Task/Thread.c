#include "Thread.h"

UINT64 ThreadCnt;

RUNNING_QUENE * RunningQueue = NULL;	//Running queue, one per cpu (logic)
SLEEPING_QUENE * SleepingQueue = NULL;
THREAD_CONTROL_BLOCK * ThreadTree = NULL;
UINT64 ClockTicks = 0;

extern void ArchTaskSchedule();
#define TIMER_IRQ 0x2

VOID Idle()
{
	while (1)
	{
		ArchHaltCpu();
	}
}

void TaskManagerInit()
{
	UINT64 Index = 0;
	
	ThreadCnt = 0;

	SYSTEM_PARAMETERS * SysPara = (SYSTEM_PARAMETERS *)SYSTEMPARABASE;
	
	RunningQueue = KMalloc(64 * sizeof(*RunningQueue));
	SleepingQueue = KMalloc(sizeof(*SleepingQueue));
	
	ThreadCnt ++;
	
	for (Index = 0; Index < 64; Index++)
	{
		INIT_LIST_HEAD(&RunningQueue[Index].ThreadList);

		RunningQueue[Index].PerCpuIdleThread = KMalloc(sizeof(THREAD_CONTROL_BLOCK));
		UINT64 StackPointer = (UINT64)&RunningQueue[Index].PerCpuIdleThread->StackFrame + MAX_STACK_FRAME_SIZE;

		ArchFrameIpSet(RunningQueue[Index].PerCpuIdleThread->StackFrame, (UINT64)Idle);
		ArchFrameSpSet(RunningQueue[Index].PerCpuIdleThread->StackFrame, StackPointer);
		ArchFrameKernelModeSet(RunningQueue[Index].PerCpuIdleThread->StackFrame);
		RunningQueue[Index].PerCpuIdleThread->StackPointer = StackPointer - 192;

		RunningQueue[Index].PerCpuIdleThread->ReSchedule = 1;
		RunningQueue[Index].PerCpuIdleThread->State = 0;
		RunningQueue[Index].PerCpuIdleThread->Counter = 0;
		RunningQueue[Index].PerCpuIdleThread->Priority = 0;
		RunningQueue[Index].PerCpuIdleThread->TID = 0;
		RunningQueue[Index].PerCpuIdleThread->ParentThread = 0;

		RunningQueue[Index].CurrentThread = NULL;
	}
	INIT_LIST_HEAD(&SleepingQueue->ThreadList);
	
	IrqRegister(0x20, ThreadTimer, NULL);
	//ArchEnableIrqn(TIMER_IRQ);
}

void ThreadLoader(THREAD_CONTROL_BLOCK * Thread, Func *EntryPoint, VOID * Arg)
{
	UINT64 RetValue;
	RetValue = (UINT64)(*EntryPoint)(Arg);
	SpinLockIrqSave(&Thread->SpinLock);
	Thread->ThreadRetValue = RetValue;
	SpinUnlockIrqRestore(&Thread->SpinLock);
	ThreadExit(Thread);
	while (1);
}

UINT64 ThreadCreate(THREAD_CONTROL_BLOCK ** ThreadPtr, Func *EntryPoint, VOID * Arg, UINT64 Flag, UINT64 Priority)
{
	UINT64 PhyPage, VirtPage;
	UINT64 StackPointer;

	THREAD_CONTROL_BLOCK * Current = GetCurrentThread();
	
	*ThreadPtr = KMalloc(sizeof(**ThreadPtr));
	(*ThreadPtr)->ParentThread = Current;

	if (Flag & CREATE_PROCESS)
	{
		//Allocate new address space
		(*ThreadPtr)->CodeMemorySpace = KMalloc(sizeof(*(*ThreadPtr)->CodeMemorySpace));
		(*ThreadPtr)->DataMemorySpace = KMalloc(sizeof(*(*ThreadPtr)->CodeMemorySpace));
		(*ThreadPtr)->StackMemorySpace = KMalloc(sizeof(*(*ThreadPtr)->CodeMemorySpace));

		if (Flag & KERNEL_THREAD)
		{
			(*ThreadPtr)->CodeMemorySpace->Base = 0xFFFF800000000000;
			(*ThreadPtr)->DataMemorySpace->Base = 0xFFFF800000000000;
			(*ThreadPtr)->StackMemorySpace->Base = 0xFFFF800000000000;
		}
		else
		{
			(*ThreadPtr)->CodeMemorySpace->Base = 0x0000000000000000;
			(*ThreadPtr)->DataMemorySpace->Base = 0x0000000000000000;
			(*ThreadPtr)->StackMemorySpace->Base = 0x0000000000000000;
		}

		(*ThreadPtr)->PageTableBase = KMalloc(0x200000);
		
		memcpy((*ThreadPtr)->PageTableBase, (VOID *)PHYS2VIRT(GetPageTableBase()), 0x200000);
		
	}
	else
	{
		if (Current)
		{
			(*ThreadPtr)->CodeMemorySpace = Current->CodeMemorySpace;
			(*ThreadPtr)->DataMemorySpace = Current->DataMemorySpace;
			(*ThreadPtr)->StackMemorySpace = Current->StackMemorySpace;
			(*ThreadPtr)->PageTableBase = Current->PageTableBase;
		}
	}
	
	if (Flag & USER_THREAD)
	{
		StackPointer = (UINT64)KMalloc(0x200000);
		PhyPage = VIRT2PHYS(StackPointer);
		if (Current)
		{
			MapPage((*ThreadPtr)->PageTableBase, (VOID *)PhyPage, (VOID *)Current->StackMemorySpace->Base, 0x200000, 0);
			StackPointer = Current->StackMemorySpace->Base;
		}
		ArchFrameUserModeSet((*ThreadPtr)->StackFrame);
	}
	else
	{
		StackPointer = (UINT64)&(*ThreadPtr)->StackFrame + MAX_STACK_FRAME_SIZE;
		ArchFrameKernelModeSet((*ThreadPtr)->StackFrame);
	}

	ArchFrameIpSet((*ThreadPtr)->StackFrame, (UINT64)ThreadLoader);
	ArchFrameSpSet((*ThreadPtr)->StackFrame, StackPointer);
	ArchFrameParaRegSet((*ThreadPtr)->StackFrame, (UINT64)(*ThreadPtr), (UINT64)EntryPoint, (UINT64)Arg, 0, 0, 0);
	(*ThreadPtr)->StackPointer = (UINT64)&(*ThreadPtr)->StackFrame + MAX_STACK_FRAME_SIZE - 192;

	(*ThreadPtr)->State = 0;
	(*ThreadPtr)->Counter = Priority;
	(*ThreadPtr)->Priority = Priority;
	(*ThreadPtr)->TID = ThreadCnt++;

	//KDEBUG("Current = %d\n", Current->TID);
	//所有的线程一定是当前运行的线程创建的

	(*ThreadPtr)->ParentThread = Current;
	//SpinLockIrqSave(&Current->SpinLock);

	KDEBUG("[CPU %02d][Thread %04d] Thread %d created.\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, (*ThreadPtr)->TID);

	//如果父线程没有子线程，表明这是第一个子线程，使父线程的孩子指针指向当前线程
	//如果父线程存在子线程，则从长子线程开始遍历，直到NextThread域为0，则将NextThread指向当前线程
	//if ((*ThreadPtr)->ParentThread)
	//{
	//	//KDEBUG("(*ThreadPtr)->ParentThread = %x\n", (*ThreadPtr)->ParentThread);

	//	
	//	//KDEBUG("(*ThreadPtr)->ParentThread->ChildThread = %x\n", (*ThreadPtr)->ParentThread->ChildThread);
	//	if ((*ThreadPtr)->ParentThread->ChildThread == 0)
	//	{
	//		(*ThreadPtr)->ParentThread->ChildThread = (*ThreadPtr);
	//		(*ThreadPtr)->PrevThread = 0;
	//		//KDEBUG("(*ThreadPtr)->ParentThread->ChildThread = %x\n", (*ThreadPtr)->ParentThread->ChildThread);
	//	}
	//	else
	//	{
	//		THREAD_CONTROL_BLOCK * Next;

	//		Next = (*ThreadPtr)->ParentThread->ChildThread;
	//		while (Next && Next->NextThread)
	//		{
	//			Next = Next->NextThread;
	//		}

	//		//SpinLockIrqSave(&Next->SpinLock);
	//		Next->NextThread = (*ThreadPtr);
	//		//SpinUnlockIrqRestore(&Next->SpinLock);
	//		(*ThreadPtr)->PrevThread = Next;

	//		//KDEBUG("(*ThreadPtr)->PrevThread = %x\n", (*ThreadPtr)->PrevThread);
	//		//KDEBUG("Next->NextThread = %x\n", Next->NextThread);
	//	}
	//}
	//SpinUnlockIrqRestore(&Current->SpinLock);

	(*ThreadPtr)->WhichCpu = RunningQueueSchedule();

	INIT_LIST_HEAD(&(*ThreadPtr)->WakeupListHead);

	//RunningQueueInsert(*ThreadPtr, (*ThreadPtr)->WhichCpu);
	//KDEBUG("RunQueue MinWorkLoad = %d\n", MinWorkLoad);
	//RunningQueueInsert(*ThreadPtr, (*ThreadPtr)->WhichCpu);
	SleepingQueueInsert(*ThreadPtr);

	return 0;
}

void ThreadWakeUp(THREAD_CONTROL_BLOCK * Thread)
{
	RUNNING_QUENE * Run = &RunningQueue[Thread->WhichCpu];
	SpinLockIrqSave(&Thread->SpinLock);
	//KDEBUG("[CPU %02d][Thread %04d] Thread %d wakes...\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, Thread->TID);
	if (Thread->State != THREAD_READY)
	{
		SleepingQueueRemove(Thread);
		Thread->State = THREAD_READY;
		/*如果线程优先级高于目的CPU上运行的线程，则执行抢占.*/
		if(Thread->Priority > Run->CurrentThread->Priority)
			Run->CurrentThread->ReSchedule = 1;	

		RunningQueueInsert(Thread, Thread->WhichCpu);
	}
	
	SpinUnlockIrqRestore(&Thread->SpinLock);

	//唤醒不需要立即运行，下一个时钟中断会重新调度
	CallIrqDest(0x20, Thread->WhichCpu);
}


void ThreadSleep(THREAD_CONTROL_BLOCK * Thread)
{
	SpinLockIrqSave(&Thread->SpinLock);
	KDEBUG("[CPU %02d][Thread %04d] Thread %d sleeps...\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, Thread->TID);
	if (Thread->State != THREAD_SLEEPING)
	{
		RunningQueueRemove(Thread, Thread->WhichCpu);
		Thread->State = THREAD_SLEEPING;
		SleepingQueueInsert(Thread);
	}
	//休眠必须立即结束目标线程
	
	SpinUnlockIrqRestore(&Thread->SpinLock);
	//Thread->Counter = 0;
	Thread->ReSchedule = 1;
	//ArchTaskSchedule();
	//asm("int $0x20");
	CallIrqDest(0x20, Thread->WhichCpu);
}

UINT64 ThreadAddWakeup(THREAD_CONTROL_BLOCK * Thread, THREAD_CONTROL_BLOCK * ThreadToWakeup)
{
	WAKEUP_NODE * WakeupNode = KMalloc(sizeof(*WakeupNode));
	WakeupNode->Thread = ThreadToWakeup;
	SpinLockIrqSave(&Thread->SpinLock);
	KDEBUG("[CPU %02d][Thread %04d] %s: Inserting Thread %d to %d 's wakeup queue.\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, ThreadToWakeup->TID, Thread->TID);
	ListAddTail(&WakeupNode->WakeupList, &Thread->WakeupListHead);
	SpinUnlockIrqRestore(&Thread->SpinLock);

}

VOID ThreadJoin(THREAD_CONTROL_BLOCK * Thread, void **ThreadReturn)
{
	THREAD_CONTROL_BLOCK * Current = GetCurrentThread();
	
	KDEBUG("[CPU %02d][Thread %04d] %s: thread %d is waiting thread %d to terminate.\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, GetCurrentThread()->TID, Thread->TID);
	
	while (1)
	{
		SpinLockIrqSave(&Thread->SpinLock);

		/* 如果等待Join的线程已经结束，无需休眠当前线程，否则将导致死锁.*/
		if (Thread->State != THREAD_STOPPED && Thread->State != THREAD_KILLED)
		{
			//MicroSecondDelay(100);
			KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d is sleeping.Thread->State = %d.\n", ArchGetCpuIdentifier(), Current->TID, __FUNCTION__, Current->TID, Current->State);
			
			/* 判断线程状态和休眠当前线程不能被打断
			否则如果判断条件成立，而ThreadExit中又提前调用了唤醒操作，这将导致死锁.*/

			/* 由于目前在临界区，中断被关闭，ThreadSleep中调用APIC中断的动作被屏蔽.*/
			ThreadSleep(Current);
			
			SpinUnlockIrqRestore(&Thread->SpinLock);
			/* 补发一个中断以防SpinUnlockIrqRestore时没有发生中断.*/
			/*中断发生，当前线程被挂起.*/
			//CallIrqAll(0x20);
			CallIrqDest(0x20, Thread->WhichCpu);
		}
		else
		{
			SpinUnlockIrqRestore(&Thread->SpinLock);
			break;
		}

		/*线程被唤醒会从这里继续运行*/
		KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d is awake.Thread->State = %d.\n", ArchGetCpuIdentifier(), Current->TID, __FUNCTION__, Current->TID, Current->State);

		//MicroSecondDelay(100);
		//KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d sleeping.Thread->State = %d.\n", ArchGetCpuIdentifier(), Current->TID, __FUNCTION__, Current->TID, Current->State);
		
		//KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d waking.Thread->State = %d.\n", ArchGetCpuIdentifier(), Current->TID, __FUNCTION__, Current->TID, Current->State);
		
		/*这里不加延时会导致卡死，开始追不到原因
		2016.12.8.
		后来发现可能是唤醒或者休眠实现的逻辑有问题，
		如果将一个已经运行的线程继续唤醒会导致同一个节点两次被插入链表，导致链表中断
		但睡眠之前加打印或者延时还是会卡死...
		2016.12.9
		问题可能是因为这里休眠的代码没有和线程退出的代码互斥，
		若休眠和唤醒同时发生或者先唤醒再休眠的话，
		就会进入死锁状态
		*/
		//MicroSecondDelay(100);						
		//KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d waking after delay.Thread->State = %d.\n", ArchGetCpuIdentifier(), Current->TID, __FUNCTION__, Current->TID, Current->State);
		
		//SpinLockIrqSave(&Thread->SpinLock);			//唤醒后继续加锁
	};
	KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d terminated.Thread->State = %d, Return value = %x\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, Thread->TID, Thread->State, Thread->ThreadRetValue);
	
	SpinLockIrqSave(&Thread->SpinLock);

	if(0 != ThreadReturn)
		*ThreadReturn = (VOID *)Thread->ThreadRetValue;

	Thread->State = THREAD_KILLED;
	Thread->Counter = 0;
	Thread->ReSchedule = 1;
	
	SpinUnlockIrqRestore(&Thread->SpinLock);
	
}

void ThreadExit(THREAD_CONTROL_BLOCK * Thread)
{
	//DisableIrqn(TIMER_IRQ);

	//这里要先递归然后再删除，否则长子被删除，后续兄弟都变成了“孤魂野鬼”
	
	//如果有子线程，则递归退出所有子线程
	KDEBUG("[CPU %02d][Thread %04d] Thread %d is terminating...\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, Thread->TID);
	//SpinLockIrqSave(&Thread->SpinLock);
	SpinLockIrqSave(&Thread->SpinLock);
	KDEBUG("[CPU %02d][Thread %04d] Thread %d is terminating...\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, Thread->TID);

	if(Thread->ChildThread)
	{
		THREAD_CONTROL_BLOCK * Next;
		
		for(Next = Thread->ChildThread; Next; Next = Next->NextThread)
		{
			KDEBUG("[CPU %02d][Thread %04d] Child Thread %d is terminating...\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, Next->TID);
			//ThreadExit(Next);
		}
	}

	//线程双链表删除操作
	if (Thread->PrevThread)
	{
		SpinLockIrqSave(&Thread->PrevThread->SpinLock);
		Thread->PrevThread->NextThread = Thread->NextThread;
		SpinUnlockIrqRestore(&Thread->PrevThread->SpinLock);
	}
		
	if (Thread->NextThread)
	{
		SpinLockIrqSave(&Thread->NextThread->SpinLock);
		Thread->NextThread->PrevThread = Thread->PrevThread;
		SpinUnlockIrqRestore(&Thread->NextThread->SpinLock);
	}

	
	KDEBUG("[CPU %02d][Thread %04d] Thread %d terminated.\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, Thread->TID);
	
	WAKEUP_NODE * NextWakeup;
	ListForEachEntry(NextWakeup, &Thread->WakeupListHead, WakeupList)
	{
		//SpinLockIrqSave(&NextWakeup->Thread->SpinLock);
		KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d is waking up thread %d...\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, Thread->TID, NextWakeup->Thread->TID);
		//KDEBUG("[CPU %02d][Thread %04d] %s:NextWakeup->State = %x\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, NextWakeup->Thread->State);
		if (NextWakeup->Thread->State == THREAD_SLEEPING)
			ThreadWakeUp(NextWakeup->Thread);
		//RunningQueueInsert(NextWakeup->Thread, NextWakeup->Thread->WhichCpu);

		//if(NextWakeup->Thread->State == THREAD_READY)
		//	KDEBUG("[CPU %02d][Thread %04d] %s:Thread %d is already running.\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, NextWakeup->Thread->TID);

		//KDEBUG("[CPU %02d][Thread %04d] %s:NextWakeup->State = %x\n", ArchGetCpuIdentifier(), GetCurrentThread()->TID, __FUNCTION__, NextWakeup->Thread->State);
		//SpinUnlockIrqRestore(&NextWakeup->Thread->SpinLock);
	}

	Thread->State = THREAD_STOPPED;
	RunningQueueRemove(Thread, Thread->WhichCpu);
	//CallIrqOther(0x20);
	SpinUnlockIrqRestore(&Thread->SpinLock);
	Thread->Counter = 0;
	Thread->ReSchedule = 1;
	/*如果SpinUnlockIrqRestore时没有发生中断，则这里主动调用一次
	如果发生了中断，该线程会被立即结束，下面的语句不会执行*/
	CallIrqDest(0x20, Thread->WhichCpu);	

	/* Cannot reach here.*/
	while (1);
}

UINT64 RunningQueueSchedule()
{
	SYSTEM_PARAMETERS * SysParam = (SYSTEM_PARAMETERS *)SYSTEMPARABASE;
	UINT64 RunningQueueCnt = SysParam->ProcessorCnt;
	UINT64 Index;
	UINT64 Min = 0xFFFFFFFF;
	UINT64 MinIndex = 0;

	for (Index = 0; Index < RunningQueueCnt; Index++)
	{
		//KDEBUG("RunningQueue[Index].QueueLength = %d\n", RunningQueue[Index].QueueLength);
		SpinLockIrqSave(&RunningQueue[Index].QueueSpinLock);
		if (RunningQueue[Index].QueueLength < Min)
		{
			Min = RunningQueue[Index].QueueLength;
			MinIndex = Index;
		}
		SpinUnlockIrqRestore(&RunningQueue[Index].QueueSpinLock);
	}

	//return 3;
	//MinIndex = 0;
	//printk("%s:%d\n", __FUNCTION__, MinIndex % RunningQueueCnt);
	return MinIndex % SysParam->ProcessorCnt;
}

THREAD_CONTROL_BLOCK * GetCurrentThread()
{
	THREAD_CONTROL_BLOCK * CurrentThread = NULL;
	UINT64 CurrentCpu = ArchGetCpuIdentifier();
	if (RunningQueue == NULL)
		return NULL;

	CurrentThread = RunningQueue[CurrentCpu].CurrentThread;
	return CurrentThread;
}

RETURN_STATUS RunningQueueInsert(THREAD_CONTROL_BLOCK * Thread, UINT64 RunQueueIndex)
{
	SpinLockIrqSave(&RunningQueue[RunQueueIndex].QueueSpinLock);
	ListAddTail(&Thread->ThreadNode, &RunningQueue[RunQueueIndex].ThreadList);
	//Thread->State = THREAD_READY;														//64Bit数对齐的话本身就是原子操作，无需加锁
	KDEBUG("Thread %d inserted into run queue %d.\n", Thread->TID, RunQueueIndex);
	RunningQueue[RunQueueIndex].QueueLength++;
	SpinUnlockIrqRestore(&RunningQueue[RunQueueIndex].QueueSpinLock);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS RunningQueueRemove(THREAD_CONTROL_BLOCK * Thread, UINT64 RunQueueIndex)
{
	SpinLockIrqSave(&RunningQueue[RunQueueIndex].QueueSpinLock);
	ListDelete(&Thread->ThreadNode);
	//Thread->State = THREAD_STOPPED;
	KDEBUG("Thread %d removed from run queue %d.\n", Thread->TID, RunQueueIndex);
	RunningQueue[RunQueueIndex].QueueLength--;
	SpinUnlockIrqRestore(&RunningQueue[RunQueueIndex].QueueSpinLock);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS SleepingQueueInsert(THREAD_CONTROL_BLOCK * Thread)
{
	SpinLockIrqSave(&SleepingQueue->QueueSpinLock);
	ListAddTail(&Thread->ThreadNode, &SleepingQueue->ThreadList);
	//Thread->State = THREAD_SLEEPING;
	KDEBUG("Thread %d inserted into sleep queue.\n", Thread->TID);
	SleepingQueue->QueueLength++;
	SpinUnlockIrqRestore(&SleepingQueue->QueueSpinLock);

	return RETURN_SUCCESS;
}

RETURN_STATUS SleepingQueueRemove(THREAD_CONTROL_BLOCK * Thread)
{
	SpinLockIrqSave(&SleepingQueue->QueueSpinLock);
	ListDelete(&Thread->ThreadNode);
	//Thread->State = THREAD_STOPPED;
	KDEBUG("Thread %d removed from sleep queue.\n", Thread->TID);
	SleepingQueue->QueueLength--;
	SpinUnlockIrqRestore(&SleepingQueue->QueueSpinLock);

	return RETURN_SUCCESS;
}

VOID ThreadTimer(VOID * Dev)
{
	//printk("%s\n", __FUNCTION__);
	TimerChip.Jiffies++;
	THREAD_CONTROL_BLOCK * CurrentThread = GetCurrentThread();

	if (CurrentThread != NULL)
	{
		if (CurrentThread->Counter == 0)
		{
			//printk("Thread %d timeout.\n", CurrentThread->TID);
			CurrentThread->ReSchedule = 1;
		}
		else
		{
			CurrentThread->Counter--;
		}
	}
}

THREAD_CONTROL_BLOCK * PickNextThread()
{
	UINT64 CurrentCpu = ArchGetCpuIdentifier();
	RUNNING_QUENE * Run = &RunningQueue[CurrentCpu];
	
	UINT64 MaxTicks = 0;
	THREAD_CONTROL_BLOCK * ThreadReady = NULL;
	THREAD_CONTROL_BLOCK * NextThread = NULL;
	
	//KDEBUG("%s,cpu = %d\n", __FUNCTION__, CurrentCpu);
	
	if (Run->CurrentThread)
	{
		SpinLockIrqSave(&Run->CurrentThread->SpinLock);
		Run->CurrentThread->ReSchedule = 0;
		Run->PerCpuIdleThread->ReSchedule = 1;			//Idle线程
		
		if (Run->CurrentThread->State == THREAD_RUNNING)
			Run->CurrentThread->State = THREAD_READY;
		SpinUnlockIrqRestore(&Run->CurrentThread->SpinLock);
	}
	
	SpinLockIrqSave(&Run->QueueSpinLock);

	ListForEachEntry(NextThread, &Run->ThreadList, ThreadNode)
	{
		SpinLockIrqSave(&NextThread->SpinLock);

		if (NextThread->State == THREAD_READY)
		{
			if (NextThread->Counter > MaxTicks)
			{
				ThreadReady = NextThread;
			}
		}

		SpinUnlockIrqRestore(&NextThread->SpinLock);
	}
	SpinUnlockIrqRestore(&Run->QueueSpinLock);
	
	if (ThreadReady == NULL)
	{
		SpinLockIrqSave(&Run->QueueSpinLock);
		ListForEachEntry(NextThread, &Run->ThreadList, ThreadNode)
		{
			NextThread->Counter = NextThread->Priority;
		}
		SpinUnlockIrqRestore(&Run->QueueSpinLock);
	}

	//SpinLockIrqSave(&ThreadReady->SpinLock);

	Run->CurrentThread = ThreadReady;
	if (ThreadReady == NULL)
	{
		Run->CurrentThread = Run->PerCpuIdleThread;
		return Run->CurrentThread;
	}

	ThreadReady->State = THREAD_RUNNING;
	
	return ThreadReady;
}