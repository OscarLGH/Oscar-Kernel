#include "AHCI.h"


RETURN_STATUS AHCISupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if (Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if (PCI_CLASS_MASS_STORAGE != PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;

	if (PCI_CLASS_MASS_STORAGE_SATADPA != PciCfgRead8(PciDev, PCI_CONF_SUBCLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;

	KDEBUG("AHCI driver supported on device:%02x:%02x.%02x\n",
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	return RETURN_SUCCESS;

}

RETURN_STATUS AHCIStart(DEVICE_NODE * Dev)
{
	KDEBUG("AHCI driver starting...\n");
	AHCIControllerInit(Dev);
	return RETURN_SUCCESS;
}

RETURN_STATUS AHCIStop(DEVICE_NODE * Dev)
{
	return RETURN_SUCCESS;
}

UINT32 AHCIRegRead(DEVICE_NODE * Dev, UINT32 Offset)
{
	PCI_DEVICE * PciDev = Dev->Device;
	UINT64 ABAR = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR5_OFF_32) & (~0xF));
	//KDEBUG("ABAR = %016x\n",ABAR);
	volatile UINT32 * Reg = (UINT32 *)(ABAR + Offset);
	//KDEBUG("AHCI Reg:Offset %08x,Value:%08x\n",Offset, *Reg);
	return *Reg;
}

void AHCIRegWrite(DEVICE_NODE * Dev, UINT32 Offset, UINT32 Value)
{
	PCI_DEVICE * PciDev = Dev->Device;
	UINT64 ABAR = PHYS2VIRT(PciCfgRead32(PciDev, PCI_CONF_BAR5_OFF_32) & (~0xF));
	//KDEBUG("ABAR = %016x\n",ABAR);
	volatile UINT32 * Reg = (UINT32 *)(ABAR + Offset);
	*Reg = Value;
	//KDEBUG("AHCI Reg:Offset %08x,Value:%08x\n",Offset, *Reg);
}


VOID
AHCIRegAnd(
	IN DEVICE_NODE 		  * Dev,
	IN UINT32               Offset,
	IN UINT32               AndData
	)
{
	UINT32 Data;

	Data = AHCIRegRead(Dev, Offset);

	Data &= AndData;

	AHCIRegWrite(Dev, Offset, Data);
}

/**
  Do OR operation with the value of AHCI Operation register.

  @param  Dev        The PCI IO protocol instance.
  @param  Offset       The operation register offset.
  @param  OrData       The data used to do OR operation.

**/
VOID
AHCIRegOr(
	IN DEVICE_NODE 		  * Dev,
	IN UINT32               Offset,
	IN UINT32               OrData
	)
{
	UINT32 Data;

	Data = AHCIRegRead(Dev, Offset);

	Data |= OrData;

	AHCIRegWrite(Dev, Offset, Data);
}

/**
  Wait for the value of the specified MMIO register set to the test value.

  @param  Offset            The MMIO address to test.
  @param  MaskValue         The mask value of memory.
  @param  TestValue         The test value of memory.
  @param  Timeout           The time out value for wait memory set, uses 100ns as a unit.

  @retval TIMEOUT       The MMIO setting is time out.
  @retval SUCCESS       The MMIO is correct set.

**/
RETURN_STATUS
AHCIWaitMmioSet(
	IN  DEVICE_NODE 		  		* Dev,
	IN  UINT32                    Offset,
	IN  UINT32                    MaskValue,
	IN  UINT32                    TestValue,
	IN  UINT64                    Timeout
	)
{
	UINT32     Value;
	UINT64     delay;
	UINT8    InfiniteWait;

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	delay = Timeout / 1000 + 1;

	do {
		//
		// Access PCI MMIO space to see if the value is the tested one.
		//
		Value = AHCIRegRead(Dev, (UINT32)Offset) & MaskValue;

		if (Value == TestValue) {
			return RETURN_SUCCESS;
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		delay--;

	} while (InfiniteWait || (delay > 0));

	return RETURN_TIMEOUT;
}

/**
  Wait for the value of the specified system memory set to the test value.

  @param  Address           The system memory address to test.
  @param  MaskValue         The mask value of memory.
  @param  TestValue         The test value of memory.
  @param  Timeout           The time out value for wait memory set, uses 100ns as a unit.

  @retval TIMEOUT       The system memory setting is time out.
  @retval SUCCESS       The system memory is correct set.

**/
RETURN_STATUS
AHCIWaitMemSet(
	IN  void*				        Address,
	IN  UINT32                    MaskValue,
	IN  UINT32                    TestValue,
	IN  UINT64                    Timeout
	)
{
	UINT32     Value;
	UINT64     delay;
	BOOLEAN    InfiniteWait;

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	delay = Timeout / 1000 + 1;

	do {
		//
		// Access sytem memory to see if the value is the tested one.
		//
		// The system memory pointed by Address will be updated by the
		// SATA Host Controller, "volatile" is introduced to prevent
		// compiler from optimizing the access to the memory address
		// to only read once.
		//
		Value = *(volatile UINT32 *)Address;
		Value &= MaskValue;

		if (Value == TestValue) {
			return RETURN_SUCCESS;
		}

		//
		// Stall for 100 microseconds.
		//

		MicroSecondDelay(100);

		delay--;

	} while (InfiniteWait || (delay > 0));

	return RETURN_TIMEOUT;
}

/**
  Check the memory status to the test value.

  @param[in]       Address           The memory address to test.
  @param[in]       MaskValue         The mask value of memory.
  @param[in]       TestValue         The test value of memory.
  @param[in, out]  Task              Optional. Pointer to the ATA_NONBLOCK_TASK used by
									 non-blocking mode. If NULL, then just try once.

  @retval NOTREADY      The memory is not set.
  @retval TIMEOUT       The memory setting retry times out.
  @retval SUCCESS       The memory is correct set.

**/
RETURN_STATUS
AHCICheckMemSet(
	IN     void                      *Address,
	IN     UINT32                    MaskValue,
	IN     UINT32                    TestValue
	)
{
	UINT32     Value;

	Value = *(volatile UINT32 *)Address;
	Value &= MaskValue;

	if (Value == TestValue) {
		return RETURN_SUCCESS;
	}

}

RETURN_STATUS AHCICheckDeviceStatus(
	IN  DEVICE_NODE * Dev,
	IN  UINT8	Port
	)
{
	UINT32      Data;
	UINT32      Offset;

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SSTS;

	Data = AHCIRegRead(Dev, Offset) & AHCI_PORT_SSTS_DET_MASK;

	if (Data == AHCI_PORT_SSTS_DET_PCE) {
		return RETURN_SUCCESS;
	}

	return RETURN_NOT_READY;
}


/**

  Clear the port interrupt and error status. It will also clear
  HBA interrupt status.

  @param      Dev          The PCI IO protocol instance.
  @param      Port           The number of port.

**/
VOID
AHCIClearPortStatus(
	IN  DEVICE_NODE 			*Dev,
	IN  UINT8                  Port
	)
{
	UINT32 Offset;

	//
	// Clear any error status
	//
	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SERR;
	AHCIRegWrite(Dev, Offset, AHCIRegRead(Dev, Offset));

	//
	// Clear any port interrupt status
	//
	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_IS;
	AHCIRegWrite(Dev, Offset, AHCIRegRead(Dev, Offset));

	//
	// Clear any HBA interrupt status
	//
	AHCIRegWrite(Dev, AHCI_IS_OFFSET, AHCIRegRead(Dev, AHCI_IS_OFFSET));
}


RETURN_STATUS
AHCIEnableFisReceive(
	IN  DEVICE_NODE				*Dev,
	IN  UINT8                     Port,
	IN  UINT64                    Timeout
	)
{
	UINT32 Offset;

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
	AHCIRegOr(Dev, Offset, AHCI_PORT_CMD_FRE);

	return AHCIWaitMmioSet(
		Dev,
		Offset,
		AHCI_PORT_CMD_FR,
		AHCI_PORT_CMD_FR,
		Timeout
		);
}

/**
  Disable the FIS running for giving port.

  @param      Dev          The PCI IO protocol instance.
  @param      Port           The number of port.
  @param      Timeout        The timeout value of disabling FIS, uses 100ns as a unit.

  @retval DEVICE_ERROR   The FIS disable setting fails.
  @retval TIMEOUT        The FIS disable setting is time out.
  @retval UNSUPPORTED    The port is in running state.
  @retval SUCCESS        The FIS disable successfully.

**/
RETURN_STATUS
AHCIDisableFisReceive(
	IN  DEVICE_NODE				*Dev,
	IN  UINT8                     Port,
	IN  UINT64                    Timeout
	)
{
	UINT32 Offset;
	UINT32 Data;

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
	Data = AHCIRegRead(Dev, Offset);

	//
	// Before disabling Fis receive, the DMA engine of the port should NOT be in running status.
	//
	if ((Data & (AHCI_PORT_CMD_ST | AHCI_PORT_CMD_CR)) != 0) {
		return RETURN_UNSUPPORTED;
	}

	//
	// Check if the Fis receive DMA engine for the port is running.
	//
	if ((Data & AHCI_PORT_CMD_FR) != AHCI_PORT_CMD_FR) {
		return RETURN_SUCCESS;
	}

	AHCIRegAnd(Dev, Offset, (UINT32)~(AHCI_PORT_CMD_FRE));

	return AHCIWaitMmioSet(
		Dev,
		Offset,
		AHCI_PORT_CMD_FR,
		0,
		Timeout
		);
}



/**
  Buid a command FIS.

  @param  CmdFis            A pointer to the AHCI_COMMAND_FIS data structure.
  @param  AtaCommandBlock   A pointer to the AHCIBuildCommandFis data structure.

**/
VOID
AHCIBuildCommandFis(
	IN OUT AHCI_COMMAND_FIS    *CmdFis,
	IN     ATA_COMMAND_BLOCK   *AtaCommandBlock
	)
{
	memset(CmdFis, 0, sizeof(AHCI_COMMAND_FIS));

	CmdFis->AHCICFisType = AHCI_FIS_REGISTER_H2D;
	//
	// Indicator it's a command
	//
	CmdFis->AHCICFisCmdInd = 0x1;
	CmdFis->AHCICFisCmd = AtaCommandBlock->AtaCommand;

	CmdFis->AHCICFisFeature = AtaCommandBlock->AtaFeatures;
	CmdFis->AHCICFisFeatureExp = AtaCommandBlock->AtaFeaturesExp;

	CmdFis->AHCICFisSecNum = AtaCommandBlock->AtaSectorNumber;
	CmdFis->AHCICFisSecNumExp = AtaCommandBlock->AtaSectorNumberExp;

	CmdFis->AHCICFisClyLow = AtaCommandBlock->AtaCylinderLow;
	CmdFis->AHCICFisClyLowExp = AtaCommandBlock->AtaCylinderLowExp;

	CmdFis->AHCICFisClyHigh = AtaCommandBlock->AtaCylinderHigh;
	CmdFis->AHCICFisClyHighExp = AtaCommandBlock->AtaCylinderHighExp;

	CmdFis->AHCICFisSecCount = AtaCommandBlock->AtaSectorCount;
	CmdFis->AHCICFisSecCountExp = AtaCommandBlock->AtaSectorCountExp;

	CmdFis->AHCICFisDevHead = (UINT8)(AtaCommandBlock->AtaDeviceHead | 0xE0);
}


/**
  Build the command list, command table and prepare the fis receiver.

  @param    Dev                 The PCI IO protocol instance.
  @param    AHCIRegisters         The pointer to the AHCI_REGISTERS.
  @param    Port                  The number of port.
  @param    PortMultiplier        The timeout value of stop.
  @param    CommandFis            The control fis will be used for the transfer.
  @param    CommandList           The command list will be used for the transfer.
  @param    AtapiCommand          The atapi command will be used for the transfer.
  @param    AtapiCommandLength    The length of the atapi command.
  @param    CommandSlotNumber     The command slot will be used for the transfer.
  @param    DataPhysicalAddr      The pointer to the data buffer pci bus master address.
  @param    DataLength            The data count to be transferred.

**/
AHCI_COMMAND_LIST *
AHCIBuildCommandTable(
	IN  	 DEVICE_NODE					*Dev,
	IN     UINT8                      Port,
	IN     UINT8                      PortMultiplier,
	IN     AHCI_COMMAND_FIS       	*CommandFis,
	IN	 AHCI_COMMAND_LIST 			*CommandList,
	IN     AHCI_ATAPI_COMMAND     	*AtapiCommand OPTIONAL,
	IN     UINT8                      AtapiCommandLength,
	IN	 AHCI_COMMAND_TABLE 		*CommandTable,
	IN OUT VOID                       *DataPhysicalAddr,
	IN     UINT32                     DataLength
	)
{
	UINT32     PrdtNumber;
	UINT32     PrdtIndex;
	UINT64     RemainedData;
	UINT64     MemAddr;
	UINT32     Offset;

	//
	// Filling the PRDT
	//
	PrdtNumber = (UINT32)(((UINT64)DataLength + AHCI_MAX_DATA_PER_PRDT - 1) / AHCI_MAX_DATA_PER_PRDT);
	//
	// According to AHCI 1.3 spec, a PRDT entry can point to a maximum 4MB data block.
	// It also limits that the maximum amount of the PRDT entry in the command table
	// is 65535.
	//
	//KDEBUG("PrdtNumber = %d\n", PrdtNumber);

	memcpy(&CommandTable->CommandFis, CommandFis, sizeof(AHCI_COMMAND_FIS));

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;

	if (AtapiCommand != NULL) {
		memcpy(
			(void *)&CommandTable->AtapiCmd,
			AtapiCommand,
			AtapiCommandLength
			);

		CommandList->AHCICmdA = 1;
		CommandList->AHCICmdP = 1;

		AHCIRegOr(Dev, Offset, (AHCI_PORT_CMD_DLAE | AHCI_PORT_CMD_ATAPI));
	}
	else {
		AHCIRegAnd(Dev, Offset, (UINT32)~(AHCI_PORT_CMD_DLAE | AHCI_PORT_CMD_ATAPI));
	}

	RemainedData = (UINT64)DataLength;
	MemAddr = (UINT64)DataPhysicalAddr;
	CommandList->AHCICmdPrdtl = PrdtNumber;

	for (PrdtIndex = 0; PrdtIndex < PrdtNumber; PrdtIndex++) {
		if (RemainedData < AHCI_MAX_DATA_PER_PRDT) {
			CommandTable->PrdtTable[PrdtIndex].AHCIPrdtDbc = (UINT32)RemainedData - 1;
		}
		else {
			CommandTable->PrdtTable[PrdtIndex].AHCIPrdtDbc = AHCI_MAX_DATA_PER_PRDT - 1;
		}

		CommandTable->PrdtTable[PrdtIndex].AHCIPrdtDba = (UINT64)MemAddr;
		CommandTable->PrdtTable[PrdtIndex].AHCIPrdtDbau = (UINT32)((UINT64)MemAddr >> 32);
		RemainedData -= AHCI_MAX_DATA_PER_PRDT;
		MemAddr += AHCI_MAX_DATA_PER_PRDT;
	}

	//
	// Set the last PRDT to Interrupt On Complete
	//
	if (PrdtNumber > 0) {
		CommandTable->PrdtTable[PrdtNumber - 1].AHCIPrdtIoc = 1;
	}

	//KDEBUG("CommandTable:%016x\n", CommandTable);

	MemAddr = VIRT2PHYS((UINT64)CommandTable);
	//KDEBUG("CommandTable:%016x\n",Data64.Uint64);
	CommandList->AHCICmdCtba = (UINT64)MemAddr;
	CommandList->AHCICmdCtbau = (UINT32)((UINT64)MemAddr >> 32);
	CommandList->AHCICmdPmp = PortMultiplier;

	return CommandList;
}

/**
  Start command for give slot on specific port.

  @param  Dev              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  CommandSlot        The number of Command Slot.
  @param  Timeout            The timeout value of start, uses 100ns as a unit.

  @retval DEVICE_ERROR   The command start unsuccessfully.
  @retval TIMEOUT        The operation is time out.
  @retval SUCCESS        The command start successfully.

**/
RETURN_STATUS
AHCIStartCommand(
	IN  DEVICE_NODE				*Dev,
	IN  UINT8                     Port,
	IN  UINT8                     CommandSlot,
	IN  UINT64                    Timeout
	)
{
	UINT32     CmdSlotBit;
	UINT64	 Status;
	UINT32     PortStatus;
	UINT32     StartCmd;
	UINT32     PortTfd;
	UINT32     Offset;
	UINT32     Capability;

	//
	// Collect AHCI controller information
	//
	Capability = AHCIRegRead(Dev, AHCI_CAPABILITY_OFFSET);

	CmdSlotBit = (UINT32)(1 << CommandSlot);

	AHCIClearPortStatus(
		Dev,
		Port
		);

	Status = AHCIEnableFisReceive(
		Dev,
		Port,
		Timeout
		);

	if (RETURN_ERROR(Status)) {
		return Status;
	}

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
	PortStatus = AHCIRegRead(Dev, Offset);

	StartCmd = 0;
	if ((PortStatus & AHCI_PORT_CMD_ALPE) != 0) {
		StartCmd = AHCIRegRead(Dev, Offset);
		StartCmd &= ~AHCI_PORT_CMD_ICC_MASK;
		StartCmd |= AHCI_PORT_CMD_ACTIVE;
	}

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
	PortTfd = AHCIRegRead(Dev, Offset);

	if ((PortTfd & (AHCI_PORT_TFD_BSY | AHCI_PORT_TFD_DRQ)) != 0) {
		if ((Capability & BIT24) != 0) {
			Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
			AHCIRegOr(Dev, Offset, AHCI_PORT_CMD_CLO);

			AHCIWaitMmioSet(
				Dev,
				Offset,
				AHCI_PORT_CMD_CLO,
				0,
				Timeout
				);
		}
	}

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
	AHCIRegOr(Dev, Offset, AHCI_PORT_CMD_ST | StartCmd);

	//
	// Setting the command
	//
	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CI;
	AHCIRegAnd(Dev, Offset, 0);
	AHCIRegOr(Dev, Offset, CmdSlotBit);

	return RETURN_SUCCESS;
}


/**
  Stop command running for giving port

  @param  Dev              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  Timeout            The timeout value of stop, uses 100ns as a unit.

  @retval DEVICE_ERROR   The command stop unsuccessfully.
  @retval TIMEOUT        The operation is time out.
  @retval SUCCESS        The command stop successfully.

**/
RETURN_STATUS
AHCIStopCommand(
	IN  DEVICE_NODE				*Dev,
	IN  UINT8                     Port,
	IN  UINT64                    Timeout
	)
{
	UINT32 Offset;
	UINT32 Data;

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
	Data = AHCIRegRead(Dev, Offset);

	if ((Data & (AHCI_PORT_CMD_ST | AHCI_PORT_CMD_CR)) == 0) {
		return RETURN_SUCCESS;
	}

	if ((Data & AHCI_PORT_CMD_ST) != 0) {
		AHCIRegAnd(Dev, Offset, (UINT32)~(AHCI_PORT_CMD_ST));
	}

	return AHCIWaitMmioSet(
		Dev,
		Offset,
		AHCI_PORT_CMD_CR,
		0,
		Timeout
		);
}


/**
  Do AHCI port reset.

  @param  Dev              The PCI IO protocol instance.
  @param  Port               The number of port.
  @param  Timeout            The timeout value of reset, uses 100ns as a unit.

  @retval DEVICE_ERROR   The port reset unsuccessfully
  @retval TIMEOUT        The reset operation is time out.
  @retval SUCCESS        The port reset successfully.

**/
RETURN_STATUS
AHCIPortReset(
	IN  DEVICE_NODE				*Dev,
	IN  UINT8                     Port,
	IN  UINT64                    Timeout
	)
{
	UINT64      	  Status;
	UINT32          Offset;

	AHCIClearPortStatus(Dev, Port);

	AHCIStopCommand(Dev, Port, Timeout);

	AHCIDisableFisReceive(Dev, Port, Timeout);

	AHCIEnableFisReceive(Dev, Port, Timeout);

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SCTL;

	AHCIRegOr(Dev, Offset, AHCI_PORT_SCTL_DET_INIT);

	//
	// wait 5 millisecond before de-assert DET
	//
	MicroSecondDelay(5000);

	AHCIRegAnd(Dev, Offset, (UINT32)AHCI_PORT_SCTL_MASK);

	//
	// wait 5 millisecond before de-assert DET
	//
	MicroSecondDelay(5000);

	//
	// Wait for communication to be re-established
	//
	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SSTS;
	Status = AHCIWaitMmioSet(
		Dev,
		Offset,
		AHCI_PORT_SSTS_DET_MASK,
		AHCI_PORT_SSTS_DET_PCE,
		Timeout
		);

	if (RETURN_ERROR(Status)) {
		return Status;
	}

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SERR;
	AHCIRegOr(Dev, Offset, AHCI_PORT_ERR_CLEAR);

	return RETURN_SUCCESS;
}

/**
  Start a DMA data transfer on specific port

  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       Read                The transfer direction.
  @param[in]       AtaCommandBlock     The ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The ATA_STATUS_BLOCK data.
  @param[in, out]  MemoryAddr          The pointer to the data Buffer.
  @param[in]       DataCount           The data count to be transferred.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
									   used by non-blocking mode.

  @retval DEVICE_ERROR    The DMA data transfer abort with error occurs.
  @retval TIMEOUT         The operation is time out.
  @retval UNSUPPORTED     The device is not ready for transfer.
  @retval SUCCESS         The DMA data transfer executes successfully.

**/
RETURN_STATUS
AHCIDmaTransfer(
	IN	 DEVICE_NODE					*Dev,
	IN     UINT8                      Port,
	IN     UINT8                      PortMultiplier,
	IN     BOOLEAN                    Read,
	IN	 BOOLEAN					NonBlocking,
	IN     ATA_COMMAND_BLOCK      	*AtaCommandBlock,
	IN OUT ATA_STATUS_BLOCK       	*AtaStatusBlock,
	IN OUT VOID                       *MemoryAddr,
	IN     UINT32                     DataCount,
	IN     UINT64                     Timeout
	)
{
	UINT64						Status;
	UINT64                      Offset;
	AHCI_COMMAND_FIS          	CFis;
	UINT64                      FisBaseAddr;
	UINT32                      PortTfd;
	UINT64						CLBufferBase;
	UINT32 						PortStatus;

	CLBufferBase = PHYS2VIRT(
		(UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB) |
		((UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU) << 32)
		);

	AHCI_COMMAND_LIST * CommandList = (AHCI_COMMAND_LIST*)KMalloc(sizeof(AHCI_COMMAND_LIST));
	AHCI_COMMAND_TABLE *CommandTable = (AHCI_COMMAND_TABLE*)KMalloc(sizeof(AHCI_COMMAND_TABLE));

	AHCIBuildCommandFis(&CFis, AtaCommandBlock);

	CFis.AHCICFisPmNum = PortMultiplier;

	memset(CommandList, 0, sizeof(AHCI_COMMAND_LIST));
	memset(CommandTable, 0, sizeof(AHCI_COMMAND_TABLE));

	//KDEBUG("MemoryAddr = %016x\n",VIRT2PHYS(MemoryAddr));
	AHCIBuildCommandTable(
		Dev,
		Port,
		PortMultiplier,
		&CFis,
		CommandList,
		NULL,
		0,
		CommandTable,
		(VOID *)VIRT2PHYS(MemoryAddr),
		DataCount
		);

	CommandList->AHCICmdCfl = AHCI_FIS_REGISTER_H2D_LENGTH / 4;
	CommandList->AHCICmdW = Read ? 0 : 1;

	memset((void *)CLBufferBase, 0, 32 * sizeof(AHCI_COMMAND_LIST));

	memcpy((void *)CLBufferBase, CommandList, sizeof(AHCI_COMMAND_LIST));

	AHCI_COMMAND_LIST * ptr = (AHCI_COMMAND_LIST *)CLBufferBase;

	//KDEBUG("CommandTable:\n");
	//DumpMem((void *)PHYS2VIRT((UINT64)ptr->AHCICmdCtba), 128);
	//Test++;
	//if(Test == 5)
	//	while(1);

	UINT32 *ReceiveFisReg = (UINT32 *)PHYS2VIRT(AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB));
	//memset(ReceiveFisReg, 0, 64);

	//DumpMem(CommandList, sizeof(AHCI_COMMAND_LIST));

	//DumpMem(CommandTable, 64);
	//while(1);

	Status = AHCIStartCommand(
		Dev,
		Port,
		0,
		Timeout
		);

	if (RETURN_ERROR(Status)) {
		goto Exit;
	}


	//PortStatus = AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD);
	
  //
  // Wait for command compelte
  //
	FisBaseAddr = PHYS2VIRT(AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB));
	Offset = FisBaseAddr + AHCI_D2H_FIS_OFFSET;

	if (NonBlocking)
	{
		Status = AHCICheckMemSet(
			(void *)Offset,
			AHCI_FIS_TYPE_MASK,
			AHCI_FIS_REGISTER_D2H
			);
	}
	else
	{
		Status = AHCIWaitMemSet(
			(void *)Offset,
			AHCI_FIS_TYPE_MASK,
			AHCI_FIS_REGISTER_D2H,
			Timeout
			);
	}
	if (RETURN_ERROR(Status)) {
		goto Exit;
	}

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;

	UINT32 delay = Timeout / 1000 + 1;
	while (1)
	{
		PortTfd = AHCIRegRead(Dev, (UINT32)Offset);
		if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
			Status = RETURN_DEVICE_ERROR;
		}

		if (NonBlocking)
			break;

		//wait BSY Flag to be cleared.
		if ((PortTfd & BIT7) == 0)
			break;

		MicroSecondDelay(100);
		delay--;

		if (delay == 0)
			goto Exit;
	}
Exit:
	//
	// For Blocking mode, the command should be stopped, the Fis should be disabled
	// and the Dev should be unmapped.
	// For non-blocking mode, only when a error is happened (if the return status is
	// NOT_READY that means the command doesn't finished, try again.), first do the
	// context cleanup, then set the packet's Asb status.
	//
	//if (Status != RETURN_NOT_READY) {
	AHCIStopCommand(
		Dev,
		Port,
		Timeout
		);

	AHCIDisableFisReceive(
		Dev,
		Port,
		Timeout
		);

	//KDEBUG("FIS Buffer:\n");
	//DumpMem(FISBufferBase, 32);

  //AHCIDumpPortStatus (Dev, Port, AtaStatusBlock);
  //KFree(CommandList);
  //KFree(CommandTable);
  //AHCIRegOr(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD, BIT3);

	PortStatus = AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD);
	//KDEBUG("Port %d Status = %08x\n", Port, PortStatus);

	PortStatus = AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_SERR);
	//KDEBUG("Port %d SERR = %08x\n", Port, PortStatus);

	KFree(CommandList);
	KFree(CommandTable);
	//while(1)
	//{
	  //PortStatus = AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD);
	  //KDEBUG("Port %d Status = %08x\n", Port, PortStatus);
	  //if(PortStatus == 0x50)
	  ///	break;
	//}
	if (Status)
		AHCIPortReset(Dev, Port, Timeout);

	return Status;

}


