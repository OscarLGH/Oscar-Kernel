#include "NvidiaGpu.h"
#include "NvidiaChannel.h"
#include "NvidiaFence.h"

RETURN_STATUS NvidiaMemoryCopy(NVIDIA_GPU * Gpu, UINT64 SrcOffset, UINT64 DstOffset, UINT64 Len)
{
	NV_CHANNEL * Chan = Gpu->MemoryCopyEngine->CopyChannel;
	SpinLockIrqSave(&Gpu->MemoryCopyEngine->SpinLock);
	Len = Len / 0x1000 ? Len : 0x1000;

	//KDEBUG("Chan->User[0x5c / 4] = %x\n", Chan->User[0x5c / 4]);

	RING_SPACE(Chan, 2);
	BEGIN_NVC0(Chan, 4, 0x0000, 1);
	OUT_RING(Chan, Gpu->MemoryCopyEngine->Commands.Class);

	RING_SPACE(Chan, 10);
	BEGIN_NVC0(Chan, 4, 0x0400, 8);
	OUT_RING(Chan, SrcOffset >> 32);
	OUT_RING(Chan, SrcOffset);
	OUT_RING(Chan, DstOffset >> 32);
	OUT_RING(Chan, DstOffset);

	OUT_RING(Chan, 0x1000);
	OUT_RING(Chan, 0x1000);
	OUT_RING(Chan, 0x1000);
	OUT_RING(Chan, Len / 0x1000);
	BEGIN_IMC0(Chan, 4, 0x0300, 0x0386);
	FIRE_RING(Chan);
	
	Chan->ChannelFence->FenceEmit(Chan);
	Chan->ChannelFence->FenceWait(Chan);
	//Chan->ChannelFence->FenceSync(Chan);
	SpinUnlockIrqRestore(&Gpu->MemoryCopyEngine->SpinLock);
	
	return RETURN_SUCCESS;
}