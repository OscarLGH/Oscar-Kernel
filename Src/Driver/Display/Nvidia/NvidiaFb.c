#include "NvidiaGpu.h"
#include "Common/Vram.h"
#include "Common/Class.h"
#include "SysConsole.h"
#include "NvidiaFence.h"

RETURN_STATUS NvidiaMemoryCopy(NVIDIA_GPU * Gpu, UINT64 SrcOffset, UINT64 DstOffset, UINT64 Len);

RETURN_STATUS NvidiaFbDrawPoint(FB_DEVICE * FbDev, UINT32 x, UINT32 y, UINT32 Color)
{
	*((UINT32 *)FbDev->ScreenInfo.FrameBufferBase + y * FbDev->ScreenInfo.PixelsPerScanLine + x) = Color;
	return RETURN_SUCCESS;
}

RETURN_STATUS NvidiaFbFillArea(FB_DEVICE * FbDev, RECTANGLE * Area, UINT32 Color)
{
	NVIDIA_GPU * Gpu = FbDev->Device;
	UINT64 BufferLen = 0x20000;
	UINT32 * Buffer = Gpu->MemoryCopyEngine->HostBuffer;
	UINT64 Index;
	for (Index = 0; Index < BufferLen / 4; Index++)
	{
		Buffer[Index] = Color;
	}

	UINT64 HostVma = Gpu->MemoryCopyEngine->HostVma;
	Gpu->VramManager->Map(Gpu->VramManager, HostVma, VIRT2PHYS(Buffer), BufferLen, NV_MEM_TARGET_HOST, 1);

	UINT64 ScreenLimit = FbDev->ScreenInfo.PixelsPerScanLine * FbDev->ScreenInfo.ScreenHeight * 4;
	for (Index = 0; Index < ScreenLimit; Index += BufferLen)
	{
		NvidiaMemoryCopy(FbDev->Device, HostVma, Index, BufferLen);
	}
	return RETURN_SUCCESS;
}

RETURN_STATUS NvidiaFbCopyArea(FB_DEVICE * FbDev, RECTANGLE * Src, RECTANGLE * Dst)
{
	UINT64 SrcVma = (Src->PosY * FbDev->ScreenInfo.PixelsPerScanLine + Src->PosX) * 4;
	UINT64 DstVma = (Dst->PosY * FbDev->ScreenInfo.PixelsPerScanLine + Dst->PosX) * 4;
	/* The length of memory must be multiple of 0x1000. */
	UINT64 Length = Src->Height * FbDev->ScreenInfo.PixelsPerScanLine * 4;
	//KDEBUG("NVIDIA:SrcVma = %x, DstVma = %x, Length = %x\n", SrcVma, DstVma, Length);
	NvidiaMemoryCopy(FbDev->Device, SrcVma, DstVma, Length);
	MicroSecondDelay(5000);
	return RETURN_SUCCESS;
}

RETURN_STATUS NvidiaFbImageBlit(FB_DEVICE * FbDev, IMAGE * Image)
{
	NVIDIA_GPU * Gpu = FbDev->Device;
	UINT64 BufferLen = Gpu->MemoryCopyEngine->HostLength;
	UINT32 * Buffer = Gpu->MemoryCopyEngine->HostBuffer;
	UINT64 Index;
	for (Index = 0; Index < BufferLen / 4; Index++)
	{
		Buffer[Index] = 0;
	}

	UINT64 HostVma = Gpu->MemoryCopyEngine->HostVma;
	Gpu->VramManager->Map(Gpu->VramManager, HostVma, VIRT2PHYS(Buffer), BufferLen, NV_MEM_TARGET_HOST, 1);

	UINT64 ScreenLimit = FbDev->ScreenInfo.PixelsPerScanLine * FbDev->ScreenInfo.ScreenHeight * 4;
	for (Index = 0; Index < ScreenLimit; Index += BufferLen)
	{
		NvidiaMemoryCopy(FbDev->Device, HostVma, Index, BufferLen);
		//MicroSecondDelay(2500);
	}
	return RETURN_SUCCESS;
}