/**
  Start a PIO data transfer on specific port.

  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       Read                The transfer direction.
  @param[in]       AtaCommandBlock     The ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The ATA_STATUS_BLOCK data.
  @param[in, out]  MemoryAddr          The pointer to the data Buffer.
  @param[in]       DataCount           The data count to be transferred.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.

  @retval DEVICE_ERROR    The PIO data transfer abort with error occurs.
  @retval TIMEOUT         The operation is time out.
  @retval UNSUPPORTED     The device is not ready for transfer.
  @retval SUCCESS         The PIO data transfer executes successfully.

**/
RETURN_STATUS
AHCIPioTransfer(
	IN	   DEVICE_NODE				*Dev,
	IN     UINT8                    Port,
	IN     UINT8                    PortMultiplier,
	IN     AHCI_ATAPI_COMMAND     	*AtapiCommand OPTIONAL,
	IN     UINT8                    AtapiCommandLength,
	IN     BOOLEAN                  Read,
	IN     ATA_COMMAND_BLOCK      	*AtaCommandBlock,
	IN OUT ATA_STATUS_BLOCK       	*AtaStatusBlock,
	IN OUT VOID                     *MemoryAddr,
	IN     UINT32                   DataCount,
	IN     UINT64                   Timeout
	)
{
	UINT64                   	  Status;
	UINT64                        FisBaseAddr;
	UINT64                        Offset;
	UINT64          			  PhyAddr;
	UINT64                        delay;
	AHCI_COMMAND_FIS          	  CFis;
	UINT32                        PortTfd;
	UINT32                        PrdCount;
	BOOLEAN                       InfiniteWait;
	BOOLEAN                       PioFisReceived;
	BOOLEAN                       D2hFisReceived;
	UINT64						  CLBufferBase;

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	CLBufferBase = PHYS2VIRT(
		(UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB) |
		((UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU) << 32)
		);

	AHCI_COMMAND_LIST * CommandList = (AHCI_COMMAND_LIST*)KMalloc(sizeof(AHCI_COMMAND_LIST));
	AHCI_COMMAND_TABLE *CommandTable = (AHCI_COMMAND_TABLE*)KMalloc(sizeof(AHCI_COMMAND_TABLE));
	//
	// Package read needed
	//
	AHCIBuildCommandFis(&CFis, AtaCommandBlock);

	memset(CommandList, 0, sizeof(AHCI_COMMAND_LIST));

	CommandList->AHCICmdCfl = AHCI_FIS_REGISTER_H2D_LENGTH / 4;
	CommandList->AHCICmdW = Read ? 0 : 1;

	AHCIBuildCommandTable(
		Dev,
		Port,
		PortMultiplier,
		&CFis,
		CommandList,
		AtapiCommand,
		AtapiCommandLength,
		CommandTable,
		(VOID *)VIRT2PHYS(MemoryAddr),
		DataCount
		);

	memcpy((void *)CLBufferBase, CommandList, sizeof(AHCI_COMMAND_LIST));

	Status = AHCIStartCommand(
		Dev,
		Port,
		0,
		Timeout
		);
	if (RETURN_ERROR(Status)) {
		goto Exit;
	}

	//
	// Check the status and wait the driver sending data
	//
	//FisBaseAddr = (UINT64)AHCIRegisters->AHCIRFis + Port * sizeof (AHCI_RECEIVED_FIS);
	FisBaseAddr = (UINT64)PHYS2VIRT(AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB));
	if (Read && (AtapiCommand == 0)) {
		//
		// Wait device sends the PIO setup fis before data transfer
		//
		Status = RETURN_TIMEOUT;
		delay = Timeout / 1000 + 1;

		do {
			PioFisReceived = FALSE;
			D2hFisReceived = FALSE;
			Offset = FisBaseAddr + AHCI_PIO_FIS_OFFSET;
			Status = AHCICheckMemSet((void *)Offset, AHCI_FIS_TYPE_MASK, AHCI_FIS_PIO_SETUP);
			if (!RETURN_ERROR(Status)) {
				PioFisReceived = TRUE;
			}
			//
			// According to SATA 2.6 spec section 11.7, D2h FIS means an error encountered.
			// But Qemu and Marvel 9230 sata controller may just receive a D2h FIS from device
			// after the transaction is finished successfully.
			// To get better device compatibilities, we further check if the PxTFD's ERR bit is set.
			// By this way, we can know if there is a real error happened.
			//
			Offset = FisBaseAddr + AHCI_D2H_FIS_OFFSET;
			Status = AHCICheckMemSet((void *)Offset, AHCI_FIS_TYPE_MASK, AHCI_FIS_REGISTER_D2H);
			if (!RETURN_ERROR(Status)) {
				D2hFisReceived = TRUE;
			}

			if (PioFisReceived || D2hFisReceived) {
				Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
				PortTfd = AHCIRegRead(Dev, (UINT32)Offset);
				//
				// PxTFD will be updated if there is a D2H or SetupFIS received. 
				//
				if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
					Status = RETURN_DEVICE_ERROR;
					break;
				}


				AHCI_COMMAND_LIST *AHCICmdList = (AHCI_COMMAND_LIST *)CLBufferBase;
				PrdCount = *(volatile UINT32 *)(&AHCICmdList[0].AHCICmdPrdbc);
				if (PrdCount == DataCount) {
					Status = RETURN_SUCCESS;
					break;
				}
			}

			//
			// Stall for 100 microseconds.
			//
			MicroSecondDelay(100);

			delay--;
			if (delay == 0) {
				Status = RETURN_TIMEOUT;
			}
		} while (InfiniteWait || (delay > 0));

	}
	else {
		//
		// Wait for D2H Fis is received
		//
		Offset = FisBaseAddr + AHCI_D2H_FIS_OFFSET;

		Status = AHCIWaitMemSet(
			(void *)Offset,
			AHCI_FIS_TYPE_MASK,
			AHCI_FIS_REGISTER_D2H,
			Timeout
			);

		if (RETURN_ERROR(Status)) {
			goto Exit;
		}

		Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
		PortTfd = AHCIRegRead(Dev, (UINT32)Offset);
		if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
			Status = RETURN_DEVICE_ERROR;
		}
	}

