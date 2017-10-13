#ifndef _AHCI_H
#define _AHCI_H

#include "Pci.h"
#include "Disk.h"
#include "Atapi.h"
#include "AtaPassThru.h"

//#define DEBUG
#include "Debug.h"

#define AHCI_BAR_INDEX                     0x05

#define AHCI_CAPABILITY_OFFSET             0x0000
#define   AHCI_CAP_SAM                     BIT18
#define   AHCI_CAP_SSS                     BIT27
#define   AHCI_CAP_S64A                    BIT31
#define AHCI_GHC_OFFSET                    0x0004
#define   AHCI_GHC_RESET                   BIT0
#define   AHCI_GHC_IE                      BIT1
#define   AHCI_GHC_ENABLE                  BIT31
#define AHCI_IS_OFFSET                     0x0008
#define AHCI_PI_OFFSET                     0x000C

#define AHCI_MAX_PORTS                     32

typedef struct {
  UINT32  Lower32;
  UINT32  Upper32;
} DATA_32;

typedef union {
  DATA_32   Uint32;
  UINT64    Uint64;
} DATA_64;

//
// Refer SATA1.0a spec section 5.2, the Phy detection time should be less than 10ms.
//
#define  AHCI_BUS_PHY_DETECT_TIMEOUT       10
//
// Refer SATA1.0a spec, the FIS enable time should be less than 500ms.
//
#define  AHCI_PORT_CMD_FR_CLEAR_TIMEOUT    TIMER_PERIOD_MILLISECONDS(500)
//
// Refer SATA1.0a spec, the bus reset time should be less than 1s.
//
#define  AHCI_BUS_RESET_TIMEOUT            TIMER_PERIOD_SECONDS(1)

#define  AHCI_ATAPI_DEVICE_SIG             0xEB140000
#define  AHCI_ATA_DEVICE_SIG               0x00000000
#define  AHCI_PORT_MULTIPLIER_SIG          0x96690000
#define  AHCI_ATAPI_SIG_MASK               0xFFFF0000

//
// Each PRDT entry can point to a memory block up to 4M byte
//
#define AHCI_MAX_DATA_PER_PRDT             0x400000

#define AHCI_FIS_REGISTER_H2D              0x27      //Register FIS - Host to Device
#define   AHCI_FIS_REGISTER_H2D_LENGTH     20 
#define AHCI_FIS_REGISTER_D2H              0x34      //Register FIS - Device to Host
#define   AHCI_FIS_REGISTER_D2H_LENGTH     20
#define AHCI_FIS_DMA_ACTIVATE              0x39      //DMA Activate FIS - Device to Host
#define   AHCI_FIS_DMA_ACTIVATE_LENGTH     4
#define AHCI_FIS_DMA_SETUP                 0x41      //DMA Setup FIS - Bi-directional
#define   AHCI_FIS_DMA_SETUP_LENGTH        28
#define AHCI_FIS_DATA                      0x46      //Data FIS - Bi-directional
#define AHCI_FIS_BIST                      0x58      //BIST Activate FIS - Bi-directional
#define   AHCI_FIS_BIST_LENGTH             12
#define AHCI_FIS_PIO_SETUP                 0x5F      //PIO Setup FIS - Device to Host
#define   AHCI_FIS_PIO_SETUP_LENGTH        20
#define AHCI_FIS_SET_DEVICE                0xA1      //Set Device Bits FIS - Device to Host
#define   AHCI_FIS_SET_DEVICE_LENGTH       8

#define AHCI_D2H_FIS_OFFSET                0x40
#define AHCI_DMA_FIS_OFFSET                0x00
#define AHCI_PIO_FIS_OFFSET                0x20
#define AHCI_SDB_FIS_OFFSET                0x58
#define AHCI_FIS_TYPE_MASK                 0xFF
#define AHCI_U_FIS_OFFSET                  0x60

