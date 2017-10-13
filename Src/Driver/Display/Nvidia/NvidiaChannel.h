#ifndef NVIDIA_CHANNEL_H
#define NVIDIA_CHANNEL_H

//#include "Common/Vram.h"
#include "Common/FifoChannel.h"
#include "NvidiaFence.h"

struct _NVIDIA_GPU;
struct _NV_FENCE;

typedef struct _NV_CHANNEL
{
	NVIDIA_GPU * Gpu;

	UINT32 ChannelId;

	struct
	{
		NV_GPU_OBJ * Buffer;
		NV_MEM Vma;
		NV_GPU_OBJ CtxDma;
	}Push;

	struct
	{
		UINT32 Max;
		UINT32 Free;
		UINT32 Current;
		UINT32 Put;
		UINT32 IbBase;
		UINT32 IbMax;
		UINT32 IbFree;
		UINT32 IbPut;
	}Dma;

	UINT32 UserGetHigh;
	UINT32 UserGetLow;
	UINT32 UserPut;

	UINT32 *User;

	struct _NV_FENCE * ChannelFence;

}NV_CHANNEL;

RETURN_STATUS NvDmaWait(NV_CHANNEL * Chan, UINT32 Slots, UINT32 Size);
UINT64 READ_GET(NV_CHANNEL * Chan, UINT64 *PrevGet, UINT64 *TimeOut);
RETURN_STATUS RING_SPACE(NV_CHANNEL * Chan, UINT32 Size);
VOID OUT_RING(NV_CHANNEL * Chan, UINT32 Data);
VOID FIRE_RING(NV_CHANNEL * Chan);
VOID WIND_RING(NV_CHANNEL *Chan);

static inline void
BEGIN_NVC0(NV_CHANNEL * Chan, UINT32 SubChan, UINT32 Methord, UINT16 Size)
{
	OUT_RING(Chan, 0x20000000 | (Size << 16) | (SubChan << 13) | (Methord >> 2));
}

static inline void
BEGIN_IMC0(NV_CHANNEL *Chan, UINT32 SubChan, UINT32 Methord, UINT16 Data)
{
	OUT_RING(Chan, 0x80000000 | (Data << 16) | (SubChan << 13) | (Methord >> 2));
}

struct _NV_FIFO;
struct _NV_FIFO_CHANNEL;

RETURN_STATUS NvidiaChannelNew(struct _NVIDIA_GPU * Gpu, NV_CHANNEL ** PChan);
RETURN_STATUS NvidiaChannelInit(struct _NVIDIA_GPU * Gpu, NV_CHANNEL *Chan);

RETURN_STATUS NvGpFifoEngineInit(struct _NV_FIFO_CHANNEL * Chan, UINT64 SubEngineIndex);
RETURN_STATUS NvGpFifoNew(struct _NV_FIFO * Fifo, UINT64 IbOffset, UINT64 IbLength, UINT64 Engine, struct _NV_FIFO_CHANNEL ** PChan);
RETURN_STATUS NvFifoNew(UINT64 Index, UINT64 Nr, struct _NV_FIFO ** PFifo);
RETURN_STATUS NvFifoOneInit(struct _NV_FIFO * Fifo);
RETURN_STATUS NvFifoInit(struct _NV_FIFO * Fifo);
RETURN_STATUS NvGpFifoInit(struct _NV_FIFO_CHANNEL * Chan);
RETURN_STATUS NvFifoRunlistCommit(struct _NV_FIFO * Fifo, UINT64 Runl);

/* FIFO methods */
#define NV01_SUBCHAN_OBJECT                                          0x00000000
#define NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH                          0x00000010
#define NV84_SUBCHAN_SEMAPHORE_ADDRESS_LOW                           0x00000014
#define NV84_SUBCHAN_SEMAPHORE_SEQUENCE                              0x00000018
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER                               0x0000001c
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_EQUAL                 0x00000001
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER_WRITE_LONG                    0x00000002
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_GEQUAL                0x00000004
#define NVC0_SUBCHAN_SEMAPHORE_TRIGGER_YIELD                         0x00001000
#define NV84_SUBCHAN_UEVENT                                          0x00000020
#define NV84_SUBCHAN_WRCACHE_FLUSH                                   0x00000024
#define NV10_SUBCHAN_REF_CNT                                         0x00000050
#define NV11_SUBCHAN_DMA_SEMAPHORE                                   0x00000060
#define NV11_SUBCHAN_SEMAPHORE_OFFSET                                0x00000064
#define NV11_SUBCHAN_SEMAPHORE_ACQUIRE                               0x00000068
#define NV11_SUBCHAN_SEMAPHORE_RELEASE                               0x0000006c
#define NV40_SUBCHAN_YIELD                                           0x00000080

/* NV_SW object class */
#define NV_SW_DMA_VBLSEM                                             0x0000018c
#define NV_SW_VBLSEM_OFFSET                                          0x00000400
#define NV_SW_VBLSEM_RELEASE_VALUE                                   0x00000404
#define NV_SW_VBLSEM_RELEASE                                         0x00000408
#define NV_SW_PAGE_FLIP                                              0x00000500


#endif