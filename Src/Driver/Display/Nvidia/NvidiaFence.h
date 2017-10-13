#ifndef _NVIDIA_FENCE_H
#define _NVIDIA_FENCE_H

#include "NvidiaChannel.h"

struct _NV_CHANNEL;
typedef struct _NV_FENCE
{
	NV_GPU_OBJ * Fence;
	UINT64 FenceVma;

	UINT32(*FenceRead)(struct _NV_CHANNEL * Chan);
	RETURN_STATUS(*FenceEmit)(struct _NV_CHANNEL * Chan);
	RETURN_STATUS(*FenceSync)(struct _NV_CHANNEL * Chan);
	RETURN_STATUS(*FenceWait)(struct _NV_CHANNEL * Chan);

	UINT32 Sequence;
	UINT32 Context;

}NV_FENCE;

RETURN_STATUS NvFenceCreate(NVIDIA_GPU * Gpu, NV_FENCE ** FencePtr);

#endif