//
// Port register
//
#define AHCI_PORT_START                    0x0100
#define AHCI_PORT_REG_WIDTH                0x0080
#define AHCI_PORT_CLB                      0x0000
#define AHCI_PORT_CLBU                     0x0004
#define AHCI_PORT_FB                       0x0008
#define AHCI_PORT_FBU                      0x000C
#define AHCI_PORT_IS                       0x0010
#define   AHCI_PORT_IS_DHRS                BIT0
#define   AHCI_PORT_IS_PSS                 BIT1
#define   AHCI_PORT_IS_SSS                 BIT2
#define   AHCI_PORT_IS_SDBS                BIT3
#define   AHCI_PORT_IS_UFS                 BIT4
#define   AHCI_PORT_IS_DPS                 BIT5
#define   AHCI_PORT_IS_PCS                 BIT6
#define   AHCI_PORT_IS_DIS                 BIT7
#define   AHCI_PORT_IS_PRCS                BIT22
#define   AHCI_PORT_IS_IPMS                BIT23
#define   AHCI_PORT_IS_OFS                 BIT24
#define   AHCI_PORT_IS_INFS                BIT26
#define   AHCI_PORT_IS_IFS                 BIT27
#define   AHCI_PORT_IS_HBDS                BIT28
#define   AHCI_PORT_IS_HBFS                BIT29
#define   AHCI_PORT_IS_TFES                BIT30
#define   AHCI_PORT_IS_CPDS                BIT31
#define   AHCI_PORT_IS_CLEAR               0xFFFFFFFF
#define   AHCI_PORT_IS_FIS_CLEAR           0x0000000F

#define AHCI_PORT_IE                       0x0014
#define AHCI_PORT_CMD                      0x0018
#define   AHCI_PORT_CMD_ST_MASK            0xFFFFFFFE
#define   AHCI_PORT_CMD_ST                 BIT0
#define   AHCI_PORT_CMD_SUD                BIT1
#define   AHCI_PORT_CMD_POD                BIT2
#define   AHCI_PORT_CMD_CLO                BIT3
#define   AHCI_PORT_CMD_CR                 BIT15
#define   AHCI_PORT_CMD_FRE                BIT4
#define   AHCI_PORT_CMD_FR                 BIT14
#define   AHCI_PORT_CMD_MASK               ~(AHCI_PORT_CMD_ST | AHCI_PORT_CMD_FRE | AHCI_PORT_CMD_COL)
#define   AHCI_PORT_CMD_PMA                BIT17
#define   AHCI_PORT_CMD_HPCP               BIT18
#define   AHCI_PORT_CMD_MPSP               BIT19
#define   AHCI_PORT_CMD_CPD                BIT20
#define   AHCI_PORT_CMD_ESP                BIT21
#define   AHCI_PORT_CMD_ATAPI              BIT24
#define   AHCI_PORT_CMD_DLAE               BIT25
#define   AHCI_PORT_CMD_ALPE               BIT26
#define   AHCI_PORT_CMD_ASP                BIT27
#define   AHCI_PORT_CMD_ICC_MASK           (BIT28 | BIT29 | BIT30 | BIT31)
#define   AHCI_PORT_CMD_ACTIVE             (1 << 28 )
#define AHCI_PORT_TFD                      0x0020
#define   AHCI_PORT_TFD_MASK               (BIT7 | BIT3 | BIT0)
#define   AHCI_PORT_TFD_BSY                BIT7
#define   AHCI_PORT_TFD_DRQ                BIT3
#define   AHCI_PORT_TFD_ERR                BIT0
#define   AHCI_PORT_TFD_ERR_MASK           0x00FF00
#define AHCI_PORT_SIG                      0x0024
#define AHCI_PORT_SSTS                     0x0028
#define   AHCI_PORT_SSTS_DET_MASK          0x000F
#define   AHCI_PORT_SSTS_DET               0x0001
#define   AHCI_PORT_SSTS_DET_PCE           0x0003
#define   AHCI_PORT_SSTS_SPD_MASK          0x00F0
#define AHCI_PORT_SCTL                     0x002C
#define   AHCI_PORT_SCTL_DET_MASK          0x000F
#define   AHCI_PORT_SCTL_MASK              (~AHCI_PORT_SCTL_DET_MASK)
#define   AHCI_PORT_SCTL_DET_INIT          0x0001
#define   AHCI_PORT_SCTL_DET_PHYCOMM       0x0003
#define   AHCI_PORT_SCTL_SPD_MASK          0x00F0
#define   AHCI_PORT_SCTL_IPM_MASK          0x0F00
#define   AHCI_PORT_SCTL_IPM_INIT          0x0300
#define   AHCI_PORT_SCTL_IPM_PSD           0x0100
#define   AHCI_PORT_SCTL_IPM_SSD           0x0200
#define AHCI_PORT_SERR                     0x0030
#define   AHCI_PORT_SERR_RDIE              BIT0
#define   AHCI_PORT_SERR_RCE               BIT1
#define   AHCI_PORT_SERR_TDIE              BIT8
#define   AHCI_PORT_SERR_PCDIE             BIT9
#define   AHCI_PORT_SERR_PE                BIT10
#define   AHCI_PORT_SERR_IE                BIT11
#define   AHCI_PORT_SERR_PRC               BIT16
#define   AHCI_PORT_SERR_PIE               BIT17
#define   AHCI_PORT_SERR_CW                BIT18
#define   AHCI_PORT_SERR_BDE               BIT19
#define   AHCI_PORT_SERR_DE                BIT20
#define   AHCI_PORT_SERR_CRCE              BIT21
#define   AHCI_PORT_SERR_HE                BIT22
#define   AHCI_PORT_SERR_LSE               BIT23
#define   AHCI_PORT_SERR_TSTE              BIT24
#define   AHCI_PORT_SERR_UFT               BIT25
#define   AHCI_PORT_SERR_EX                BIT26
#define   AHCI_PORT_ERR_CLEAR              0xFFFFFFFF
#define AHCI_PORT_SACT                     0x0034
#define AHCI_PORT_CI                       0x0038
#define AHCI_PORT_SNTF                     0x003C

