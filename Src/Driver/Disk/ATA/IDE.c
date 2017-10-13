#include "IDE.h"



RETURN_STATUS IDESupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if (Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if (PCI_CLASS_MASS_STORAGE != PciCfgRead8(PciDev, PCI_CONF_BASECLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;

	if (PCI_CLASS_IDE != PciCfgRead8(PciDev, PCI_CONF_SUBCLASSCODE_OFF_8))
		return RETURN_UNSUPPORTED;

	KDEBUG("IDE controller driver supported on device:%02x:%02x.%02x\n",
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	return RETURN_SUCCESS;

}

RETURN_STATUS IDEStart(DEVICE_NODE * Dev)
{
	KDEBUG("IDE controller driver starting...\n");
	PCI_DEVICE * PciDev = Dev->Device;
	
	UINT16 PciCommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PciCommand |= (BIT0 | BIT1 | BIT2);	/* Enable bus master, memory, io. */
	PciCommand ^= BIT10;			/* Enable Interrupt.*/
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PciCommand);

	IDEControllerInit(Dev);
	return RETURN_SUCCESS;
}

RETURN_STATUS IDEStop(DEVICE_NODE * Dev)
{
	return RETURN_SUCCESS;
}


/**
read a one-byte data from a IDE port.

@param  Port   The IDE Port number

@return  the one-byte data read from IDE port
**/
UINT8
IdeReadPortB(
	IN  UINT16                Port
	)
{
	return In8(Port);
}

/**
write a 1-byte data to a specific IDE port.

@param  Port   The IDE port to be writen
@param  Data   The data to write to the port
**/
VOID
IdeWritePortB(
	IN  UINT16                Port,
	IN  UINT8                 Data
	)
{
	Out8(Port, Data);
}

/**
write a 1-word data to a specific IDE port.

@param  Port   The IDE port to be writen
@param  Data   The data to write to the port
**/
VOID
IdeWritePortW(
	IN  UINT16                Port,
	IN  UINT16                Data
	)
{
	//UINT16 BEData = (Data<<8)|(Data>>8) ;
	//Out16(Port, BEData);
	Out16(Port, Data);
}

/**
write a 2-word data to a specific IDE port.

@param  Port   The IDE port to be writen
@param  Data   The data to write to the port
**/
VOID
IdeWritePortDW(
	IN  UINT16                Port,
	IN  UINT32                Data
	)
{
	//UINT8 *Array = (UINT8 *)&Data;
	//UINT32 BEData = ((Array[0]<<24))|(Array[1]<<16)|(Array[2] << 8)|Array[3];
	//Out32(Port, BEData);
	Out32(Port, Data);
}

/**
Write multiple words of data to the IDE data port.
Call the IO abstraction once to do the complete read,
not one word at a time

@param  Port       IO port to read
@param  Count      No. of UINT16's to read
@param  Buffer     Pointer to the data buffer for read

**/
VOID
IdeWritePortWMultiple(
	IN  UINT16                Port,
	IN  UINT32                Count,
	IN  VOID                  *Buffer
	)
{
	UINT32 i = 0;
	UINT16 * OutBuffer = Buffer;
	//UINT16 BEData;
	for (i = 0; i < Count; i++)
	{
		//BEData = (OutBuffer[i] << 8) | (OutBuffer[i] >> 8);
		//Out16(Port, BEData);
		Out16(Port, OutBuffer[i]);
	}
}

/**
Reads multiple words of data from the IDE data port.
Call the IO abstraction once to do the complete read,
not one word at a time

@param  Port     IO port to read
@param  Count    Number of UINT16's to read
@param  Buffer   Pointer to the data buffer for read

**/
VOID
IdeReadPortWMultiple(
	IN  UINT16                Port,
	IN  UINT32                Count,
	IN  VOID                  *Buffer
	)
{
	UINT32 i = 0;
	UINT16 * InBuffer = Buffer;
	//UINT16 BEData;
	for (i = 0; i < Count; i++)
	{
		//BEData = In16(Port);
		//OutBuffer[i] = (BEData << 8) | (BEData >> 8);
		InBuffer[i] = In16(Port);
	}
}

/**
This function is used to analyze the Status Register and print out
some debug information and if there is ERR bit set in the Status
Register, the Error Register's value is also be parsed and print out.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.

**/
VOID
DumpAllIdeRegisters(
	IN     DEVICE_NODE			*Dev,
	IN     UINT32				Channel,
	IN OUT ATA_STATUS_BLOCK     *AtaStatusBlock
	)
{
	ATA_STATUS_BLOCK StatusBlock;
	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	memset(&StatusBlock, 0, sizeof(ATA_STATUS_BLOCK));

	StatusBlock.AtaStatus = IdeReadPortB(IdeRegisters->CmdOrStatus);
	StatusBlock.AtaError = IdeReadPortB(IdeRegisters->ErrOrFeature);
	StatusBlock.AtaSectorCount = IdeReadPortB(IdeRegisters->SectorCount);
	StatusBlock.AtaSectorCountExp = IdeReadPortB(IdeRegisters->SectorCount);
	StatusBlock.AtaSectorNumber = IdeReadPortB(IdeRegisters->SectorNumber);
	StatusBlock.AtaSectorNumberExp = IdeReadPortB(IdeRegisters->SectorNumber);
	StatusBlock.AtaCylinderLow = IdeReadPortB(IdeRegisters->CylinderLsb);
	StatusBlock.AtaCylinderLowExp = IdeReadPortB(IdeRegisters->CylinderLsb);
	StatusBlock.AtaCylinderHigh = IdeReadPortB(IdeRegisters->CylinderMsb);
	StatusBlock.AtaCylinderHighExp = IdeReadPortB(IdeRegisters->CylinderMsb);
	StatusBlock.AtaDeviceHead = IdeReadPortB(IdeRegisters->Head);

	if (AtaStatusBlock != NULL) {
		//
		// Dump the content of all ATA registers.
		//
		memcpy(AtaStatusBlock, &StatusBlock, sizeof(ATA_STATUS_BLOCK));
	}

	// Debug Code
	if ((StatusBlock.AtaStatus & ATA_STSREG_DWF) != 0) {
		KDEBUG( "CheckRegisterStatus()-- %02x : Error : Write Fault\n", StatusBlock.AtaStatus);
	}

	if ((StatusBlock.AtaStatus & ATA_STSREG_CORR) != 0) {
		KDEBUG( "CheckRegisterStatus()-- %02x : Error : Corrected Data\n", StatusBlock.AtaStatus);
	}

	if ((StatusBlock.AtaStatus & ATA_STSREG_ERR) != 0) {
		if ((StatusBlock.AtaError & ATA_ERRREG_BBK) != 0) {
			KDEBUG( "CheckRegisterStatus()-- %02x : Error : Bad Block Detected\n", StatusBlock.AtaError);
		}

		if ((StatusBlock.AtaError & ATA_ERRREG_UNC) != 0) {
			KDEBUG( "CheckRegisterStatus()-- %02x : Error : Uncorrectable Data\n", StatusBlock.AtaError);
		}

		if ((StatusBlock.AtaError & ATA_ERRREG_MC) != 0) {
			KDEBUG( "CheckRegisterStatus()-- %02x : Error : Media Change\n", StatusBlock.AtaError);
		}

		if ((StatusBlock.AtaError & ATA_ERRREG_ABRT) != 0) {
			KDEBUG( "CheckRegisterStatus()-- %02x : Error : Abort\n", StatusBlock.AtaError);
		}

		if ((StatusBlock.AtaError & ATA_ERRREG_TK0NF) != 0) {
			KDEBUG( "CheckRegisterStatus()-- %02x : Error : Track 0 Not Found\n", StatusBlock.AtaError);
		}

		if ((StatusBlock.AtaError & ATA_ERRREG_AMNF) != 0) {
			KDEBUG( "CheckRegisterStatus()-- %02x : Error : Address Mark Not Found\n", StatusBlock.AtaError);
		}
	}
}

/**
This function is used to analyze the Status Register at the condition that BSY is zero.
if there is ERR bit set in the Status Register, then return error.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.

@retval RETURN_SUCCESS       No err information in the Status Register.
@retval RETURN_DEVICE_ERROR  Any err information in the Status Register.

**/
RETURN_STATUS
CheckStatusRegister(
	IN  DEVICE_NODE           *Dev,
	IN     UINT32			 Channel
	)
{
	UINT8           StatusRegister;
	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	StatusRegister = IdeReadPortB(IdeRegisters->CmdOrStatus);
	//KDEBUG("Line = %d, StatusRegister = %x\n",__LINE__, StatusRegister);
	if ((StatusRegister & ATA_STSREG_BSY) == 0) {
		if ((StatusRegister & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {
			return RETURN_SUCCESS;
		}
		else {
			return RETURN_DEVICE_ERROR;
		}
	}
	return RETURN_SUCCESS;
}

/**
This function is used to poll for the DRQ bit clear in the Status
Register. DRQ is cleared when the device is finished transferring data.
So this function is called after data transfer is finished.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS     DRQ bit clear within the time out.

@retval RETURN_TIMEOUT     DRQ bit not clear within the time out.

@note
Read Status Register will clear interrupt status.

**/
RETURN_STATUS

DRQClear(
	IN  DEVICE_NODE            *Dev,
	IN  UINT32				  Channel,
	IN  UINT64                Timeout
	)
{
	UINT64  Delay;
	UINT8   StatusRegister;
	BOOLEAN InfiniteWait;
	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		StatusRegister = IdeReadPortB(IdeRegisters->CmdOrStatus);

		//
		// Wait for BSY == 0, then judge if DRQ is clear
		//
		if ((StatusRegister & ATA_STSREG_BSY) == 0) {
			if ((StatusRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
				return RETURN_DEVICE_ERROR;
			}
			else {
				return RETURN_SUCCESS;
			}
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;

	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}
/**
This function is used to poll for the DRQ bit clear in the Alternate
Status Register. DRQ is cleared when the device is finished
transferring data. So this function is called after data transfer
is finished.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS     DRQ bit clear within the time out.

@retval RETURN_TIMEOUT     DRQ bit not clear within the time out.
@note   Read Alternate Status Register will not clear interrupt status.

**/
RETURN_STATUS
DRQClear2(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   AltRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		AltRegister = IdeReadPortB(IdeRegisters->AltOrDev);

		//
		// Wait for BSY == 0, then judge if DRQ is clear
		//
		if ((AltRegister & ATA_STSREG_BSY) == 0) {
			if ((AltRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
				return RETURN_DEVICE_ERROR;
			}
			else {
				return RETURN_SUCCESS;
			}
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;

	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}

/**
This function is used to poll for the DRQ bit set in the
Status Register.
DRQ is set when the device is ready to transfer data. So this function
is called after the command is sent to the device and before required
data is transferred.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS          DRQ bit set within the time out.
@retval RETURN_TIMEOUT          DRQ bit not set within the time out.
@retval RETURN_ABORTED          DRQ bit not set caused by the command abort.

@note  Read Status Register will clear interrupt status.

**/
RETURN_STATUS
DRQReady(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   StatusRegister;
	UINT8   ErrorRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		//
		// Read Status Register will clear interrupt
		//
		StatusRegister = IdeReadPortB(IdeRegisters->CmdOrStatus);

		//
		// Wait for BSY == 0, then judge if DRQ is clear or ERR is set
		//
		if ((StatusRegister & ATA_STSREG_BSY) == 0) {
			if ((StatusRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
				ErrorRegister = IdeReadPortB(IdeRegisters->ErrOrFeature);

				if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
					return RETURN_ABORTED;
				}
				return RETURN_DEVICE_ERROR;
			}

			if ((StatusRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
				return RETURN_SUCCESS;
			}
			else {
				return RETURN_NOT_READY;
			}
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;
	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}
/**
This function is used to poll for the DRQ bit set in the Alternate Status Register.
DRQ is set when the device is ready to transfer data. So this function is called after
the command is sent to the device and before required data is transferred.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS           DRQ bit set within the time out.
@retval RETURN_TIMEOUT           DRQ bit not set within the time out.
@retval RETURN_ABORTED           DRQ bit not set caused by the command abort.
@note  Read Alternate Status Register will not clear interrupt status.

**/
RETURN_STATUS
DRQReady2(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   AltRegister;
	UINT8   ErrorRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;

	do {
		//
		// Read Alternate Status Register will not clear interrupt status
		//
		AltRegister = IdeReadPortB(IdeRegisters->AltOrDev);
		//
		// Wait for BSY == 0, then judge if DRQ is clear or ERR is set
		//
		if ((AltRegister & ATA_STSREG_BSY) == 0) {
			if ((AltRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
				ErrorRegister = IdeReadPortB(IdeRegisters->ErrOrFeature);

				if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
					return RETURN_ABORTED;
				}
				return RETURN_DEVICE_ERROR;
			}

			if ((AltRegister & ATA_STSREG_DRQ) == ATA_STSREG_DRQ) {
				return RETURN_SUCCESS;
			}
			else {
				return RETURN_NOT_READY;
			}
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;
	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}

/**
This function is used to poll for the DRDY bit set in the Status Register. DRDY
bit is set when the device is ready to accept command. Most ATA commands must be
sent after DRDY set except the ATAPI Packet Command.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS         DRDY bit set within the time out.
@retval RETURN_TIMEOUT         DRDY bit not set within the time out.

@note  Read Status Register will clear interrupt status.
**/
RETURN_STATUS
DRDYReady(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   StatusRegister;
	UINT8   ErrorRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		StatusRegister = IdeReadPortB(IdeRegisters->CmdOrStatus);
		//
		// Wait for BSY == 0, then judge if DRDY is set or ERR is set
		//
		if ((StatusRegister & ATA_STSREG_BSY) == 0) {
			if ((StatusRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
				ErrorRegister = IdeReadPortB(IdeRegisters->ErrOrFeature);

				if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
					return RETURN_ABORTED;
				}
				return RETURN_DEVICE_ERROR;
			}

			if ((StatusRegister & ATA_STSREG_DRDY) == ATA_STSREG_DRDY) {
				return RETURN_SUCCESS;
			}
			else {
				return RETURN_DEVICE_ERROR;
			}
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;
	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}

/**
This function is used to poll for the DRDY bit set in the Alternate Status Register.
DRDY bit is set when the device is ready to accept command. Most ATA commands must
be sent after DRDY set except the ATAPI Packet Command.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS      DRDY bit set within the time out.
@retval RETURN_TIMEOUT      DRDY bit not set within the time out.

@note  Read Alternate Status Register will clear interrupt status.

**/
RETURN_STATUS
DRDYReady2(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   AltRegister;
	UINT8   ErrorRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		AltRegister = IdeReadPortB(IdeRegisters->AltOrDev);
		//
		// Wait for BSY == 0, then judge if DRDY is set or ERR is set
		//
		if ((AltRegister & ATA_STSREG_BSY) == 0) {
			if ((AltRegister & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
				ErrorRegister = IdeReadPortB(IdeRegisters->ErrOrFeature);

				if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
					return RETURN_ABORTED;
				}
				return RETURN_DEVICE_ERROR;
			}

			if ((AltRegister & ATA_STSREG_DRDY) == ATA_STSREG_DRDY) {
				return RETURN_SUCCESS;
			}
			else {
				return RETURN_DEVICE_ERROR;
			}
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;
	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}

/**
This function is used to poll for the BSY bit clear in the Status Register. BSY
is clear when the device is not busy. Every command must be sent after device is not busy.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS          BSY bit clear within the time out.
@retval RETURN_TIMEOUT          BSY bit not clear within the time out.

@note Read Status Register will clear interrupt status.
**/
RETURN_STATUS
WaitForBSYClear(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   StatusRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		StatusRegister = IdeReadPortB(IdeRegisters->CmdOrStatus);

		if ((StatusRegister & ATA_STSREG_BSY) == 0x00) {
			return RETURN_SUCCESS;
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;

	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}

/**
This function is used to poll for the BSY bit clear in the Status Register. BSY
is clear when the device is not busy. Every command must be sent after device is not busy.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS          BSY bit clear within the time out.
@retval RETURN_TIMEOUT          BSY bit not clear within the time out.

@note Read Status Register will clear interrupt status.
**/
RETURN_STATUS
WaitForBSYClear2(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT64  Delay;
	UINT8   AltStatusRegister;
	BOOLEAN InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;
	do {
		AltStatusRegister = IdeReadPortB(IdeRegisters->AltOrDev);

		if ((AltStatusRegister & ATA_STSREG_BSY) == 0x00) {
			return RETURN_SUCCESS;
		}

		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);

		Delay--;

	} while (InfiniteWait || (Delay > 0));

	return RETURN_TIMEOUT;
}

/**
Get IDE i/o port registers' base addresses by mode.

In 'Compatibility' mode, use fixed addresses.
In Native-PCI mode, get base addresses from BARs in the PCI IDE controller's
Configuration Space.

The steps to get IDE i/o port registers' base addresses for each channel
as follows:

1. Examine the Programming Interface byte of the Class Code fields in PCI IDE
controller's Configuration Space to determine the operating mode.

2. a) In 'Compatibility' mode, use fixed addresses shown in the Table 1 below.
___________________________________________
|           | Command Block | Control Block |
|  Channel  |   Registers   |   Registers   |
|___________|_______________|_______________|
|  Primary  |  1F0h - 1F7h  |  3F6h - 3F7h  |
|___________|_______________|_______________|
| Secondary |  170h - 177h  |  376h - 377h  |
|___________|_______________|_______________|

Table 1. Compatibility resource mappings

b) In Native-PCI mode, IDE registers are mapped into IO space using the BARs
in IDE controller's PCI Configuration Space, shown in the Table 2 below.
___________________________________________________
|           |   Command Block   |   Control Block   |
|  Channel  |     Registers     |     Registers     |
|___________|___________________|___________________|
|  Primary  | BAR at offset 0x10| BAR at offset 0x14|
|___________|___________________|___________________|
| Secondary | BAR at offset 0x18| BAR at offset 0x1C|
|___________|___________________|___________________|

Table 2. BARs for Register Mapping

@param[in]      Dev          Pointer to the DEVICE_NODE Dev
@param[in, out] IdeRegisters    Pointer to IDE_REGISTERS which is used to
store the IDE i/o port registers' base addresses

@retval RETURN_UNSUPPORTED        Return this value when the BARs is not IO type
@retval RETURN_SUCCESS            Get the Base address successfully
@retval Other                  Read the pci configureation data error

**/
RETURN_STATUS
GetIdeRegisterIoAddr(
	IN OUT     DEVICE_NODE              *Dev
	)
{
	RETURN_STATUS     Status;
	PCI_TYPE00        *PciData;
	UINT16            CommandBlockBaseAddr;
	UINT16            ControlBlockBaseAddr;
	UINT16            BusMasterBaseAddr;
	PCI_DEVICE *PciDev = Dev->Device;
	UINT32			  Index;

	if ((Dev == NULL)) {
		return RETURN_INVALID_PARAMETER;
	}

	IDE_REGISTERS * IdeRegisters = KMalloc(2 * sizeof(IDE_REGISTERS));
	Dev->PrivateData = IdeRegisters;

	//PciData = (PCI_TYPE00*)GetPciCfgAddress(PciDev, 0);
	PciData = KMalloc(0x100);
	UINT32 * PciCfgPtr = (UINT32 *)PciData;
	for (Index = 0; Index < 0x100 / 4; Index++)
		PciCfgPtr[Index] = PciCfgRead32(PciDev, Index);
	
	BusMasterBaseAddr = (UINT16)((PciData->Device.Bar[4] & 0x0000fff0));

	if ((PciData->Hdr.ClassCode[0] & IDE_PRIMARY_OPERATING_MODE) == 0) {
		CommandBlockBaseAddr = 0x1f0;
		ControlBlockBaseAddr = 0x3f6;
	}
	else {
		//
		// The BARs should be of IO type
		//
		if ((PciData->Device.Bar[0] & BIT0) == 0 ||
			(PciData->Device.Bar[1] & BIT0) == 0) {
			return RETURN_UNSUPPORTED;
		}

		CommandBlockBaseAddr = (UINT16)(PciData->Device.Bar[0] & 0x0000fff8);
		ControlBlockBaseAddr = (UINT16)((PciData->Device.Bar[1] & 0x0000fffc) + 2);
	}

	//
	// Calculate IDE primary channel I/O register base address.
	//
	IdeRegisters[0].Data = CommandBlockBaseAddr;
	IdeRegisters[0].ErrOrFeature = (UINT16)(CommandBlockBaseAddr + 0x01);
	IdeRegisters[0].SectorCount = (UINT16)(CommandBlockBaseAddr + 0x02);
	IdeRegisters[0].SectorNumber = (UINT16)(CommandBlockBaseAddr + 0x03);
	IdeRegisters[0].CylinderLsb = (UINT16)(CommandBlockBaseAddr + 0x04);
	IdeRegisters[0].CylinderMsb = (UINT16)(CommandBlockBaseAddr + 0x05);
	IdeRegisters[0].Head = (UINT16)(CommandBlockBaseAddr + 0x06);
	IdeRegisters[0].CmdOrStatus = (UINT16)(CommandBlockBaseAddr + 0x07);
	IdeRegisters[0].AltOrDev = ControlBlockBaseAddr;
	IdeRegisters[0].BusMasterBaseAddr = BusMasterBaseAddr;

	KDEBUG("IDE Channel [%d], Command Port:%04x\n",0, IdeRegisters[0].Data);
	KDEBUG("IDE Channel [%d], ControlBlock Port:%04x\n", 0, IdeRegisters[0].AltOrDev);
	KDEBUG("IDE Channel [%d], BusMasterBase Port:%04x\n", 0, IdeRegisters[0].BusMasterBaseAddr);

	if ((PciData->Hdr.ClassCode[0] & IDE_SECONDARY_OPERATING_MODE) == 0) {
		CommandBlockBaseAddr = 0x170;
		ControlBlockBaseAddr = 0x376;
	}
	else {
		//
		// The BARs should be of IO type
		//
		if ((PciData->Device.Bar[2] & BIT0) == 0 ||
			(PciData->Device.Bar[3] & BIT0) == 0) {
			return RETURN_UNSUPPORTED;
		}

		CommandBlockBaseAddr = (UINT16)(PciData->Device.Bar[2] & 0x0000fff8);
		ControlBlockBaseAddr = (UINT16)((PciData->Device.Bar[3] & 0x0000fffc) + 2);
	}

	//
	// Calculate IDE secondary channel I/O register base address.
	//
	IdeRegisters[1].Data = CommandBlockBaseAddr;
	IdeRegisters[1].ErrOrFeature = (UINT16)(CommandBlockBaseAddr + 0x01);
	IdeRegisters[1].SectorCount = (UINT16)(CommandBlockBaseAddr + 0x02);
	IdeRegisters[1].SectorNumber = (UINT16)(CommandBlockBaseAddr + 0x03);
	IdeRegisters[1].CylinderLsb = (UINT16)(CommandBlockBaseAddr + 0x04);
	IdeRegisters[1].CylinderMsb = (UINT16)(CommandBlockBaseAddr + 0x05);
	IdeRegisters[1].Head = (UINT16)(CommandBlockBaseAddr + 0x06);
	IdeRegisters[1].CmdOrStatus = (UINT16)(CommandBlockBaseAddr + 0x07);
	IdeRegisters[1].AltOrDev = ControlBlockBaseAddr;
	IdeRegisters[1].BusMasterBaseAddr = (UINT16)(BusMasterBaseAddr + 0x8);

	KDEBUG("IDE Channel [%d], Command Port:%04x\n", 1, IdeRegisters[1].Data);
	KDEBUG("IDE Channel [%d], ControlBlock Port:%04x\n", 1, IdeRegisters[1].AltOrDev);
	KDEBUG("IDE Channel [%d], BusMasterBase Port:%04x\n", 1, IdeRegisters[1].BusMasterBaseAddr);

	return RETURN_SUCCESS;
}

/**
This function is used to implement the Soft Reset on the specified device. But,
the ATA Soft Reset mechanism is so strong a reset method that it will force
resetting on both devices connected to the same cable.

It is called by IdeBlkIoReset(), a interface function of Block
I/O protocol.

This function can also be used by the ATAPI device to perform reset when
ATAPI Reset command is failed.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval RETURN_SUCCESS       Soft reset completes successfully.
@retval RETURN_DEVICE_ERROR  Any step during the reset process is failed.

@note  The registers initial values after ATA soft reset are different
to the ATA device and ATAPI device.
**/
RETURN_STATUS
AtaSoftReset(
	IN  DEVICE_NODE           *Dev,
	IN  UINT32				 Channel,
	IN  UINT64               Timeout
	)
{
	UINT8 DeviceControl;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	DeviceControl = 0;
	//
	// disable Interrupt and set SRST bit to initiate soft reset
	//
	DeviceControl = ATA_CTLREG_SRST | ATA_CTLREG_IEN_L;

	IdeWritePortB(IdeRegisters->AltOrDev, DeviceControl);

	//
	// SRST should assert for at least 5 us, we use 10 us for
	// better compatibility
	//
	MicroSecondDelay(10);

	//
	// Enable interrupt to support UDMA, and clear SRST bit
	//
	DeviceControl = 0;
	IdeWritePortB(IdeRegisters->AltOrDev, DeviceControl);

	//
	// Wait for at least 10 ms to check BSY status, we use 10 ms
	// for better compatibility
	//
	MicroSecondDelay(10);

	//
	// slave device needs at most 31ms to clear BSY
	//
	if (WaitForBSYClear(Dev, Channel, Timeout) == RETURN_TIMEOUT) {
		return RETURN_DEVICE_ERROR;
	}

	return RETURN_SUCCESS;
}

/**
Send ATA Ext command into device with NON_DATA protocol.

@param Dev            A pointer to DEVICE_NODE data structure.
@param IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param AtaCommandBlock  A pointer to ATA_COMMAND_BLOCK data structure.
@param Timeout          The time to complete the command, uses 100ns as a unit.

@retval  RETURN_SUCCESS Reading succeed
@retval  RETURN_DEVICE_ERROR Error executing commands on this device.

**/
RETURN_STATUS
AtaIssueCommand(
	IN  DEVICE_NODE                *Dev,
	IN  UINT32				      Channel,
	IN  ATA_COMMAND_BLOCK         *AtaCommandBlock,
	IN  UINT64                    Timeout
	)
{
	RETURN_STATUS  Status;
	UINT8       DeviceHead;
	UINT8       AtaCommand;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	DeviceHead = AtaCommandBlock->AtaDeviceHead;
	AtaCommand = AtaCommandBlock->AtaCommand;

	Status = WaitForBSYClear(Dev, Channel, Timeout);
	KDEBUG("Line = %d,Status = %x\n", __LINE__, Status);

	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	//
	// Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
	//
	IdeWritePortB(IdeRegisters->Head, (UINT8)(0xe0 | DeviceHead));

	//
	// set all the command parameters
	// Before write to all the following registers, BSY and DRQ must be 0.
	//
	Status = DRQClear2(Dev, Channel, Timeout);

	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	//KDEBUG("IdeRegisters->CmdOrStatus = %x\n", IdeRegisters->CmdOrStatus);

	//
	// Fill the feature register, which is a two-byte FIFO. Need write twice.
	//
	IdeWritePortB(IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeaturesExp);
	IdeWritePortB(IdeRegisters->ErrOrFeature, AtaCommandBlock->AtaFeatures);

	//
	// Fill the sector count register, which is a two-byte FIFO. Need write twice.
	//
	IdeWritePortB(IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCountExp);
	IdeWritePortB(IdeRegisters->SectorCount, AtaCommandBlock->AtaSectorCount);

	//
	// Fill the start LBA registers, which are also two-byte FIFO
	//
	IdeWritePortB(IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumberExp);
	IdeWritePortB(IdeRegisters->SectorNumber, AtaCommandBlock->AtaSectorNumber);

	IdeWritePortB(IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLowExp);
	IdeWritePortB(IdeRegisters->CylinderLsb, AtaCommandBlock->AtaCylinderLow);

	IdeWritePortB(IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHighExp);
	IdeWritePortB(IdeRegisters->CylinderMsb, AtaCommandBlock->AtaCylinderHigh);

	//
	// Send command via Command Register
	//
	IdeWritePortB(IdeRegisters->CmdOrStatus, AtaCommand);

	//
	// Stall at least 400 microseconds.
	//
	MicroSecondDelay(400);

	return RETURN_SUCCESS;
}

/**
This function is used to send out ATA commands conforms to the PIO Data In Protocol.

@param[in]      Dev            A pointer to DEVICE_NODE data
structure.
@param[in]      IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param[in, out] Buffer           A pointer to the source buffer for the data.
@param[in]      ByteCount        The length of the data.
@param[in]      Read             Flag used to determine the data transfer direction.
Read equals 1, means data transferred from device
to host;Read equals 0, means data transferred
from host to device.
@param[in]      AtaCommandBlock  A pointer to ATA_COMMAND_BLOCK data structure.
@param[in, out] AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.
@param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
@param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
used by non-blocking mode.

@retval RETURN_SUCCESS      send out the ATA command and device send required data successfully.
@retval RETURN_DEVICE_ERROR command sent failed.

**/
RETURN_STATUS
AtaPioDataInOut(
	IN     DEVICE_NODE                *Dev,
	IN     UINT32				     Channel,
 	IN OUT VOID                      *Buffer,
	IN     UINT64                    ByteCount,
	IN     BOOLEAN                   Read,
	IN     ATA_COMMAND_BLOCK         *AtaCommandBlock,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock,
	IN     UINT64                    Timeout,
	IN     BOOLEAN		             NonBlock
	)
{
	UINT32       WordCount;
	UINT32       Increment;
	UINT16      *Buffer16;
	RETURN_STATUS  Status;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if ((Dev == NULL) || (IdeRegisters == NULL) || (Buffer == NULL) || (AtaCommandBlock == NULL)) {
		return RETURN_INVALID_PARAMETER;
	}

	//
	// Issue ATA command
	//
	Status = AtaIssueCommand(Dev, Channel, AtaCommandBlock, Timeout);
	if (RETURN_ERROR(Status)) {
		Status = RETURN_DEVICE_ERROR;
		goto Exit;
	}

	Buffer16 = (UINT16 *)Buffer;

	//
	// According to PIO data in protocol, host can perform a series of reads to
	// the data register after each time device set DRQ ready;
	// The data size of "a series of read" is command specific.
	// For most ATA command, data size received from device will not exceed
	// 1 sector, hence the data size for "a series of read" can be the whole data
	// size of one command request.
	// For ATA command such as Read Sector command, the data size of one ATA
	// command request is often larger than 1 sector, according to the
	// Read Sector command, the data size of "a series of read" is exactly 1
	// sector.
	// Here for simplification reason, we specify the data size for
	// "a series of read" to 1 sector (256 words) if data size of one ATA command
	// request is larger than 256 words.
	//
	Increment = 256;

	//
	// used to record bytes of currently transfered data
	//
	WordCount = 0;

	while (WordCount < RShiftU64(ByteCount, 1)) {

		//
		// Poll DRQ bit set, data transfer can be performed only when DRQ is ready
		//
		Status = DRQReady2(Dev, Channel, Timeout);
		if (RETURN_ERROR(Status)) {
			Status = RETURN_DEVICE_ERROR;
			goto Exit;
		}

		//
		// Get the byte count for one series of read
		//
		if ((WordCount + Increment) > RShiftU64(ByteCount, 1)) {
			Increment = (UINT32)(RShiftU64(ByteCount, 1) - WordCount);
		}

		if (Read) {
			IdeReadPortWMultiple(
				IdeRegisters->Data,
				Increment,
				Buffer16
				);
		}
		else {
			IdeWritePortWMultiple(
				IdeRegisters->Data,
				Increment,
				Buffer16
				);
		}

		Status = CheckStatusRegister(Dev, Channel);
		if (RETURN_ERROR(Status)) {
			KDEBUG("FUNCTION = %s, LINE = %d,Status: %x\n", __FUNCTION__, __LINE__, Status);
			Status = RETURN_DEVICE_ERROR;
			goto Exit;
		}

		WordCount += Increment;
		Buffer16 += Increment;
	}

	Status = DRQClear(Dev, Channel, Timeout);
	if (RETURN_ERROR(Status)) {
		Status = RETURN_DEVICE_ERROR;
		goto Exit;
	}

Exit:
	//
	// Dump All Ide registers to ATA_STATUS_BLOCK
	//
	DumpAllIdeRegisters(Dev, Channel, AtaStatusBlock);

	//
	// Not support the Non-blocking now,just do the blocking process.
	//
	return Status;
}

/**
Send ATA command into device with NON_DATA protocol

@param[in]      Dev            A pointer to DEVICE_NODE
data structure.
@param[in]      IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param[in]      AtaCommandBlock  A pointer to ATA_COMMAND_BLOCK data
structure.
@param[in, out] AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.
@param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
@param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
used by non-blocking mode.

@retval  RETURN_SUCCESS Reading succeed
@retval  RETURN_ABORTED Command failed
@retval  RETURN_DEVICE_ERROR Device status error.

**/
RETURN_STATUS

AtaNonDataCommandIn(
	IN     DEVICE_NODE            *Dev,
	IN     UINT32				 Channel,
	IN     ATA_COMMAND_BLOCK     *AtaCommandBlock,
	IN OUT ATA_STATUS_BLOCK      *AtaStatusBlock,
	IN     UINT64                Timeout,
	IN     BOOLEAN               NonBlock
	)
{
	RETURN_STATUS  Status;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if ((Dev == NULL) || (IdeRegisters == NULL) || (AtaCommandBlock == NULL)) {
		return RETURN_INVALID_PARAMETER;
	}

	//
	// Issue ATA command
	//
	Status = AtaIssueCommand(Dev, Channel, AtaCommandBlock, Timeout);
	if (RETURN_ERROR(Status)) {
		Status = RETURN_DEVICE_ERROR;
		goto Exit;
	}

	//
	// Wait for command completion
	//
	Status = WaitForBSYClear(Dev, Channel, Timeout);
	if (RETURN_ERROR(Status)) {
		Status = RETURN_DEVICE_ERROR;
		goto Exit;
	}

	Status = CheckStatusRegister(Dev, Channel);
	if (RETURN_ERROR(Status)) {
		Status = RETURN_DEVICE_ERROR;
		goto Exit;
	}

Exit:
	//
	// Dump All Ide registers to ATA_STATUS_BLOCK
	//
	DumpAllIdeRegisters(Dev, Channel, AtaStatusBlock);

	//
	// Not support the Non-blocking now,just do the blocking process.
	//
	return Status;
}

/**
Wait for memory to be set.

@param[in]  Dev           The PCI IO protocol Dev.
@param[in]  IdeRegisters    A pointer to IDE_REGISTERS data structure.
@param[in]  Timeout         The time to complete the command, uses 100ns as a unit.

@retval RETURN_DEVICE_ERROR  The memory is not set.
@retval RETURN_TIMEOUT       The memory setting is time out.
@retval RETURN_SUCCESS       The memory is correct set.

**/
RETURN_STATUS
AtaUdmStatusWait(
	IN  DEVICE_NODE                *Dev,
	IN  UINT32				      Channel,
	IN  UINT64                    Timeout
	)
{
	UINT8                         RegisterValue;
	RETURN_STATUS                 Status;
	UINT16                        IoPortForBmis;
	UINT64                        Delay;
	BOOLEAN                       InfiniteWait;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	if (Timeout == 0) {
		InfiniteWait = TRUE;
	}
	else {
		InfiniteWait = FALSE;
	}

	Delay = DivU64x32(Timeout, 1000) + 1;

	do {
		Status = CheckStatusRegister(Dev, Channel);
		if (RETURN_ERROR(Status)) {
			Status = RETURN_DEVICE_ERROR;
			break;
		}

		IoPortForBmis = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIS_OFFSET);
		RegisterValue = IdeReadPortB(IoPortForBmis);
		if (((RegisterValue & BMIS_ERROR) != 0)) {
			KDEBUG("ATA UDMA operation failed.BMIS = %x\n", RegisterValue);
			Status = RETURN_DEVICE_ERROR;
			break;
		}

		if ((RegisterValue & BMIS_INTERRUPT) != 0) {
			if ((RegisterValue & BIT0) == 0)
			{
				Status = RETURN_SUCCESS;
				break;
			}
		}
		//
		// Stall for 100 microseconds.
		//
		MicroSecondDelay(100);
		Delay--;
	} while (InfiniteWait || (Delay > 0));

	return Status;
}

/**
Check if the memory to be set.

@param[in]  Dev           The PCI IO protocol Dev.
@param[in]  Task            Optional. Pointer to the ATA_NONBLOCK_TASK
used by non-blocking mode.
@param[in]  IdeRegisters    A pointer to IDE_REGISTERS data structure.

@retval RETURN_DEVICE_ERROR  The memory setting met a issue.
@retval RETURN_NOT_READY     The memory is not set.
@retval RETURN_TIMEOUT       The memory setting is time out.
@retval RETURN_SUCCESS       The memory is correct set.

**/
RETURN_STATUS
AtaUdmStatusCheck(
	IN     DEVICE_NODE             *Dev,
	IN     UINT32				  Channel,
	IN     BOOLEAN                NonBlock
	)
{
	UINT8          RegisterValue;
	UINT16         IoPortForBmis;
	RETURN_STATUS     Status;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	Status = CheckStatusRegister(Dev, Channel);
	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	IoPortForBmis = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIS_OFFSET);
	RegisterValue = IdeReadPortB(IoPortForBmis);

	if ((RegisterValue & BMIS_ERROR) != 0) {
		KDEBUG("ATA UDMA operation fails\n");
		return RETURN_DEVICE_ERROR;
	}

	if ((RegisterValue & BMIS_INTERRUPT) != 0) {
		return RETURN_SUCCESS;
	}

	//
	// The memory is not set.
	//
	return RETURN_NOT_READY;
	
}

/**
Perform an ATA Udma operation (Read, ReadExt, Write, WriteExt).

@param[in]      Dev         A pointer to DEVICE_NODE data
structure.
@param[in]      IdeRegisters     A pointer to IDE_REGISTERS data structure.
@param[in]      Read             Flag used to determine the data transfer
direction. Read equals 1, means data transferred
from device to host;Read equals 0, means data
transferred from host to device.
@param[in]      DataBuffer       A pointer to the source buffer for the data.
@param[in]      DataLength       The length of  the data.
@param[in]      AtaCommandBlock  A pointer to ATA_COMMAND_BLOCK data structure.
@param[in, out] AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.
@param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
@param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
used by non-blocking mode.

@retval RETURN_SUCCESS          the operation is successful.
@retval RETURN_OUT_OF_RESOURCES Build PRD table failed
@retval RETURN_UNSUPPORTED      Unknown channel or operations command
@retval RETURN_DEVICE_ERROR     Ata command execute failed

**/
RETURN_STATUS
AtaUdmaInOut(
	IN     DEVICE_NODE                *Dev,
	IN     UINT32				     Channel,
	IN     BOOLEAN                   Read,
	IN     BOOLEAN                   NonBlock,
	IN     ATA_COMMAND_BLOCK         *AtaCommandBlock,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock,
	IN     VOID                      *DataBuffer,
	IN     UINT64                    DataLength,
	IN     UINT64                    Timeout
	)
{
	RETURN_STATUS                 Status;
	UINT16                        IoPortForBmic;
	UINT16                        IoPortForBmis;
	UINT16                        IoPortForBmid;

	UINT32                        PrdTableSize;
	VOID                          *PrdTableMapAddr;
	VOID                          *PrdTableBaseAddr;
	ATA_DMA_PRD                   *TempPrdBaseAddr;
	UINT32                        PrdTableNum;

	UINT8                         RegisterValue;
	
	UINT32                        ByteCount;
	UINT32                        ByteRemaining;
	UINT8                         DeviceControl;

	VOID                          *BufferMap;
	UINT8                         *BufferMapAddress;

	UINT8                         DeviceHead;

	UINT32                        AlignmentMask;
	UINT32                        RealPageCount;
	VOID                          *BaseAddr;
	VOID                          *BaseMapAddr;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	Status = RETURN_SUCCESS;
	BufferMap = NULL;
	RealPageCount = 0;
	BaseAddr = 0;
	
	if ((Dev == NULL) || (IdeRegisters == NULL) || (DataBuffer == NULL) || (AtaCommandBlock == NULL)) {
		return RETURN_INVALID_PARAMETER;
	}

	//
	// Before starting the Blocking BlockIO operation, push to finish all non-blocking
	// BlockIO tasks.
	// Delay 1ms to simulate the blocking time out checking.
	//
	if (NonBlock == FALSE)
	{
		MicroSecondDelay(1000);
	}

	//
	// The data buffer should be even alignment
	//
	if (((UINT64)DataBuffer & 0x1) != 0) {
		return RETURN_INVALID_PARAMETER;
	}

	//
	// Set relevant IO Port address.
	//
	IoPortForBmic = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIC_OFFSET);
	IoPortForBmis = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMIS_OFFSET);
	IoPortForBmid = (UINT16)(IdeRegisters->BusMasterBaseAddr + BMID_OFFSET);

	//
	// For Blocking mode, start the command.
	// For non-blocking mode, when the command is not started, start it, otherwise
	// go to check the status.
	//
	if (NonBlock == FALSE) {
		//
		// Calculate the number of PRD entry.
		// Every entry in PRD table can specify a 64K memory region.
		//
		PrdTableNum = (UINT32)(RShiftU64(DataLength, 16) + 1);

		//
		// Make sure that the memory region of PRD table is not cross 64K boundary
		//
		PrdTableSize = PrdTableNum * sizeof(ATA_DMA_PRD);
		if (PrdTableSize > 0x10000) {
			return RETURN_INVALID_PARAMETER;
		}

		//
		// Allocate buffer for PRD table initialization.
		// Note Ide Bus Master spec said the descriptor table must be aligned on a 4 byte
		// boundary and the table cannot cross a 64K boundary in memory.
		//
		PrdTableBaseAddr = KMalloc(SIZE_64KB);
		memset(PrdTableBaseAddr, 0, SIZE_64KB);
		PrdTableMapAddr = (VOID *)VIRT2PHYS((UINT64)PrdTableBaseAddr);

		BufferMapAddress = (VOID *)VIRT2PHYS((UINT64)DataBuffer);

		KDEBUG("PrdTableMapAddr = %016x, BufferMapAddress = %016x\n", PrdTableMapAddr, BufferMapAddress);

		//
		// Make sure that PageCount plus RETURN_SIZE_TO_PAGES (SIZE_64KB) does not overflow.
		//
		
		//
		// Calculate the 64K align address as PRD Table base address.
		//
		//ByteCount = (DataLength / SIZE_64KB + 1) * SIZE_64KB;
		//
		// Fill the PRD table with appropriate bus master address of data buffer and data length.
		//
		ByteRemaining = DataLength;

		KDEBUG("ByteRemaining = %d\n", ByteRemaining);

		TempPrdBaseAddr = (ATA_DMA_PRD*)PrdTableBaseAddr;
		while (ByteRemaining != 0) {
			if (ByteRemaining <= 0x10000) {
				TempPrdBaseAddr->RegionBaseAddr = (UINT64)BufferMapAddress;
				TempPrdBaseAddr->ByteCount = (UINT16)ByteRemaining;
				TempPrdBaseAddr->EndOfTable = 0x8000;
				break;
			}

			TempPrdBaseAddr->RegionBaseAddr = (UINT64)BufferMapAddress;
			TempPrdBaseAddr->ByteCount = (UINT16)0x0;

			ByteRemaining -= 0x10000;
			BufferMapAddress += 0x10000;
			TempPrdBaseAddr++;
		}

		//
		// Start to enable the DMA operation
		//
		DeviceHead = AtaCommandBlock->AtaDeviceHead;
		IdeWritePortB(IdeRegisters->Head, (UINT8)(0xe0 | DeviceHead));

		//
		// Enable interrupt to support UDMA
		//
		DeviceControl = 0;
		IdeWritePortB(IdeRegisters->AltOrDev, DeviceControl);

		//
		// Read BMIS register and clear ERROR and INTR bit
		//
		RegisterValue = IdeReadPortB(IoPortForBmis);
		RegisterValue |= (BMIS_INTERRUPT | BMIS_ERROR);
		IdeWritePortB(IoPortForBmis, RegisterValue);

		//
		// Set the base address to BMID register
		//
		IdeWritePortDW(IoPortForBmid, (UINT64)PrdTableMapAddr);

		//
		// Set BMIC register to identify the operation direction
		//
		RegisterValue = IdeReadPortB(IoPortForBmic);
		if (Read) {
			RegisterValue |= BMIC_NREAD;
		}
		else {
			RegisterValue &= ~((UINT8)BMIC_NREAD);
		}
		IdeWritePortB(IoPortForBmic, RegisterValue);

		//
		// Issue ATA command
		//
		Status = AtaIssueCommand(Dev, Channel, AtaCommandBlock, Timeout);

		if (RETURN_ERROR(Status)) {
			Status = RETURN_DEVICE_ERROR;
			goto Exit;
		}

		Status = CheckStatusRegister(Dev, Channel);
		if (RETURN_ERROR(Status)) {
			Status = RETURN_DEVICE_ERROR;
			goto Exit;
		}
		//
		// Set START bit of BMIC register
		//
		RegisterValue = IdeReadPortB(IoPortForBmic);
		RegisterValue |= BMIC_START;
		IdeWritePortB(IoPortForBmic, RegisterValue);

	}

	//
	// Check the INTERRUPT and ERROR bit of BMIS
	//
	if (NonBlock == TRUE) {
		Status = AtaUdmStatusCheck(Dev, Channel, NonBlock);
	}
	else {
		Status = AtaUdmStatusWait(Dev, Channel, 0);
	}

	//MicroSecondDelay(100000);
	//
	// For blocking mode, clear registers and free buffers.
	// For non blocking mode, when the related registers have been set or time
	// out, or a error has been happened, it needs to clear the register and free
	// Buffer.
	//
	if ((NonBlock == FALSE)) {
		//
		// Read BMIS register and clear ERROR and INTR bit
		//
		RegisterValue = IdeReadPortB(IoPortForBmis);
		RegisterValue |= (BMIS_INTERRUPT | BMIS_ERROR);
		IdeWritePortB(IoPortForBmis, RegisterValue);

		//
		// Read Status Register of IDE device to clear interrupt
		//
		RegisterValue = IdeReadPortB(IdeRegisters->CmdOrStatus);

		//
		// Clear START bit of BMIC register
		//
		RegisterValue = IdeReadPortB(IoPortForBmic);
		RegisterValue &= ~((UINT8)BMIC_START);
		IdeWritePortB(IoPortForBmic, RegisterValue);

		//
		// Disable interrupt of Select device
		//
		DeviceControl = IdeReadPortB(IdeRegisters->AltOrDev);
		DeviceControl |= ATA_CTLREG_IEN_L;
		IdeWritePortB(IdeRegisters->AltOrDev, DeviceControl);
		//
		// Stall for 10 milliseconds.
		//
		//MicroSecondDelay(10000);

	}

Exit:
	
	if ((NonBlock == FALSE) || Status != RETURN_NOT_READY) {
		
		//
		// Dump All Ide registers to ATA_STATUS_BLOCK
		//
		DumpAllIdeRegisters(Dev, Channel, AtaStatusBlock);

		//
		// Free all allocated resource
		//
		KFree(PrdTableBaseAddr);
	}

	if (Status) {
		AtaSoftReset(Dev, Channel, Timeout);
	}

	return Status;
}

/**
This function reads the pending data in the device.

@param Dev         A pointer to DEVICE_NODE data structure.
@param IdeRegisters  A pointer to IDE_REGISTERS data structure.

@retval RETURN_SUCCESS   Successfully read.
@retval RETURN_NOT_READY The BSY is set avoiding reading.

**/
RETURN_STATUS
AtaPacketReadPendingData(
	IN  DEVICE_NODE            *Dev,
	IN  UINT32				  Channel
	)
{
	UINT8     AltRegister;
	UINT16    TempWordBuffer;
	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	AltRegister = IdeReadPortB(IdeRegisters->AltOrDev);
	if ((AltRegister & ATA_STSREG_BSY) == ATA_STSREG_BSY) {
		return RETURN_NOT_READY;
	}

	if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
		TempWordBuffer = IdeReadPortB(IdeRegisters->AltOrDev);
		while ((TempWordBuffer & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
			IdeReadPortWMultiple(
				IdeRegisters->Data,
				1,
				&TempWordBuffer
				);
			TempWordBuffer = IdeReadPortB(IdeRegisters->AltOrDev);
		}
	}
	return RETURN_SUCCESS;
}

/**
This function is called by AtaPacketCommandExecute().
It is used to transfer data between host and device. The data direction is specified
by the fourth parameter.

@param Dev         A pointer to DEVICE_NODE data structure.
@param IdeRegisters  A pointer to IDE_REGISTERS data structure.
@param Buffer        Buffer contained data transferred between host and device.
@param ByteCount     Data size in byte unit of the Buffer.
@param Read          Flag used to determine the data transfer direction.
Read equals 1, means data transferred from device to host;
Read equals 0, means data transferred from host to device.
@param Timeout       Timeout value for wait DRQ ready before each data stream's transfer
, uses 100ns as a unit.

@retval RETURN_SUCCESS      data is transferred successfully.
@retval RETURN_DEVICE_ERROR the device failed to transfer data.
**/
RETURN_STATUS
AtaPacketReadWrite(
	IN     DEVICE_NODE                *Dev,
	IN     UINT32				     Channel,
	IN OUT VOID                      *Buffer,
	IN     UINT64                    ByteCount,
	IN     BOOLEAN                   Read,
	IN     UINT64                    Timeout
	)
{
	UINT32      RequiredWordCount;
	UINT32      ActualWordCount;
	UINT32      WordCount;
	RETURN_STATUS  Status;
	UINT16      *PtrBuffer;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	//
	// No data transfer is premitted.
	//
	if (ByteCount == 0) {
		return RETURN_SUCCESS;
	}

	PtrBuffer = Buffer;
	RequiredWordCount = (UINT32)RShiftU64(ByteCount, 1);
	//
	// ActuralWordCount means the word count of data really transferred.
	//
	ActualWordCount = 0;

	while (ActualWordCount < RequiredWordCount) {
		//
		// before each data transfer stream, the host should poll DRQ bit ready,
		// to see whether indicates device is ready to transfer data.
		//
		Status = DRQReady2(Dev, Channel, Timeout);
		if (RETURN_ERROR(Status)) {
			return CheckStatusRegister(Dev, Channel);
		}

		//
		// get current data transfer size from Cylinder Registers.
		//
		WordCount = IdeReadPortB(IdeRegisters->CylinderMsb) << 8;
		WordCount = WordCount | IdeReadPortB(IdeRegisters->CylinderLsb);
		WordCount = WordCount & 0xffff;
		WordCount /= 2;

		WordCount = MIN(WordCount, (RequiredWordCount - ActualWordCount));

		if (Read) {
			IdeReadPortWMultiple(
				IdeRegisters->Data,
				WordCount,
				PtrBuffer
				);
		}
		else {
			IdeWritePortWMultiple(
				IdeRegisters->Data,
				WordCount,
				PtrBuffer
				);
		}

		//
		// read status register to check whether error happens.
		//
		Status = CheckStatusRegister(Dev, Channel);
		if (RETURN_ERROR(Status)) {
			return RETURN_DEVICE_ERROR;
		}

		PtrBuffer += WordCount;
		ActualWordCount += WordCount;
	}

	if (Read) {
		//
		// In the case where the drive wants to send more data than we need to read,
		// the DRQ bit will be set and cause delays from DRQClear2().
		// We need to read data from the drive until it clears DRQ so we can move on.
		//
		AtaPacketReadPendingData(Dev, Channel);
	}

	//
	// read status register to check whether error happens.
	//
	Status = CheckStatusRegister(Dev, Channel);
	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	//
	// After data transfer is completed, normally, DRQ bit should clear.
	//
	Status = DRQClear(Dev, Channel, Timeout);
	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	return Status;
}

/**
This function is used to send out ATAPI commands conforms to the Packet Command
with PIO Data In Protocol.

@param[in] Dev          Pointer to the DEVICE_NODE Dev
@param[in] IdeRegisters   Pointer to IDE_REGISTERS which is used to
store the IDE i/o port registers' base addresses
@param[in] Channel        The channel number of device.
@param[in] Device         The device number of device.
@param[in] Packet         A pointer to EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET data structure.

@retval RETURN_SUCCESS       send out the ATAPI packet command successfully
and device sends data successfully.
@retval RETURN_DEVICE_ERROR  the device failed to send data.

**/


//RETURN_STATUS
//AtaPacketCommandExecute(
//	IN  DEVICE_NODE                                *Dev,
//	IN  UINT32				                      Channel,
//	IN  UINT8                                     Device,
//	IN  EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET    *Packet
//	)
//{
//	ATA_COMMAND_BLOCK       AtaCommandBlock;
//	RETURN_STATUS                  Status;
//	UINT8                       Count;
//	UINT8                       PacketCommand[12];
//
//	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];
//
//	ZeroMem(&AtaCommandBlock, sizeof(ATA_COMMAND_BLOCK));
//
//	//
//	// Fill ATAPI Command Packet according to CDB.
//	// For Atapi cmd, its length should be less than or equal to 12 bytes.
//	//
//	if (Packet->CdbLength > 12) {
//		return RETURN_INVALID_PARAMETER;
//	}
//
//	ZeroMem(PacketCommand, 12);
//	CopyMem(PacketCommand, Packet->Cdb, Packet->CdbLength);
//
//	//
//	// No OVL; No DMA
//	//
//	AtaCommandBlock.AtaFeatures = 0x00;
//	//
//	// set the transfersize to ATAPI_MAX_BYTE_COUNT to let the device
//	// determine how many data should be transferred.
//	//
//	AtaCommandBlock.AtaCylinderLow = (UINT8)(ATAPI_MAX_BYTE_COUNT & 0x00ff);
//	AtaCommandBlock.AtaCylinderHigh = (UINT8)(ATAPI_MAX_BYTE_COUNT >> 8);
//	AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);
//	AtaCommandBlock.AtaCommand = ATA_CMD_PACKET;
//
//	IdeWritePortB(IdeRegisters->Head, (UINT8)(0xe0 | (Device << 0x4)));
//	//
//	//  Disable interrupt
//	//
//	IdeWritePortB(IdeRegisters->AltOrDev, ATA_DEFAULT_CTL);
//
//	//
//	// Issue ATA PACKET command firstly
//	//
//	Status = AtaIssueCommand(Dev, IdeRegisters, &AtaCommandBlock, Packet->Timeout);
//	if (RETURN_ERROR(Status)) {
//		return Status;
//	}
//
//	Status = DRQReady(Dev, IdeRegisters, Packet->Timeout);
//	if (RETURN_ERROR(Status)) {
//		return Status;
//	}
//
//	//
//	// Send out ATAPI command packet
//	//
//	for (Count = 0; Count < 6; Count++) {
//		IdeWritePortW(Dev, IdeRegisters->Data, *((UINT16*)PacketCommand + Count));
//		//
//		// Stall for 10 microseconds.
//		//
//		MicroSecondDelay(10);
//	}
//
//	//
//	// Read/Write the data of ATAPI Command
//	//
//	if (Packet->DataDirection == EXT_SCSI_DATA_DIRECTION_READ) {
//		Status = AtaPacketReadWrite(
//			Dev,
//			Channel,
//			Packet->InDataBuffer,
//			Packet->InTransferLength,
//			TRUE,
//			Packet->Timeout
//			);
//	}
//	else {
//		Status = AtaPacketReadWrite(
//			Dev,
//			Channel,
//			Packet->OutDataBuffer,
//			Packet->OutTransferLength,
//			FALSE,
//			Packet->Timeout
//			);
//	}
//
//	return Status;
//}


/**
Set the calculated Best transfer mode to a detected device.

@param Dev               A pointer to DEVICE_NODE data structure.
@param Channel                The channel number of device.
@param Device                 The device number of device.
@param TransferMode           A pointer to ATA_TRANSFER_MODE data structure.
@param AtaStatusBlock         A pointer to ATA_STATUS_BLOCK data structure.

@retval RETURN_SUCCESS          Set transfer mode successfully.
@retval RETURN_DEVICE_ERROR     Set transfer mode failed.
@retval RETURN_OUT_OF_RESOURCES Allocate memory failed.

**/
RETURN_STATUS
SetDeviceTransferMode(
	IN     DEVICE_NODE                *Dev,
	IN     UINT32                    Channel,
	IN     UINT8                     Device,
	IN     ATA_TRANSFER_MODE         *TransferMode,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock
	)
{
	RETURN_STATUS              Status;
	ATA_COMMAND_BLOCK   AtaCommandBlock;
	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_SET_FEATURES;
	AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);
	AtaCommandBlock.AtaFeatures = 0x03;
	AtaCommandBlock.AtaSectorCount = *((UINT8 *)TransferMode);

	//
	// Send SET FEATURE command (sub command 0x03) to set pio mode.
	//
	Status = AtaNonDataCommandIn(
		Dev,
		Channel,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT,
		FALSE
		);

	return Status;
}

/**
Set drive parameters for devices not support PACKETS command.

@param Dev         A pointer to DEVICE_NODE data structure.
@param Channel          The channel number of device.
@param Device           The device number of device.
@param DriveParameters  A pointer to RETURN_ATA_DRIVE_PARMS data structure.
@param AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.

@retval RETURN_SUCCESS          Set drive parameter successfully.
@retval RETURN_DEVICE_ERROR     Set drive parameter failed.
@retval RETURN_OUT_OF_RESOURCES Allocate memory failed.

**/
RETURN_STATUS
SetDriveParameters(
	IN     DEVICE_NODE				 *Dev,
	IN     UINT32                    Channel,
	IN     UINT8                     Device,
	IN     ATA_DRIVE_PARMS           *DriveParameters,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock
	)
{
	RETURN_STATUS              Status;
	ATA_COMMAND_BLOCK   AtaCommandBlock;

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_INIT_DRIVE_PARAM;
	AtaCommandBlock.AtaSectorCount = DriveParameters->Sector;
	AtaCommandBlock.AtaDeviceHead = (UINT8)((Device << 0x4) + DriveParameters->Heads);

	//
	// Send Init drive parameters
	//
	Status = AtaNonDataCommandIn(
		Dev,
		Channel,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT,
		FALSE
		);

	//
	// Send Set Multiple parameters
	//
	AtaCommandBlock.AtaCommand = ATA_CMD_SET_MULTIPLE_MODE;
	AtaCommandBlock.AtaSectorCount = DriveParameters->MultipleSector;
	AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

	Status = AtaNonDataCommandIn(
		Dev,
		Channel,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT,
		FALSE
		);

	return Status;
}

/**
Send SMART Return Status command to check if the execution of SMART cmd is successful or not.

@param Dev         A pointer to DEVICE_NODE data structure.
@param Channel          The channel number of device.
@param Device           The device number of device.
@param AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.

@retval RETURN_SUCCESS     Successfully get the return status of S.M.A.R.T command execution.
@retval Others          Fail to get return status data.

**/
RETURN_STATUS
IdeAtaSmartReturnStatusCheck(
	IN     DEVICE_NODE			     *Dev,
	IN     UINT32                    Channel,
	IN     UINT8                     Device,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock
	)
{
	RETURN_STATUS              Status;
	ATA_COMMAND_BLOCK   AtaCommandBlock;
	UINT8                   LBAMid;
	UINT8                   LBAHigh;

	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_SMART;
	AtaCommandBlock.AtaFeatures = ATA_SMART_RETURN_STATUS;
	AtaCommandBlock.AtaCylinderLow = ATA_CONSTANT_4F;
	AtaCommandBlock.AtaCylinderHigh = ATA_CONSTANT_C2;
	AtaCommandBlock.AtaDeviceHead = (UINT8)((Device << 0x4) | 0xe0);

	//
	// Send S.M.A.R.T Read Return Status command to device
	//
	Status = AtaNonDataCommandIn(
		Dev,
		Channel,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT,
		FALSE
		);

	if (RETURN_ERROR(Status)) {
		return RETURN_DEVICE_ERROR;
	}

	LBAMid = IdeReadPortB(IdeRegisters->CylinderLsb);
	LBAHigh = IdeReadPortB(IdeRegisters->CylinderMsb);

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

	return RETURN_SUCCESS;
}

/**
Enable SMART command of the Dev if supported.

@param Dev              A pointer to DEVICE_NODE data structure.
@param Channel          The channel number of device.
@param Device           The device number of device.
@param IdentifyData     A pointer to data buffer which is used to contain IDENTIFY data.
@param AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.

**/
VOID
IdeAtaSmartSupport(
	IN     DEVICE_NODE			     *Dev,
	IN     UINT32                    Channel,
	IN     UINT8                     Device,
	IN     IDENTIFY_DATA             *IdentifyData,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock
	)
{
	RETURN_STATUS              Status;
	ATA_COMMAND_BLOCK   AtaCommandBlock;
	IDE_REGISTERS *IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[Channel];

	//
	// Detect if the device supports S.M.A.R.T.
	//
	if ((IdentifyData->AtaData.command_set_supported_82 & 0x0001) != 0x0001) {
		//
		// S.M.A.R.T is not supported by the device
		//
		KDEBUG("S.M.A.R.T feature is not supported at [%s] channel [%s] device!\n",
			(Channel == 1) ? "secondary" : "primary", (Device == 1) ? "slave" : "master");
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
			AtaCommandBlock.AtaDeviceHead = (UINT8)((Device << 0x4) | 0xe0);

			//
			// Send S.M.A.R.T Enable command to device
			//
			Status = AtaNonDataCommandIn(
				Dev,
				Channel,
				&AtaCommandBlock,
				AtaStatusBlock,
				ATA_ATAPI_TIMEOUT,
				FALSE
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
				AtaCommandBlock.AtaDeviceHead = (UINT8)((Device << 0x4) | 0xe0);

				Status = AtaNonDataCommandIn(
					Dev,
					Channel,
					&AtaCommandBlock,
					AtaStatusBlock,
					ATA_ATAPI_TIMEOUT,
					FALSE
					);
				if (!RETURN_ERROR(Status)) {
					Status = IdeAtaSmartReturnStatusCheck(
						Dev,
						Channel,
						Device,
						AtaStatusBlock
						);
				}
			}
		}

		KDEBUG("Enabled S.M.A.R.T feature at [%a] channel [%a] device!\n",
			(Channel == 1) ? "secondary" : "primary", (Device == 1) ? "slave" : "master");

	}

	return;
}


/**
Sends out an ATA Identify Command to the specified device.

This function is called by DiscoverIdeDevice() during its device
identification. It sends out the ATA Identify Command to the
specified device. Only ATA device responses to this command. If
the command succeeds, it returns the Identify data structure which
contains information about the device. This function extracts the
information it needs to fill the IDE_BLK_IO_DEV data structure,
including device type, media block size, media capacity, and etc.

@param Dev              A pointer to DEVICE_NODE data structure.
@param Channel          The channel number of device.
@param Device           The device number of device.
@param Buffer           A pointer to data buffer which is used to contain IDENTIFY data.
@param AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.

@retval RETURN_SUCCESS          Identify ATA device successfully.
@retval RETURN_DEVICE_ERROR     ATA Identify Device Command failed or device is not ATA device.
@retval RETURN_OUT_OF_RESOURCES Allocate memory failed.

**/
RETURN_STATUS
AtaIdentify(
	IN     DEVICE_NODE                *Dev,
	IN     UINT32                    Channel,
	IN     UINT8                     Device,
	IN OUT IDENTIFY_DATA             *Buffer,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock
	)
{
	RETURN_STATUS             Status;
	ATA_COMMAND_BLOCK  AtaCommandBlock;

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_IDENTIFY_DRIVE;
	AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

	Status = AtaPioDataInOut(
		Dev,
		Channel,
		Buffer,
		sizeof(IDENTIFY_DATA),
		TRUE,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT,
		FALSE
		);

	return Status;
}

/**
This function is called by DiscoverIdeDevice() during its device
identification.
Its main purpose is to get enough information for the device media
to fill in the Media data structure of the Block I/O Protocol interface.

There are 5 steps to reach such objective:
1. Sends out the ATAPI Identify Command to the specified device.
Only ATAPI device responses to this command. If the command succeeds,
it returns the Identify data structure which filled with information
about the device. Since the ATAPI device contains removable media,
the only meaningful information is the device module name.
2. Sends out ATAPI Inquiry Packet Command to the specified device.
This command will return inquiry data of the device, which contains
the device type information.
3. Allocate sense data space for future use. We don't detect the media
presence here to improvement boot performance, especially when CD
media is present. The media detection will be performed just before
each BLK_IO read/write

@param Dev         A pointer to DEVICE_NODE data structure.
@param Channel          The channel number of device.
@param Device           The device number of device.
@param Buffer           A pointer to data buffer which is used to contain IDENTIFY data.
@param AtaStatusBlock   A pointer to ATA_STATUS_BLOCK data structure.

@retval RETURN_SUCCESS          Identify ATAPI device successfully.
@retval RETURN_DEVICE_ERROR     ATA Identify Packet Device Command failed or device type
is not supported by this IDE driver.
@retval RETURN_OUT_OF_RESOURCES Allocate memory failed.

**/
RETURN_STATUS
AtaIdentifyPacket(
	IN     DEVICE_NODE                *Dev,
	IN     UINT32                    Channel,
	IN     UINT8                     Device,
	IN OUT IDENTIFY_DATA             *Buffer,
	IN OUT ATA_STATUS_BLOCK          *AtaStatusBlock
	)
{
	RETURN_STATUS             Status;
	ATA_COMMAND_BLOCK  AtaCommandBlock;

	memset(&AtaCommandBlock, 0, sizeof(ATA_COMMAND_BLOCK));

	AtaCommandBlock.AtaCommand = ATA_CMD_IDENTIFY_DEVICE;
	AtaCommandBlock.AtaDeviceHead = (UINT8)(Device << 0x4);

	//
	// Send ATAPI Identify Command to get IDENTIFY data.
	//
	Status = AtaPioDataInOut(
		Dev,
		Channel,
		(VOID *)Buffer,
		sizeof(IDENTIFY_DATA),
		TRUE,
		&AtaCommandBlock,
		AtaStatusBlock,
		ATA_ATAPI_TIMEOUT,
		FALSE
		);

	return Status;
}


/**
This function is used for detect whether the IDE device exists in the
specified Channel as the specified Device Number.

There is two IDE channels: one is Primary Channel, the other is
Secondary Channel.(Channel is the logical name for the physical "Cable".)
Different channel has different register group.

On each IDE channel, at most two IDE devices attach,
one is called Device 0 (Master device), the other is called Device 1
(Slave device). The devices on the same channel co-use the same register
group, so before sending out a command for a specified device via command
register, it is a must to select the current device to accept the command
by set the device number in the Head/Device Register.

@param Dev         A pointer to DEVICE_NODE data structure.
@param IdeChannel       The channel number of device.

@retval RETURN_SUCCESS successfully detects device.
@retval other       any failure during detection process will return this value.

**/
RETURN_STATUS
DetectAndConfigIdeDevice(
	IN  DEVICE_NODE                    *Dev,
	IN  UINT8                         IdeChannel
	)
{
	RETURN_STATUS                        Status;
	UINT8                             SectorCountReg;
	UINT8                             LBALowReg;
	UINT8                             LBAMidReg;
	UINT8                             LBAHighReg;
	ATA_DEVICE_TYPE                   DeviceType;
	UINT8                             IdeDevice;
	IDE_REGISTERS                 *IdeRegisters;
	IDENTIFY_DATA                    *Buffer;

	//ATA_COLLECTIVE_MODE           *SupportedModes;
	ATA_TRANSFER_MODE             TransferMode;
	ATA_DRIVE_PARMS               DriveParameters;
	UINT8  CharBuffer[512] = { 0 };

	Buffer = KMalloc(sizeof(IDENTIFY_DATA));
	IdeRegisters = &((IDE_REGISTERS *)Dev->PrivateData)[IdeChannel];

	for (IdeDevice = 0; IdeDevice < IdeMaxDevice; IdeDevice++) {
		//
		// Send ATA Device Execut Diagnostic command.
		// This command should work no matter DRDY is ready or not
		//


		IdeWritePortB(IdeRegisters->CmdOrStatus, ATA_CMD_EXEC_DRIVE_DIAG);

		Status = WaitForBSYClear(Dev, IdeChannel, 350000000);
		if (RETURN_ERROR(Status)) {
			KDEBUG("New detecting method: Send Execute Diagnostic Command: WaitForBSYClear: Status: %d\n", Status);
			continue;
		}

		//
		// Select Master or Slave device to get the return signature for ATA DEVICE DIAGNOSTIC cmd.
		//
		IdeWritePortB(IdeRegisters->Head, (UINT8)((IdeDevice << 4) | 0xe0));
		//
		// Stall for 1 milliseconds.
		//
		MicroSecondDelay(1000);

		SectorCountReg = IdeReadPortB(IdeRegisters->SectorCount);
		LBALowReg = IdeReadPortB(IdeRegisters->SectorNumber);
		LBAMidReg = IdeReadPortB(IdeRegisters->CylinderLsb);
		LBAHighReg = IdeReadPortB(IdeRegisters->CylinderMsb);

		//
		// Refer to ATA/ATAPI 4 Spec, section 9.1
		//
		if ((SectorCountReg == 0x1) && (LBALowReg == 0x1) && (LBAMidReg == 0x0) && (LBAHighReg == 0x0)) {
			DeviceType = IdeHarddisk;
		}
		else if ((LBAMidReg == 0x14) && (LBAHighReg == 0xeb)) {
			DeviceType = IdeCdrom;
		}
		else {
			continue;
		}

		KDEBUG("[%s] Channel:[%s] [%s] device\n",
			(IdeChannel == 1) ? "Secondary" : "Primary  ", (IdeDevice == 1) ? "Slave " : "Master",
			DeviceType == IdeCdrom ? "CDROM" : "HardDisk");
		//
		// Send IDENTIFY cmd to the device to test if it is really attached.
		//
		
		if (DeviceType == IdeHarddisk)
		{
			Status = AtaIdentify(Dev, IdeChannel, IdeDevice, Buffer, NULL);
			KDEBUG("AtaIdentify Status: %x.\n", Status);

			if (Status)
				continue;

			DEVICE_NODE * Disk = (DEVICE_NODE *)KMalloc(sizeof(DEVICE_NODE));
			Disk->DeviceType = DEVICE_TYPE_STORAGE;
			Disk->BusType = BUS_TYPE_IDE;
			Disk->DeviceID = 0x4200 + IdeDevice;
			Disk->Position = (IdeDevice << 8) + (IdeChannel << 9);
			Disk->ParentDevice = Dev;
			Disk->ChildDevice = 0;
			Disk->NextDevice = 0;
			Disk->PrevDevice = 0;
			Disk->PrivateData = (IDE_DISK_FEATURE *)KMalloc(sizeof(IDE_DISK_FEATURE));

			int i;
			printk("ATA Disk SN:");
			for (i = 0; i < sizeof(Buffer->AtaData.SerialNo); i+=2)
			{
				printk("%c", Buffer->AtaData.SerialNo[i+1]);
				printk("%c", Buffer->AtaData.SerialNo[i]);
			}
			printk("\n");

			printk("ATA Disk Model:");
			for (i = 0; i < sizeof(Buffer->AtaData.ModelName); i+=2)
			{
				printk("%c", Buffer->AtaData.ModelName[i+1]);
				printk("%c", Buffer->AtaData.ModelName[i]);
			}
			printk("\n");

			KDEBUG("DMA Supported:[%s]\n", Buffer->AtaData.capabilities_49 & (1<<8) ? "Yes":"No");
			KDEBUG("LBA Supported:[%s]\n", Buffer->AtaData.capabilities_49 & (1<<9) ? "Yes" : "No");
			
			UINT64 Capacity = 
				Buffer->AtaData.maximum_lba_for_48bit_addressing[0] +
				((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[1] << 16) +
				((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[2] << 16*2) +
				((UINT64)Buffer->AtaData.maximum_lba_for_48bit_addressing[3] << 16*3);

			((IDE_DISK_FEATURE *)(Disk->PrivateData))->LBA48Support = 1;

			if (Capacity == 0)
			{
				Capacity = 
					((UINT64)Buffer->AtaData.user_addressable_sectors_hi << 16) +
					(UINT64)Buffer->AtaData.user_addressable_sectors_lo;
				((IDE_DISK_FEATURE *)(Disk->PrivateData))->LBA48Support = 0;
			}

			printk("ATA Disk Capacity:%d.%d %s.\n",
				Capacity * 512 > 1 * 1024 * 1024 * 1024 ? Capacity * 512 / 1024 / 1024 / 1024 : Capacity * 512 / 1024 / 1024,
				Capacity * 512 > 1 * 1024 * 1024 * 1024 ? Capacity * 512 / 1024 / 1024 % 1024 * 100 / 1024 : Capacity * 512 / 1024 % 1024 * 100 / 1024,
				Capacity * 512 > 1 * 1024 * 1024 * 1024 ? "GB" : "MB");

			KDEBUG("LBA48 Supported:[%s]\n", ((IDE_DISK_FEATURE *)(Disk->PrivateData))->LBA48Support? "Yes" : "No");


			KDEBUG("Disk found and registed.\n");

			((IDE_DISK_FEATURE *)(Disk->PrivateData))->DMASupport = (Buffer->AtaData.capabilities_49 & (1 << 8)) ? 1 : 0;
			((IDE_DISK_FEATURE *)(Disk->PrivateData))->LBASupport = (Buffer->AtaData.capabilities_49 & (1 << 9)) ? 1 : 0;

			DISK_NODE * NewDisk = KMalloc(sizeof(DISK_NODE));
			NewDisk->Device = Disk;
			NewDisk->StartingSectors = 0;
			NewDisk->NumberOfSectors = Capacity / 512;
			NewDisk->SectorSize = 512;
			NewDisk->Next = NULL;

			DeviceRegister(Disk);
			DiskRegister(NewDisk);
		}
		else
		{
			Status = AtaIdentifyPacket(Dev, IdeChannel, IdeDevice, Buffer, NULL);
			KDEBUG("LINE = %d,AtaIdentify Status: %x\n", __LINE__, Status);

			if (Status)
				continue;

			int k;
			memset(CharBuffer, 0, 512);
			for (k = 0; k < sizeof(Buffer->AtaData.SerialNo); k += 2)
			{
				CharBuffer[k] = Buffer->AtaData.SerialNo[k + 1];
				CharBuffer[k + 1] = Buffer->AtaData.SerialNo[k];
				//printk("%c", Buffer->AtaData.SerialNo[k + 1]);
				//printk("%c", Buffer->AtaData.SerialNo[k]);
			}
			printk("ATA CDROM SN:%s\n", CharBuffer);


			for (k = 0; k < sizeof(Buffer->AtaData.ModelName); k += 2)
			{
				CharBuffer[k] = Buffer->AtaData.ModelName[k + 1];
				CharBuffer[k + 1] = Buffer->AtaData.ModelName[k];
				//printk("%c", Buffer->AtaData.ModelName[k + 1]);
				//printk("%c", Buffer->AtaData.ModelName[k]);
			}
			printk("ATA CDROM Model:%s\n", CharBuffer);
		}

		//Status = AtaSoftReset(Dev, IdeChannel, 50000);

		//KDEBUG("Ata Soft Reset:Status = %x\n", Status);
		//if (DeviceType == IdeHarddisk) {
		//	Status = AtaIdentify(Dev, IdeChannel, IdeDevice, &Buffer, NULL);
		//	//
		//	// if identifying ata device is failure, then try to send identify packet cmd.
		//	//

		//	KDEBUG("LINE = %d,AtaIdentify Status: %x\n", __LINE__, Status);

		//	if (RETURN_ERROR(Status)) {
		//		DeviceType = IdeCdrom;
		//		Status = AtaIdentifyPacket(Dev, IdeChannel, IdeDevice, &Buffer, NULL);

		//		KDEBUG("LINE = %d,AtaIdentify Status: %x\n", __LINE__, Status);
		//	}
		//}
		//else {
		//	Status = AtaIdentifyPacket(Dev, IdeChannel, IdeDevice, &Buffer, NULL);
		//	KDEBUG("LINE = %d,AtaIdentify Status: %x\n", __LINE__, Status);
		//	//
		//	// if identifying atapi device is failure, then try to send identify cmd.
		//	//
		//	if (RETURN_ERROR(Status)) {
		//		DeviceType = IdeHarddisk;
		//		Status = AtaIdentify(Dev, IdeChannel, IdeDevice, &Buffer, NULL);
		//		KDEBUG("LINE = %d,AtaIdentify Status: %x\n", __LINE__, Status);
		//	}
		//}

		//if (RETURN_ERROR(Status)) {
		//	//
		//	// No device is found at this port
		//	//
		//	KDEBUG("No device is found at port %d\n", IdeDevice);
		//	continue;
		//}

		//
		// If the device is a hard Dev, then try to enable S.M.A.R.T feature
		//
		/*if ((DeviceType == IdeHarddisk)) {
			IdeAtaSmartSupport(
				Dev,
				IdeChannel,
				IdeDevice,
				&Buffer,
				FALSE
				);
		}*/

		//
		// Submit identify data to IDE controller init driver
		//
		//IdeInit->SubmitData(IdeInit, IdeChannel, IdeDevice, &Buffer);

		//
		// Now start to config ide device parameter and transfer mode.
		//
		/*Status = IdeInit->CalculateMode(
			IdeInit,
			IdeChannel,
			IdeDevice,
			&SupportedModes
			);
		if (RETURN_ERROR(Status)) {
			KDEBUG("Calculate Mode Fail, Status = %r\n", Status);
			continue;
		}*/

		//
		// Set best supported PIO mode on this IDE device
		//
		/*if (SupportedModes->PioMode.Mode <= EfiAtaPioMode2) {
			TransferMode.ModeCategory = ATA_MODE_DEFAULT_PIO;
		}
		else {
			TransferMode.ModeCategory = ATA_MODE_FLOW_PIO;
		}

		TransferMode.ModeNumber = (UINT8)(SupportedModes->PioMode.Mode);

		if (SupportedModes->ExtModeCount == 0) {
			Status = SetDeviceTransferMode(Dev, IdeChannel, IdeDevice, &TransferMode, NULL);

			if (RETURN_ERROR(Status)) {
				KDEBUG("Set transfer Mode Fail, Status = %r\n", Status);
				continue;
			}
		}*/

		//
		// Set supported DMA mode on this IDE device. Note that UDMA & MDMA cann't
		// be set together. Only one DMA mode can be set to a device. If setting
		// DMA mode operation fails, we can continue moving on because we only use
		// PIO mode at boot time. DMA modes are used by certain kind of OS booting
		//
		/*if (SupportedModes->UdmaMode.Valid) {
			TransferMode.ModeCategory = ATA_MODE_UDMA;
			TransferMode.ModeNumber = (UINT8)(SupportedModes->UdmaMode.Mode);
			Status = SetDeviceTransferMode(Dev, IdeChannel, IdeDevice, &TransferMode, NULL);

			if (RETURN_ERROR(Status)) {
				KDEBUG("Set transfer Mode Fail, Status = %r\n", Status);
				continue;
			}
		}
		else if (SupportedModes->MultiWordDmaMode.Valid) {
			TransferMode.ModeCategory = ATA_MODE_MDMA;
			TransferMode.ModeNumber = (UINT8)SupportedModes->MultiWordDmaMode.Mode;
			Status = SetDeviceTransferMode(Dev, IdeChannel, IdeDevice, &TransferMode, NULL);

			if (RETURN_ERROR(Status)) {
				KDEBUG("Set transfer Mode Fail, Status = %r\n", Status);
				continue;
			}
		}*/


		//TransferMode.ModeCategory = ATA_MODE_UDMA;
		//TransferMode.ModeCategory = ATA_MODE_DEFAULT_PIO;
		//TransferMode.ModeNumber = 0;
		//Status = SetDeviceTransferMode(Dev, IdeChannel, IdeDevice, &TransferMode, NULL);

		//
		// Set Parameters for the device:
		// 1) Init
		// 2) Establish the block count for READ/WRITE MULTIPLE (EXT) command
		//


		//if (DeviceType == EfiIdeHardDev) {
		//	//
		//	// Init driver parameters
		//	//
		//	DriveParameters.Sector = (UINT8)((ATA5_IDENTIFY_DATA *)(&Buffer->AtaData))->sectors_per_track;
		//	DriveParameters.Heads = (UINT8)(((ATA5_IDENTIFY_DATA *)(&Buffer->AtaData))->heads - 1);
		//	DriveParameters.MultipleSector = (UINT8)((ATA5_IDENTIFY_DATA *)(&Buffer->AtaData))->multi_sector_cmd_max_sct_cnt;

		//	Status = SetDriveParameters(Dev, IdeChannel, IdeDevice, &DriveParameters, NULL);
		//}

		//
		// Set IDE controller Timing Blocks in the PCI Configuration Space
		//
		//IdeInit->SetTiming(IdeInit, IdeChannel, IdeDevice, SupportedModes);

		//
		// IDE controller and IDE device timing is configured successfully.
		// Now insert the device into device list.
		//
		/*Status = CreateNewDeviceInfo(Dev, IdeChannel, IdeDevice, DeviceType, &Buffer);
		if (RETURN_ERROR(Status)) {
			continue;
		}

		if (DeviceType == EfiIdeHardDev) {
		}*/
	}
	KFree(Buffer);
	return RETURN_SUCCESS;
}


void AtaISP(void *);

/**
Initialize ATA host controller at IDE mode.

The function is designed to initialize ATA host controller.

@param[in]  Dev          A pointer to the DEVICE_NODE Dev.

**/
RETURN_STATUS
IDEControllerInit(
	IN  DEVICE_NODE    *Dev
	)
{
	RETURN_STATUS                     Status;
	UINT8                             Channel;
	UINT8                             IdeChannel;
	BOOLEAN                           ChannelEnabled;
	UINT8                             MaxDevices;

	//
	// Obtain IDE IO port registers' base addresses
	//
	Status = GetIdeRegisterIoAddr(Dev);
	if (RETURN_ERROR(Status)) {
		goto ErrorExit;
	}

	DetectAndConfigIdeDevice(Dev, 0);
	DetectAndConfigIdeDevice(Dev, 1);

	IrqRegister(0x2E, AtaISP, Dev);
	IrqRegister(0x2F, AtaISP, Dev);

ErrorExit:
	return Status;
}


void AtaISP(void * Dev)
{
	printk("Ata interrupt.\n");
}



RETURN_STATUS IDEDiskRead(DISK_NODE * Disk, UINT64 Sector, UINT64 Length, UINT8 * Buffer)
{
	KDEBUG("IDE Disk Read...\n");
	ATA_COMMAND_BLOCK ata_cmb;
	ATA_STATUS_BLOCK ata_sb;
	memset(&ata_cmb, 0, sizeof(ATA_COMMAND_BLOCK));
	UINT64 StartLba = Sector;
	UINT64 TransferLength = Length;
	UINT64 PortMultiplierPort = (Disk->Device->Position >> 8) & 0x01;
	RETURN_STATUS Status = RETURN_SUCCESS;

	KDEBUG("TransferLength = %d\n", TransferLength);

	IDE_DISK_FEATURE * DiskFeature = Disk->Device->PrivateData;

	ata_cmb.AtaSectorNumber = (UINT8)StartLba;
	ata_cmb.AtaCylinderLow = (UINT8)(StartLba >> 8);
	ata_cmb.AtaCylinderHigh = (UINT8)(StartLba >> 16);
	ata_cmb.AtaDeviceHead = (UINT8)(BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4));
	ata_cmb.AtaSectorCount = (UINT8)(TransferLength / 512 ? TransferLength / 512 : 1);

	if (DiskFeature->LBA48Support == 1)
	{
		ata_cmb.AtaSectorNumberExp = (UINT8)(StartLba >> 24);
		ata_cmb.AtaCylinderLowExp = (UINT8)(StartLba >> 32);
		ata_cmb.AtaCylinderHighExp = (UINT8)(StartLba >> 40);
		ata_cmb.AtaSectorCountExp = (UINT8)((TransferLength / 512) >> 8);
	}
	else
		ata_cmb.AtaDeviceHead = (UINT8)(ata_cmb.AtaDeviceHead | (StartLba >> 24));

	/*KDEBUG("ATA Command Block:AtaSectorNumber = %d\n", ata_cmb.AtaSectorNumber);
	KDEBUG("ATA Command Block:AtaSectorNumberExp = %d\n", ata_cmb.AtaSectorNumberExp);
	KDEBUG("ATA Command Block:AtaCylinderLow = %d\n", ata_cmb.AtaCylinderLow);
	KDEBUG("ATA Command Block:AtaCylinderLowExp = %d\n", ata_cmb.AtaCylinderLowExp);
	KDEBUG("ATA Command Block:AtaCylinderHigh = %d\n", ata_cmb.AtaCylinderHigh);
	KDEBUG("ATA Command Block:AtaCylinderHighExp = %d\n", ata_cmb.AtaCylinderHighExp);
	KDEBUG("ATA Command Block:AtaSectorCount = %d\n", ata_cmb.AtaSectorCount);
	KDEBUG("ATA Command Block:AtaSectorCountExp = %d\n", ata_cmb.AtaSectorCountExp);
	KDEBUG("ATA Command Block:AtaDeviceHead = %x\n", ata_cmb.AtaDeviceHead);*/

	if (DiskFeature->DMASupport == 1)
	{

		ata_cmb.AtaCommand = DiskFeature->LBA48Support ? ATA_CMD_READ_DMA_EXT: ATA_CMD_READ_DMA;

		Status = AtaUdmaInOut(
			Disk->Device->ParentDevice,
			(Disk->Device->Position >> 9),
			1,
			0,
			&ata_cmb,
			&ata_sb,
			Buffer,
			TransferLength,
			2000
		);
	}
	else
	{
		ata_cmb.AtaCommand = DiskFeature->LBA48Support ? ATA_CMD_READ_SECTORS_EXT : ATA_CMD_READ_SECTORS;

		Status = AtaPioDataInOut(
			Disk->Device->ParentDevice,
			(Disk->Device->Position >> 9),
			Buffer,
			TransferLength,
			1,
			&ata_cmb,
			&ata_sb,
			3000,
			FALSE
		);
	}	
	
	KDEBUG("Disk Read Status = %x\n", Status);
	return Status;
}




RETURN_STATUS IDEDiskWrite(DISK_NODE * Disk, UINT64 Sector, UINT64 Length, UINT8 * Buffer)
{
	ATA_COMMAND_BLOCK ata_cmb;
	ATA_STATUS_BLOCK ata_sb;
	memset(&ata_cmb, 0, sizeof(ATA_COMMAND_BLOCK));
	UINT64 StartLba = Sector;
	UINT64 TransferLength = Length;
	UINT64 PortMultiplierPort = (Disk->Device->Position >> 8) & 0x01;
	UINT8 LBA48bit = 0;
	RETURN_STATUS Status = 0;

	IDE_DISK_FEATURE * DiskFeature = Disk->Device->PrivateData;

	//ata_cmb.AtaCommand = ATA_CMD_READ_DMA_EXT;
	//ata_cmb.AtaCommand = ATA_CMD_READ_SECTORS_WITH_RETRY;

	ata_cmb.AtaSectorNumber = (UINT8)StartLba;
	ata_cmb.AtaCylinderLow = (UINT8)(StartLba >> 8);
	ata_cmb.AtaCylinderHigh = (UINT8)(StartLba >> 16);
	ata_cmb.AtaDeviceHead = (UINT8)(BIT7 | BIT6 | BIT5 | (PortMultiplierPort << 4));
	ata_cmb.AtaSectorCount = (UINT8)(TransferLength / 512 ? TransferLength / 512 : 1);

	if (DiskFeature->LBA48Support == 1)
	{
		ata_cmb.AtaSectorNumberExp = (UINT8)(StartLba >> 24);
		ata_cmb.AtaCylinderLowExp = (UINT8)(StartLba >> 32);
		ata_cmb.AtaCylinderHighExp = (UINT8)(StartLba >> 40);
		ata_cmb.AtaSectorCountExp = (UINT8)((TransferLength / 512) >> 8);
	}
	else
		ata_cmb.AtaDeviceHead = (UINT8)(ata_cmb.AtaDeviceHead | (StartLba >> 24));

	if (DiskFeature->DMASupport == 1)
	{

		ata_cmb.AtaCommand = DiskFeature->LBA48Support ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_WRITE_DMA;

		Status = AtaUdmaInOut(
			Disk->Device->ParentDevice,
			(Disk->Device->Position >> 9),
			0,
			0,
			&ata_cmb,
			&ata_sb,
			Buffer,
			TransferLength,
			2000
		);
	}
	else
	{
		ata_cmb.AtaCommand = DiskFeature->LBA48Support ? ATA_CMD_WRITE_SECTORS_EXT : ATA_CMD_WRITE_SECTORS;

		Status = AtaPioDataInOut(
			Disk->Device->ParentDevice,
			(Disk->Device->Position >> 9),
			Buffer,
			TransferLength,
			0,
			&ata_cmb,
			&ata_sb,
			3000,
			FALSE
		);
	}

	KDEBUG("Disk Write Status = %x\n", Status);
	return Status;
}



RETURN_STATUS IDEDiskSupported(DEVICE_NODE * Dev)
{
	if (Dev->BusType != BUS_TYPE_IDE)
		return RETURN_UNSUPPORTED;

	KDEBUG("IDE disk driver supported on device:%016x\n", Dev->Position);
	return RETURN_SUCCESS;
}

RETURN_STATUS IDEDiskStart(DEVICE_NODE * Dev)
{
	if (Dev->BusType != BUS_TYPE_IDE)
		return RETURN_UNSUPPORTED;

	KDEBUG("IDE Disk Driver starting...\n", Dev->Position);

	return RETURN_SUCCESS;
}




DRIVER_NODE IDEControllerDriver =
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = IDESupported,
	.DriverStart = IDEStart,
	//.DriverStop = IDEStop,
	.Operations = 0,
	.PrevDriver = 0,
	.NextDriver = 0
};


DISK_DRIVER IDEDiskOperations =
{
	.DiskOpen = 0,
	.GetDiskInfo = 0,
	.DiskRead = IDEDiskRead,
	.DiskWrite = IDEDiskWrite,
	.DiskClose = 0
};

DRIVER_NODE IDEDiskDriver =
{
	.BusType = BUS_TYPE_IDE,
	.DriverSupported = IDEDiskSupported,
	.DriverStart = IDEDiskStart,
	.DriverStop = 0,
	.Operations = &IDEDiskOperations,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void IDEInit()
{
	DriverRegister(&IDEControllerDriver, &DriverList[DEVICE_TYPE_STORAGE]);
	DriverRegister(&IDEDiskDriver, &DriverList[DEVICE_TYPE_STORAGE]);
}
MODULE_INIT(IDEInit);