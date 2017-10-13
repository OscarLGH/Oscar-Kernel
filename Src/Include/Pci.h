/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                              PCI.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
														Oscar 2015.9
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _PCI_H
#define _PCI_H

#include "Type.h"
#include "SystemParameters.h"
#include "Device.h"
#include "Lib/Memory.h"

//#include "Irq.h"

//#define DEBUG
#include "Debug.h"

#define PCI_GET_BUS(x) ((x>>8))
#define PCI_GET_DEVICE(x) (((x&0xFF)>>3))
#define PCI_GET_FUNCTION(x) ((x&0x7))

#define MAKE_PCI_ADDRESS(b,d,f) (((b&0xFF)<<8)|((d&0x1F)<<3)|(f&0x7))


//PCI Configuration Space Offsets
#define PCI_CONF_VENDORID_OFF_16 		0x0
#define PCI_CONF_VIDDID_OFF_32			0x0
#define PCI_CONF_DEVICEID_OFF_16 		0x2
#define PCI_CONF_COMMAND_OFF_16	 		0x4
#define PCI_CONF_STATUS_OFF_16   		0x6
#define PCI_CONF_REVISIONID_OFF_16 		0x8

#define PCI_CONF_INTERFACECODE_OFF_8	0x9
#define PCI_CONF_SUBCLASSCODE_OFF_8		0xA
#define PCI_CONF_BASECLASSCODE_OFF_8	0xB

#define PCI_CONF_CACHELINESIZE_OFF_8 	0xC
#define PCI_CONF_LATENCYTIMER_OFF_8		0xD
#define PCI_CONF_HEADERTYPE_OFF_8		0xE
#define PCI_CONF_BIST_OFF_8				0xF

#define PCI_CONF_BAR0_OFF_32			0x10
#define PCI_CONF_BAR0_OFF_64			0x10
#define PCI_CONF_BAR1_OFF_32			0x14
#define PCI_CONF_BAR2_OFF_32			0x18
#define PCI_CONF_BAR1_OFF_64			0x18
#define PCI_CONF_BAR3_OFF_32			0x1C
#define PCI_CONF_BAR4_OFF_32			0x20
#define PCI_CONF_BAR2_OFF_64			0x20
#define PCI_CONF_BAR5_OFF_32			0x24

#define PCI_CONF_CARDBUSCISPOINTER_OFF_32			0x28
#define PCI_CONF_SUBSYSTEMVENDORID_OFF_16			0x2C
#define PCI_CONF_SUBSYSTEMID_OFF_16					0x2E

#define PCI_CONF_EXPANSIONROMBAR_OFF_32				0x30
#define PCI_CONF_CAPABILITIESPOINTER_OFF_32			0x34

#define PCI_CONF_INTERRUPTLINE_OFF_8				0x3C
#define PCI_CONF_INTERRUPTPIN_OFF_8					0x3D
#define PCI_CONF_MINGNT_OFF_8						0x3E
#define PCI_CONF_MAXLAT_OFF_8						0x3F

#pragma pack(1)
typedef struct {
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT16  Command;
  UINT16  Status;
  UINT8   RevisionID;
  UINT8   ClassCode[3];
  UINT8   CacheLineSize;
  UINT8   LatencyTimer;
  UINT8   HeaderType;
  UINT8   BIST;
} PCI_DEVICE_INDEPENDENT_REGION;

typedef struct {
  UINT32  Bar[6];
  UINT32  CISPtr;
  UINT16  SubsystemVendorID;
  UINT16  SubsystemID;
  UINT32  ExpansionRomBar;
  UINT8   CapabilityPtr;
  UINT8   Reserved1[3];
  UINT32  Reserved2;
  UINT8   InterruptLine;
  UINT8   InterruptPin;
  UINT8   MinGnt;
  UINT8   MaxLat;
} PCI_DEVICE_HEADER_TYPE_REGION;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  PCI_DEVICE_HEADER_TYPE_REGION Device;
} PCI_TYPE00;

