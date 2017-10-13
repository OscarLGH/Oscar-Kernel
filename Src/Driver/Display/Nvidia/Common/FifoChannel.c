#include "FifoChannel.h"

RETURN_STATUS NvGpFifoNew(NV_FIFO * Fifo, UINT64 IbVmaAddr, UINT64 IbLength, UINT64 Engine, NV_FIFO_CHANNEL ** PChan)
{
	NV_FIFO_CHANNEL * Chan;
	NVIDIA_GPU * Gpu = Fifo->Object.Gpu;

	/* Allocate the channel. */
	*PChan = Chan = KMalloc(sizeof(*Chan));
	memset(Chan, 0, sizeof(*Chan));
	Chan->Object.Gpu = Gpu;
	Chan->Fifo = Fifo;
	Chan->RunL = Engine;

	/* instance memory */
	NvGpuObjNew(Gpu, 0x1000, 0x1000, &Chan->Inst);

	/* allocate channel id */
	Gpu->ChannelCnt++;
	Chan->ChannelId = Gpu->ChannelCnt - 1;

	KDEBUG("Channel ID = %d\n", Chan->ChannelId);
	/* determine address of this channel's user registers */
	Chan->Addr = PHYS2VIRT(PciGetBarBase(Gpu->PciDev, 1)) + Fifo->User.Bar.Virt + 0x200 * Chan->ChannelId;
	Chan->Size = 0x200;
	KDEBUG("Chan->Addr = %x\n", Chan->Addr);

	/* Page directory. */
	NvGpuObjNew(Gpu, 0x10000, 0x1000, &Chan->Pgd);

	GpuObjWr32(Chan->Inst, 0x0200, Chan->Pgd->Addr);
	GpuObjWr32(Chan->Inst, 0x0204, Chan->Pgd->Addr >> 32);
	GpuObjWr32(Chan->Inst, 0x0208, 0xFFFFFFFF);
	GpuObjWr32(Chan->Inst, 0x020c, 0x000000FF);

	NvGpuObjCopy(Gpu->VramManager->Bar1Vm->Pgd, Chan->Pgd);

	/* Clear channel control registers. */
	UINT64 UserMem = Chan->ChannelId * 0x200;
	IbLength = OrderBase2(IbLength / 8);
	int i;
	for (i = 0; i < 0x200; i += 4)
		GpuObjWr32(Fifo->User.Mem, UserMem + i, 0x00000000);

	UserMem += Fifo->User.Mem->Addr;

	/* RAMFC */
	GpuObjWr32(Chan->Inst, 0x08, UserMem & 0xffffffff);
	GpuObjWr32(Chan->Inst, 0x0c, UserMem >> 32);
	GpuObjWr32(Chan->Inst, 0x10, 0x0000face);
	GpuObjWr32(Chan->Inst, 0x30, 0xfffff902);
	GpuObjWr32(Chan->Inst, 0x48, IbVmaAddr & 0xffffffff);
	GpuObjWr32(Chan->Inst, 0x4c, (IbVmaAddr >> 32) |
		(IbLength << 16));
	GpuObjWr32(Chan->Inst, 0x84, 0x20400000);
	GpuObjWr32(Chan->Inst, 0x94, 0x30000001);
	GpuObjWr32(Chan->Inst, 0x9c, 0x00000100);
	GpuObjWr32(Chan->Inst, 0xac, 0x0000001f);
	GpuObjWr32(Chan->Inst, 0xe8, Chan->ChannelId);
	GpuObjWr32(Chan->Inst, 0xb8, 0xf8000000);
	GpuObjWr32(Chan->Inst, 0xf8, 0x10003080); /* 0x002310 */
	GpuObjWr32(Chan->Inst, 0xfc, 0x10000010); /* 0x002350 */

	return RETURN_SUCCESS;
}


