#include "LongMode.h"

void SetGateDescriptor(
		GATE_DESCRIPTOR_64 *IDT,
		UINT8 Vector,
		UINT16 Selector,
		VOID * Offset,
		UINT8 IST,
		UINT8 Attr
		)
{
	IDT[Vector].Selector = Selector;
	IDT[Vector].Offset0_15 = (UINT64)Offset&0xFFFF;
	IDT[Vector].Offset16_31 = ((UINT64)Offset>>16)&0xFFFF;
	IDT[Vector].Offset32_63 = ((UINT64)Offset>>32)&0xFFFFFFFF;
	IDT[Vector].IST = IST;
	IDT[Vector].Attr = Attr;
	IDT[Vector].Reserved = 0;
}

void SetSegmentDescriptor(
		SEGMENT_DESCRIPTOR *GDT,
		UINT16 Index,
		VOID * Base,
		UINT32 Limit,
		UINT8 Attr1,
		UINT8 Attr2
		)
{
	GDT[Index].Base0_15 = (UINT64)Base&0xFFFF;
	GDT[Index].Base16_23 = ((UINT64)Base>>16)&0xFF;
	GDT[Index].Base24_31 = ((UINT64)Base>>24)&0xFF;
	GDT[Index].Base32_63 = ((UINT64)Base>>32)&0xFFFFFFFF;
	GDT[Index].Limit0_15 = Limit&0xFFFF;
	GDT[Index].Limit16_19_Attr2 = ((Limit>>16)&0xF)|(Attr2&0xF0);
	GDT[Index].Attr1 = Attr1;
	GDT[Index].Reserved = 0;
}
		
	