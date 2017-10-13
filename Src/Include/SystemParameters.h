//=====================================================================
//SystemParameters.h
//Oscar 2015.4
//=====================================================================
#ifndef _SYSTEMPARAMETERS_H
#define _SYSTEMPARAMETERS_H

extern int KERNEL_SPACE;
#define SYSTEMPARABASE (0x10000+(UINT64)&KERNEL_SPACE) //此处不加括号将导致BUG！！！
#define PHYS2VIRT(x) (((UINT64)(x))+(UINT64)&KERNEL_SPACE)
#define VIRT2PHYS(x) (((UINT64)(x))-(UINT64)&KERNEL_SPACE)

#pragma pack(1)
typedef struct
{
	UINT64 BaseAddr;
	UINT64 Length;
	UINT64 Type;
	UINT64 Reserved;
}ARDS;
#pragma pack(0)

typedef struct
{
	UINT64 AcpiRsdp;
	UINT64 ScreenWidth;
	UINT64 ScreenHeight;
	UINT64 PixelsPerScanLine;
	UINT64 FrameBufferBase;
	UINT64 ProcessorCnt;
	UINT64 CpuIdArray[512];
	UINT64 Ardc_Cnt;
	ARDS   ArdcArray[256];
}SYSTEM_PARAMETERS;

#endif


