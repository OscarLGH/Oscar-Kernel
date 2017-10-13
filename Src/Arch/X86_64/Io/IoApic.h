#ifndef _IOAPIC_H
#define _IOAPIC_H

#include "../Cpu/LocalApic.h"
#include "Irq.h"


//IO APIC Base
#define IOAPIC_INDEX					0xFEC00000
#define IOAPIC_DATA						0xFEC00010
#define IOAPIC_EOI						0xFEC00040

//Register Index
#define IOAPIC_ID						0x0
#define IOAPIC_VERSION 					0x1
#define REDIRECTION_TABLE_BASE 			0x10
#define REDIRECTION_TABLE(x)			(REDIRECTION_TABLE_BASE + 2 * x)

//Redirection Table Registers:
#define APIC_REG_MASKED 				BIT16
#define APIC_TRI_LEVEL  				BIT15
#define APIC_DESTINATION_MODE_LOGICAL 	BIT11

//I/O APIC IRQ
#define APIC_8259_IRQ 0
#define APIC_KEYBOARD_IRQ 1
#define APIC_HPET_0_IRQ 2
#define APIC_8254_0_IRQ 2
#define APIC_SERIALA_IRQ 3
#define APIC_SERIALB_IRQ 4
#define APIC_PARALLELA_IRQ 5
#define APIC_FLOPPY_IRQ 6
#define APIC_PARALLELB_IRQ 7
#define APIC_RTC_IRQ 8
#define APIC_HPET1_IRQ 8
#define APIC_HPET2_IRQ 11
#define APIC_HPET3_IRQ 12
#define APIC_FERR_IRQ 13
#define APIC_SATA_P_IRQ 14
#define APIC_SATA_S_IRQ 15
#define APIC_PIRQA 16
#define APIC_PIRQB 17
#define APIC_PIRQC 18
#define APIC_PIRQD 19
#define APIC_PIRQE 20
#define APIC_PIRQF 21
#define APIC_PIRQG 22
#define APIC_PIRQH 23

UINT64 IoApicRegRead(UINT32 Index);
UINT64 IoApicRegWrite(UINT32 Index, UINT64 Value);

VOID IoApicInit();

#endif
