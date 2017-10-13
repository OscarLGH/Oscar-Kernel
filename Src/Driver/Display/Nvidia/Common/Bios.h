#include "../NvidiaGpu.h"

RETURN_STATUS NvidiaBiosInit(SUBDEV_BIOS * Bios);
UINT16 DcbTable(SUBDEV_BIOS * Bios, UINT8 * Ver, UINT8 * Hdr, UINT8 * Cnt, UINT8 *Len);