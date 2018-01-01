#ifndef _APIC_H
#define _APIC_H

#include "Type.h"
#include "Cpu.h"
#include "SystemParameters.h"

//Local APIC 常量:

#define APIC_REG_BASE						0xFEE00000

#define APIC_REG_ID							0x20
#define APIC_REG_VER						0x30
#define APIC_REG_TPR						0x80
#define APIC_REG_APR						0x90
#define APIC_REG_PPR						0xA0
#define APIC_REG_EOI						0xB0
#define APIC_REG_RRD						0xC0
#define APIC_REG_LDR						0xD0
#define APIC_REG_DFR						0xE0
#define APIC_REG_SVR						0xF0
#define APIC_REG_ISR0						0x100
#define APIC_REG_ISR1						0x110
#define APIC_REG_ISR2						0x120
#define APIC_REG_ISR3						0x130
#define APIC_REG_ISR4						0x140
#define APIC_REG_ISR5						0x150
#define APIC_REG_ISR6						0x160
#define APIC_REG_ISR7						0x170
#define APIC_REG_TMR0						0x180
#define APIC_REG_TMR1						0x190
#define APIC_REG_TMR2						0x1A0
#define APIC_REG_TMR3						0x1B0
#define APIC_REG_TMR4						0x1C0
#define APIC_REG_TMR5						0x1D0
#define APIC_REG_TMR6						0x1E0
#define APIC_REG_TMR7						0x1F0
#define APIC_REG_IRR0						0x200
#define APIC_REG_IRR1						0x210
#define APIC_REG_IRR2						0x220
#define APIC_REG_IRR3						0x230
#define APIC_REG_IRR4						0x240
#define APIC_REG_IRR5						0x250
#define APIC_REG_IRR6						0x260
#define APIC_REG_IRR7						0x270
#define APIC_REG_ESR						0x280
#define APIC_REG_ICR0						0x300
#define APIC_REG_ICR1						0x310
#define APIC_REG_LVT_CMCI					0x2F0
#define APIC_REG_LVT_TIMER					0x320
#define APIC_REG_LVT_THERMAL				0x330
#define APIC_REG_LVT_PERFMON				0x340
#define APIC_REG_LVT_LINT0					0x350
#define APIC_REG_LVT_LINT1					0x360
#define APIC_REG_LVT_ERROR					0x370
#define APIC_REG_TIMER_ICR					0x380
#define APIC_REG_TIMER_CCR					0x390
#define APIC_REG_TIMER_DCR					0x3E0

//APIC timer 值定义
#define	APIC_TIMER_ONE_SHOT					0x0
#define APIC_TIMER_PERIODIC					0x20000
#define APIC_TIMER_TSC						0x40000

//定义 delivery mode 值
#define APIC_ICR_FIXED                       0x0000
#define APIC_ICR_LOWEST                      0x0100
#define APIC_ICR_SMI                         0x0200
#define APIC_ICR_NMI                         0x0400
#define APIC_ICR_EXTINT                      0x0700
#define APIC_ICR_INIT                        0x0500
#define APIC_ICR_FIXED_DELIVERY				 0x0000
#define APIC_ICR_LOWEST_DELIVERY             0x0100
#define APIC_ICR_SMI_DELIVERY                0x0200
#define APIC_ICR_NMI_DELIVERY                0x0400
#define APIC_ICR_EXTINT_DELIVERY             0x0700
#define APIC_ICR_INIT_DELIVERY               0x0500


#define APIC_ICR_MASKED						 0x10000
#define APIC_ICR_LOGICAL					 0x0800
#define APIC_ICR_PHYSICAL					 0x0000
#define APIC_ICR_EDGE						 0x0000
#define APIC_ICR_ASSERT						 0x4000
#define APIC_ICR_LEVEL						 0x8000
#define APIC_ICR_NOSHORTHAND				 0x00000
#define APIC_ICR_SELF						 0x40000
#define APIC_ICR_ALL_IN_SELF				 0x80000
#define APIC_ICR_ALL_EX_SELF				 0xC0000


//IPI 消息发送模式

#define STARTUP_IPI                             0x000C4600
#define INIT_IPI                                0x000C4500

#define PHYSICAL_ID                             (APIC_ICR_ASSERT | APIC_ICR_EDGE | APIC_ICR_PHYSICAL | APIC_ICR_NOSHORTHAND | APIC_ICR_FIXED)
#define LOGICAL_ID                              (APIC_ICR_ASSERT | APIC_ICR_EDGE | APIC_ICR_LOGICAL | APIC_ICR_NOSHORTHAND | APIC_ICR_FIXED)

#define X86_64_MAX_IRQ_CNT 256

UINT32 LocalApicRegRead(UINT32 Offset);
UINT64 LocalApicRegWrite(UINT32 Offset, UINT32 Value);

VOID ArchCallIrqLocal(UINT32 Vector);
VOID ArchCallIrqAll(UINT32 Vector);
VOID ArchCallIrqOther(UINT32 Vector);
VOID ArchCallIrqDest(UINT32 Vector, UINT32 ApciId);
#endif