Exit:
	AHCIStopCommand(
		Dev,
		Port,
		Timeout
		);

	AHCIDisableFisReceive(
		Dev,
		Port,
		Timeout
		);

	KFree(CommandList);
	KFree(CommandTable);

	if (Status)
		AHCIPortReset(Dev, Port, Timeout);

	return Status;
}



/**
  Start a non data transfer on specific port.

  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtaCommandBlock     The ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The ATA_STATUS_BLOCK data.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
									   used by non-blocking mode.

  @retval DEVICE_ERROR    The non data transfer abort with error occurs.
  @retval TIMEOUT         The operation is time out.
  @retval UNSUPPORTED     The device is not ready for transfer.
  @retval SUCCESS         The non data transfer executes successfully.

**/
RETURN_STATUS
AHCINonDataTransfer(
	IN	   DEVICE_NODE				  *Dev,
	IN     UINT8                      Port,
	IN     UINT8                      PortMultiplier,
	IN     ATA_COMMAND_BLOCK          *AtaCommandBlock,
	IN OUT ATA_STATUS_BLOCK           *AtaStatusBlock,
	IN     UINT64                     Timeout
	)
{
	UINT64                   Status;
	UINT64                   FisBaseAddr;
	UINT64                   Offset;
	UINT32                   PortTfd;
	AHCI_COMMAND_FIS         CFis;
	AHCI_COMMAND_LIST        CmdList;
	UINT64				   CLBufferBase;


	CLBufferBase = PHYS2VIRT(
		(UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB) |
		((UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU) << 32)
		);

	AHCI_COMMAND_LIST * CommandList = (AHCI_COMMAND_LIST*)KMalloc(sizeof(AHCI_COMMAND_LIST));
	AHCI_COMMAND_TABLE *CommandTable = (AHCI_COMMAND_TABLE*)KMalloc(sizeof(AHCI_COMMAND_TABLE));

	//
	// Package read needed
	//
	AHCIBuildCommandFis(&CFis, AtaCommandBlock);

	memset(&CmdList, 0, sizeof(AHCI_COMMAND_LIST));

	CmdList.AHCICmdCfl = AHCI_FIS_REGISTER_H2D_LENGTH / 4;

	AHCIBuildCommandTable(
		Dev,
		Port,
		PortMultiplier,
		&CFis,
		&CmdList,
		NULL,
		0,
		NULL,
		NULL,
		0
		);


	memcpy((void *)CLBufferBase, &CommandList[0], sizeof(AHCI_COMMAND_LIST));

	Status = AHCIStartCommand(
		Dev,
		Port,
		0,
		Timeout
		);
	if (RETURN_ERROR(Status)) {
		goto Exit;
	}

	//
	// Wait device sends the Response Fis
	//
	//FisBaseAddr = (UINT64)AHCIRegisters->AHCIRFis + Port * sizeof (AHCI_RECEIVED_FIS);
	FisBaseAddr = (UINT64)PHYS2VIRT(AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB));
	Offset = FisBaseAddr + AHCI_D2H_FIS_OFFSET;
	Status = AHCIWaitMemSet(
		(void *)Offset,
		AHCI_FIS_TYPE_MASK,
		AHCI_FIS_REGISTER_D2H,
		Timeout
		);

	if (RETURN_ERROR(Status)) {
		goto Exit;
	}

	Offset = AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;
	PortTfd = AHCIRegRead(Dev, (UINT32)Offset);
	if ((PortTfd & AHCI_PORT_TFD_ERR) != 0) {
		Status = RETURN_DEVICE_ERROR;
	}

