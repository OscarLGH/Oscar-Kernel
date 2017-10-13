#include "Display.h"
#include "../../Include/SystemParameters.h"

RETURN_STATUS VesaFbDrawPoint(FB_DEVICE * FbDev, UINT32 x, UINT32 y, UINT32 Color)
{
	*((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + y * FbDev->ScreenInfo.PixelsPerScanLine + x) = Color;
	return RETURN_SUCCESS;
}

RETURN_STATUS VesaFbFillArea(FB_DEVICE * FbDev, RECTANGLE * Area, UINT32 Color)
{
	UINT64 PosX = 0,PosY = 0;
	UINT32 * Pointer = NULL;
	UINT64 Value;
	//printk("x = %d, y = %d, w = %d, h = %d\n", Area->PosX, Area->PosY, Area->Width, Area->Height);
	//printk("FbDev->ScreenInfo.FrameBufferBase = %x, PixelsPerScanLine = %d\n", FbDev->ScreenInfo.FrameBufferBase, FbDev->ScreenInfo.PixelsPerScanLine);
	//printk("%x\n", Color * 165);
	//while (1);
	//return 0;
	for (PosY = Area->PosY; PosY < Area->PosY + Area->Height; PosY++)
	{
		for (PosX = Area->PosX; PosX < Area->PosX + Area->Width; PosX++)
		{
			//VesaFbDrawPoint(FbDev, PosX, PosY, Color);
			Pointer = (UINT32 *)FbDev->ScreenInfo.FrameBufferBase + PosY * FbDev->ScreenInfo.PixelsPerScanLine + PosX;
			//printk("Pointer = %x\n", Pointer);
			//*Pointer = 0;
			if ((UINT64)Pointer & 0xFFFF800000000000 != 0xFFFF800000000000)
			{
				printk("Invalid Pointer.\n");
			}
			else
			{
				*Pointer = Color;
			}
		}
		
		//*((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + PosY * FbDev->ScreenInfo.PixelsPerScanLine + PosX) = Color;
	}
	//MicroSecondDelay(1);
	return RETURN_SUCCESS;
}

/* This function is terribly slow! */
RETURN_STATUS VesaFbCopyArea(FB_DEVICE * FbDev, RECTANGLE * Src, RECTANGLE * Dst)
{
	INT64 XDelta = Dst->PosX > Src->PosX ? -1 : 1;
	INT64 YDelta = Dst->PosY > Src->PosY ? -1 : 1;

	INT64 SrcX = XDelta == -1 ? Src->PosX + Src->Width - 1: Src->PosX;
	INT64 SrcY = YDelta == -1 ? Src->PosY + Src->Height - 1: Src->PosY;
	
	INT64 DstX = XDelta == -1 ? Dst->PosX + Dst->Width - 1: Dst->PosX;
	INT64 DstY = YDelta == -1 ? Dst->PosY + Dst->Height - 1: Dst->PosY;
	
	UINT64 XIndex = 0;
	UINT64 YIndex = 0;

	UINT32 Color;

	for (YIndex = 0; YIndex < Src->Height; YIndex++)
	{
		/* 横坐标不置零将导致“隔行扫描” :( */
		SrcX = 0;
		DstX = 0;
		for (XIndex = 0; XIndex < Src->Width; XIndex++)
		{
			Color = *((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + SrcY * FbDev->ScreenInfo.PixelsPerScanLine + SrcX);
			*((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + DstY * FbDev->ScreenInfo.PixelsPerScanLine + DstX) = Color;
			//VesaFbDrawPoint(FbDev, DstX, DstY, 0);
			SrcX += XDelta;
			DstX += XDelta;
		}
		SrcY += YDelta;
		DstY += YDelta;
	}
	return RETURN_SUCCESS;
}

RETURN_STATUS VesaFbImageBlit(FB_DEVICE * FbDev, IMAGE * Image)
{
	INT64 SrcX = 0;
	INT64 SrcY = 0;

	INT64 DstX = Image->Rect.PosX;
	INT64 DstY = Image->Rect.PosY;

	INT64 XIndex = 0;
	INT64 YIndex = 0;

	UINT32 Color;

	for (XIndex = 0; XIndex < Image->Rect.Width; XIndex++)
		for (YIndex = 0; YIndex < Image->Rect.Height; YIndex++)
		{
			Color = *((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + SrcY * FbDev->ScreenInfo.PixelsPerScanLine + SrcX);
			*((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + DstY * FbDev->ScreenInfo.PixelsPerScanLine + DstX) = Color;

			SrcX += 1;
			SrcY += 1;
			DstX += 1;
			DstY += 1;
		}

	return RETURN_SUCCESS;
}

RETURN_STATUS VesaFbStart(DEVICE_NODE * Dev)
{
	FB_DEVICE * VesaFb = NULL;
	VesaFb = KMalloc(sizeof(FB_DEVICE));
	SYSTEM_PARAMETERS *SysParam = (SYSTEM_PARAMETERS *)SYSTEMPARABASE;
	VesaFb->ScreenInfo.FrameBufferBase = (UINT32 *)PHYS2VIRT(SysParam->FrameBufferBase);
	VesaFb->ScreenInfo.ScreenWidth = SysParam->ScreenWidth;
	VesaFb->ScreenInfo.ScreenHeight = SysParam->ScreenHeight;
	VesaFb->ScreenInfo.PixelsPerScanLine = SysParam->PixelsPerScanLine;
	VesaFb->ScreenInfo.BytesPerPixel = 4;
	VesaFb->DrawPoint = VesaFbDrawPoint;
	VesaFb->FillArea = VesaFbFillArea;
	VesaFb->ImageBlit = VesaFbImageBlit;
	VesaFb->CopyArea = VesaFbCopyArea;

	DisplayAdapterRegister(VesaFb);
	return RETURN_SUCCESS;
}
