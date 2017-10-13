#ifndef _NVIDIA_GPU_H
#define _NVIDIA_GPU_H

#include "Display.h"
#include "Pci.h"
#include "Timer.h"
//#define DEBUG
#include "Debug.h"

#define NVIDIA_VENDOR_ID 0x10DE

struct _NVIDIA_GPU;
struct _NV_OBJECT;

enum NvSubDevIndex {
	NVKM_SUBDEV_PCI,
	NVKM_SUBDEV_VBIOS,
	NVKM_SUBDEV_DEVINIT,
	NVKM_SUBDEV_TOP,
	NVKM_SUBDEV_IBUS,
	NVKM_SUBDEV_GPIO,
	NVKM_SUBDEV_I2C,
	NVKM_SUBDEV_FUSE,
	NVKM_SUBDEV_MXM,
	NVKM_SUBDEV_MC,
	NVKM_SUBDEV_BUS,
	NVKM_SUBDEV_TIMER,
	NVKM_SUBDEV_INSTMEM,
	NVKM_SUBDEV_FB,
	NVKM_SUBDEV_LTC,
	NVKM_SUBDEV_MMU,
	NVKM_SUBDEV_BAR,
	NVKM_SUBDEV_PMU,
	NVKM_SUBDEV_VOLT,
	NVKM_SUBDEV_ICCSENSE,
	NVKM_SUBDEV_THERM,
	NVKM_SUBDEV_CLK,
	NVKM_SUBDEV_SECBOOT,

	NVKM_ENGINE_BSP,

	NVKM_ENGINE_CE0,
	NVKM_ENGINE_CE1,
	NVKM_ENGINE_CE2,
	NVKM_ENGINE_CE3,
	NVKM_ENGINE_CE4,
	NVKM_ENGINE_CE5,
	NVKM_ENGINE_CE_LAST = NVKM_ENGINE_CE5,

	NVKM_ENGINE_CIPHER,
	NVKM_ENGINE_DISP,
	NVKM_ENGINE_DMAOBJ,
	NVKM_ENGINE_FIFO,
	NVKM_ENGINE_GR,
	NVKM_ENGINE_IFB,
	NVKM_ENGINE_ME,
	NVKM_ENGINE_MPEG,
	NVKM_ENGINE_MSENC,
	NVKM_ENGINE_MSPDEC,
	NVKM_ENGINE_MSPPP,
	NVKM_ENGINE_MSVLD,

	NVKM_ENGINE_NVENC0,
	NVKM_ENGINE_NVENC1,
	NVKM_ENGINE_NVENC2,
	NVKM_ENGINE_NVENC_LAST = NVKM_ENGINE_NVENC2,

	NVKM_ENGINE_NVDEC,
	NVKM_ENGINE_PM,
	NVKM_ENGINE_SEC,
	NVKM_ENGINE_SW,
	NVKM_ENGINE_VIC,
	NVKM_ENGINE_VP,

	NVKM_SUBDEV_NR
};

struct _NVIDIA_GPU;

typedef struct _NV_OBJECT
{
	struct _NVIDIA_GPU *Gpu;
	UINT64 Size;
}NV_OBJECT;

typedef struct _NV_GPU_OBJ
{
	NV_OBJECT Object;
	UINT64 Addr;
	UINT64 Size;
	VOID * Map;
}NV_GPU_OBJ;

typedef struct _SUBDEV_BIOS
{
	struct _NVIDIA_GPU * Parent;
	UINT32 * MmioBase;

	UINT64 BiosSize;
	UINT8 *BiosData;

	RETURN_STATUS(*OneInit)(struct _SUBDEV_BIOS * Dev);
	RETURN_STATUS(*Init)(struct _SUBDEV_BIOS * Dev);
	
}SUBDEV_BIOS;

typedef struct _BAR_VM
{
	NV_GPU_OBJ *Mem;
	NV_GPU_OBJ *Pgd;
}BAR_VM;

