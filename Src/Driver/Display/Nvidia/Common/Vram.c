#include "Vram.h"

UINT32 InstVramRd32(NVIDIA_GPU *Gpu, UINT64 Addr)
{
	UINT64 Base = Addr & 0xFFFFFF00000ULL;
	UINT64 Offset = Addr & 0x000000FFFFFULL;
	NvMmioWr32(Gpu, 0x1700, Base >> 16);
	return NvMmioRd32(Gpu, 0x700000 + Offset);
}

VOID InstVramWr32(NVIDIA_GPU *Gpu, UINT64 Addr, UINT32 Value)
{
	UINT64 Base = Addr & 0xFFFFFF00000ULL;
	UINT64 Offset = Addr & 0x000000FFFFFULL;
	NvMmioWr32(Gpu, 0x1700, Base >> 16);
	NvMmioWr32(Gpu, 0x700000 + Offset, Value);
}

RETURN_STATUS VramRead(NVIDIA_GPU *Gpu, UINT32* Buffer, UINT64 Length, UINT64 Addr)
{
	UINT64 Base = 0xFFFFF;
	UINT64 Index = 0;

	for (Index = 0; Index < (Length >> 2); Index ++, Addr += 4)
	{
		
		if (Base != Addr & 0xFFFFFF00000ULL)
		{
			Base = Addr & 0xFFFFFF00000ULL;
			NvMmioWr32(Gpu, 0x1700, Base >> 16);
		}
		Buffer[Index] = NvMmioRd32(Gpu, 0x700000 + (Addr & 0xFFFFF));
	}
}

RETURN_STATUS VramWrite(NVIDIA_GPU *Gpu, UINT32* Buffer, UINT64 Length, UINT64 Addr)
{
	UINT64 Base = 0xFFFFF;
	UINT64 Index = 0;

	for (Index = 0; Index < (Length >> 2); Index++, Addr += 4)
	{

		if (Base != Addr & 0xFFFFFF00000ULL)
		{
			Base = Addr & 0xFFFFFF00000ULL;
			NvMmioWr32(Gpu, 0x1700, Base >> 16);
		}
		NvMmioWr32(Gpu, Buffer[Index], 0x700000 + (Addr & 0xFFFFF));
	}
}

RETURN_STATUS VramZero(NVIDIA_GPU *Gpu, UINT64 Addr, UINT64 Length)
{
	UINT64 Base = 0xFFFFF;
	UINT64 Index = 0;
	
	for (Index = 0; Index < (Length >> 2); Index++, Addr += 4)
	{

		if (Base != (Addr & 0xFFFFFF00000ULL))
		{
			Base = (Addr & 0xFFFFFF00000ULL);
			NvMmioWr32(Gpu, 0x1700, Base >> 16);
		}
		NvMmioWr32(Gpu, 0x700000 + (Addr & 0xFFFFF), 0);
	}
}

INT64 VramAllocate(SUBDEV_MEM * Vram, UINT64 Size, UINT64 Align)
{
	UINT64 Start, End, Index0, Index1;

	MEMORY_REGION * RegionPtr = Vram->VramRegion;
	UINT64 ReturnIndex;
	while (RegionPtr)
	{
		Start = RegionPtr->AllocatingIndex * RegionPtr->PageSize;
		End = RegionPtr->Length;

		if (Size < (End - Start))
		{
			ReturnIndex = RegionPtr->AllocatingIndex;
			RegionPtr->AllocatingIndex += ( Size > RegionPtr->PageSize ? Size / RegionPtr->PageSize : 1 );
			//for (Index1 = Index0; Index1 < Size / RegionPtr->PageSize; Index1++)
			//	RegionPtr->AllocatingTable[Index1] = 1;
			return ReturnIndex * RegionPtr->PageSize + RegionPtr->Base;
		}
		
		RegionPtr = RegionPtr->Next;
	}
	return RETURN_OUT_OF_RESOURCES;
}

INT64 VramZallocate(SUBDEV_MEM * Vram, UINT64 Size, UINT64 Align)
{
	NVIDIA_GPU * Gpu = Vram->Parent;
	INT64 Offset = VramAllocate(Vram, Size, Align);
	UINT64 Index;

	if (Offset > 0)
		VramZero(Gpu, Offset, Size);

	return Offset;
}

RETURN_STATUS VramFree(SUBDEV_MEM * Vram, UINT64 Offset)
{

}