typedef struct {
  UINT32  Bar[2];
  UINT8   PrimaryBus;
  UINT8   SecondaryBus;
  UINT8   SubordinateBus;
  UINT8   SecondaryLatencyTimer;
  UINT8   IoBase;
  UINT8   IoLimit;
  UINT16  SecondaryStatus;
  UINT16  MemoryBase;
  UINT16  MemoryLimit;
  UINT16  PrefetchableMemoryBase;
  UINT16  PrefetchableMemoryLimit;
  UINT32  PrefetchableBaseUpper32;
  UINT32  PrefetchableLimitUpper32;
  UINT16  IoBaseUpper16;
  UINT16  IoLimitUpper16;
  UINT8   CapabilityPtr;
  UINT8   Reserved[3];
  UINT32  ExpansionRomBAR;
  UINT8   InterruptLine;
  UINT8   InterruptPin;
  UINT16  BridgeControl;
} PCI_BRIDGE_CONTROL_REGISTER;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  PCI_BRIDGE_CONTROL_REGISTER   Bridge;
} PCI_TYPE01;

typedef union {
  PCI_TYPE00  Device;
  PCI_TYPE01  Bridge;
} PCI_TYPE_GENERIC;

typedef struct {
  UINT32  CardBusSocketReg; // Cardus Socket/ExCA Base
  // Address Register
  //
  UINT16  Reserved;
  UINT16  SecondaryStatus;      // Secondary Status
  UINT8   PCI_GET_BUSNumber;         // PCI Bus Number
  UINT8   CardBusBusNumber;     // CardBus Bus Number
  UINT8   SubordinateBusNumber; // Subordinate Bus Number
  UINT8   CardBusLatencyTimer;  // CardBus Latency Timer
  UINT32  MemoryBase0;          // Memory Base Register 0
  UINT32  MemoryLimit0;         // Memory Limit Register 0
  UINT32  MemoryBase1;
  UINT32  MemoryLimit1;
  UINT32  IoBase0;
  UINT32  IoLimit0;             // I/O Base Register 0
  UINT32  IoBase1;              // I/O Limit Register 0
  UINT32  IoLimit1;
  UINT8   InterruptLine;        // Interrupt Line
  UINT8   InterruptPin;         // Interrupt Pin
  UINT16  BridgeControl;        // Bridge Control
} PCI_CARDBUS_CONTROL_REGISTER;
#pragma pack(0)

//
// Definitions of PCI class bytes and manipulation macros.
//
#define PCI_CLASS_OLD                 	0x00
#define PCI_CLASS_OLD_OTHER           	0x00
#define PCI_CLASS_OLD_VGA             	0x01

#define PCI_CLASS_MASS_STORAGE        	0x01
#define PCI_CLASS_MASS_STORAGE_SCSI   	0x00
#define PCI_CLASS_MASS_STORAGE_IDE    	0x01  // obsolete
#define PCI_CLASS_IDE                 	0x01
#define PCI_CLASS_MASS_STORAGE_FLOPPY	0x02
#define PCI_CLASS_MASS_STORAGE_IPI    	0x03
#define PCI_CLASS_MASS_STORAGE_RAID   	0x04
#define PCI_CLASS_MASS_STORAGE_SATADPA  0x06
#define   PCI_IF_MASS_STORAGE_SATA         0x00
#define   PCI_IF_MASS_STORAGE_AHCI         0x01

#define PCI_CLASS_MASS_STORAGE_OTHER  0x80


/// PCI_CLASS_WIRELESS, Base Class 0Dh.
///
///@{
#define PCI_SUBCLASS_ETHERNET_80211A    0x20
#define PCI_SUBCLASS_ETHERNET_80211B    0x21