RETURN_STATUS NvFifoRunlistCommit(NV_FIFO * Fifo, UINT64 Runl)
{
	NVIDIA_GPU * Gpu = Fifo->Object.Gpu;
	NV_FIFO_CHANNEL * Chan = ContainerOf(Fifo, NV_FIFO_CHANNEL, Fifo);
	UINT32 Target = 0; //vram
	UINT32 Cnt = 0;
	NV_GPU_OBJ * Mem;
	//Runl = 0;
	//KDEBUG("Fifo->RunList[Runl].Next = %d\n", Fifo->RunList[Runl].Next);
	Mem = Fifo->RunList[Runl].Mem[Fifo->RunList[Runl].Next];
	Fifo->RunList[Runl].Next = !Fifo->RunList[Runl].Next;

	/* For every channel. */
	UINT64 Index = 0;

	//GpuObjWr32(Mem, Index * 8 + 0x0, Chan->ChannelId);	//ChannelID
	GpuObjWr32(Mem, Index * 8 + 0x0, 0);
	GpuObjWr32(Mem, Index * 8 + 0x4, 0x00000000);
	Cnt++;

	NvMmioWr32(Gpu, 0x002270, (Mem->Addr >> 12) | (Target << 28));
	NvMmioWr32(Gpu, 0x002274, (Runl << 20) | Cnt);

	//KDEBUG("FIFO:0x2284:%08x\n", NvMmioRd32(Gpu, 0x2284 + Runl * 8));
	while (NvMmioRd32(Gpu, 0x002284 + Runl * 8) & 0x00100000);

	return RETURN_SUCCESS;
}

RETURN_STATUS NvFifoOneInit(NV_FIFO * Fifo)
{
	RETURN_STATUS Status;
	NVIDIA_GPU * Gpu = Fifo->Object.Gpu;

	/* Determine number of PBDMAs by checking valid enable bits. */
	NvMmioWr32(Gpu, 0x000204, 0xffffffff);
	Fifo->PbDmaCnt = Weight32(NvMmioRd32(Gpu, 0x000204));

	/* Read PBDMA->runlist(s) mapping from HW. */
	int i;
	for (i = 0; i < Fifo->PbDmaCnt; i++)
	{
		//KDEBUG("%x:%d:%x\n", 0x2390 + i * 0x4, i, NvMmioRd32(Gpu, 0x002390 + i * 0x04));
	}
	Fifo->RunListCnt = 0x20;

	for (i = 0; i < Fifo->RunListCnt; i++)
	{
		Fifo->RunList[i].Next = 0;
		Status = NvGpuObjNew(Gpu, 0x8000, 0x1000, &Fifo->RunList[i].Mem[0]);

		if (Status)
			return Status;

		Status = NvGpuObjNew(Gpu, 0x8000, 0x1000, &Fifo->RunList[i].Mem[1]);

		if (Status)
			return Status;
	}

	Status = NvGpuObjNew(Gpu, Fifo->Nr * 0x200, 0x1000, &Fifo->User.Mem);

	if (Status)
		return Status;

	/* Map Chan->User to user addr.*/
	Fifo->User.Bar.Virt = Gpu->VramManager->VmAllocate(Gpu->VramManager, Fifo->Nr * 0x200, 0x1000, 1);

	Gpu->VramManager->Map(Gpu->VramManager, Fifo->User.Bar.Virt, Fifo->User.Mem->Addr, Fifo->Nr * 0x200, 0, 1);

	return RETURN_SUCCESS;
}

