#include "NvidiaFence.h"

RETURN_STATUS NvFenceEmit32(NV_CHANNEL * Chan)
{
	RETURN_STATUS Status;
	NV_FENCE * Fence = Chan->ChannelFence;
	UINT64 Virtual = Fence->FenceVma;
	UINT64 Sequence = Fence->Sequence;
	
	Status = RING_SPACE(Chan, 6);
	
	if (Status == RETURN_SUCCESS)
	{
		BEGIN_NVC0(Chan, 0, NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH, 5);
		OUT_RING(Chan, Virtual << 32);
		OUT_RING(Chan, Virtual);
		OUT_RING(Chan, Sequence);
		OUT_RING(Chan, NV84_SUBCHAN_SEMAPHORE_TRIGGER_WRITE_LONG);
		OUT_RING(Chan, 0x00000000);
		FIRE_RING(Chan);
	}
}

RETURN_STATUS NvFenceSync32(NV_CHANNEL * Chan)
{
	RETURN_STATUS Status;
	NV_FENCE * Fence = Chan->ChannelFence;
	UINT64 Virtual = Fence->FenceVma;
	UINT64 Sequence = Fence->Sequence;

	Status = RING_SPACE(Chan, 6);
	if (Status == RETURN_SUCCESS)
	{
		BEGIN_NVC0(Chan, 0, NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH, 4);
		OUT_RING(Chan, Virtual << 32);
		OUT_RING(Chan, Virtual);
		OUT_RING(Chan, Sequence);
		OUT_RING(Chan, NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_GEQUAL |
			NVC0_SUBCHAN_SEMAPHORE_TRIGGER_YIELD);
		FIRE_RING(Chan);
	}
}

UINT32 NvFenceRead(NV_CHANNEL * Chan)
{
	NV_FENCE * Fence = Chan->ChannelFence;
	return GpuObjRd32(Fence->Fence, 0);
}

RETURN_STATUS NvFenceWait(NV_CHANNEL * Chan)
{
	NV_FENCE * Fence = Chan->ChannelFence;
	UINT32 Value;
	
	do 
	{
		Value = NvFenceRead(Chan);
		//KDEBUG("Sequence = %08x,Value = %x\n", Fence->Sequence, Value);
	} while (Value != Fence->Sequence);

	Fence->Sequence++;
	
	GpuObjWr32(Fence->Fence, 0, 0);
	return RETURN_SUCCESS;
}

UINT32 FenceSequence = 0x12345678;

RETURN_STATUS NvFenceCreate(NVIDIA_GPU * Gpu, NV_FENCE ** FencePtr)
{
	RETURN_STATUS Status = RETURN_SUCCESS;

	*FencePtr = KMalloc(sizeof(**FencePtr));
	(*FencePtr)->Context = 0;
	(*FencePtr)->Sequence = FenceSequence++;
	Status = NvGpuObjNew(Gpu, 0x1000, 0x1000, &(*FencePtr)->Fence);

	(*FencePtr)->FenceVma = (*FencePtr)->Fence->Addr;
	(*FencePtr)->FenceEmit = NvFenceEmit32;
	(*FencePtr)->FenceSync = NvFenceSync32;
	(*FencePtr)->FenceRead = NvFenceRead;
	(*FencePtr)->FenceWait = NvFenceWait;

	return RETURN_SUCCESS;
}