typedef struct _SUBDEV_MEM
{
	struct _NVIDIA_GPU * Parent;

	UINT64 VramSize;
	UINT64 Bar1Size;
	UINT64 Bar3Size;

	MEMORY_REGION * VramRegion;
	MEMORY_REGION * Bar1Region;
	MEMORY_REGION * Bar3Region;

	BAR_VM * Bar1Vm;
	BAR_VM * Bar3Vm;

	UINT64 IoMmuEnable;
	
	RETURN_STATUS(*OneInit)(struct _SUBDEV_MEM * Dev);
	RETURN_STATUS (*Init)(struct _SUBDEV_MEM * Dev);
	RETURN_STATUS (*DirectVramRead)(struct _SUBDEV_MEM * Dev, UINT64 Buffer, UINT64 Length, UINT64 Offset);
	RETURN_STATUS (*DirectVramWrite)(struct _SUBDEV_MEM * Dev, UINT64 Buffer, UINT64 Length, UINT64 Offset);
	VOID * (*Map)(struct _SUBDEV_MEM * Dev, UINT64 VirtAddr, UINT64 PhysAddr, UINT64 Length, UINT64 Attr, UINT64 Bar);
	VOID (*Ummap)(struct _SUBDEV_MEM * Dev, UINT64 VirtAddr);
	INT64 (*VramAllocate)(struct _SUBDEV_MEM * Dev, UINT64 Size, UINT64 Align);
	INT64 (*VmAllocate)(struct _SUBDEV_MEM * Dev, UINT64 Size, UINT64 Align, UINT64 Bar);
	RETURN_STATUS (*VramFree)(struct _SUBDEV_MEM * Dev, UINT64 Offset);
	RETURN_STATUS (*VmFree)(struct _SUBDEV_MEM * Dev, UINT64 Offset);

}SUBDEV_MEM;

typedef struct _NV_DMA_OBJ
{
	NV_OBJECT Object;
	UINT64 Target;
	UINT64 Access;
	UINT64 Start;
	UINT64 Limit;
	UINT64 Flags;
}NV_DMA_OBJ;

typedef struct _NV_MEM
{
	UINT64 Phys;
	UINT64 Virt;
	UINT64 Length;
	UINT64 Flag;
	NV_GPU_OBJ * Pgd;
	UINT64 Tag;
}NV_MEM;

typedef struct _SUBDEV_THERM
{
	struct _NVIDIA_GPU * Parent;
	UINT32 * MmioBase;
	UINT64 FanCnt;
	UINT64 SensorCnt;

	RETURN_STATUS(*OneInit)(struct _SUBDEV_THERM * Dev);
	RETURN_STATUS(*Init)(struct _SUBDEV_THERM * Dev);
	UINT64(*TemperatureGet)(struct _SUBDEV_THERM * Dev, UINT64 SensorId);
	UINT64(*FanPwmGet)(struct _SUBDEV_THERM * Dev, UINT64 SensorId);
	RETURN_STATUS(*FanPwmSet)(struct _SUBDEV_THERM * Dev, UINT64 Value, UINT64 SensorId);

}SUBDEV_THERM;

struct _NV_CHANNEL;

typedef struct _MEMORY_COPY_ENGINE
{
	struct _NVIDIA_GPU * Parent;

	/* Used for copying host memory to GPU.*/
	UINT64 HostLength;
	UINT32 * HostBuffer;
	UINT64 HostVma;
	SPIN_LOCK SpinLock;

	RETURN_STATUS(*OneInit)(struct _MEMORY_COPY_ENGINE * Engine);
	RETURN_STATUS(*Init)(struct _MEMORY_COPY_ENGINE * Engine);

	struct _NV_CHANNEL * CopyChannel;
	struct
	{
		UINT64 Class;
		UINT64 Methord;
	}Commands;
#define NV_COPY_VRAM_TO_VRAM		0
#define NV_COPY_SYSRAM_TO_VRAM		1
#define NV_COPY_VRAM_TO_SYSRAM		2
#define NV_COPY_SYSRAM_TO_SYSRAM	3

}MEMORY_COPY_ENGINE;