#include "NvidiaChannel.h"
RETURN_STATUS NvidiaFbInit(NVIDIA_GPU * Gpu)
{
	extern CONSOLE SysCon;
	extern SYSTEM_PARAMETERS * SysParam;
	
	FB_DEVICE * NvidiaFb = KMalloc(sizeof(*NvidiaFb));
	printk("Switching to NVIDIA FB.\n");
	
	NvidiaFb->Device = Gpu;
	NvidiaFb->ScreenInfo.FrameBufferBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Gpu->PciDev, 1));
	printk("NvidiaFb->ScreenInfo.FrameBufferBase = %016x\n", NvidiaFb->ScreenInfo.FrameBufferBase);
	NvidiaFb->ScreenInfo.ScreenWidth = SysParam->ScreenWidth;
	NvidiaFb->ScreenInfo.ScreenHeight = SysParam->ScreenHeight;
	NvidiaFb->ScreenInfo.PixelsPerScanLine = SysParam->PixelsPerScanLine;
	NvidiaFb->ScreenInfo.BytesPerPixel = 4;
	NvidiaFb->DrawPoint = NvidiaFbDrawPoint;
	NvidiaFb->FillArea = NvidiaFbFillArea;
	NvidiaFb->ImageBlit = NvidiaFbImageBlit;
	NvidiaFb->CopyArea = NvidiaFbCopyArea;

	/* Only use 1st card.*/
	if(SysCon.FbDevice->Device == 0)
	{
		SysCon.FbDevice = NvidiaFb;
	}
	else
	{
		RECTANGLE Rect = {0, 0, 3840,  2160};
		UINT32 Color = 0xFF301024;
		NvidiaFbFillArea(NvidiaFb, &Rect, Color);
	}
	//return 0;

	printk("NVIDIA:Initializing Graphics Framebuffer...\n");
	NV_CHANNEL * Chan = Gpu->MemoryCopyEngine->CopyChannel;

#if 0
	//printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0x1f, 0x20, 1);
	FIRE_RING(Chan);
	printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	MilliSecondDelay(1000);

	//printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0x1f, 0x20, 1);
	FIRE_RING(Chan);
	printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	MilliSecondDelay(1000);

	//printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0x1f, 0x20, 1);
	FIRE_RING(Chan);
	printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	MilliSecondDelay(1000);

	//printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0x1f, 0x20, 1);
	FIRE_RING(Chan);
	printk("MMIO 0x2100:%08x\n", NvMmioRd32(Gpu, 0x2100));
	MilliSecondDelay(1000);

	return 0;
#endif
#if 0
	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0, 0x0000, 1);
	FIRE_RING(Chan);
	MilliSecondDelay(1000);

	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0, 0x0000, 1);
	FIRE_RING(Chan);
	MilliSecondDelay(1000);

	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0, 0x0000, 1);
	FIRE_RING(Chan);
	MilliSecondDelay(1000);

	RING_SPACE(Chan, 1);
	BEGIN_NVC0(Chan, 0, 0x0000, 1);
	FIRE_RING(Chan);
	MilliSecondDelay(1000);
#endif
	//UINT32 base = 0x84000;//PPVLD
	//NvMmioWr32(Gpu, base + 0x10c, 0x00000001); /* BLOCK_ON_FIFO */
	//NvMmioWr32(Gpu, base + 0x104, 0x00000000); /* ENTRY */
	//NvMmioWr32(Gpu, base + 0x100, 0x00000002); /* TRIGGER */
	//NvMmioWr32(Gpu, base + 0x048, 0x00000003); /* FIFO | CHSW */

	//NvMmioWr32(Gpu, 0x084010, 0x0000fff2);
	//NvMmioWr32(Gpu, 0x08401c, 0x0000fff2);

	//base = 0x85000;//PPDEC
	//NvMmioWr32(Gpu, base + 0x10c, 0x00000001); /* BLOCK_ON_FIFO */
	//NvMmioWr32(Gpu, base + 0x104, 0x00000000); /* ENTRY */
	//NvMmioWr32(Gpu, base + 0x100, 0x00000002); /* TRIGGER */
	//NvMmioWr32(Gpu, base + 0x048, 0x00000003); /* FIFO | CHSW */

	//NvMmioWr32(Gpu, 0x085010, 0x0000fff2);
	//NvMmioWr32(Gpu, 0x08501c, 0x0000fff2);

	
