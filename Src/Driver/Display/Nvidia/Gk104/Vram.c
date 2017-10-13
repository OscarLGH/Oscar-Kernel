#include "Gk104.h"

RETURN_STATUS Gk104RamOneInit(SUBDEV_MEM * Mem)
{
	NVIDIA_GPU *Gpu = Mem->Parent;
	UINT64 Parts = NvMmioRd32(Gpu, 0x022438);
	UINT64 PMask = NvMmioRd32(Gpu, 0x022554);
	UINT64 BSize = NvMmioRd32(Gpu, 0x10f20c) << 20;
	UINT64 PSize = 0;
	UINT64 VramSize = 0;
	UINT64 Uniform = 1;
	UINT64 Ranks = 0;

	int i;
	for (i = 0; i < Parts; i++)
	{
		if (PMask & (1 << i))
			continue;

		PSize = (UINT64)NvMmioRd32(Gpu, 0x11020c + (i * 0x1000)) << 20;
		KDEBUG("NVIDIA:Mem Partition %d, Size = %d MB\n", i, PSize >> 20);
		
		if (PSize != BSize)
		{
			if (PSize < BSize)
				BSize = PSize;
			Uniform = 0;
		}
		VramSize += PSize;
	}

	Gpu->VRamSize = VramSize;
	KDEBUG("NVIDIA:Mem total size = %d MB,%s\n", VramSize >> 20, Uniform ? "Uniformed." : "Not uniformed.");
	Ranks = (NvMmioRd32(Gpu, 0x10f200) & 0x00000004) ? 2 : 1;
	KDEBUG("NVIDIA:Mem Ranks = %d\n", Ranks);


	Mem->VramRegion = KMalloc(sizeof(MEMORY_REGION));

	/* if all controllers have the same amount attached, there's no holes */
	if (Uniform)
	{
		Mem->VramRegion->Base = 0;
		Mem->VramRegion->Length = VramSize;
		Mem->VramRegion->Attribute = 0;
		Mem->VramRegion->PageSize = 0x1000;
		Mem->VramRegion->AllocatingTable = KMalloc(Mem->VramRegion->Base / Mem->VramRegion->PageSize);
		Mem->VramRegion->AllocatingIndex = 0x2000000 / Mem->VramRegion->PageSize;
		memset(Mem->VramRegion->AllocatingTable, 0, Mem->VramRegion->Base / Mem->VramRegion->PageSize);
		Mem->VramRegion->Next = NULL;
		KDEBUG("NVIDIA:MM Region:0x%016x -> 0x%016x\n", 
			Mem->VramRegion->Base, 
			Mem->VramRegion->Base + Mem->VramRegion->Length
			);
	}
	else
	{
		/* otherwise, address lowest common amount from 0GiB */
		Mem->VramRegion->Base = 0;
		Mem->VramRegion->Length = VramSize;
		Mem->VramRegion->Attribute = 0;
		Mem->VramRegion->PageSize = 0x1000;
		Mem->VramRegion->AllocatingTable = KMalloc(Mem->VramRegion->Base / Mem->VramRegion->PageSize);
		Mem->VramRegion->AllocatingIndex = 0x2000000 / Mem->VramRegion->PageSize;				//32MB Reserved for framebuffer
		memset(Mem->VramRegion->AllocatingTable, 0, Mem->VramRegion->Base / Mem->VramRegion->PageSize);
		Mem->VramRegion->Next = KMalloc(sizeof(MEMORY_REGION));
		KDEBUG("NVIDIA:MM Region:0x%016x -> 0x%016x\n",
			Mem->VramRegion->Base,
			Mem->VramRegion->Base + Mem->VramRegion->Length
			);
		/* and the rest starting from (8GiB + common_size) */
		Mem->VramRegion->Next->Base = 0x0000000200000000ULL + BSize;
		Mem->VramRegion->Next->Length = VramSize - (BSize * Parts);
		Mem->VramRegion->Next->Attribute = 0;
		Mem->VramRegion->Next->PageSize = 0x1000;
		Mem->VramRegion->Next->AllocatingTable = KMalloc(Mem->VramRegion->Next->Base / Mem->VramRegion->Next->PageSize);
		Mem->VramRegion->Next->AllocatingIndex = 0;		
		memset(Mem->VramRegion->Next->AllocatingTable, 0, Mem->VramRegion->Next->Base / Mem->VramRegion->Next->PageSize);
		Mem->VramRegion->Next->Next = NULL;
		KDEBUG("NVIDIA:MM Region:0x%016x -> 0x%016x\n",
			Mem->VramRegion->Next->Base,
			Mem->VramRegion->Next->Base + Mem->VramRegion->Next->Length
			);
	}

	Mem->Bar1Region = KMalloc(sizeof(MEMORY_REGION));
	Mem->Bar1Region->Base = 0;
	Mem->Bar1Region->Length = PciGetBarSize(Gpu->PciDev, 1);
	Mem->Bar1Region->PageSize = 0x1000;
	Mem->Bar1Region->AllocatingIndex = 0x2000000 / Mem->Bar1Region->PageSize;	//32MB Reserved for framebuffer
	Mem->Bar1Region->Next = NULL;
	KDEBUG("NVIDIA:VM BAR1 space length = %dMB.\n", Mem->Bar1Region->Length >> 20);

	Mem->Bar3Region = KMalloc(sizeof(MEMORY_REGION));
	Mem->Bar3Region->Base = 0;
	Mem->Bar3Region->Length = PciGetBarSize(Gpu->PciDev, 3);
	Mem->Bar3Region->PageSize = 0x1000;
	Mem->Bar3Region->AllocatingIndex = 0;
	Mem->Bar3Region->Next = NULL;
	KDEBUG("NVIDIA:VM BAR3 space length = %dMB.\n", Mem->Bar3Region->Length >> 20);

	Mem->VramAllocate = VramZallocate;
	Mem->VmAllocate = VmAllocate;
	Mem->VramFree = VramFree;
	//Mem->VmFree = VmFree;
	Mem->Map = VramMap;
	//Mem->UmMap = VramUnMap;

	Mem->Bar1Vm = KMalloc(sizeof(BAR_VM));
	memset(Mem->Bar1Vm, 0, sizeof(BAR_VM));
	Mem->Bar3Vm = KMalloc(sizeof(BAR_VM));
	memset(Mem->Bar3Vm, 0, sizeof(BAR_VM));

	BarVmInit(Gpu, Mem->Bar1Vm, 1);
	BarVmInit(Gpu, Mem->Bar3Vm, 3);
	

	//Test
	//UINT32 * TestPtr = NvMapUser(Mem, 0x200000, 0x200000);
	//int i;
	//for (i = 0; i < 0x100000; i += 4)
	//{
	//	*(TestPtr + i / 4) = 0x0012345678;
	//}
	//UINT64 SysAddr = 0x100000000;
	//UINT32 * TestPtr = (UINT32 *)PHYS2VIRT(PciGetBarBase(Gpu->PciDev, 3));
	
	//UINT32 * SysPtr = (UINT32 *)PHYS2VIRT(SysAddr);
	//KDEBUG("Sysram mapping test:%08x,%08x\n", *TestPtr, *SysPtr);
	//*TestPtr = 0xabcdefab;
	//KDEBUG("Sysram mapping test:%08x,%08x\n", *TestPtr, *SysPtr);
	//*SysPtr = 0xabcdefab;
	//KDEBUG("Sysram mapping test:%08x,%08x\n", *TestPtr, *SysPtr);
	//UINT32 Value;
	//for (i = 0; i < 16 * 1024 * 1024 / 4; i++)
	//{
	//	*(TestPtr + i) = 0x0055f00f;
	//	KDEBUG("VM Test:%08x\n", *(TestPtr + i));
	//	Value = InstVramRd32(Gpu, i);
	//	//if(Value != 0x0055aa55)
	//	//	KDEBUG("%08x:VM:%08x,Inst:%08x\n", i, *(TestPtr + i), Value);
	//}
	/*UINT32 Value;
	UINT32 * Ptr;
	Ptr = (UINT32 *)PHYS2VIRT(PCIGetBarBase(Gpu->PciDev->Position, 1));
	UINT64 BarSize = PCIGetBarSize(Gpu->PciDev->Position, 1);
	for (i = 0; i < 0x80000000; i += 4)
	{
		InstVramWr32(Gpu, i, i);
	}
	for (i = 0x1000000; i < 0x80000000; i += 4)
	{
		if (i % 0x1000000 == 0)
			KDEBUG("Mem Testing:%x...\n", i);

		if(i < BarSize)
			Value = Ptr[i/4];
		else
			Value = InstVramRd32(Gpu, i);
		if(Value != i)
			KDEBUG("InstVramTest:%08x:%08x\n", i, Value);
	}*/
	
}
