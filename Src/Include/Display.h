#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "Device.h"

typedef struct _RECTANGLE
{
	UINT64 PosX;
	UINT64 PosY;
	UINT64 Width;
	UINT64 Height;
}RECTANGLE;

typedef struct _IMAGE
{
	RECTANGLE Rect;
	UINT64 PixelFormat;
	UINT32 * ImageBuffer;
}IMAGE;

typedef struct
{
	UINT64 ScreenWidth;
	UINT64 ScreenHeight;
	UINT64 PixelsPerScanLine;
	UINT64 BytesPerPixel;
	UINT64 PixelFormat;
	UINT64 Reserved;
	void * FrameBufferBase;
	UINT64 FrameBufferLength;
}SCREEN_INFO;

typedef struct _FB_DEVICE
{
	SCREEN_INFO ScreenInfo;
	//UINT64 PixelFormat;
	VOID * Device;

	RETURN_STATUS(*DrawPoint)(struct _FB_DEVICE * Fb, UINT32 PosX, UINT32 PosY, UINT32 Color);
	RETURN_STATUS(*FillArea)(struct _FB_DEVICE * Fb, RECTANGLE * Area, UINT32 Color);
	RETURN_STATUS(*ImageBlit)(struct _FB_DEVICE * Fb, IMAGE * Image);
	RETURN_STATUS(*CopyArea)(struct _FB_DEVICE * Fb, RECTANGLE * Src, RECTANGLE * Dst);
	
}FB_DEVICE;

typedef struct _DISPLAY_NODE
{
	FB_DEVICE * FbDevice;
	struct _DISPLAY_NODE * Next;
}DISPLAY_NODE;

extern DRIVER_NODE * DriverList[32];

extern DEVICE_NODE DeviceRoot;
extern DRIVER_NODE BasicFBDriver;
extern DISPLAY_NODE * DisplayAdapterList;

void DisplayEarlyInit(void);

DISPLAY_NODE * DisplayOpen(UINT32 DevNum);
void DisplayAdapterRegister(FB_DEVICE * Dev);


#endif
