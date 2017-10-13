#include "Gp104.h"

RETURN_STATUS Gp104CopyEngineInit(MEMORY_COPY_ENGINE * Engine)
{
	NVIDIA_GPU * Gpu = Engine->Parent;

	Engine->Commands.Class = PASCAL_DMA_COPY_A;
	NvidiaChannelNew(Engine->Parent, &Engine->CopyChannel);
	NvidiaChannelInit(Engine->Parent, Engine->CopyChannel);

	Engine->HostLength = 0x1000000;
	Engine->HostBuffer = KMalloc(Engine->HostLength);
	Engine->HostVma = Gpu->VramManager->VmAllocate(Gpu->VramManager, Engine->HostLength, 0x1000, 1);
}