RETURN_STATUS NvFifoInit(NV_FIFO * Fifo)
{
	NVIDIA_GPU * Gpu = Fifo->Object.Gpu;
	int i;

	/* Enable PBDMAs. */
	NvMmioWr32(Gpu, 0x000204, (1 << Fifo->PbDmaCnt) - 1);

	/* PBDMA[n] */
	for (i = 0; i < Fifo->PbDmaCnt; i++) {
		NvMmioMask(Gpu, 0x04013c + (i * 0x2000), 0x10000100, 0x00000000);
		NvMmioWr32(Gpu, 0x040108 + (i * 0x2000), 0xffffffff); /* INTR */
		NvMmioWr32(Gpu, 0x04010c + (i * 0x2000), 0xfffffeff); /* INTREN */
	}

	/* PBDMA[n].HCE */
	for (i = 0; i < Fifo->PbDmaCnt; i++) {
		NvMmioWr32(Gpu, 0x040148 + (i * 0x2000), 0xffffffff); /* INTR */
		NvMmioWr32(Gpu, 0x04014c + (i * 0x2000), 0xffffffff); /* INTREN */
	}

	KDEBUG("Fifo->User.Bar.Virt = %08x\n", Fifo->User.Bar.Virt);
	NvMmioWr32(Gpu, 0x002254, 0x10000000 | (Fifo->User.Bar.Virt >> 12));

	NvMmioWr32(Gpu, 0x002100, 0xffffffff);
	NvMmioWr32(Gpu, 0x002140, 0x7fffffff);

	return RETURN_SUCCESS;
}

RETURN_STATUS NvGpFifoEngineConstructor()
{

}

static UINT32 NvGpFifoEngineAddr(UINT64 SubDevIndex)
{
	switch (SubDevIndex) {
	case NVKM_ENGINE_SW:
	case NVKM_ENGINE_CE0:
	case NVKM_ENGINE_CE1:
	case NVKM_ENGINE_CE2: return 0x0000;
	case NVKM_ENGINE_GR: return 0x0210;
	case NVKM_ENGINE_SEC: return 0x0220;
	case NVKM_ENGINE_MSPDEC: return 0x0250;
	case NVKM_ENGINE_MSPPP: return 0x0260;
	case NVKM_ENGINE_MSVLD: return 0x0270;
	case NVKM_ENGINE_VIC: return 0x0280;
	case NVKM_ENGINE_MSENC: return 0x0290;
	case NVKM_ENGINE_NVDEC: return 0x02100270;
	case NVKM_ENGINE_NVENC0: return 0x02100290;
	case NVKM_ENGINE_NVENC1: return 0x0210;
	default:
		return 0;
	}
}

RETURN_STATUS NvGpFifoEngineInit(NV_FIFO_CHANNEL * Chan, UINT64 SubEngineIndex)
{

	UINT64 Offset = NvGpFifoEngineAddr(SubEngineIndex);

	if (Offset)
	{
		UINT64 Addr = Chan->Engn[SubEngineIndex].Vma->VmBaseAddr;
		Addr = 0;
		GpuObjWr32(Chan->Inst, Offset & 0xFFFF + 0x00, Addr | 0x00000004);
		GpuObjWr32(Chan->Inst, Offset & 0xFFFF + 0x04, Addr >> 32);
		if (Offset >>= 16)
		{
			GpuObjWr32(Chan->Inst, Offset + 0x00, Addr | 0x00000004);
			GpuObjWr32(Chan->Inst, Offset + 0x04, Addr >> 32);
		}
	}
	return RETURN_SUCCESS;
}


RETURN_STATUS NvGpFifoKick(NV_FIFO_CHANNEL * Chan)
{
	NVIDIA_GPU * Gpu = Chan->Object.Gpu;
	NvMmioWr32(Gpu, 0x002634, Chan->ChannelId);

	return RETURN_SUCCESS;
}

RETURN_STATUS NvGpFifoInit(NV_FIFO_CHANNEL * Chan)
{
	NVIDIA_GPU * Gpu = Chan->Object.Gpu;
	UINT32 Addr = Chan->Inst->Addr >> 12;
	UINT32 Coff = Chan->ChannelId * 8;

	NvMmioMask(Gpu, 0x800004 + Coff, 0x000f0000, Chan->RunL << 16);
	NvMmioWr32(Gpu, 0x800000 + Coff, 0x80000000 | Addr);

	/* Enable trigger. */
	NvMmioMask(Gpu, 0x800004 + Coff, 0x00000400, 0x00000400);
	NvFifoRunlistCommit(Chan->Fifo, Chan->RunL);
	NvMmioMask(Gpu, 0x800004 + Coff, 0x00000400, 0x00000400);

	return RETURN_SUCCESS;
}

