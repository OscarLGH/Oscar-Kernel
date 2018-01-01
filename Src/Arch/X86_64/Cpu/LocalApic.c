#include "LocalApic.h"

BOOLEAN X2ApicSupported()
{
	X64_CPUID_BUFFER Buffer;
	X64GetCpuId(0x1, 0x0, &Buffer);
	if (Buffer.ECX & BIT21 == 1)
		return TRUE;
	return FALSE;
}

RETURN_STATUS LocalApicEnable()
{
	UINT64 Value = X64ReadMsr(MSR_IA32_APICBASE);

	Value |= BIT11;
	if (X2ApicSupported() == TRUE) {
		Value |= BIT10;
	}
	X64WriteMsr(MSR_IA32_APICBASE, Value);
	
	return RETURN_SUCCESS;
}

RETURN_STATUS ApicBaseSet(UINT32 ApicBase)
{
	UINT64 Value = X64ReadMsr(MSR_IA32_APICBASE);

	Value &= (BIT8 | BIT10 | BIT11);
	Value |= (ApicBase & (~0xfff));
	
	X64WriteMsr(MSR_IA32_APICBASE, Value);
	
	return RETURN_SUCCESS;
}

BOOLEAN IsBsp()
{
	UINT64 Value = X64ReadMsr(MSR_IA32_APICBASE);
	if (Value & BIT8)
		return TRUE;
	return FALSE;
}

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