#pragma pack(1)
//
// Command List structure includes total 32 entries.
// The entry data structure is listed at the following.
//
typedef struct {
  UINT32   AHCICmdCfl:5;      //Command FIS Length
  UINT32   AHCICmdA:1;        //ATAPI
  UINT32   AHCICmdW:1;        //Write
  UINT32   AHCICmdP:1;        //Prefetchable
  UINT32   AHCICmdR:1;        //Reset
  UINT32   AHCICmdB:1;        //BIST
  UINT32   AHCICmdC:1;        //Clear Busy upon R_OK
  UINT32   AHCICmdRsvd:1;
  UINT32   AHCICmdPmp:4;      //Port Multiplier Port
  UINT32   AHCICmdPrdtl:16;   //Physical Region Descriptor Table Length
  UINT32   AHCICmdPrdbc;      //Physical Region Descriptor Byte Count
  UINT32   AHCICmdCtba;       //Command Table Descriptor Base Address
  UINT32   AHCICmdCtbau;      //Command Table Descriptor Base Address Upper 32-BITs
  UINT32   AHCICmdRsvd1[4]; 
} AHCI_COMMAND_LIST;

//
// This is a software constructed FIS.
// For data transfer operations, this is the H2D Register FIS format as 
// specified in the Serial ATA Revision 2.6 specification.
//
typedef struct {
  UINT8    AHCICFisType;
  UINT8    AHCICFisPmNum:4;
  UINT8    AHCICFisRsvd:1;
  UINT8    AHCICFisRsvd1:1;
  UINT8    AHCICFisRsvd2:1;
  UINT8    AHCICFisCmdInd:1;
  UINT8    AHCICFisCmd;
  UINT8    AHCICFisFeature;
  UINT8    AHCICFisSecNum;
  UINT8    AHCICFisClyLow;
  UINT8    AHCICFisClyHigh;
  UINT8    AHCICFisDevHead;
  UINT8    AHCICFisSecNumExp;
  UINT8    AHCICFisClyLowExp;
  UINT8    AHCICFisClyHighExp;
  UINT8    AHCICFisFeatureExp;
  UINT8    AHCICFisSecCount;
  UINT8    AHCICFisSecCountExp;
  UINT8    AHCICFisRsvd3;
  UINT8    AHCICFisControl;
  UINT8    AHCICFisRsvd4[4];
  UINT8    AHCICFisRsvd5[44];
} AHCI_COMMAND_FIS;

//
// ACMD: ATAPI command (12 or 16 bytes)
//
typedef struct {
  UINT8    AtapiCmd[0x10];
} AHCI_ATAPI_COMMAND;

//
// Physical Region Descriptor Table includes up to 65535 entries
// The entry data structure is listed at the following.
// the actual entry number comes from the PRDTL field in the command
// list entry for this command slot. 
//
typedef struct {
  UINT32   AHCIPrdtDba;       //Data Base Address
  UINT32   AHCIPrdtDbau;      //Data Base Address Upper 32-BITs
  UINT32   AHCIPrdtRsvd;
  UINT32   AHCIPrdtDbc:22;    //Data Byte Count
  UINT32   AHCIPrdtRsvd1:9;
  UINT32   AHCIPrdtIoc:1;     //Interrupt on Completion
} AHCI_COMMAND_PRDT;

//
// Command table data strucute which is pointed to by the entry in the command list
//
typedef struct {
  AHCI_COMMAND_FIS      CommandFis;       // A software constructed FIS.
  AHCI_ATAPI_COMMAND    AtapiCmd;         // 12 or 16 bytes ATAPI cmd.
  UINT8                 Reserved[0x30];
  AHCI_COMMAND_PRDT     PrdtTable[65535];     // The scatter/gather list for data transfer
} AHCI_COMMAND_TABLE;