#if 0
	UINT8 PixelFormat = 0xe6;	//depth 32, just user 24.
	UINT64 FbVmaOffset = 0;
	
	RING_SPACE(Chan, 58);

	BEGIN_NVC0(Chan, NvSub2D, 0x0000, 1);
	OUT_RING(Chan, FERMI_TWOD_A);
	BEGIN_NVC0(Chan, NvSub2D, 0x0290, 1);
	OUT_RING(Chan, 0);
	BEGIN_NVC0(Chan, NvSub2D, 0x0888, 1);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x02ac, 1);
	OUT_RING(Chan, 3);
	BEGIN_NVC0(Chan, NvSub2D, 0x02a0, 1);
	OUT_RING(Chan, 0x55);
	BEGIN_NVC0(Chan, NvSub2D, 0x08c0, 4);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x0580, 2);
	OUT_RING(Chan, 4);
	OUT_RING(Chan, PixelFormat);
	BEGIN_NVC0(Chan, NvSub2D, 0x02e8, 2);
	OUT_RING(Chan, 2);
	OUT_RING(Chan, 1);

	BEGIN_NVC0(Chan, NvSub2D, 0x0804, 1);
	OUT_RING(Chan, PixelFormat);
	BEGIN_NVC0(Chan, NvSub2D, 0x0800, 1);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x0808, 3);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x081c, 1);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x0840, 4);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x0200, 10);
	OUT_RING(Chan, PixelFormat);
	OUT_RING(Chan, 1);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, SysParam->PixelsPerScanLine);
	OUT_RING(Chan, SysParam->ScreenWidth);
	OUT_RING(Chan, SysParam->ScreenHeight);
	OUT_RING(Chan, FbVmaOffset << 32);
	OUT_RING(Chan, FbVmaOffset);
	BEGIN_NVC0(Chan, NvSub2D, 0x0230, 10);
	OUT_RING(Chan, PixelFormat);
	OUT_RING(Chan, 1);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, 1);
	OUT_RING(Chan, 0);
	OUT_RING(Chan, SysParam->PixelsPerScanLine);
	OUT_RING(Chan, SysParam->ScreenWidth);
	OUT_RING(Chan, SysParam->ScreenHeight);
	OUT_RING(Chan, FbVmaOffset << 32);
	OUT_RING(Chan, FbVmaOffset);
	FIRE_RING(Chan);

	
	UINT32 Color = 0x00ff00ff;
	RECTANGLE Rect = { 0, 0, 640, 480 };
	/* Fill test.*/
	
	RING_SPACE(Chan, 11);
	BEGIN_NVC0(Chan, NvSub2D, 0x02ac, 1);
	OUT_RING(Chan, 1);
	BEGIN_NVC0(Chan, NvSub2D, 0x0588, 1);
	OUT_RING(Chan, Color);
	BEGIN_NVC0(Chan, NvSub2D, 0x0600, 4);
	OUT_RING(Chan, Rect.PosX);
	OUT_RING(Chan, Rect.PosY);
	OUT_RING(Chan, Rect.PosX + Rect.Width);
	OUT_RING(Chan, Rect.PosY + Rect.Height);
	BEGIN_NVC0(Chan, NvSub2D, 0x02ac, 1);
	OUT_RING(Chan, 3);

	FIRE_RING(Chan);
	
	//while (1);
	return 0;

#endif
}