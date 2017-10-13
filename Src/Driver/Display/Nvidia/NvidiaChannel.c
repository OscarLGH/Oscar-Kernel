#include "NvidiaChannel.h"

RETURN_STATUS NvDmaWait(NV_CHANNEL * Chan, UINT32 Slots, UINT32 Size)
{
	UINT64 PrevGet = 0;
	UINT64 Cnt = 0;
	INT64 Get = 0;

	while (Chan->Dma.IbFree < Size + 1)
	{
		Get = Chan->User[0x88 / 4];
		if (Get != PrevGet)
		{
			PrevGet = Get;
			Cnt = 0;
		}

		if ((++Cnt & 0xFF) == 0)
		{
			MicroSecondDelay(1);
			if (Cnt > 100000)
				return RETURN_TIMEOUT;
		}

		Chan->Dma.IbFree = Get - Chan->Dma.IbPut;
		if (Chan->Dma.IbFree <= 0)
		{
			Chan->Dma.IbFree += Chan->Dma.IbMax;
		}
	}

	PrevGet = 0;
	Cnt = 0;
	Get = 0;

	while (Chan->Dma.Free < Size)
	{
		Get = READ_GET(Chan, &PrevGet, &Cnt);
		if (Get == RETURN_TIMEOUT)
			return Get;

		if (Get <= Chan->Dma.Current)
		{
			Chan->Dma.Free = Chan->Dma.Max - Chan->Dma.Current;
			if (Chan->Dma.Free >= Size)
				break;

			FIRE_RING(Chan);

			do {
				Get = READ_GET(Chan, &PrevGet, &Cnt);
				if (Get < 0)
				{
					if (Get == RETURN_INVALID_PARAMETER)
						continue;
					return Get;
				}
			} while (Get == 0);
			Chan->Dma.Current = 0;
			Chan->Dma.Put = 0;
		}
		
		Chan->Dma.Free = Get - Chan->Dma.Current - 1;
	}
	return RETURN_SUCCESS;
}

RETURN_STATUS RING_SPACE(NV_CHANNEL * Chan, UINT32 Size)
{
	RETURN_STATUS Status = 0;

	Status = NvDmaWait(Chan, 1, Size);
	if (Status)
		return Status;

	Chan->Dma.Free -= Size;

	return RETURN_SUCCESS;
}

UINT64 READ_GET(NV_CHANNEL * Chan, UINT64 *PrevGet, UINT64 *TimeOut)
{
	UINT64 Val;
	Val = Chan->User[Chan->UserGetLow] | ((UINT64)Chan->User[Chan->UserGetHigh] << 32);

	if (Val != *PrevGet)
	{
		*PrevGet = Val;
		*TimeOut = 0;
	}

	if ((++*TimeOut & 0xFF) == 0)
	{
		MicroSecondDelay(1);
		if (*TimeOut > 100000)
			return RETURN_TIMEOUT;
	}

	if (Val < Chan->Push.Vma.Virt ||
		Val > Chan->Push.Vma.Virt + (Chan->Dma.Max << 2))
		return RETURN_INVALID_PARAMETER;

	return (Val - Chan->Push.Vma.Virt) >> 2;
}

VOID OUT_RING(NV_CHANNEL * Chan, UINT32 Data)
{
	UINT32 * Ptr = Chan->Push.Buffer->Map;
	Ptr[Chan->Dma.Current++] = Data;
}

VOID FIRE_RING(NV_CHANNEL * Chan)
{
	UINT32 * PushBufferPtr = Chan->Push.Buffer->Map;

	if (Chan->Dma.Current == Chan->Dma.Put)
		return;

	NVIDIA_GPU * Gpu = Chan->Gpu;
	
	UINT64 IbEntry = Chan->Dma.IbPut * 2 + Chan->Dma.IbBase;
	UINT64 Delta = Chan->Dma.Put << 2;
	UINT64 Length = (Chan->Dma.Current - Chan->Dma.Put) << 2;
	UINT64 Offset;
	//Find push buffer vm address
	Offset = Chan->Push.Vma.Virt + Delta;
	//KDEBUG("FIRE_RING: IB Entry:[%08x] = %08x %08x\n", IbEntry, Offset, (Offset >> 32) | (Length << 8));
	//KDEBUG("FIRE_RING: Dma IB Put:%08x\n", Chan->Dma.IbPut);
	//GpuObjWr32(Chan->Push.Buffer, Ip++, Offset);
	//GpuObjWr32(Chan->Push.Buffer, Ip++, (Offset >> 32) | (Length << 8));
	PushBufferPtr[IbEntry++] = Offset;
	PushBufferPtr[IbEntry++] = (Offset >> 32) | (Length << 8);

	Chan->Dma.IbPut = (Chan->Dma.IbPut + 1) & Chan->Dma.IbMax;

	//mb()

	/* Flush writes. */
	Offset = PushBufferPtr[0];

	Chan->User[0x8c / 4] = Chan->Dma.IbPut;
	//KDEBUG("Chan->User:0x8c:%x\n", Chan->User[0x8c / 4]);
	//KDEBUG("IB Get:%08x\n", Chan->User[0x88 / 4]);
	Chan->Dma.IbFree--;
	Chan->Dma.Put = Chan->Dma.Current;
}

