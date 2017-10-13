#include "Gp104.h"

VOID Gp104Constructor(NVIDIA_GPU * Gpu)
{
	KDEBUG("NVIDIA:Chipset:GP104.\n");

	Gpu->VBios = KMalloc(sizeof(SUBDEV_BIOS));
	memset(Gpu->VBios, 0, sizeof(SUBDEV_BIOS));
	Gpu->VBios->Parent = Gpu;
	Gpu->VBios->Init = NvidiaBiosInit;

	Gpu->VramManager = KMalloc(sizeof(SUBDEV_MEM));
	memset(Gpu->VramManager, 0, sizeof(SUBDEV_MEM));
	Gpu->VramManager->Parent = Gpu;
	Gpu->VramManager->Init = Gp104RamOneInit;

	Gpu->Therm = KMalloc(sizeof(SUBDEV_THERM));
	memset(Gpu->Therm, 0, sizeof(SUBDEV_THERM));
	Gpu->Therm->Parent = Gpu;

	Gpu->MemoryCopyEngine = KMalloc(sizeof(MEMORY_COPY_ENGINE));
	memset(Gpu->MemoryCopyEngine, 0, sizeof(MEMORY_COPY_ENGINE));
	Gpu->MemoryCopyEngine->Parent = Gpu;
	Gpu->MemoryCopyEngine->Init = Gp104CopyEngineInit;

	Gpu->DisplayEngine = KMalloc(sizeof(DISPLAY_ENGINE));
	memset(Gpu->DisplayEngine, 0, sizeof(DISPLAY_ENGINE));
	Gpu->DisplayEngine->Parent = Gpu;
	Gpu->DisplayEngine->Init = Gp104DisplayInit;

	Gpu->GraphicEngine = KMalloc(sizeof(GRAPHIC_ENGINE));
	memset(Gpu->GraphicEngine, 0, sizeof(GRAPHIC_ENGINE));
	Gpu->GraphicEngine->Parent = Gpu;

}

VOID Gp104Destructor(NVIDIA_GPU * Gpu)
{
	KDEBUG("GP104 Destructor\n");

	if (Gpu->VBios)
		KFree(Gpu->VBios);
	if (Gpu->VramManager)
		KFree(Gpu->VramManager);
	if (Gpu->Therm)
		KFree(Gpu->Therm);
	if (Gpu->DisplayEngine)
		KFree(Gpu->DisplayEngine);
	if (Gpu->GraphicEngine)
		KFree(Gpu->GraphicEngine);
}