RETURN_STATUS NvGpuObjNew(NVIDIA_GPU * Gpu, UINT64 Size, UINT64 Align, NV_GPU_OBJ **Obj)
{
	*Obj = KMalloc(sizeof(NV_GPU_OBJ));
	memset(*Obj, 0, sizeof(NV_GPU_OBJ));
	(*Obj)->Object.Gpu = Gpu;
	(*Obj)->Addr = Gpu->VramManager->VramAllocate(Gpu->VramManager, Size, Align);
	
	//KDEBUG("New GPU Obj:Addr:%016x,Size:%x\n", (*Obj)->Addr, Size);
	(*Obj)->Map = NULL;
	//KDEBUG("(*Obj)->Map = %x\n", (*Obj)->Map);
	if ((*Obj)->Addr == 0)
		return RETURN_ABORTED;

	(*Obj)->Size = RoundUp(Size, 0x1000);

	return RETURN_SUCCESS;
}

RETURN_STATUS NvGpuObjFree(NVIDIA_GPU * Gpu, NV_GPU_OBJ **Obj)
{
	return RETURN_SUCCESS;
}

UINT32 GpuObjRd32(NV_GPU_OBJ * Obj, UINT64 Offset)
{
	//KDEBUG("Obj->Map = %x\n", Obj->Map);
	if (Obj->Map != NULL)
		return *((UINT32 *)Obj->Map);
	else
		return InstVramRd32(Obj->Object.Gpu, Obj->Addr + Offset);
}
VOID GpuObjWr32(NV_GPU_OBJ * Obj, UINT64 Offset, UINT32 Value)
{
	//KDEBUG("Obj->Map = %x\n", Obj->Map);
	if (Obj->Map != NULL)
		*((UINT32 *)Obj->Map) = Value;
	else
		InstVramWr32(Obj->Object.Gpu, Obj->Addr + Offset, Value);
}

RETURN_STATUS NvGpuObjCopy(NV_GPU_OBJ *SrcObj, NV_GPU_OBJ *DstObj)
{
	UINT64 Index = 0;
	UINT32 Value = 0;
	for (Index = 0; Index < SrcObj->Size; Index += 4)
	{
		Value = GpuObjRd32(SrcObj, Index);
		GpuObjWr32(DstObj, Index, Value);
	}
	return RETURN_SUCCESS;
}


/*******************************************
* IO MMU, VM routines .                  
*******************************************/

static RETURN_STATUS VmMapPgt(NV_GPU_OBJ * Pgd, UINT64 Index, NV_GPU_OBJ *Spgt, NV_GPU_OBJ *Lpgt)
{
	UINT32 Pde[2] = { 0 };

	if (Pgd == NULL)
		return RETURN_ABORTED;

	if (Spgt)
		Pde[1] = NV_SPT_PRESENT | ((Spgt->Addr) >> 8);
	if (Lpgt)
		Pde[0] = NV_LPT_PRESENT | ((Lpgt->Addr) >> 8);

	GpuObjWr32(Pgd, (Index * 8) + 0x0, Pde[0]);
	GpuObjWr32(Pgd, (Index * 8) + 0x4, Pde[1]);

	return RETURN_SUCCESS;
}

static RETURN_STATUS VmMapPage(NV_GPU_OBJ * Pgt, UINT64 Index, UINT64 Pt0, UINT64 Pt1)
{
	if (Pgt == NULL)
		return RETURN_ABORTED;
	//KDEBUG("PT[%03x] = %08x %08x\n", Index, Pt0, Pt1);
	GpuObjWr32(Pgt, (Index * 8) + 0x0, Pt0);
	GpuObjWr32(Pgt, (Index * 8) + 0x4, Pt1);

	return RETURN_SUCCESS;
}

RETURN_STATUS VmNewSpace(NV_GPU_OBJ * Pgd, UINT64 Length, UINT64 Flag)
{
	NVIDIA_GPU * Gpu = Pgd->Object.Gpu;
	UINT64 PgdIndex = 0;
	UINT64 PageSize = 0;
	UINT64 PageCnt = 0;
	UINT64 PtSize = 0;
	UINT64 RemainSize = 0;
	UINT64 LargePt = 0;
	NV_GPU_OBJ * PT = NULL;
	NV_GPU_OBJ * Spgt = NULL;
	NV_GPU_OBJ * Lpgt = NULL;

	if (Flag & NV_MEM_LARGE_PAGE)
	{
		LargePt = 1;
		PageSize = 0x20000;
	}
	else
	{
		LargePt = 0;
		PageSize = 0x1000;
	}

	KDEBUG("%s:Flag:%x,Length:0x%x,Pages:%d\n",
		__FUNCTION__,
		Flag,
		Length,
		Length / PageSize);

	/* Calculate the PT size. */
	PageCnt = Length / PageSize;
	PtSize = PageCnt * 8;

	for (RemainSize = PtSize; RemainSize > NVC0_VM_BLOCK_SIZE; RemainSize -= NVC0_VM_BLOCK_SIZE)
	{
		NvGpuObjNew(Gpu, NVC0_VM_BLOCK_SIZE, 0x1000, &PT);
		
		if (LargePt)
			Lpgt = PT;
		else
			Spgt = PT;

		VmMapPgt(Pgd, PgdIndex, Spgt, Lpgt);
		PgdIndex ++;
	}

	if(RemainSize > 0)
	{
		NvGpuObjNew(Gpu, RemainSize, 0x1000, &PT);
		
		if (LargePt)
			Lpgt = PT;
		else
			Spgt = PT;

		VmMapPgt(Pgd, PgdIndex, Spgt, Lpgt);
	}
	
	return RETURN_SUCCESS;
}

