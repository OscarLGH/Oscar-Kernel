#include "Irq.h"

IRQ_VECTOR IrqVector = { 0 };
PIC_CHIP PicChip = {0};

UINT64 GetIrqStatus(void)
{
	return ArchGetIrqStatus();
}
void EnableLocalIrq(void)
{
	ArchEnableCpuIrq();
}

void ResumeLocalIrq(void)
{
	ArchEnableCpuIrq();
}

void DisableLocalIrq(void)
{
	ArchDisableCpuIrq();
}

void DisableIrqn(UINT64 Irq)
{
	PicChip.IrqDisable(Irq);
}

void EnableIrqn(UINT64 Irq)
{
	PicChip.IrqEnable(Irq);
}

void CallIrqDest(UINT64 Irq, UINT64 Cpu)
{
	//ArchCallIrqDest(Irq, Cpu);
	VOID LocalApicSendIpi(UINT64 ApicId, UINT64 Vector);
	LocalApicSendIpi(Cpu, Irq);
}

UINT64 IrqAllocate()
{
	UINT64 Value = 0;
	UINT64 Index = 0;
	UINT64 Bit = 0;
	/*while (1)
	{
		Value = IrqVector.IrqTable[Index ++];
		while (Bit < 64)
		{
			if (!(Value & (1ULL << Bit)))
			{
				Value |= (1ULL << Bit);
				IrqVector.IrqTable[Index] = Value;
				return Index * 64 + Bit;
			}
		}
		Bit = 0;
	}*/
	return 0;
}

void IrqRegister(UINT64 Vector, void Fun(void *), void * Dev)
{
	IRQ_DESCRIPTOR * IrqDesc = &IrqVector.IrqTable[Vector];
	IrqDesc->NestedIrq = 0;
	IRQ_FUN_NODE * IrqNode = IrqDesc->IrqFunListHead;
	
	SpinLockIrqSave(&IrqDesc->SpinLock);

	if(0 != IrqNode)
	{
		while(IrqNode->NextIrq)
		{
			IrqNode = IrqNode->NextIrq;
		}
		IrqNode->NextIrq = (IRQ_FUN_NODE *)KMalloc(sizeof(IRQ_FUN_NODE));
		IrqNode->NextIrq->Device = Dev;
		IrqNode->NextIrq->Fun = Fun;
		IrqNode->NextIrq->NextIrq = 0;
	}
	else
	{
		IrqNode = (IRQ_FUN_NODE *)KMalloc(sizeof(IRQ_FUN_NODE));
		IrqNode->Device = Dev;
		IrqNode->Fun = Fun;
		IrqNode->NextIrq = 0;
		IrqVector.IrqTable[Vector].IrqFunListHead = IrqNode;
	}
	SpinUnlockIrqRestore(&IrqDesc->SpinLock);
}

void IrqHandler(UINT64 Vector)
{
	IRQ_DESCRIPTOR * IrqDesc = &IrqVector.IrqTable[Vector];
	IRQ_FUN_NODE * IrqNode = NULL;

	IrqDesc->NestedIrq++;

	//printk("IRQ %x on cpu %02d.\n", Vector, ArchGetCpuIdentifier());
	if (Vector == 0xff)
		printk("VMM Injected Interrupt.\n");

	
	if (IrqVector.IrqTable[Vector].NestedIrq > (0x10000 / 160 - 1))
	{
		//printk("IRQ %d ReEntered %d times.Kernel stack corrupted.\n", Vector, IrqVector.IrqTable[Vector].NestedIrq);
	}

	if(Vector != 0x20)					//用于任务调度的时钟中断不可抢占，重入
		EnableLocalIrq();
	
	if (Vector < 0x20)
		TraceHandler(Vector);

	IrqNode = IrqVector.IrqTable[Vector].IrqFunListHead;
	if(IrqNode)
	{
		while(IrqNode)
		{
			IrqNode->Fun(IrqNode->Device);
			IrqNode = IrqNode->NextIrq;
		}
	}
	else
	{
		if(Vector < 0x20)
			TraceHandler(Vector);
	}

	//MilliSecondDelay(20);
	
	DisableLocalIrq();
	IrqDesc->NestedIrq--;
}