Exit:
	AHCIStopCommand(
		Dev,
		Port,
		Timeout
		);

	AHCIDisableFisReceive(
		Dev,
		Port,
		Timeout
		);

	//AHCIDumpPortStatus (Dev, Port, AtaStatusBlock);

	return Status;
}


/**
Send SMART Return Status command to check if the execution of SMART cmd is successful or not.

@param  Dev               The PCI IO protocol instance.
@param  AHCIRegisters       The pointer to the AHCI_REGISTERS.
@param  Port                The number of port.
@param  PortMultiplier      The timeout value of stop.
@param  AtaStatusBlock      A pointer to ATA_STATUS_BLOCK data structure.

@retval SUCCESS     Successfully get the return status of S.M.A.R.T command execution.
@retval Others          Fail to get return status data.

**/
RETURN_STATUS
AHCIAtaSmartReturnStatusCheck(
	IN DEVICE_NODE				   *Dev,
	IN UINT8                       Port,
	IN UINT8                       PortMultiplier,
	IN OUT ATA_STATUS_BLOCK        *AtaStatusBlock
	)
{
	RETURN_STATUS           Status;
	ATA_COMMAND_BLOCK		AtaCommandBlock;
	UINT8                   LBAMid;
	UINT8                   LBAHigh;
	UINT64                  FisBaseAddr;
	UINT32                  Value;

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_SMART;
	AtaCommandBlock.AtaFeatures = ATA_SMART_RETURN_STATUS;
	AtaCommandBlock.AtaCylinderLow = ATA_CONSTANT_4F;
	AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;

	//
	// Send S.M.A.R.T Read Return Status command to device
	//
	Status = AHCINonDataTransfer(
		Dev,
		(UINT8)Port,
		(UINT8)PortMultiplier,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT
		);

	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	FisBaseAddr = (UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB) |
		((UINT64)AHCIRegRead(Dev, AHCI_PORT_START + Port * AHCI_PORT_REG_WIDTH + AHCI_PORT_FBU) << 32);

	FisBaseAddr = PHYS2VIRT(FisBaseAddr);

	Value = *(UINT32 *)(FisBaseAddr + AHCI_D2H_FIS_OFFSET);

	if ((Value & AHCI_FIS_TYPE_MASK) == AHCI_FIS_REGISTER_D2H) {
		LBAMid = ((UINT8 *)(UINT64)(FisBaseAddr + AHCI_D2H_FIS_OFFSET))[5];
		LBAHigh = ((UINT8 *)(UINT64)(FisBaseAddr + AHCI_D2H_FIS_OFFSET))[6];

		if ((LBAMid == 0x4f) && (LBAHigh == 0xc2)) {
			//
			// The threshold exceeded condition is not detected by the device
			//
			KDEBUG("The S.M.A.R.T threshold exceeded condition is not detected\n");

		}
		else if ((LBAMid == 0xf4) && (LBAHigh == 0x2c)) {
			//
			// The threshold exceeded condition is detected by the device
			//
			KDEBUG("The S.M.A.R.T threshold exceeded condition is detected\n");

		}
	}

	return RETURN_SUCCESS;
}