typedef struct _DISPLAY_ENGINE
{
	struct _NVIDIA_GPU * Parent;

	RETURN_STATUS(*OneInit)(struct _DISPLAY_ENGINE * Engine);
	RETURN_STATUS(*Init)(struct _DISPLAY_ENGINE * Engine);
	SPIN_LOCK SpinLock;
	
}DISPLAY_ENGINE;

typedef struct _GRAPHIC_ENGINE
{
	struct _NVIDIA_GPU * Parent;
	RETURN_STATUS(*OneInit)(struct _GRAPHIC_ENGINE * Engine);
	RETURN_STATUS(*Init)(struct _GRAPHIC_ENGINE * Engine);
	SPIN_LOCK SpinLock;
	struct _NV_CHANNEL * KernelChannel;
}GRAPHIC_ENGINE;

typedef struct _NVIDIA_GPU
{
	PCI_DEVICE * PciDev;
	CHAR8 * DeviceName;
	UINT64 Chipset;
	UINT64 VRamSize;
	volatile UINT32 *MmioBase;

	UINT64 ChannelCnt;
	SUBDEV_BIOS * VBios;
	SUBDEV_MEM * VramManager;
	SUBDEV_THERM * Therm;

	MEMORY_COPY_ENGINE *MemoryCopyEngine;
	DISPLAY_ENGINE *DisplayEngine;
	GRAPHIC_ENGINE *GraphicEngine;

}NVIDIA_GPU;

static UINT8 NvMmioRd08(NVIDIA_GPU * Gpu, UINT64 Offset);
static VOID NvMmioWr08(NVIDIA_GPU * Gpu, UINT64 Offset, UINT32 Value);
static UINT32 NvMmioRd32(NVIDIA_GPU * Gpu, UINT64 Offset);
static VOID NvMmioWr32(NVIDIA_GPU * Gpu, UINT64 Offset, UINT32 Value);
static VOID NvMmioMask(NVIDIA_GPU * Gpu, UINT64 Offset, UINT64 Mask, UINT64 Value);

static inline UINT8 NvMmioRd08(NVIDIA_GPU * Gpu, UINT64 Offset)
{
	return *(UINT8 *)((UINT8 *)Gpu->MmioBase + Offset);
}

static inline VOID NvMmioWr08(NVIDIA_GPU * Gpu, UINT64 Offset, UINT32 Value)
{
	*(UINT8 *)((UINT8 *)Gpu->MmioBase + Offset) = Value;
}

static inline UINT32 NvMmioRd32(NVIDIA_GPU * Gpu, UINT64 Offset)
{
	return *(Gpu->MmioBase + (Offset >> 2));
}

static inline VOID NvMmioWr32(NVIDIA_GPU * Gpu, UINT64 Offset, UINT32 Value)
{
	*(Gpu->MmioBase + (Offset >> 2)) = Value;
}

static inline VOID NvMmioMask(NVIDIA_GPU * Gpu, UINT64 Offset, UINT64 Mask, UINT64 Value)
{
	UINT32 Temp = NvMmioRd32(Gpu, Offset);
	NvMmioWr32(Gpu, Offset, (Temp & ~Mask) | Value);
}

UINT32 InstVramRd32(NVIDIA_GPU *Gpu, UINT64 Addr);
VOID InstVramWr32(NVIDIA_GPU *Gpu, UINT64 Addr, UINT32 Value);
RETURN_STATUS InstVramRead(NVIDIA_GPU *Gpu, UINT32* Buffer, UINT64 Length, UINT64 Addr);
RETURN_STATUS InstVramWrite(NVIDIA_GPU *Gpu, UINT32* Buffer, UINT64 Length, UINT64 Addr);

UINT32 GpuObjRd32(NV_GPU_OBJ * Obj, UINT64 Offset);
VOID GpuObjWr32(NV_GPU_OBJ * Obj, UINT64 Offset, UINT32 Value);
RETURN_STATUS NvGpuObjCopy(NV_GPU_OBJ *SrcObj, NV_GPU_OBJ *DstObj);

#endif