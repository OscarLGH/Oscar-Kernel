#include "X64Pci.h"


#define PCI_MMIO_ADDR(b,d,f,o) \
	(PHYS2VIRT(HostBridge->MmioBaseAddress) + (((b) << 20) | ((d) << 15) | ((f) << 12) | o))

RETURN_STATUS X64MmioPciCfgRead(PCI_HOST_BRIDGE *HostBridge, PCI_DEVICE *PciDev, UINT64 Offset, UINT64 Length, VOID *Data)
{
	switch(Length)
	{
		case 1:
			*(UINT8 *)Data = *(UINT8 *)
				PCI_MMIO_ADDR(
					PciDev->PciAddress.Bus,
					PciDev->PciAddress.Device,
					PciDev->PciAddress.Function,
					Offset);
			break;
		case 2:
			*(UINT16 *)Data = *(UINT16 *)
				PCI_MMIO_ADDR(
					PciDev->PciAddress.Bus,
					PciDev->PciAddress.Device,
					PciDev->PciAddress.Function,
					Offset);
			break;
		case 4:
			*(UINT32 *)Data = *(UINT32 *)
				PCI_MMIO_ADDR(
					PciDev->PciAddress.Bus,
					PciDev->PciAddress.Device,
					PciDev->PciAddress.Function,
					Offset);
			break;
	}
	return RETURN_SUCCESS;
}

RETURN_STATUS X64MmioPciCfgWrite(PCI_HOST_BRIDGE *HostBridge, PCI_DEVICE *PciDev, UINT64 Offset, UINT64 Length, VOID *Data)
{
	switch(Length)
	{
		case 1:
			*(UINT8 *)
				PCI_MMIO_ADDR(
					PciDev->PciAddress.Bus,
					PciDev->PciAddress.Device,
					PciDev->PciAddress.Function,
					Offset) = *(UINT8 *)Data;
			break;
		case 2:
			*(UINT16 *)
				PCI_MMIO_ADDR(
					PciDev->PciAddress.Bus,
					PciDev->PciAddress.Device,
					PciDev->PciAddress.Function,
					Offset) = *(UINT16 *)Data;
			break;
		case 4:
			*(UINT32 *)
				PCI_MMIO_ADDR(
					PciDev->PciAddress.Bus,
					PciDev->PciAddress.Device,
					PciDev->PciAddress.Function,
					Offset) = *(UINT32 *)Data;
			break;
	}
	return RETURN_SUCCESS;
}


#define PCI_PIO_ADDR(b,d,f,o) \
	(0x80000000 | ((b) << 16) | ((d) << 11) | ((f) << 8) | ((o) & ~(0x3)))


RETURN_STATUS X64PioPciCfgRead(PCI_HOST_BRIDGE *HostBridge, PCI_DEVICE *PciDev, UINT64 Offset, UINT64 Length, VOID *Data)
{
	UINT32 PciAddr;
	switch(Length)
	{
		case 1:
			PciAddr = PCI_PIO_ADDR(PciDev->PciAddress.Bus,
				PciDev->PciAddress.Device,
				PciDev->PciAddress.Function,
				Offset);
			Out32(0xcf8, PciAddr);
			*(UINT8 *)Data = In8(0xcfc + (Offset & 0x3));
			
			break;
		case 2:
			PciAddr = PCI_PIO_ADDR(PciDev->PciAddress.Bus,
				PciDev->PciAddress.Device,
				PciDev->PciAddress.Function,
				Offset);
			Out32(0xcf8, PciAddr);
			*(UINT16 *)Data = In16(0xcfc + (Offset & 0x3));
			break;
		case 4:
			PciAddr = PCI_PIO_ADDR(PciDev->PciAddress.Bus,
				PciDev->PciAddress.Device,
				PciDev->PciAddress.Function,
				Offset);
			Out32(0xcf8, PciAddr);
			*(UINT32 *)Data = In32(0xcfc + (Offset & 0x3));
			break;
	}
	return RETURN_SUCCESS;
}

RETURN_STATUS X64PioPciCfgWrite(PCI_HOST_BRIDGE *HostBridge, PCI_DEVICE *PciDev, UINT64 Offset, UINT64 Length, VOID *Data)
{
	switch(Length)
	{
		UINT32 PciAddr;
		case 1:
			PciAddr = PCI_PIO_ADDR(PciDev->PciAddress.Bus,
				PciDev->PciAddress.Device,
				PciDev->PciAddress.Function,
				Offset);
			Out32(0xcf8, PciAddr);
			Out8(0xcfc + (Offset & 0x3), *(UINT8 *)Data);
			
			break;
		case 2:
			PciAddr = PCI_PIO_ADDR(PciDev->PciAddress.Bus,
				PciDev->PciAddress.Device,
				PciDev->PciAddress.Function,
				Offset);
			Out32(0xcf8, PciAddr);
			Out16(0xcfc + (Offset & 0x3), *(UINT16 *)Data);
			break;
		case 4:
			PciAddr = PCI_PIO_ADDR(PciDev->PciAddress.Bus,
				PciDev->PciAddress.Device,
				PciDev->PciAddress.Function,
				Offset);
			Out32(0xcf8, PciAddr);
			Out32(0xcfc + (Offset & 0x3), *(UINT32 *)Data);
			break;
	}
	return RETURN_SUCCESS;
}

void X64PciBusInit()
{
	ACPI_MCFG *McfgPtr = (ACPI_MCFG *)AcpiGetDescriptor("MCFG");
	if (McfgPtr == NULL) 
	{
		KDEBUG("PCI:No 'MCFG' Structure found in ACPI.\n");
		KDEBUG("PCI:Using IO Port CF8,CFC.\n");
		PciHostBridge.SegmentNumber = 0;
		PciHostBridge.StartBusNum = 0;
		PciHostBridge.EndBusNum = 255;
		PciHostBridge.PciCfgRead = X64PioPciCfgRead;
		PciHostBridge.PciCfgWrite = X64PioPciCfgWrite;
	}
	else
	{
		KDEBUG("PCI:ACPI MCFG:BaseAddress = %x\n", McfgPtr->ConfigureArray[0].BaseAddress);
		
		PciHostBridge.SegmentNumber = McfgPtr->ConfigureArray[0].PCISegmentGroupNumber;
		PciHostBridge.StartBusNum = McfgPtr->ConfigureArray[0].StartBusNumber;
		PciHostBridge.EndBusNum = McfgPtr->ConfigureArray[0].EndBusNumber;
		PciHostBridge.MmioBaseAddress = McfgPtr->ConfigureArray[0].BaseAddress;
		PciHostBridge.PciCfgRead = X64MmioPciCfgRead;
		PciHostBridge.PciCfgWrite = X64MmioPciCfgWrite;
	}
	PciScan();
}

ARCH_INIT(X64PciBusInit);