#define PCI_CLASS_NETWORK             0x02
#define PCI_CLASS_NETWORK_ETHERNET    0x00
#define PCI_CLASS_ETHERNET            0x00  // obsolete
#define PCI_CLASS_NETWORK_TOKENRING   0x01
#define PCI_CLASS_NETWORK_FDDI        0x02
#define PCI_CLASS_NETWORK_ATM         0x03
#define PCI_CLASS_NETWORK_ISDN        0x04
#define PCI_CLASS_NETWORK_OTHER       0x80

#define PCI_CLASS_DISPLAY             0x03
#define PCI_CLASS_DISPLAY_CTRL        0x03  // obsolete
#define PCI_CLASS_DISPLAY_VGA         0x00
#define PCI_CLASS_VGA                 0x00  // obsolete
#define PCI_CLASS_DISPLAY_XGA         0x01
#define PCI_CLASS_DISPLAY_3D          0x02
#define PCI_CLASS_DISPLAY_OTHER       0x80
#define PCI_CLASS_DISPLAY_GFX         0x80
#define PCI_CLASS_GFX                 0x80  // obsolete
#define PCI_CLASS_BRIDGE              0x06
#define PCI_CLASS_BRIDGE_HOST         0x00
#define PCI_CLASS_BRIDGE_ISA          0x01
#define PCI_CLASS_ISA                 0x01  // obsolete
#define PCI_CLASS_BRIDGE_EISA         0x02
#define PCI_CLASS_BRIDGE_MCA          0x03
#define PCI_CLASS_BRIDGE_P2P          0x04
#define PCI_CLASS_BRIDGE_PCMCIA       0x05
#define PCI_CLASS_BRIDGE_NUBUS        0x06
#define PCI_CLASS_BRIDGE_CARDBUS      0x07
#define PCI_CLASS_BRIDGE_RACEWAY      0x08
#define PCI_CLASS_BRIDGE_ISA_PDECODE  0x80
#define PCI_CLASS_ISA_POSITIVE_DECODE 0x80  // obsolete

#define PCI_CLASS_SCC                 0x07  // Simple communications controllers 
#define PCI_SUBCLASS_SERIAL           0x00
#define PCI_IF_GENERIC_XT             0x00
#define PCI_IF_16450                  0x01
#define PCI_IF_16550                  0x02
#define PCI_IF_16650                  0x03
#define PCI_IF_16750                  0x04
#define PCI_IF_16850                  0x05
#define PCI_IF_16950                  0x06
#define PCI_SUBCLASS_PARALLEL         0x01
#define PCI_IF_PARALLEL_PORT          0x00
#define PCI_IF_BI_DIR_PARALLEL_PORT   0x01
#define PCI_IF_ECP_PARALLEL_PORT      0x02
#define PCI_IF_1284_CONTROLLER        0x03
#define PCI_IF_1284_DEVICE            0xFE
#define PCI_SUBCLASS_MULTIPORT_SERIAL 0x02
#define PCI_SUBCLASS_MODEM            0x03
#define PCI_IF_GENERIC_MODEM          0x00
#define PCI_IF_16450_MODEM            0x01
#define PCI_IF_16550_MODEM            0x02
#define PCI_IF_16650_MODEM            0x03
#define PCI_IF_16750_MODEM            0x04
#define PCI_SUBCLASS_OTHER            0x80

#define PCI_CLASS_SYSTEM_PERIPHERAL   0x08
#define PCI_SUBCLASS_PIC              0x00
#define PCI_IF_8259_PIC               0x00
#define PCI_IF_ISA_PIC                0x01
#define PCI_IF_EISA_PIC               0x02
#define PCI_IF_APIC_CONTROLLER        0x10 // I/O APIC interrupt controller , 32 bye none-prefectable memory.  
#define PCI_IF_APIC_CONTROLLER2       0x20 
#define PCI_SUBCLASS_TIMER            0x02
#define PCI_IF_8254_TIMER             0x00
#define PCI_IF_ISA_TIMER              0x01
#define PCI_EISA_TIMER                0x02
#define PCI_SUBCLASS_RTC              0x03
#define PCI_IF_GENERIC_RTC            0x00
#define PCI_IF_ISA_RTC                0x00
#define PCI_SUBCLASS_PNP_CONTROLLER   0x04 // HotPlug Controller