static VOID FifoIntrFault(NVIDIA_GPU * Gpu, UINT32 Unit)
{
	UINT32 Inst = NvMmioRd32(Gpu, 0x2800 + (Unit * 0x10));
	UINT32 Valo = NvMmioRd32(Gpu, 0x2804 + (Unit * 0x10));
	UINT32 Vahi = NvMmioRd32(Gpu, 0x2808 + (Unit * 0x10));
	UINT32 Stat = NvMmioRd32(Gpu, 0x280c + (Unit * 0x10));
	UINT32 Gpc = (Stat & 0x1f000000) >> 24;
	UINT32 Client = (Stat & 0x00001f00) >> 8;
	UINT32 Write = (Stat & 0x00000080) >> 8;
	UINT32 Hub = (Stat & 0x00000040);
	UINT32 Reason = (Stat & 0x0000000f);

	printk("%s fault at 0x%016x,engine %02x,client %02x,reason %02x\n",
		Write ? "Write" : "Read",
		(UINT64)Vahi << 32 | Valo,
		Unit,
		Client,
		Reason
	);
}

static VOID FifoIntrRunlist(NVIDIA_GPU * Gpu)
{
	UINT32 Mask = NvMmioRd32(Gpu, 0x002a00);
	while (Mask)
	{
		int Runl = FirstBitSet(Mask);
		NvMmioWr32(Gpu, 0x002a00, 1 << Runl);
		Mask &= ~(1 << Runl);
	}
}

static VOID PbDmaIntr0(NVIDIA_GPU * Gpu, UINT32 Unit)
{
	UINT32 Mask = NvMmioRd32(Gpu, 0x04010c + (Unit * 0x2000));
	UINT32 Stat = NvMmioRd32(Gpu, 0x040108 + (Unit * 0x2000)) & Mask;
	UINT32 Addr = NvMmioRd32(Gpu, 0x0400c0 + (Unit * 0x2000));
	UINT32 Data = NvMmioRd32(Gpu, 0x0400c4 + (Unit * 0x2000));
	UINT32 Chid = NvMmioRd32(Gpu, 0x040120 + (Unit * 0x2000));
	UINT32 Subc = (Addr & 0x00070000) >> 16;
	UINT32 Mthd = (Addr & 0x00003ffc);

	NvMmioWr32(Gpu, 0x0400c0 + (Unit * 0x2000), 0x80600008);

	printk("PBDMA%d:%08x ch %d subc %d mthd %04x data %08x\n", Unit, Stat, Chid, Mthd, Data);

	NvMmioWr32(Gpu, 0x040108 + (Unit * 0x2000), Stat);

}

static VOID PbDmaIntr1(NVIDIA_GPU * Gpu, UINT32 Unit)
{
	UINT32 Mask = NvMmioRd32(Gpu, 0x04014c + (Unit * 0x2000));
	UINT32 Stat = NvMmioRd32(Gpu, 0x040148 + (Unit * 0x2000)) & Mask;
	UINT32 Addr = NvMmioRd32(Gpu, 0x040150 + (Unit * 0x2000));
	UINT32 Data = NvMmioRd32(Gpu, 0x040154 + (Unit * 0x2000));
	UINT32 Chid = NvMmioRd32(Gpu, 0x040120 + (Unit * 0x2000));
	
	printk("PBDMA%d:%08x ch %d %08x %08x\n", Unit, Stat, Chid, Addr, Data);

	NvMmioWr32(Gpu, 0x040148 + (Unit * 0x2000), Stat);
}


