#include "LocalApic.h"

UINT32 LocalApicRegRead(UINT32 Offset)
{
	UINT64 LocalApicAddr = PHYS2VIRT(APIC_REG_BASE);
	
	return *(UINT32*)(LocalApicAddr + Offset);
}

UINT64 LocalApicRegWrite(UINT32 Offset, UINT32 Value)
{
	UINT64 LocalApicAddr = PHYS2VIRT(APIC_REG_BASE);
	
	*(UINT32*)(LocalApicAddr + Offset) = Value;
}

VOID LocalApicSendIpi(UINT64 ApicId, UINT64 Vector)
{
	UINT32 Destination = (ApicId << 24);
	LocalApicRegWrite(APIC_REG_ICR1, Destination);
	LocalApicRegWrite(APIC_REG_ICR0, Vector | APIC_ICR_FIXED | APIC_ICR_PHYSICAL | APIC_ICR_NOSHORTHAND);
}