#define PCI_CLASS_INPUT_DEVICE        0x09
#define PCI_SUBCLASS_KEYBOARD         0x00
#define PCI_SUBCLASS_PEN              0x01
#define PCI_SUBCLASS_MOUSE_CONTROLLER 0x02
#define PCI_SUBCLASS_SCAN_CONTROLLER  0x03
#define PCI_SUBCLASS_GAMEPORT         0x04

#define PCI_CLASS_DOCKING_STATION     0x0A

#define PCI_CLASS_PROCESSOR           0x0B
#define PCI_SUBCLASS_PROC_386         0x00
#define PCI_SUBCLASS_PROC_486         0x01
#define PCI_SUBCLASS_PROC_PENTIUM     0x02
#define PCI_SUBCLASS_PROC_ALPHA       0x10
#define PCI_SUBCLASS_PROC_POWERPC     0x20
#define PCI_SUBCLASS_PROC_MIPS        0x30
#define PCI_SUBCLASS_PROC_CO_PORC     0x40 // Co-Processor

#define PCI_CLASS_SERIAL              0x0C
#define PCI_CLASS_SERIAL_FIREWIRE     0x00
#define PCI_CLASS_SERIAL_ACCESS_BUS   0x01
#define PCI_CLASS_SERIAL_SSA          0x02
#define PCI_CLASS_SERIAL_USB          0x03
#define PCI_CLASS_SERIAL_FIBRECHANNEL 0x04
#define PCI_CLASS_SERIAL_SMB          0x05

#define PCI_CLASS_WIRELESS            0x0D
#define PCI_SUBCLASS_IRDA             0x00
#define PCI_SUBCLASS_IR               0x01
#define PCI_SUBCLASS_RF               0x02

#define PCI_CLASS_INTELLIGENT_IO      0x0E

#define PCI_CLASS_SATELLITE           0x0F
#define PCI_SUBCLASS_TV               0x01
#define PCI_SUBCLASS_AUDIO            0x02
#define PCI_SUBCLASS_VOICE            0x03
#define PCI_SUBCLASS_DATA             0x04

#define PCI_SECURITY_CONTROLLER       0x10 // Encryption and decryption controller
#define PCI_SUBCLASS_NET_COMPUT       0x00
#define PCI_SUBCLASS_ENTERTAINMENT    0x10 

#define PCI_CLASS_DPIO                0x11

typedef struct _PCI_ADDRESS
{
	UINT8 Bus		: 8;
	UINT8 Device	: 5;
	UINT8 Function	: 3;
	UINT16 Reserved0;
	UINT32 Reserved1;
}PCI_ADDRESS;

typedef struct _PCI_DEVICE
{
	PCI_ADDRESS PciAddress;

	UINT64 PciCfgIoBase;
	PCI_TYPE_GENERIC *PciCfgMmioBase;
	
	UINT64 InterruptCap;	
	/* BIT0: By IntX, BIT1:By MSI, BIT2:By MSI-X , BIT7:64 bit supported, BIT8:MaskBits Supported. 
	BIT16-BIT23: CapOffset
	*/

	/*UINT64(*PciReadConfig)(struct _PCI_DEVICE *Dev, UINT64 Offset, UINT64 Length);
	VOID(*PciWriteConfig)(struct _PCI_DEVICE *Dev, UINT64 Offset, UINT64 Length, UINT64 Value);

	UINT64(*PciGetBarBase)(struct _PCI_DEVICE *Dev, UINT64 BarIndex);
	UINT64(*PciGetBarSize)(struct _PCI_DEVICE *Dev, UINT64 BarIndex);*/

}PCI_DEVICE;

typedef struct _CAPABILITY_ENTRY_HEADER
{
	UINT8 CapabilityId;
	UINT8 NextPtr;
}CAPABILITY_ENTRY_HEADER;

