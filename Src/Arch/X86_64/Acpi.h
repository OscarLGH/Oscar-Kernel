//*********************************************
//ACPI.h
//Oscar 2015.9
//*********************************************

#ifndef _ACPI_H
#define _ACPI_H

#include "Type.h"
#include "SystemParameters.h"
#include "BaseLib.h"
//#define DEBUG
#include "Debug.h"

//Root System Description Pointer
#pragma pack(1)
typedef struct
{
	UINT8	Signature[8];		//"RSD PTR "
	UINT8	Checksum;			//前20个字节校验和为0
	UINT8	OEMID[6];
	UINT8	Revision;
	UINT32	RsdtAddress;
	UINT32	Length;
	UINT64	XsdtAddress;		
	UINT8	ExtChecksum;		//整个table的校验和
	UINT8	Reserved[3];
}ACPI_RSDP;

//System Description Table Header
typedef struct
{
	UINT8	Signature[4];
	UINT32	Length;
	UINT8	Revision;
	UINT8	Checksum;
	UINT8	OEMID[6];
	UINT64	OEMTableID;
	UINT32	OEMRevision;
	UINT32	CreatorID;
	UINT32	CreatorRevision;
}ACPI_SDT_HEADER;

typedef struct
{
	ACPI_SDT_HEADER Header;
	UINT32 Entry[32];
}RSDT;

typedef struct
{
	ACPI_SDT_HEADER Header;
	UINT64 Entry[32];
}XSDT;

//APIC

enum APCI_STRUCTURE_TYPES
{
	PROCESSOR_LOCAL_APCI = 0,
	IO_APIC,
	INTERRUPT_SOURCE_OVERRIDE,
	NMI_INTERRUPT_SOURCE,
	LOCAL_APIC_NMI,
	LOCAL_APIC_ADDRESS_OVERRIDE,
	IO_SAPIC,
	LOCAL_SAPIC,
	PLATFORM_INTERRUPT_SOURCES,
	PROCESSOR_LOCAL_X2APIC,
	LOCAL_X2APIC_NMI,
	GIC,
	GICD,
	RESERVED,
};

typedef struct
{
	UINT8 Type;
	UINT8 Length;
	UINT8 AcpiProcessorId;
	UINT8 ApciId;
	UINT32 Flags;
}PROCESSOR_LOCAL_APIC_STRUCTURE;

typedef struct
{
	UINT8 Type;
	UINT8 Length;
	UINT8 IoApciId;
	UINT8 Reserved;
	UINT32 IoApicAddress;
	UINT32 GlobalSystemInterruptBase;
}IO_APIC_STRUCTURE;

typedef struct
{
	ACPI_SDT_HEADER Header;
	UINT32 LocalApicAddress;
	UINT32 Flags;
}ACPI_MADT;

//MCFG 

typedef struct
{
	UINT64	BaseAddress;
	UINT16	PCISegmentGroupNumber;
	UINT8	StartBusNumber;
	UINT8	EndBusNumber;
	UINT32	Reserved;
}MMCS_BAR;

typedef struct
{
	ACPI_SDT_HEADER	Header;
	UINT64		Reserved;
	MMCS_BAR		ConfigureArray[32];
}ACPI_MCFG;

//DMAR
enum REMAPPING_STRUCTURE_TYPES
{
	DRHD = 0,//DMA Remapping Hardware Unit Definition Reporting
	RMRR,	 //Reserved Memory Region Reporting Reporting
	ATSR,	 //Root Port ATS Capability Reporting
};

typedef struct
{
	UINT8 Type;
	UINT8 Length;
	UINT16 Reserved;
	UINT8 EnumeratioinId;
	UINT8 StartBusNumber;
	//PATH
}DEVICE_SCOPE_STRUCTURE;

typedef struct
{
	UINT16 Type;
	UINT16 Length;
	UINT16 Reserved;
	UINT16 SegmentNumber;
	UINT64 ReservedMemoryRegionBaseAddress;
	UINT64 ReservedMemoryRegionLimitAdddress;
	//DEVICE_SCOPE_STRUCTURE
}RESERVED_MEMORY_REGION_STRUCTURE;

typedef struct
{
	UINT16 Type;
	UINT16 Length;
	UINT8 Flags;
	UINT8 Reserved;
	UINT16 SegmentNumber;
	//DEVICE_SCOPE_STRUCTURE
}ROOT_PORT_ATS_STRUCTURE;

typedef struct
{
	UINT16 Type;
	UINT16 Length;
	UINT8  Flags;
	UINT8  Reserved;
	UINT16 SegmentNumber;
	UINT64 RegisterBaseAddress;
	//DEVICE_SCOPE_STRUCTURE;
}DRHD_STRUCTURE;

typedef struct
{
	ACPI_SDT_HEADER	Header;
	UINT8 HostAddressWidth;
	UINT8 Flags;
	UINT8 Reserved[10];
	//REMAPPING_STRUCTURES
}ACPI_DMAR;

//HPET

typedef struct
{
	ACPI_SDT_HEADER	Header;
	UINT32 EventTimerBlockId;
	UINT8 AddressSpaceId;
	UINT8 RegisterBitWidth;
	UINT8 RegisterBitOffset;
	UINT8 Reserved;
	UINT64 BaseAddress;
	UINT8 HpetNumber;
	UINT16 MainCounterMinClock;
	UINT8 Attribute;
}ACPI_HPET;

#pragma pack()


VOID *AcpiGetDescriptor(CHAR8 *Name);

#endif