VOID WIND_RING(NV_CHANNEL *Chan)
{
	Chan->Dma.Current = Chan->Dma.Put;
}

#define PUSHBUFFER_SIZE 0x12000
#define IB_OFFSET 0x10000
#define IB_SIZE PUSHBUFFER_SIZE-IB_OFFSET

NV_FIFO * NvFifo;
RETURN_STATUS NvidiaChannelNew(NVIDIA_GPU * Gpu, NV_CHANNEL ** PChan)
{
	NV_CHANNEL *Chan;
	
	RETURN_STATUS Status = RETURN_SUCCESS;
	Chan = *PChan = KMalloc(sizeof(*Chan));
	//FifoChan = KMalloc(sizeof(*FifoChan));
	memset(Chan, 0, sizeof(*Chan));
	//memset(FifoChan, 0, sizeof(*FifoChan));
	Chan->Gpu = Gpu;

	/* Allocate dma push buffer */
	//Status = NvGpuObjNew(Gpu, 0x12000, 0x1000, &Chan->Push.Buffer);
	//if (Status)
	//	return Status;

	Chan->Push.Buffer = KMalloc(sizeof(NV_GPU_OBJ));
	Chan->Push.Buffer->Map = KMalloc(PUSHBUFFER_SIZE);
	memset(Chan->Push.Buffer->Map, 0, PUSHBUFFER_SIZE);
	Chan->Push.Buffer->Addr = VIRT2PHYS(Chan->Push.Buffer->Map);

	Chan->Push.Vma.Virt = Gpu->VramManager->VmAllocate(Gpu->VramManager, PUSHBUFFER_SIZE, 0x1000, 1);
	Gpu->VramManager->Map(Gpu->VramManager, Chan->Push.Vma.Virt, Chan->Push.Buffer->Addr, PUSHBUFFER_SIZE, NV_MEM_TARGET_HOST, 1);
	KDEBUG("Push buffer Phys:%016x Virt:%016x\n", Chan->Push.Buffer->Addr, Chan->Push.Vma.Virt);
	/* Create channel object */
	
	NvFifo = KMalloc(sizeof(*NvFifo));
	memset(NvFifo, 0, sizeof(*NvFifo));
	NvFifo->Object.Gpu = Gpu;
	NvFifo->Nr = 64;

	NvFifoOneInit(NvFifo);
	NvFifoInit(NvFifo);

	NV_FIFO_CHANNEL * FifoChan;

	NvGpFifoNew(NvFifo, 0x10000 + Chan->Push.Vma.Virt, IB_SIZE, 0x0, &FifoChan);
	NvGpFifoEngineInit(FifoChan, 0);
	Chan->User = (UINT32 *)FifoChan->Addr;
	KDEBUG("Chan->User = %x\n", Chan->User);
	/* Init channel */
	FifoChan->Object.Gpu = Gpu;
	NvGpFifoInit(FifoChan);
	
	NvFenceCreate(Gpu, &Chan->ChannelFence);
}

RETURN_STATUS NvidiaChannelInit(NVIDIA_GPU * Gpu, NV_CHANNEL *Chan)
{
	RETURN_STATUS Status = RETURN_SUCCESS;

	/* allocate dma objects to cover all allowed vram, and gart */

	/* initialise dma tracking parameters */
	Chan->UserPut = 0x40;
	Chan->UserGetLow = 0x40;
	Chan->UserGetHigh = 0x60;
	Chan->Dma.IbBase = 0x10000 / 4;
	Chan->Dma.IbMax = (0x2000 / 8) - 1;
	Chan->Dma.IbPut = 0;
	Chan->Dma.IbFree = Chan->Dma.IbMax - Chan->Dma.IbPut;
	Chan->Dma.Max = Chan->Dma.IbBase;

	Chan->Dma.Put = 0;
	Chan->Dma.Current = Chan->Dma.Put;
	Chan->Dma.Free = Chan->Dma.Max - Chan->Dma.Current;

	RING_SPACE(Chan, 128 / 4);
	int i;
	for (i = 0; i < 128 / 4; i++)
		OUT_RING(Chan, 0);

	return Status;
}