RETURN_STATUS VmMapSpace(NV_GPU_OBJ * Pgd, UINT64 VirtAddr, UINT64 PhysAddr, UINT64 Length, UINT64 Flag)
{
	UINT64 PtSize = 0;
	UINT64 PageAttr = 0;
	UINT64 PageSize = 0;
	UINT64 Addr = 0;
	UINT64 PdeIndex = 0; 
	UINT64 PteIndex = 0;
	UINT64 PtAddr = 0;
	UINT32 Pt0 = 0;
	UINT32 Pt1 = 0;
	UINT64 PtPosition = 0;
	NVIDIA_GPU * Gpu = Pgd->Object.Gpu;
	//之前这里没有赋值导致一个极其难追的BUG，成员Gpu没有赋值导致MMIO写飞，修改前的代码此处在一个执行不到的分支上
	NV_GPU_OBJ PTTemp = *Pgd;			
	NV_GPU_OBJ * PT = &PTTemp;

	if (Flag & NV_MEM_TARGET_HOST_NOSNOOP)
		PageAttr |= NV_PTE_TARGET_SYSRAM_NOSNOOP;
	else if (Flag & NV_MEM_TARGET_HOST)
		PageAttr |= NV_PTE_TARGET_SYSRAM_SNOOP;
	else
		PageAttr |= 0;		//Mem

	if (Flag & NV_MEM_ACCESS_RO)
		PageAttr |= NV_PTE_READ_ONLY;

	if (Flag & NV_MEM_ACCESS_SYS)
		PageAttr |= NV_PTE_SUPERVISOR_ONLY;

	if (Flag & NV_MEM_LARGE_PAGE)
		PageSize = 0x20000;
	else
		PageSize = 0x1000;

	KDEBUG("Map: %s : Virt:0x%016x, Phys:0x%016x, Flag:%x, Length:0x%x, Pages:%d\n",
		Flag & (NV_MEM_TARGET_HOST_NOSNOOP | NV_MEM_TARGET_HOST) ? "SYSRAM" : "VRAM",
		VirtAddr,
		PhysAddr,
		Flag,
		Length,
		Length / PageSize);

	for (Addr = 0; Addr < Length; Addr += PageSize)
	{
		PdeIndex = NVC0_PDE(VirtAddr);
		Pt0 = (PhysAddr >> 8) | PageAttr | NV_PTE_PRESENT;
		Pt1 = (PageAttr >> 32);

		if (Flag & NV_MEM_LARGE_PAGE)
		{
			PtPosition = 0;
			PteIndex = NVC0_LPTE(VirtAddr);
		}
		else
		{
			PtPosition = 4;
			PteIndex = NVC0_SPTE(VirtAddr);
		}

		PtAddr = GpuObjRd32(Pgd, (PdeIndex * 8 + PtPosition));
		if (PtAddr != 0)
		{
			PT->Addr = ((PtAddr & 0xFFFFFFFE) << 8);
			//KDEBUG("PT->Addr = %x\n", PT->Addr);
			//KDEBUG("PteIndex:%x, Pt0:%08x, Pt1:%08x\n", PteIndex, Pt0, Pt1);
			VmMapPage(PT, PteIndex, Pt0, Pt1);
		}
		else
		{
			KDEBUG("Error: Map %x out of range.\n", VirtAddr);
			return RETURN_OUT_OF_RESOURCES;
		}

		VirtAddr += PageSize;
		PhysAddr += PageSize;
	}
	
	NvMmioRd32(Gpu, 0x1000c80);
	NvMmioWr32(Gpu, 0x100cb8, Pgd->Addr >> 8);
	NvMmioWr32(Gpu, 0x100cbc, 0x80000000 | 0x00000001);
	return RETURN_SUCCESS;
}
		
