#include "../Common/Bios.h"
#include "../Common/Vram.h"
#include "../Common/FifoChannel.h"
#include "../Common/Class.h"

RETURN_STATUS Gk104RamOneInit(SUBDEV_MEM * Vram);
RETURN_STATUS Gk104DisplayInit(DISPLAY_ENGINE * Engine);
RETURN_STATUS Gk104CopyEngineInit(MEMORY_COPY_ENGINE * Engine);