/**
Enable SMART command of the disk if supported.

@param  Dev               The PCI IO protocol instance.
@param  AHCIRegisters       The pointer to the AHCI_REGISTERS.
@param  Port                The number of port.
@param  PortMultiplier      The timeout value of stop.
@param  IdentifyData        A pointer to data buffer which is used to contain IDENTIFY data.
@param  AtaStatusBlock      A pointer to ATA_STATUS_BLOCK data structure.

**/
VOID
AHCIAtaSmartSupport(
	IN		DEVICE_NODE			  *Dev,
	IN		AHCI_REGISTERS        *AHCIRegisters,
	IN		UINT8                 Port,
	IN		UINT8                 PortMultiplier,
	IN		IDENTIFY_DATA         *IdentifyData,
	IN OUT  ATA_STATUS_BLOCK      *AtaStatusBlock
	)
{
	RETURN_STATUS               Status;
	ATA_COMMAND_BLOCK    AtaCommandBlock;

	//
	// Detect if the device supports S.M.A.R.T.
	//
	if ((IdentifyData->AtaData.command_set_supported_82 & 0x0001) != 0x0001) {
		//
		// S.M.A.R.T is not supported by the device
		//
		KDEBUG("S.M.A.R.T feature is not supported at port [%d] PortMultiplier [%d]!\n",
			Port, PortMultiplier);
			
	}
	else {
		//
		// Check if the feature is enabled. If not, then enable S.M.A.R.T.
		//
		if ((IdentifyData->AtaData.command_set_feature_enb_85 & 0x0001) != 0x0001) {

			memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

			AtaCommandBlock.AtaCommand = ATA_CMD_SMART;
			AtaCommandBlock.AtaFeatures = ATA_SMART_ENABLE_OPERATION;
			AtaCommandBlock.AtaCylinderLow = ATA_CONSTANT_4F;
			AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;

			//
			// Send S.M.A.R.T Enable command to device
			//
			Status = AHCINonDataTransfer(
				Dev,
				(UINT8)Port,
				(UINT8)PortMultiplier,
				&AtaCommandBlock,
				AtaStatusBlock,
				ATA_ATAPI_TIMEOUT
				);


			if (!RETURN_ERROR(Status)) {
				//
				// Send S.M.A.R.T AutoSave command to device
				//
				memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

				AtaCommandBlock.AtaCommand = ATA_CMD_SMART;
				AtaCommandBlock.AtaFeatures = 0xD2;
				AtaCommandBlock.AtaSectorCount = 0xF1;
				AtaCommandBlock.AtaCylinderLow = ATA_CONSTANT_4F;
				AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;

				Status = AHCINonDataTransfer(
					Dev,
					(UINT8)Port,
					(UINT8)PortMultiplier,
					&AtaCommandBlock,
					AtaStatusBlock,
					ATA_ATAPI_TIMEOUT
					);

				if (!RETURN_ERROR(Status)) {
					Status = AHCIAtaSmartReturnStatusCheck(
						Dev,
						(UINT8)Port,
						(UINT8)PortMultiplier,
						AtaStatusBlock
						);
				}
			}
		}
		KDEBUG("Enabled S.M.A.R.T feature at port [%d] PortMultiplier [%d]!\n",
			Port, PortMultiplier);
	}

	return;
}

