#include "Acpi.h"

//************************************************************//
//	void GetAcpi()											  //
//	Get Acpi structure from Legacy BIOS Area 0xE0000-0xFFFFF //
//************************************************************//

extern UINT64 StartBusNum;
extern UINT64 EndBusNum;
extern UINT64 PCIMMIOBase;

extern SYSTEM_PARAMETERS * SysParam;


VOID *AcpiGetDescriptor(CHAR8 *Name)
{
	UINT64 AcpiAddr;
	UINT64 TableEntrySize;
	UINT64 Index;

	/*
	for(AcpiAddr = PHYS2VIRT(0xE0000); AcpiAddr<PHYS2VIRT(0xFFFFF); AcpiAddr++)
	{
		if(!memcmp((void *)AcpiAddr,"RSD PTR ",8))
			break;
	}
	if(PHYS2VIRT(0xFFFFF) == AcpiAddr)
	{
		printk("Acpi Error:No Acpi Signature found.\n");
		return;
	}
	KDEBUG("Acpi:%016x\n",AcpiAddr);
	*/
	
	AcpiAddr = PHYS2VIRT(SysParam->AcpiRsdp);
	//KDEBUG("Acpi: RSDP = %016x\n",AcpiAddr);
	
	if(memcmp((void *)AcpiAddr, "RSD PTR ", 8))
	{
		printk("Acpi Error:No Acpi Signature found.\n");
		return NULL;
	}
	
	ACPI_RSDP *RsdPtr = (ACPI_RSDP *)AcpiAddr;

	//if (RsdPtr->Revision == 0)
	//	printk("ACPI Version:v1.0.\n");
	
	//KDEBUG("RSDT Length:%d\n", RsdPtr->Length);
	
	//KDEBUG("RSDT Addr:%x\n", RsdPtr->RsdtAddress);
	
	ACPI_SDT_HEADER *HeaderPtr = NULL,*ReturnPtr = NULL;
	
	if ((RsdPtr->Revision == 2))
	{
		//KDEBUG("XSDT Addr:%x\n", RsdPtr->XsdtAddress);
		XSDT * XsdtPtr = (XSDT *)PHYS2VIRT(RsdPtr->XsdtAddress);
		
		//KDEBUG("XSDT:Length = %d\n",XsdtPtr->Header.Length);
		
		for (Index = 0; Index < (XsdtPtr->Header.Length - sizeof(ACPI_SDT_HEADER)) / 8; Index++)
		{
			//KDEBUG("XSDT:Entry%d = %x\n", Index, XsdtPtr->Entry[Index]);
			HeaderPtr = (ACPI_SDT_HEADER *)PHYS2VIRT(XsdtPtr->Entry[Index]);
			//KDEBUG("%c%c%c%c\n", HeaderPtr->Signature[0], HeaderPtr->Signature[1], HeaderPtr->Signature[2], HeaderPtr->Signature[3]);
			
			if(!memcmp(HeaderPtr->Signature, Name, 4))
			{
				ReturnPtr = HeaderPtr;
				break;
			}
		}
		return ReturnPtr;
	}
	else
	{
		//KDEBUG("Invalid XSDT Address.System will use RSDT instead.\n");
		RSDT * RsdtPtr = (RSDT *)PHYS2VIRT(RsdPtr->RsdtAddress);
		
		//KDEBUG("RSDT:Length = %d\n",RsdtPtr->Header.Length);
		
		for (Index = 0; Index < (RsdtPtr->Header.Length - sizeof(ACPI_SDT_HEADER)) / 4; Index++)
		{
			//KDEBUG("RSDT:Entry%d = %x\n",i,RsdtPtr->Entry[i]);
			HeaderPtr = (ACPI_SDT_HEADER *)PHYS2VIRT(RsdtPtr->Entry[Index]);
			//KDEBUG("%c%c%c%c\n",HeaderPtr->Signature[0],HeaderPtr->Signature[1],HeaderPtr->Signature[2],HeaderPtr->Signature[3]);
			if(!memcmp(HeaderPtr->Signature, Name, 4))
			{
				ReturnPtr = HeaderPtr;
				break;
			}
		}
		return ReturnPtr;
	}
}


	
void InitPciByAcpi()
{
	
	ACPI_MCFG	*pMCFG;
	ACPI_MADT	*pMADT;
	UINT8   *ptr;
	UINT16  *ptr1;
	
	ACPI_DMAR *pDMAR = (ACPI_DMAR *)AcpiGetDescriptor("DMAR");
	KDEBUG("DMAR:Host Address Width = %d\n", pDMAR->HostAddressWidth);
	KDEBUG("DMAR:%s\n", pDMAR->Flags ? "Supported":"Unsupported");
	ptr1 = (UINT16 *)((UINTN)pDMAR + sizeof(*pDMAR));
	DRHD_STRUCTURE *pDRHD;
	RESERVED_MEMORY_REGION_STRUCTURE *pRMRR;
	ROOT_PORT_ATS_STRUCTURE *pATSR;
	while (ptr1[1] != 0 && (UINTN)ptr1 < (UINTN)pDMAR + pDMAR->Header.Length) {
		switch (ptr1[0])
		{
			case DRHD:
				pDRHD = (DRHD_STRUCTURE *)ptr1;
				KDEBUG("DMAR:SegmentNumber = %d, Flags = %x, Regisger Base Address = %x.\n", pDRHD->SegmentNumber, pDRHD->Flags, pDRHD->RegisterBaseAddress);
				break;
			case RMRR:
				pRMRR = (RESERVED_MEMORY_REGION_STRUCTURE *)ptr1;
				KDEBUG("DMAR:SegmentNumber = %d, Reserved Region:%x - %.\n", pRMRR->SegmentNumber, pRMRR->ReservedMemoryRegionBaseAddress, pRMRR->ReservedMemoryRegionLimitAdddress);
				break;
			case ATSR:
				pATSR = (ROOT_PORT_ATS_STRUCTURE *)ptr1;
				break;
		}
		ptr1 += ptr1[1];
	}


	
			

	PROCESSOR_LOCAL_APIC_STRUCTURE *LapicPtr;
	IO_APIC_STRUCTURE *IoApicPtr;

			
	pMADT = (ACPI_MADT *)AcpiGetDescriptor("APIC");
	KDEBUG("MADT:LocalApicAddress = %x\n", pMADT->LocalApicAddress);
	ptr = (UINT8 *)((UINTN)pMADT + sizeof(*pMADT));
	while (ptr[1] != 0 && (UINTN)ptr < (UINTN)pMADT + pMADT->Header.Length) {
		switch(ptr[0])
		{
			case PROCESSOR_LOCAL_APCI:
				LapicPtr = (PROCESSOR_LOCAL_APIC_STRUCTURE *)ptr;
				KDEBUG("Local APIC ID = %02x,%s\n", LapicPtr->ApciId, LapicPtr->Flags & 0x1 ? "Enabled" : "Disabled");
				break;
			case IO_APIC:
				IoApicPtr = (IO_APIC_STRUCTURE *)ptr;
				KDEBUG("IO APIC ID = %02x,Address = %x\n", IoApicPtr->IoApciId, IoApicPtr->IoApicAddress);
				break;
			case INTERRUPT_SOURCE_OVERRIDE:
				break;
			case NMI_INTERRUPT_SOURCE:
				break;
			case LOCAL_APIC_NMI:
				break;
			case LOCAL_APIC_ADDRESS_OVERRIDE:
				break;
			case IO_SAPIC:
				break;
			case LOCAL_SAPIC:
				break;
			case PLATFORM_INTERRUPT_SOURCES:
				break;
			case PROCESSOR_LOCAL_X2APIC:
				break;
			case LOCAL_X2APIC_NMI:
				break;
			case GIC:
				break;
			case GICD:
				break;
		}
		ptr += ptr[1];
	}
			
	//while(1);
}

