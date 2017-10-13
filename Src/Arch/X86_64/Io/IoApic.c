#include "IoApic.h"
#include "InitCall.h"

UINT64 IoApicRegRead(UINT32 Index)
{
	volatile UINT32 * IOAPICIndex = (UINT32 *)PHYS2VIRT(IOAPIC_INDEX);
	*IOAPICIndex = Index;
	volatile UINT64 ValueL = *(volatile UINT32 *)PHYS2VIRT(IOAPIC_DATA);
	*IOAPICIndex = Index + 1;
	volatile UINT64 ValueH = *(volatile UINT32 *)PHYS2VIRT(IOAPIC_DATA);

	return (ValueH << 32) | (ValueL);
}

UINT64 IoApicRegWrite(UINT32 Index, UINT64 Value)
{
	volatile UINT32 * IOAPICIndex = (volatile UINT32 *)PHYS2VIRT(IOAPIC_INDEX);
	*IOAPICIndex = Index;
	volatile UINT32 *ValueL = (volatile UINT32 *)PHYS2VIRT(IOAPIC_DATA);
	*ValueL = Value;
	*IOAPICIndex = Index + 1;
	volatile UINT32 *ValueH = (volatile UINT32 *)PHYS2VIRT(IOAPIC_DATA);
	*ValueH = (Value >> 32);
}

RETURN_STATUS IoApicChipInit()
{
	/* Enable Apic */
	LocalApicRegWrite(APIC_REG_SVR, LocalApicRegRead(APIC_REG_SVR) | BIT8);
	
	/* Disable 8259A */
	LocalApicRegWrite(APIC_REG_LVT_LINT0, LocalApicRegRead(APIC_REG_LVT_LINT0) | BIT16);
	return RETURN_SUCCESS;
}

RETURN_STATUS IoApicDisableIrq(UINT64 Irq)
{
	UINT64 Value = IoApicRegRead(REDIRECTION_TABLE(Irq));
	Value |= APIC_REG_MASKED;
	IoApicRegWrite(REDIRECTION_TABLE(Irq), Value);

	return RETURN_SUCCESS;
}

RETURN_STATUS IoApicEnableIrq(UINT64 Irq)
{
	UINT64 Value = IoApicRegRead(REDIRECTION_TABLE(Irq));
	Value &= (~APIC_REG_MASKED);
	IoApicRegWrite(REDIRECTION_TABLE(Irq), Value);

	return RETURN_SUCCESS;
}

RETURN_STATUS IoApicIrqRemapping(UINT64 Irq, UINT64 Vector, UINT64 Cpu)
{
	IoApicRegWrite(REDIRECTION_TABLE(Irq), (Cpu << 56) | Vector | APIC_REG_MASKED);

	return RETURN_SUCCESS;
}

VOID IoApicInit()
{
	PicChip.ChipInit = IoApicChipInit;
	PicChip.IrqDisable = IoApicDisableIrq;
	PicChip.IrqEnable = IoApicEnableIrq;
	PicChip.IrqRemapping = IoApicIrqRemapping;

	PicChip.ChipInit();
}

POSTCORE_INIT(IoApicInit);