/**
Send Buffer cmd to specific device.

@param  Dev               The PCI IO protocol instance.
@param  AHCIRegisters       The pointer to the AHCI_REGISTERS.
@param  Port                The number of port.
@param  PortMultiplier      The timeout value of stop.
@param  Buffer              The data buffer to store IDENTIFY PACKET data.

@retval DEVICE_ERROR    The cmd abort with error occurs.
@retval TIMEOUT         The operation is time out.
@retval UNSUPPORTED     The device is not ready for executing.
@retval SUCCESS         The cmd executes successfully.

**/
RETURN_STATUS
AHCIIdentify(
	IN DEVICE_NODE				*Dev,
	IN UINT8					Port,
	IN UINT8					PortMultiplier,
	IN OUT IDENTIFY_DATA	    *Buffer
	)
{
	RETURN_STATUS                   Status;
	ATA_COMMAND_BLOCK        AtaCommandBlock;
	ATA_STATUS_BLOCK         AtaStatusBlock;

	if (Dev == NULL || Buffer == NULL) {
		return RETURN_INVALID_PARAMETER;
	}

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));
	memset(&AtaStatusBlock, 0, sizeof(ATA_STATUS_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_IDENTIFY_DRIVE;
	AtaCommandBlock.AtaSectorCount = 1;

	Status = AHCIPioTransfer(
		Dev,
		Port,
		PortMultiplier,
		NULL,
		0,
		TRUE,
		&AtaCommandBlock,
		&AtaStatusBlock,
		Buffer,
		sizeof(ATA_IDENTIFY_DATA),
		ATA_ATAPI_TIMEOUT
		);

	return Status;
}

/**
Send Buffer cmd to specific device.

@param  Dev               The PCI IO protocol instance.
@param  AHCIRegisters       The pointer to the AHCI_REGISTERS.
@param  Port                The number of port.
@param  PortMultiplier      The timeout value of stop.
@param  Buffer              The data buffer to store IDENTIFY PACKET data.

@retval DEVICE_ERROR    The cmd abort with error occurs.
@retval TIMEOUT         The operation is time out.
@retval UNSUPPORTED     The device is not ready for executing.
@retval SUCCESS         The cmd executes successfully.

**/
RETURN_STATUS

AHCIIdentifyPacket(
	IN DEVICE_NODE				*Dev,
	IN UINT8					Port,
	IN UINT8					PortMultiplier,
	IN OUT IDENTIFY_DATA		*Buffer
	)
{
	RETURN_STATUS                   Status;
	ATA_COMMAND_BLOCK        AtaCommandBlock;
	ATA_STATUS_BLOCK         AtaStatusBlock;

	if (Dev == NULL) {
		return RETURN_INVALID_PARAMETER;
	}

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));
	memset(&AtaStatusBlock, 0, sizeof(ATA_STATUS_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_IDENTIFY_DEVICE;
	AtaCommandBlock.AtaSectorCount = 1;

	Status = AHCIPioTransfer(
		Dev,
		Port,
		PortMultiplier,
		NULL,
		0,
		TRUE,
		&AtaCommandBlock,
		&AtaStatusBlock,
		Buffer,
		sizeof(ATA_IDENTIFY_DATA),
		ATA_ATAPI_TIMEOUT
		);

	return Status;
}

/**
  Send SET FEATURE cmd on specific device.

  @param  Dev               The PCI IO protocol instance.
  @param  AHCIRegisters       The pointer to the AHCI_REGISTERS.
  @param  Port                The number of port.
  @param  PortMultiplier      The timeout value of stop.
  @param  Feature             The data to send Feature register.
  @param  FeatureSpecificData The specific data for SET FEATURE cmd.

  @retval DEVICE_ERROR    The cmd abort with error occurs.
  @retval TIMEOUT         The operation is time out.
  @retval UNSUPPORTED     The device is not ready for executing.
  @retval SUCCESS         The cmd executes successfully.

**/
RETURN_STATUS
AHCIDeviceSetFeature(
	IN	 DEVICE_NODE			*Dev,
	IN UINT8                  Port,
	IN UINT8                  PortMultiplier,
	IN UINT16                 Feature,
	IN UINT32                 FeatureSpecificData
	)
{
	UINT64               Status;
	ATA_COMMAND_BLOCK    AtaCommandBlock;
	ATA_STATUS_BLOCK     AtaStatusBlock;

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));
	memset(&AtaStatusBlock, 0, sizeof(ATA_STATUS_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_SET_FEATURES;
	AtaCommandBlock.AtaFeatures = (UINT8)Feature;
	AtaCommandBlock.AtaFeaturesExp = (UINT8)(Feature >> 8);
	AtaCommandBlock.AtaSectorCount = (UINT8)FeatureSpecificData;
	AtaCommandBlock.AtaSectorNumber = (UINT8)(FeatureSpecificData >> 8);
	AtaCommandBlock.AtaCylinderLow = (UINT8)(FeatureSpecificData >> 16);
	AtaCommandBlock.AtaCylinderHigh = (UINT8)(FeatureSpecificData >> 24);

	Status = AHCINonDataTransfer(
		Dev,
		(UINT8)Port,
		(UINT8)PortMultiplier,
		&AtaCommandBlock,
		&AtaStatusBlock,
		30000
		);

	return Status;
}

void AhciIrq(void * Dev)
{
	printk("AHCI IRQ:\n");
}

void AHCIControllerInit(DEVICE_NODE * Dev)
{
	KDEBUG("AHCI Controller init...\n");

	UINT32 HBA_Cap = AHCIRegRead(Dev, AHCI_CAPABILITY_OFFSET);
	KDEBUG("AHCI HBA Capabilities:%08x\n", HBA_Cap);

	UINT32 HBA_GHC = AHCIRegRead(Dev, AHCI_GHC_OFFSET);
	KDEBUG("AHCI HBA Global Control Status:%08x\n", HBA_GHC);

	//Enable AHCI Controller
	if ((HBA_GHC & AHCI_CAP_SAM) == 0)
	{
		AHCIRegWrite(Dev, AHCI_GHC_OFFSET, HBA_GHC | AHCI_GHC_ENABLE | AHCI_GHC_IE);
	}

	//Reset AHCI Controller
	AHCIRegWrite(Dev, AHCI_GHC_OFFSET, HBA_GHC | AHCI_GHC_RESET);


	while (1)
	{
		UINT32 Value = AHCIRegRead(Dev, AHCI_GHC_OFFSET);
		if ((Value & AHCI_GHC_RESET) == 0)
			break;

		MicroSecondDelay(100);
	}


	KDEBUG("AHCI HBA Global Control after reset:%08x\n", AHCIRegRead(Dev, AHCI_GHC_OFFSET));

	//Enable AHCI Controller
	if ((HBA_GHC & AHCI_CAP_SAM) == 0)
	{
		AHCIRegWrite(Dev, AHCI_GHC_OFFSET, HBA_GHC | AHCI_GHC_ENABLE | AHCI_GHC_IE);
	}

	//Wait for 1000 microseconds
	MicroSecondDelay(1000);

	UINT32 HBA_PI = AHCIRegRead(Dev, AHCI_PI_OFFSET);
	KDEBUG("AHCI HBA Available Ports:%08x\n", HBA_PI);

	KDEBUG("AHCI Active Ports:\n");
	int i;
	UINT32 PortOffset;
	DEVICE_NODE * Disk;
	UINT64  FISBufferBase;
	UINT64  CLBufferBase;
	UINT64 	Offset;
	UINT64  Status;
	UINT32                           DeviceType;
	IDENTIFY_DATA					 *Buffer;
	UINT8  CharBuffer[512] = { 0 };

	Buffer = KMalloc(sizeof(IDENTIFY_DATA));
	for (i = 0; i < 32; i++)
	{
		if (HBA_PI & (1 << i))
		{
			if (RETURN_SUCCESS == AHCICheckDeviceStatus(Dev, i))
			{
				KDEBUG("    SATA%d\n", i);
				AHCIPortReset(Dev, i, 10000);

				UINT64 *CLBufferBase = (UINT64 *)(UINT64)KMalloc(0x1000);
				UINT64 *FISBufferBase = (UINT64 *)(UINT64)KMalloc(0x1000);
				//KDEBUG("CLB:%016x,FIB:%016x\n", CLBufferBase, FISBufferBase);
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB;
				AHCIRegWrite(Dev, Offset, (UINT64)CLBufferBase & 0xFFFFF000);
				AHCIRegWrite(Dev, Offset + 4, VIRT2PHYS((UINT64)CLBufferBase) >> 32);
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB;
				AHCIRegWrite(Dev, Offset, (UINT64)FISBufferBase & 0xFFFFF000);
				AHCIRegWrite(Dev, Offset + 4, VIRT2PHYS((UINT64)FISBufferBase) >> 32);

				KDEBUG("Port:%d CL Buffer:%016x\n",
					i,
					(UINT64)AHCIRegRead(Dev, AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLB) |
					((UINT64)AHCIRegRead(Dev, AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_CLBU) << 32));
				KDEBUG("Port:%d FIS Buffer:%016x\n",
					i,
					(UINT64)AHCIRegRead(Dev, AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_FB) |
					((UINT64)AHCIRegRead(Dev, AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_FBU) << 32));

				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
				UINT32 Data = AHCIRegRead(Dev, Offset);
				if ((Data & AHCI_PORT_CMD_CPD) != 0) {
					AHCIRegOr(Dev, Offset, AHCI_PORT_CMD_POD);
				}

				//
				// Disable aggressive power management.
				//
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_SCTL;
				AHCIRegOr(Dev, Offset, AHCI_PORT_SCTL_IPM_INIT);
				//
				// Enable the reporting of the corresponding interrupt to system software.
				//
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_IE;
				AHCIRegOr(Dev, Offset, 0xFFFFFFFF);

				//
				// Enable FIS Receive DMA engine for the first D2H FIS.
				//
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
				AHCIRegOr(Dev, Offset, AHCI_PORT_CMD_FRE);
				RETURN_STATUS Status = AHCIWaitMmioSet(
					Dev,
					Offset,
					AHCI_PORT_CMD_FR,
					AHCI_PORT_CMD_FR,
					500
					);
				if (RETURN_ERROR(Status)) {
					continue;
				}

				//
				// Wait no longer than 10 ms to wait the Phy to detect the presence of a device.
				// It's the requirment from SATA1.0a spec section 5.2.
				//
				UINT64 PhyDetectDelay = AHCI_BUS_PHY_DETECT_TIMEOUT;
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_SSTS;
				do {
					Data = AHCIRegRead(Dev, Offset) & AHCI_PORT_SSTS_DET_MASK;
					if ((Data == AHCI_PORT_SSTS_DET_PCE) || (Data == AHCI_PORT_SSTS_DET)) {
						break;
					}

					MicroSecondDelay(10000);
					PhyDetectDelay--;
				} while (PhyDetectDelay > 0);

				if (PhyDetectDelay == 0) {
					//
					// No device detected at this port.
					// Clear PxCMD.SUD for those ports at which there are no device present.
					//
					Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_CMD;
					AHCIRegWrite(Dev, Offset, (UINT32)~(AHCI_PORT_CMD_SUD));
					continue;
				}

				//
				// According to SATA1.0a spec section 5.2, we need to wait for PxTFD.BSY and PxTFD.DRQ
				// and PxTFD.ERR to be zero. The maximum wait time is 16s which is defined at ATA spec.
				//
				PhyDetectDelay = 16 * 1000;
				do {
					Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_SERR;
					if (AHCIRegRead(Dev, Offset) != 0) {
						AHCIRegWrite(Dev, Offset, AHCIRegRead(Dev, Offset));
					}
					Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_TFD;

					Data = AHCIRegRead(Dev, Offset) & AHCI_PORT_TFD_MASK;
					if (Data == 0) {
						break;
					}

					MicroSecondDelay(1000);
					PhyDetectDelay--;
				} while (PhyDetectDelay > 0);

				if (PhyDetectDelay == 0) {
					continue;
				}

				//
				// When the first D2H register FIS is received, the content of PxSIG register is updated.
				//
				Offset = AHCI_PORT_START + i * AHCI_PORT_REG_WIDTH + AHCI_PORT_SIG;
				Status = AHCIWaitMmioSet(
					Dev,
					Offset,
					0x0000FFFF,
					0x00000101,
					16 * 1000 * 1000
					);
				if (RETURN_ERROR(Status)) {
					continue;
				}

				Data = AHCIRegRead(Dev, Offset);
				if ((Data & AHCI_ATAPI_SIG_MASK) == AHCI_ATAPI_DEVICE_SIG) {
					Status = AHCIIdentifyPacket(Dev, i, 0, Buffer);

					if (RETURN_ERROR(Status)) {
						continue;
					}

					DeviceType = IdeCdrom;

					int k;
					
					memset(CharBuffer, 0, 512);
					for (k = 0; k < sizeof(Buffer->AtaData.SerialNo); k += 2)
					{
						CharBuffer[k] = Buffer->AtaData.SerialNo[k + 1];
						CharBuffer[k + 1] = Buffer->AtaData.SerialNo[k];
						//printk("%c", Buffer->AtaData.SerialNo[k + 1]);
						//printk("%c", Buffer->AtaData.SerialNo[k]);
					}
					printk("AHCI CDROM SN:%s\n", CharBuffer);

					
					for (k = 0; k < sizeof(Buffer->AtaData.ModelName); k += 2)
					{
						CharBuffer[k] = Buffer->AtaData.ModelName[k + 1];
						CharBuffer[k + 1] = Buffer->AtaData.ModelName[k];
						//printk("%c", Buffer->AtaData.ModelName[k + 1]);
						//printk("%c", Buffer->AtaData.ModelName[k]);
					}
					printk("AHCI CDROM Model:%s\n", CharBuffer);
				}
				else if ((Data & AHCI_ATAPI_SIG_MASK) == AHCI_ATA_DEVICE_SIG) {
					Status = AHCIIdentify(Dev, i, 0, Buffer);

					if (RETURN_ERROR(Status)) {
						continue;
					}

					DeviceType = IdeHarddisk;

					Disk = (DEVICE_NODE *)KMalloc(sizeof(DEVICE_NODE));
					Disk->DeviceType = DEVICE_TYPE_STORAGE;
					Disk->BusType = BUS_TYPE_SATA;
					Disk->DeviceID = 0x4100 + i;
					Disk->Position = (i << 8);
					Disk->ParentDevice = Dev;
					Disk->ChildDevice = 0;
					Disk->NextDevice = 0;
					Disk->PrevDevice = 0;

					int k;
					memset(CharBuffer, 0, 512);
					for (k = 0; k < sizeof(Buffer->AtaData.SerialNo); k += 2)
					{
						CharBuffer[k] = Buffer->AtaData.SerialNo[k + 1];
						CharBuffer[k + 1] = Buffer->AtaData.SerialNo[k];
						//printk("%c", Buffer->AtaData.SerialNo[k + 1]);
						//printk("%c", Buffer->AtaData.SerialNo[k]);
					}
					printk("AHCI Harddisk SN:%s\n", CharBuffer);


					for (k = 0; k < sizeof(Buffer->AtaData.ModelName); k += 2)
					{
						CharBuffer[k] = Buffer->AtaData.ModelName[k + 1];
						CharBuffer[k + 1] = Buffer->AtaData.ModelName[k];
						//printk("%c", Buffer->AtaData.ModelName[k + 1]);
						//printk("%c", Buffer->AtaData.ModelName[k]);
					}
					printk("AHCI Harddisk Model:%s\n", CharBuffer);

					KDEBUG("DMA Supported:[%s]\n", Buffer->AtaData.capabilities_49 & (1 << 8) ? "Yes" : "No");
					KDEBUG("LBA Supported:[%s]\n", Buffer->AtaData.capabilities_49 & (1 << 9) ? "Yes" : "No");

					UINT64 Capacity =
						Buffer->AtaData.maximum_lba_for_48bit_addressing[0] +
						((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[1] << 16) +
						((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[2] << 16 * 2) +
						((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[3] << 16 * 3);

					if (Capacity == 0)
					{
						Capacity =
							((UINT64)Buffer->AtaData.user_addressable_sectors_hi << 16) +
							(UINT64)Buffer->AtaData.user_addressable_sectors_lo;
					}

					printk("AHCI Disk Capacity:%d.%d %s.\n",
						Capacity * 512 > 1 * 1024 * 1024 * 1024 ? Capacity * 512 / 1024 / 1024 / 1024 : Capacity * 512 / 1024 / 1024,
						Capacity * 512 > 1 * 1024 * 1024 * 1024 ? Capacity * 512 / 1024 / 1024 % 1024 * 100 / 1024 : Capacity * 512 / 1024 % 1024 * 100 / 1024,
						Capacity * 512 > 1 * 1024 * 1024 * 1024 ? "GB" : "MB");

					
					DISK_NODE * NewDisk = KMalloc(sizeof(DISK_NODE));
					NewDisk->Device = Disk;
					NewDisk->StartingSectors = 0;
					NewDisk->NumberOfSectors = Capacity / 512;
					NewDisk->SectorSize = 512;
					NewDisk->Next = NULL;

					DeviceRegister(Disk);
					DiskRegister(NewDisk);
					
				}
				else {
					continue;
				}
				KDEBUG("Port [%d] Port Mulitplier [%d] has a [%s]\n",
					i, 0, DeviceType == IdeCdrom ? "CDROM" : "HardDisk");
				
			}
		}
	}
	KFree(Buffer);

	PCI_DEVICE * PciDev = Dev->Device;
	PciMsiCheck(PciDev);
	UINT64 Irqs = PciMsiGetMultipleMessageCapable(PciDev);
	KDEBUG("AHCI:PCI-MSI:Request %d IRQs.\n", Irqs);
	PciMsiMultipleMessageCapableEnable(PciDev, Irqs);
	MSI_CAP_STRUCTURE_64_MASK PciMsi;
	PciMsi.MessageAddr = 0xFEE00000;
	PciMsi.MessageData = 0x41;
	PciMsiSet(PciDev, &PciMsi);
	PciMsiEnable(PciDev);
	IrqRegister(0x41, AhciIrq, Dev);

	KDEBUG("\n");
	return;
}


RETURN_STATUS AHCIDiskRead(DISK_NODE * Disk, UINT64 Sector, UINT64 Length, UINT8 * Buffer)
{
	ATA_COMMAND_BLOCK ata_cmb;
	ATA_STATUS_BLOCK ata_sb;
	memset(&ata_cmb, 0, sizeof(ATA_COMMAND_BLOCK));
	UINT64 StartLba = Sector;
	UINT64 TransferLength = Length;
	UINT64 PortMultiplierPort = 0;
	UINT8 LBA48bit = 1;
	RETURN_STATUS Status = 0;

	//ata_cmb.AtaCommand = ATA_CMD_READ_SECTORS;
	ata_cmb.AtaCommand = ATA_CMD_READ_DMA_EXT;
	ata_cmb.AtaSectorNumber = (UINT8)StartLba;
	ata_cmb.AtaCylinderLow = (UINT8)(StartLba >> 8);
	ata_cmb.AtaCylinderHigh = (UINT8)(StartLba >> 16);
	ata_cmb.AtaDeviceHead = (UINT8)(BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4));
	ata_cmb.AtaSectorCount = (UINT8)(TransferLength / 512 ? TransferLength / 512 : 1);

	if (LBA48bit)
	{
		ata_cmb.AtaSectorNumberExp = (UINT8)(StartLba >> 24);
		ata_cmb.AtaCylinderLowExp = (UINT8)(StartLba >> 32);
		ata_cmb.AtaCylinderHighExp = (UINT8)(StartLba >> 40);
		ata_cmb.AtaSectorCountExp = (UINT8)((TransferLength / 512) >> 8);
	}
	else
		ata_cmb.AtaDeviceHead = (UINT8)(ata_cmb.AtaDeviceHead | (StartLba >> 24));

	Status = AHCIDmaTransfer(
		Disk->Device->ParentDevice,
		(Disk->Device->Position >> 8),
		0,
		1,
		0,
		&ata_cmb,
		&ata_sb,
		Buffer,
		TransferLength,
		200000
		);

	/*
	AHCIPioTransfer(
		Dev,
		0,
		0,
		NULL,
		0,
		1,
		&ata_cmb,
		&ata_sb,
		Buffer,
		TransferLength,
		20000
		);
		*/
	return Status;
}




RETURN_STATUS AHCIDiskWrite(DISK_NODE * Disk, UINT64 Sector, UINT64 Length, UINT8 * Buffer)
{
	ATA_COMMAND_BLOCK ata_cmb;
	ATA_STATUS_BLOCK ata_sb;
	memset(&ata_cmb, 0, sizeof(ATA_COMMAND_BLOCK));
	UINT64 StartLba = Sector;
	UINT64 TransferLength = Length;
	UINT64 PortMultiplierPort = 0;
	UINT8 LBA48bit = 1;
	RETURN_STATUS Status = 0;

	//ata_cmb.AtaCommand = ATA_CMD_READ_SECTORS;
	ata_cmb.AtaCommand = ATA_CMD_WRITE_DMA_EXT;
	ata_cmb.AtaSectorNumber = (UINT8)StartLba;
	ata_cmb.AtaCylinderLow = (UINT8)(StartLba >> 8);
	ata_cmb.AtaCylinderHigh = (UINT8)(StartLba >> 16);
	ata_cmb.AtaDeviceHead = (UINT8)(BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4));
	ata_cmb.AtaSectorCount = (UINT8)(TransferLength / 512 ? TransferLength / 512 : 1);

	if (LBA48bit)
	{
		ata_cmb.AtaSectorNumberExp = (UINT8)(StartLba >> 24);
		ata_cmb.AtaCylinderLowExp = (UINT8)(StartLba >> 32);
		ata_cmb.AtaCylinderHighExp = (UINT8)(StartLba >> 40);
		ata_cmb.AtaSectorCountExp = (UINT8)((TransferLength / 512) >> 8);
	}
	else
		ata_cmb.AtaDeviceHead = (UINT8)(ata_cmb.AtaDeviceHead | (StartLba >> 24));

	Status = AHCIDmaTransfer(
		Disk->Device->ParentDevice,
		(Disk->Device->Position >> 8),
		0,
		0,
		0,
		&ata_cmb,
		&ata_sb,
		Buffer,
		TransferLength,
		200000
		);


	/*
	AHCIPioTransfer(
		Dev,
		0,
		0,
		NULL,
		0,
		1,
		&ata_cmb,
		&ata_sb,
		Buffer,
		TransferLength,
		20000
		);
		*/
	return Status;
}



RETURN_STATUS AHCIDiskSupported(DEVICE_NODE * Dev)
{
	if (Dev->BusType != BUS_TYPE_SATA)
		return RETURN_UNSUPPORTED;

	KDEBUG("AHCI disk driver supported on device:%016x\n", Dev->Position);

	return RETURN_SUCCESS;
}

RETURN_STATUS AHCIDiskStart(DEVICE_NODE * Dev)
{
	if (Dev->BusType != BUS_TYPE_SATA)
		return RETURN_UNSUPPORTED;

	KDEBUG("AHCI Disk Driver starting...\n", Dev->Position);

	return RETURN_SUCCESS;
}



DRIVER_NODE AHCIControllerDriver =
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = AHCISupported,
	.DriverStart = AHCIStart,
	.DriverStop = AHCIStop,
	.Operations = 0,
	.PrevDriver = 0,
	.NextDriver = 0
};


DISK_DRIVER AHCIDiskOperations =
{
	.DiskOpen = 0,
	.GetDiskInfo = 0,
	.DiskRead = AHCIDiskRead,
	.DiskWrite = AHCIDiskWrite,
	.DiskClose = 0
};

DRIVER_NODE AHCIDiskDriver =
{
	.BusType = BUS_TYPE_SATA,
	.DriverSupported = AHCIDiskSupported,
	.DriverStart = AHCIDiskStart,
	.DriverStop = 0,
	.Operations = &AHCIDiskOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void AHCIInit()
{
	DriverRegister(&AHCIControllerDriver, &DriverList[DEVICE_TYPE_STORAGE]);
	DriverRegister(&AHCIDiskDriver, &DriverList[DEVICE_TYPE_STORAGE]);
}

MODULE_INIT(AHCIInit);


