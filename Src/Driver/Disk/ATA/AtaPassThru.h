/** @file
  The ATA_PASS_THRU_PROTOCOL provides information about an ATA controller and the ability
  to send ATA Command Blocks to any ATA device attached to that ATA controller. The information
  includes the attributes of the ATA controller.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _ATA_PASS_THROUGH_H__
#define _ATA_PASS_THROUGH_H__

typedef struct {
  UINT32 Attributes;
  UINT32 IoAlign;
} ATA_PASS_THRU_MODE;

///
/// If this bit is set, then the ATA_PASS_THRU_PROTOCOL interface is for physical
/// devices on the ATA controller.
///
#define ATA_PASS_THRU_ATTRIBUTES_PHYSICAL   0x0001
///
/// If this bit is set, then the ATA_PASS_THRU_PROTOCOL interface is for logical
/// devices on the ATA controller.
///
#define ATA_PASS_THRU_ATTRIBUTES_LOGICAL    0x0002
///
/// If this bit is set, then the ATA_PASS_THRU_PROTOCOL interface supports non blocking
/// I/O. Every ATA_PASS_THRU_PROTOCOL must support blocking I/O. The support of non-blocking
/// I/O is optional.
///
#define ATA_PASS_THRU_ATTRIBUTES_NONBLOCKIO 0x0004

typedef struct ATA_COMMAND_BLOCK {
  UINT8 Reserved1[2];
  UINT8 AtaCommand;
  UINT8 AtaFeatures;
  UINT8 AtaSectorNumber;
  UINT8 AtaCylinderLow;
  UINT8 AtaCylinderHigh;
  UINT8 AtaDeviceHead;
  UINT8 AtaSectorNumberExp;
  UINT8 AtaCylinderLowExp;
  UINT8 AtaCylinderHighExp; 
  UINT8 AtaFeaturesExp;
  UINT8 AtaSectorCount;
  UINT8 AtaSectorCountExp;
  UINT8 Reserved2[6];
} ATA_COMMAND_BLOCK;

typedef struct ATA_STATUS_BLOCK {
  UINT8 Reserved1[2];
  UINT8 AtaStatus;
  UINT8 AtaError;
  UINT8 AtaSectorNumber;
  UINT8 AtaCylinderLow;
  UINT8 AtaCylinderHigh;
  UINT8 AtaDeviceHead;
  UINT8 AtaSectorNumberExp;
  UINT8 AtaCylinderLowExp;
  UINT8 AtaCylinderHighExp; 
  UINT8 Reserved2;
  UINT8 AtaSectorCount;
  UINT8 AtaSectorCountExp;
  UINT8 Reserved3[6];
} ATA_STATUS_BLOCK;

typedef UINT8 ATA_PASS_THRU_CMD_PROTOCOL;

#define ATA_PASS_THRU_PROTOCOLATA_HARDWARE_RESET 0x00
#define ATA_PASS_THRU_PROTOCOLATA_SOFTWARE_RESET 0x01
#define ATA_PASS_THRU_PROTOCOLATA_NON_DATA       0x02
#define ATA_PASS_THRU_PROTOCOL_PIO_DATA_IN        0x04
#define ATA_PASS_THRU_PROTOCOL_PIO_DATA_OUT       0x05
#define ATA_PASS_THRU_PROTOCOL_DMA                0x06
#define ATA_PASS_THRU_PROTOCOL_DMA_QUEUED         0x07
#define ATA_PASS_THRU_PROTOCOL_DEVICE_DIAGNOSTIC  0x08
#define ATA_PASS_THRU_PROTOCOL_DEVICE_RESET       0x09
#define ATA_PASS_THRU_PROTOCOL_UDMA_DATA_IN       0x0A
#define ATA_PASS_THRU_PROTOCOL_UDMA_DATA_OUT      0x0B
#define ATA_PASS_THRU_PROTOCOL_FPDMA              0x0C
#define ATA_PASS_THRU_PROTOCOL_RETURN_RESPONSE    0xFF

typedef UINT8 ATA_PASS_THRU_LENGTH;

#define ATA_PASS_THRU_LENGTH_BYTES                0x80


#define ATA_PASS_THRU_LENGTH_MASK                 0x70
#define ATA_PASS_THRU_LENGTH_NO_DATA_TRANSFER     0x00
#define ATA_PASS_THRU_LENGTH_FEATURES             0x10
#define ATA_PASS_THRU_LENGTH_SECTOR_COUNT         0x20
#define ATA_PASS_THRU_LENGTH_TPSIU                0x30

#define ATA_PASS_THRU_LENGTH_COUNT                0x0F

#endif