//Bar VM
RETURN_STATUS BarVmInit(NVIDIA_GPU * Gpu, BAR_VM * BarVm, UINT64 Bar)
{
	if (Bar != 1 && Bar != 3)
		return RETURN_UNSUPPORTED;

	UINT64 Flag;
	UINT64 Length;

	UINT64 BarLength = PciGetBarSize(Gpu->PciDev, Bar);
	NvGpuObjNew(Gpu, 0x1000, 0x1000, &BarVm->Mem);

	NvGpuObjNew(Gpu, 0x2000 * 8, 0x1000, &BarVm->Pgd);

	GpuObjWr32(BarVm->Mem, 0x200, BarVm->Pgd->Addr);
	GpuObjWr32(BarVm->Mem, 0x204, BarVm->Pgd->Addr >> 32);
	GpuObjWr32(BarVm->Mem, 0x208, BarLength - 1);
	GpuObjWr32(BarVm->Mem, 0x20c, (BarLength - 1) >> 32);

	KDEBUG("NVIDIA:VM: Mapping BAR%d, Size = %x\n", Bar, BarLength);

	Length = BarLength;

	Flag = NV_MEM_TARGET_VRAM;

	VmNewSpace(BarVm->Pgd, Length, Flag);

	/* Identical mapping for user framebuffer. */
	if(Bar == 1)
		VmMapSpace(BarVm->Pgd, 0, 0, 0x4000000, Flag);

	NvMmioMask(Gpu, 0x000200, 0x00000100, 0x00000000);
	NvMmioMask(Gpu, 0x000200, 0x00000100, 0x00000100);

	//NvMmioWr32(Gpu, 0x100c30, 0x00208000);

	if (Bar == 1)
		NvMmioWr32(Gpu, 0x001704, 0x80000000 | (BarVm->Mem->Addr >> 12));
	else
		NvMmioWr32(Gpu, 0x001714, 0xc0000000 | (BarVm->Mem->Addr >> 12));

	MicroSecondDelay(1000);

	return RETURN_SUCCESS;
}

INT64 VmAllocate(SUBDEV_MEM * Mem, UINT64 Size, UINT64 Align, UINT64 Bar)
{
	if (Bar != 1 && Bar != 3)
		return RETURN_ABORTED;

	UINT64 Start, End, Index0, Index1;

	MEMORY_REGION * RegionPtr = (Bar == 1 ? Mem->Bar1Region : Mem->Bar3Region);
	UINT64 ReturnIndex;
	while (RegionPtr)
	{
		Start = RegionPtr->AllocatingIndex * RegionPtr->PageSize;
		End = RegionPtr->Length;

		if (Size < (End - Start))
		{
			ReturnIndex = RegionPtr->AllocatingIndex;
			RegionPtr->AllocatingIndex += (Size > RegionPtr->PageSize ? Size / RegionPtr->PageSize : 1);
			//for (Index1 = Index0; Index1 < Size / RegionPtr->PageSize; Index1++)
			//	RegionPtr->AllocatingTable[Index1] = 1;
			KDEBUG("NVIDIA: VM %s Allocated: %d 0x%08x\n", Bar == 1 ? "User" : "Kernel", ReturnIndex, ReturnIndex * RegionPtr->PageSize);
			return ReturnIndex * RegionPtr->PageSize + RegionPtr->Base;
		}

		RegionPtr = RegionPtr->Next;
	}

	return RETURN_OUT_OF_RESOURCES;
}

VOID VmFree(SUBDEV_MEM * Mem, UINT64 VmAddr)
{

}

VOID * VramMap(SUBDEV_MEM * Mem, UINT64 VirtAddr, UINT64 PhysAddr, UINT64 Length, UINT64 Attr, UINT64 Bar)
{
	NVIDIA_GPU * Gpu = Mem->Parent;
	NV_GPU_OBJ * Pgd = (Bar == 1 ? Mem->Bar1Vm->Pgd : Mem->Bar3Vm->Pgd);
	KDEBUG("Map: VirtAddr: %x ,PhysAddr: %x ,Bar: %d ,Length: %x \n", VirtAddr, PhysAddr, Bar, Length);
	VmMapSpace(Pgd, VirtAddr, PhysAddr, Length, Attr);
	VirtAddr += (Bar == 1 ? PciGetBarBase(Gpu->PciDev, 1) : PciGetBarBase(Gpu->PciDev, 3));
	return (VOID *)PHYS2VIRT(VirtAddr);
}

VOID * NvMapUser(SUBDEV_MEM * Mem, UINT64 PhysAddr, UINT64 Length)
{
	UINT64 VmAddr = Mem->VmAllocate(Mem, Length, 0x1000, 1);
	return Mem->Map(Mem, VmAddr, PhysAddr, Length, 0, 1);
}

VOID * NvMapKernel(SUBDEV_MEM * Mem, UINT64 PhysAddr, UINT64 Length)
{
	UINT64 VmAddr = Mem->VmAllocate(Mem, Length, 0x1000, 3);
	return Mem->Map(Mem, VmAddr, PhysAddr, Length, 0, 3);
}
