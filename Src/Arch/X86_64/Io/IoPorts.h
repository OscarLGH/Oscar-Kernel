#ifndef _IOPORTS_H
#define _IOPORTS_H

#include "Type.h"
//=====================================================================
//IOPort.h
//常用IO端口操作
//Oscar 2015.8
//=====================================================================

UINT8 	In8(UINT16 Port);
UINT16 	In16(UINT16 Port);	
UINT32 	In32(UINT16 Port);
void	Out8(UINT16 Port,UINT8 Value);
void	Out16(UINT16 Port,UINT16 Value);
void	Out32(UINT16 Port,UINT32 Value);

#define IOPIC1						0x20
#define IOTIMER0					0x40
#define IOTIMER1					0x50

#define IOKEYBOARDBUFFER 0x60
#define IOKEYBOARDSTATUS 0x64

#define IORTC						0x70

#define IOPIC2						0xA0
#define IOSERIAL1				0x2F8
#define IOIDE1						0x170
#define IOSERIAL2				0x3F8

#define GetKeyboardStatus() In8(IOKEYBOARDSTATUS)
#define GetKeyboardData() In8(IOKEYBOARDBUFFER)
#define IOReadPCIConf(pci) (Out32(0xCF8,pci),(In32(0xCFC)))

#endif