RETURN_STATUS NvidiaGpuFifoIrq(NVIDIA_GPU * Gpu)
{
	UINT32 IntrMask = NvMmioRd32(Gpu, 0x2140);
	UINT32 IntrStat = NvMmioRd32(Gpu, 0x2100);
	IntrStat &= IntrMask;

	printk("IntrStat:%x\n", IntrStat);
	if (IntrStat & 0x00000001)
	{

		NvMmioWr32(Gpu, 0x2100, 0x00000001);
		IntrStat &= ~0x00000001;
	}

	if (IntrStat & 0x00000010)
	{
		printk("NVIDIA GPU:FIFO:PIO_ERROR\n");
		NvMmioWr32(Gpu, 0x2100, 0x00000010);
		IntrStat &= ~0x00000010;
	}

	if (IntrStat & 0x00000100)
	{

		NvMmioWr32(Gpu, 0x2100, 0x00000100);
		IntrStat &= ~0x00000100;
	}

	if (IntrStat & 0x00010000)
	{

		NvMmioWr32(Gpu, 0x2100, 0x00010000);
		IntrStat &= ~0x00010000;
	}

	if (IntrStat & 0x00800000)
	{
		printk("NVIDIA GPU:FIFO:FB_FLUSH_TIMEOUT\n");
		NvMmioWr32(Gpu, 0x2100, 0x00800000);
		IntrStat &= ~0x00800000;
	}

	if (IntrStat & 0x01000000)
	{
		printk("NVIDIA GPU:FIFO:LB_ERROR\n");
		NvMmioWr32(Gpu, 0x2100, 0x01000000);
		IntrStat &= ~0x01000000;
	}

	if (IntrStat & 0x08000000)
	{
		printk("NVIDIA GPU:FIFO:DROPPED FAULT\n");
		NvMmioWr32(Gpu, 0x2100, 0x08000000);
		IntrStat &= ~0x08000000;
	}

	if (IntrStat & 0x10000000)
	{
		printk("NVIDIA GPU:FIFO fault.\n");
		UINT32 Mask = NvMmioRd32(Gpu, 0x259c);
		while (Mask)
		{
			UINT32 Unit = FirstBitSet(Mask);
			FifoIntrFault(Gpu, Unit);
			NvMmioWr32(Gpu, 0x259c, (1 << Unit));
			Mask &= ~(1 << Unit);
		}
		NvMmioWr32(Gpu, 0x2100, 0x10000000);
		IntrStat &= ~0x10000000;
	}

	if (IntrStat & 0x20000000)
	{
		UINT32 Mask = NvMmioRd32(Gpu, 0x0025a0);
		while (Mask)
		{
			UINT32 Unit = FirstBitSet(Mask);
			PbDmaIntr0(Gpu, Unit);
			PbDmaIntr1(Gpu, Unit);
			NvMmioWr32(Gpu, 0x0025a0, (1 << Unit));
			Mask &= ~(1 << Unit);
		}
		NvMmioWr32(Gpu, 0x2100, 0x20000000);
		IntrStat &= ~0x20000000;
	}

	if (IntrStat & 0x40000000)
	{
		printk("NVIDIA GPU:FIFO:RunList.\n");
		FifoIntrRunlist(Gpu);
		NvMmioWr32(Gpu, 0x2100, 0x40000000);
		IntrStat &= ~0x40000000;
	}

	if (IntrStat & 0x80000000)
	{
		printk("NVIDIA GPU:FIFO:Engine\n");
		NvMmioWr32(Gpu, 0x2100, 0x80000000);
		IntrStat &= ~0x80000000;
	}

	if (IntrStat)
	{
		NvMmioMask(Gpu, 0x2140, IntrStat, 0x00000000);
		NvMmioWr32(Gpu, 0x2100, IntrStat);
	}
}