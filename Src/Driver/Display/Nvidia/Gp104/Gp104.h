#include "../Common/Bios.h"
#include "../Common/Vram.h"
#include "../Common/FifoChannel.h"
#include "../Common/Class.h"

RETURN_STATUS Gp104RamOneInit(SUBDEV_MEM * Vram);
RETURN_STATUS Gp104DisplayInit(DISPLAY_ENGINE * Engine);
RETURN_STATUS Gp104CopyEngineInit(MEMORY_COPY_ENGINE * Engine);