typedef struct _MSI_CAP_STRUCTURE_32
{
	CAPABILITY_ENTRY_HEADER Header;
	UINT16 MessageControl;
	UINT32 MessageAddr;
	UINT16 MessageData;
}MSI_CAP_STRUCTURE_32;

typedef struct _MSI_CAP_STRUCTURE_64
{
	CAPABILITY_ENTRY_HEADER Header;
	UINT16 MessageControl;
	UINT64 MessageAddr;
	UINT16 MessageData;
}MSI_CAP_STRUCTURE_64;

typedef struct _MSI_CAP_STRUCTURE_32_MASK
{
	CAPABILITY_ENTRY_HEADER Header;
	UINT16 MessageControl;
	UINT32 MessageAddr;
	UINT16 MessageData;
	UINT16 Reserved;
	UINT32 MaskBits;
	UINT32 PendingBits;
}MSI_CAP_STRUCTURE_32_MASK;

typedef struct _MSI_CAP_STRUCTURE_64_MASK
{
	CAPABILITY_ENTRY_HEADER Header;
	UINT16 MessageControl;
	UINT64 MessageAddr;
	UINT16 MessageData;
	UINT16 Reserved;
	UINT32 MaskBits;
	UINT32 PendingBits;
}MSI_CAP_STRUCTURE_64_MASK;

typedef struct _PCI_MSI_X_ENTRY
{
	UINT64 MessageAddr;
	UINT32 MessageData;
	UINT32 VectorControl;
}PCI_MSI_X_ENTRY;

typedef struct _PCI_HOST_BRIDGE
{
	UINT64 MmioBaseAddress;
	UINT64 IoBaseAddress;
	UINT64 SegmentNumber;
	UINT64 StartBusNum;
	UINT64 EndBusNum;
	UINT64 Reserved;

	RETURN_STATUS (*PciCfgRead)(struct _PCI_HOST_BRIDGE *HostBridge, PCI_DEVICE *PciDev, UINT64 Offset, UINT64 Length, VOID *Data);
	RETURN_STATUS (*PciCfgWrite)(struct _PCI_HOST_BRIDGE *HostBridge, PCI_DEVICE *PciDev, UINT64 Offset, UINT64 Length, VOID *Data);
}PCI_HOST_BRIDGE;

UINT8 PciCfgRead8(PCI_DEVICE *PciDev, UINT64 Offset);
UINT16 PciCfgRead16(PCI_DEVICE *PciDev, UINT64 Offset);
UINT32 PciCfgRead32(PCI_DEVICE *PciDev, UINT64 Offset);

UINT64 PciCfgWrite8(PCI_DEVICE *PciDev, UINT64 Offset,UINT8 Value);
UINT64 PciCfgWrite16(PCI_DEVICE *PciDev, UINT64 Offset,UINT16 Value);
UINT64 PciCfgWrite32(PCI_DEVICE *PciDev, UINT64 Offset,UINT32 Value);

UINT64 PciGetBarBase(PCI_DEVICE *PciDev, UINT64 BarIndex);
UINT64 PciGetBarSize(PCI_DEVICE *PciDev, UINT64 BarIndex);

#define PCI_MSI_CAP_ID 0x5
#define PCI_MSIX_CAP_ID 0x11

UINT8 PciCapabilityFind(PCI_DEVICE *PciDev, UINT8 CapId);

RETURN_STATUS PciMsiEnable(PCI_DEVICE *PciDev);
RETURN_STATUS PciMsiCheck(PCI_DEVICE *PciDev);
UINT8 PciMsiGetMultipleMessageCapable(PCI_DEVICE *PciDev);
RETURN_STATUS PciMsiMultipleMessageCapableEnable(PCI_DEVICE *PciDev, UINT8 Vectors);
RETURN_STATUS PciMsiSet(PCI_DEVICE *PciDev, MSI_CAP_STRUCTURE_64_MASK * MsiData);



void PciScan(void);

#endif
