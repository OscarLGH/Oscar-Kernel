/** @file
  Header file for IDE mode of ATA host controller.
  
  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/
#ifndef _IDE_H
#define _IDE_H

#include "Pci.h"
#include "Disk.h"
#include "BaseLib.h"
#include "Atapi.h"
#include "AtaPassThru.h"



//#define DEBUG
#include "Debug.h"

UINT8 	In8(UINT16 Port);
UINT16 	In16(UINT16 Port);
UINT32 	In32(UINT16 Port);
void	Out8(UINT16 Port, UINT8 Value);
void	Out16(UINT16 Port, UINT16 Value);
void	Out32(UINT16 Port, UINT32 Value);

typedef enum {
  IdePrimary    = 0,
  IdeSecondary  = 1,
  IdeMaxChannel = 2
} IDE_CHANNEL;

typedef enum {
  IdeMaster    = 0,
  IdeSlave     = 1,
  IdeMaxDevice = 2
} IDE_DEVICE;

///
/// PIO mode definition
///
typedef enum {
  AtaPioModeBelow2,
  AtaPioMode2,
  AtaPioMode3,
  AtaPioMode4
} ATA_PIO_MODE;

//
// Multi word DMA definition
//
typedef enum {
  AtaMdmaMode0,
  AtaMdmaMode1,
  AtaMdmaMode2
} ATA_MDMA_MODE;

//
// UDMA mode definition
//
typedef enum {
  AtaUdmaMode0,
  AtaUdmaMode1,
  AtaUdmaMode2,
  AtaUdmaMode3,
  AtaUdmaMode4,
  AtaUdmaMode5
} ATA_UDMA_MODE;

//
// Bus Master Reg
//
#define BMIC_NREAD      BIT3
#define BMIC_START      BIT0
#define BMIS_INTERRUPT  BIT2
#define BMIS_ERROR      BIT1

#define BMIC_OFFSET    0x00
#define BMIS_OFFSET    0x02
#define BMID_OFFSET    0x04

//
// IDE transfer mode
//
#define ATA_MODE_DEFAULT_PIO 0x00
#define ATA_MODE_FLOW_PIO    0x01
#define ATA_MODE_MDMA        0x04
#define ATA_MODE_UDMA        0x08

//
// IDE registers set
//
typedef struct {
  UINT16                          Data;
  UINT16                          ErrOrFeature;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  UINT16                          CmdOrStatus;
  UINT16                          AltOrDev;

  UINT16                          BusMasterBaseAddr;
} IDE_REGISTERS;

//
// Bit definitions in Programming Interface byte of the Class Code field
// in PCI IDE controller's Configuration Space
//
#define IDE_PRIMARY_OPERATING_MODE            BIT0
#define IDE_PRIMARY_PROGRAMMABLE_INDICATOR    BIT1
#define IDE_SECONDARY_OPERATING_MODE          BIT2
#define IDE_SECONDARY_PROGRAMMABLE_INDICATOR  BIT3

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

  @param[in]      Dev          Pointer to the DEVICE_NODE instance
  @param[in, out] IdeRegisters   Pointer to IDE_REGISTERS which is used to
                                 store the IDE i/o port registers' base addresses
           
  @retval UNSUPPORTED        Return this value when the BARs is not IO type
  @retval SUCCESS            Get the Base address successfully
  @retval Other                  Read the pci configureation data error

**/
RETURN_STATUS
GetIdeRegisterIoAddr(
	IN OUT     DEVICE_NODE              *Dev
	);
/**
  This function is used to send out ATAPI commands conforms to the Packet Command 
  with PIO Data In Protocol.

  @param[in] Dev          Pointer to the DEVICE_NODE instance
  @param[in] IdeRegisters   Pointer to IDE_REGISTERS which is used to
                            store the IDE i/o port registers' base addresses
  @param[in] Channel        The channel number of device.
  @param[in] Device         The device number of device.
  @param[in] Packet         A pointer to EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET data structure.

  @retval SUCCESS       send out the ATAPI packet command successfully
                            and device sends data successfully.
  @retval DEVICE_ERROR  the device failed to send data.

**/

typedef struct {
	UINT8 DMASupport;
	UINT8 LBASupport;
	UINT8 LBA48Support;
	UINT8 IordySupport;
}IDE_DISK_FEATURE;

RETURN_STATUS
IDEControllerInit(
	IN  DEVICE_NODE    *Dev
	);


#define DivU64x32(x,y) (x/y)
#define RShiftU64(x,y) (x>>y)

#define MIN(x,y) (x<y?x:y)

#endif