//
// Received FIS structure
//
typedef struct {
  UINT8    AHCIDmaSetupFis[0x1C];         // Dma Setup Fis: offset 0x00
  UINT8    AHCIDmaSetupFisRsvd[0x04];
  UINT8    AHCIPioSetupFis[0x14];         // Pio Setup Fis: offset 0x20
  UINT8    AHCIPioSetupFisRsvd[0x0C];     
  UINT8    AHCID2HRegisterFis[0x14];      // D2H Register Fis: offset 0x40
  UINT8    AHCID2HRegisterFisRsvd[0x04];
  UINT64   AHCISetDeviceBitsFis;          // Set Device Bits Fix: offset 0x58
  UINT8    AHCIUnknownFis[0x40];          // Unkonwn Fis: offset 0x60
  UINT8    AHCIUnknownFisRsvd[0x60];      
} AHCI_RECEIVED_FIS; 

#pragma pack()

typedef struct {
  AHCI_RECEIVED_FIS     	*AHCIRFis;
  AHCI_COMMAND_LIST    		*AHCICmdList;
  AHCI_COMMAND_TABLE    	*AHCICommandTable;
  AHCI_RECEIVED_FIS     	*AHCIRFisPciAddr;
  AHCI_COMMAND_LIST         *AHCICmdListPciAddr;
  AHCI_COMMAND_TABLE    	*AHCICommandTablePciAddr;
  UINT64                    MaxCommandListSize;
  UINT64                    MaxCommandTableSize;
  UINT64                    MaxReceiveFisSize;
  VOID                      *MapRFis;
  VOID                      *MapCmdList;
  VOID                      *MapCommandTable;
} AHCI_REGISTERS;


RETURN_STATUS AHCISupported(DEVICE_NODE * Dev);
RETURN_STATUS AHCIStart(DEVICE_NODE * Dev);
RETURN_STATUS AHCIStop(DEVICE_NODE * Dev);

UINT32 AHCIRegRead(DEVICE_NODE * Dev, UINT32 Offset);
void AHCIRegWrite(DEVICE_NODE * Dev, UINT32 Offset, UINT32 Value);

VOID
AHCIRegAnd (
  IN DEVICE_NODE 		  * Dev,
  IN UINT32               Offset,
  IN UINT32               AndData
  );
VOID
AHCIRegOr (
  IN DEVICE_NODE 		  * Dev,
  IN UINT32               Offset,
  IN UINT32               OrData
  );


void AHCIControllerInit(DEVICE_NODE * Dev);

RETURN_STATUS
AHCICheckDeviceStatus (
  IN  DEVICE_NODE * Dev,
  IN  UINT8	Port
  );

RETURN_STATUS
AHCIWaitMmioSet (
  IN  DEVICE_NODE 		  		* Dev,
  IN  UINT32                    Offset,
  IN  UINT32                    MaskValue,
  IN  UINT32                    TestValue,
  IN  UINT64                    Timeout
  );

RETURN_STATUS
AHCIWaitMemSet (
  IN  void*				        Address,
  IN  UINT32                    MaskValue,
  IN  UINT32                    TestValue,
  IN  UINT64                    Timeout
  );

RETURN_STATUS
AHCICheckMemSet (
  IN     void*                     Address,
  IN     UINT32                    MaskValue,
  IN     UINT32                    TestValue
  );

VOID
AHCIClearPortStatus (
  IN  DEVICE_NODE 			*Dev,
  IN  UINT8                  Port
  );

RETURN_STATUS
AHCIEnableFisReceive (
  IN  DEVICE_NODE				*Dev,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  );

RETURN_STATUS
AHCIDisableFisReceive (
  IN  DEVICE_NODE				*Dev,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  );

VOID
AHCIBuildCommand (
  IN  	 DEVICE_NODE					*Dev,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     AHCI_COMMAND_FIS       	*CommandFis,
  IN     AHCI_COMMAND_LIST      	*CommandList,
  IN     AHCI_ATAPI_COMMAND     	*AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,
  IN     UINT8                      CommandSlotNumber,
  IN OUT VOID                       *DataPhysicalAddr,
  IN     UINT32                     DataLength
  );

RETURN_STATUS
AHCIStartCommand (
  IN  DEVICE_NODE				*Dev,
  IN  UINT8                     Port,
  IN  UINT8                     CommandSlot,
  IN  UINT64                    Timeout
  );

RETURN_STATUS
AHCIStopCommand (
  IN  DEVICE_NODE				*Dev,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  );

RETURN_STATUS
AHCIPortReset (
  IN  DEVICE_NODE				*Dev,
  IN  UINT8                     Port,
  IN  UINT64                    Timeout
  );

RETURN_STATUS
AHCIDmaTransfer (
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
  );

#endif