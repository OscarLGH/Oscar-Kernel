#include "../Common/Bios.h"
#include "../Common/Vram.h"
#include "../Common/FifoChannel.h"
#include "../Common/Class.h"


#define NV_PTOP_SCAL_NUM_FBPAS                          0x0002243C /* R--4R */

#define NV_PTOP_SCAL_NUM_FBPAS_VALUE                           4:0 /* R-IVF */


#define NV_PTOP_SCAL_NUM_FBPA_PER_FBP                   0x00022458 /* R--4R */

#define NV_PTOP_SCAL_NUM_FBPA_PER_FBP_VALUE                    4:0 /* R-IVF */


#define NV_FUSE_STATUS_OPT_ROP_L2_FBP(i)        (0x00021d70+(i)*4) /* R-I4A */
#define NV_FUSE_STATUS_OPT_ROP_L2_FBP__SIZE_1                   16 /*       */
#define NV_FUSE_STATUS_OPT_ROP_L2_FBP_DATA                    31:0 /* R-IVF */

#define NV_FUSE_STATUS_OPT_FBIO                         0x00021C14 /* R-I4R */
#define NV_FUSE_STATUS_OPT_FBIO_DATA                          15:0 /* R-IVF */

#define NV_PFB_FBHUB_NUM_ACTIVE_FBPS                    0x00100800 /* RW-4R */
#define NV_PFB_FBHUB_NUM_ACTIVE_FBPS_MIXED_MEM_DENSITY         4:4 /*       */

#define NV_PMC_INTR_MODE(i)             (0x00000120+(i)*4) /* R--4A */
#define NV_PMC_INTR_EN_SET(i)           (0x00000160+(i)*4) /* -W-4A */
#define NV_PMC_INTR_EN_CLEAR(i)         (0x00000180+(i)*4) /* -W-4A */
#define NV_PMC_INTR_SW(i)               (0x000001A0+(i)*4) /* RW-4A */
#define NV_PMC_INTR_LTC                         0x000001C0 /* R--4R */
#define NV_PMC_INTR_FBPA                        0x000001D0 /* R--4R */


#define NV_PMC_INTR_EN(i)                           (0x00000140+(i)*4)


RETURN_STATUS Gp100RamOneInit(SUBDEV_MEM * Vram);
RETURN_STATUS Gp100DisplayInit(DISPLAY_ENGINE * Engine);
RETURN_STATUS Gp100CopyEngineInit(MEMORY_COPY_ENGINE * Engine);
