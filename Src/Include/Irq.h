#ifndef _IRQ_H
#define _IRQ_H

#include "Type.h"
#include "SpinLock.h"
#include "../Arch/Arch.h"


//#define DEBUG
//#include "Debug.h"
#ifdef DEBUG
#undef DEBUG
#endif

void * KMalloc(UINT64 Len);
void   KFree(void *Obj);
int printk(const char *fmt, ...);

#define MAX_IRQ_CNT 512

typedef struct _IRQ_FUN_NODE
{
	void * Device;
	void (*Fun)(void * Para);
	struct _IRQ_FUN_NODE * NextIrq;
}IRQ_FUN_NODE;

typedef struct _IRQ_DESCRIPTOR
{
	UINT64 NestedIrq;
	SPIN_LOCK SpinLock;
	IRQ_FUN_NODE * IrqFunListHead;
}IRQ_DESCRIPTOR;

typedef struct _IRQ_VECTOR
{
	IRQ_DESCRIPTOR IrqTable[MAX_IRQ_CNT];
	UINT64 IrqsOnCpu[256];
	UINT64 IrqFlag;
}IRQ_VECTOR;

typedef struct
{
	RETURN_STATUS (*ChipInit)(VOID);
	RETURN_STATUS (*IrqEnable)(UINT64 Irq);
	RETURN_STATUS (*IrqDisable)(UINT64 Irq);
	RETURN_STATUS (*IrqRemapping)(UINT64 Irq, UINT64 Vector, UINT64 Cpu);
	RETURN_STATUS (*SendIpi)(UINT64 Irq, UINT64 Vector, UINT64 Cpu);
}PIC_CHIP;

extern PIC_CHIP PicChip;

UINT64 GetIrqStatus(void);
void EnableLocalIrq(void);
void ResumeLocalIrq(void);
void DisableLocalIrq(void);
void DisableIrqn(UINT64 Irq);
void EnableIrqn(UINT64 Irq);
void CallIrqDest(UINT64 Irq, UINT64 Cpu);

void IrqRegister(UINT64 Vector, void Fun(void *), void * Dev);
void IrqHandler(UINT64 Vector);
UINT64 IrqAllocate();
void TraceHandler(UINT64 Vector);

#endif
