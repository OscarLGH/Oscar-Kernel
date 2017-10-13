#include "Gmc.h"

static UINT32 GoldenSettingsPolaris10[] =
{
	mmMC_ARB_WTM_GRPWT_RD, 0x00000003, 0x00000000,
	mmVM_PRT_APERTURE0_LOW_ADDR, 0x0fffffff, 0x0fffffff,
	mmVM_PRT_APERTURE1_LOW_ADDR, 0x0fffffff, 0x0fffffff,
	mmVM_PRT_APERTURE2_LOW_ADDR, 0x0fffffff, 0x0fffffff,
	mmVM_PRT_APERTURE3_LOW_ADDR, 0x0fffffff, 0x0fffffff
};

static VOID GmcGoldenRegisterInit(AMD_GPU * Gpu)
{
	AmdGpuProgramRegisterSequence(Gpu, GoldenSettingsPolaris10, sizeof(GoldenSettingsPolaris10));
}

RETURN_STATUS GmcWaitForIdle(AMD_GPU * Gpu)
{
	UINT32 Tmp;
	UINT32 i;

	for (i = 0; i < 2000; i++) {
		/* read MC_STATUS */
		Tmp = RREG32(mmSRBM_STATUS) & (SRBM_STATUS__MCB_BUSY_MASK |
			SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK |
			SRBM_STATUS__MCC_BUSY_MASK |
			SRBM_STATUS__MCD_BUSY_MASK |
			SRBM_STATUS__VMC_BUSY_MASK |
			SRBM_STATUS__VMC1_BUSY_MASK);
		if (!Tmp)
			return RETURN_SUCCESS;

		MicroSecondDelay(1);
	}
	return RETURN_TIMEOUT;
}

static VOID GmcMcStop(AMD_GPU * Gpu)
{
	UINT32 Blackout;

	KDEBUG("AMD GPU:GMC Stopped.\n");

	GmcWaitForIdle(Gpu);

	RREG32(mmMC_SHARED_BLACKOUT_CNTL);
	if (REG_GET_FIELD(Blackout, MC_SHARED_BLACKOUT_CNTL, BLACKOUT_MODE) != 1) {
		/* Block CPU access */
		WREG32(mmBIF_FB_EN, 0);
		/* blackout the MC */
		Blackout = REG_SET_FIELD(Blackout,
			MC_SHARED_BLACKOUT_CNTL, BLACKOUT_MODE, 1);
		WREG32(mmMC_SHARED_BLACKOUT_CNTL, Blackout);
	}
	/* wait for the MC to settle */
	MicroSecondDelay(100);
}

static VOID GmcMcResume(AMD_GPU * Gpu)
{
	UINT32 Tmp;

	/* unblackout the MC */
	Tmp = RREG32(mmMC_SHARED_BLACKOUT_CNTL);
	Tmp = REG_SET_FIELD(Tmp, MC_SHARED_BLACKOUT_CNTL, BLACKOUT_MODE, 0);
	WREG32(mmMC_SHARED_BLACKOUT_CNTL, Tmp);
	/* allow CPU access */
	Tmp = REG_SET_FIELD(0, BIF_FB_EN, FB_READ_EN, 1);
	Tmp = REG_SET_FIELD(Tmp, BIF_FB_EN, FB_WRITE_EN, 1);
	WREG32(mmBIF_FB_EN, Tmp);

	KDEBUG("AMD GPU:GMC Resumed.\n");
	//if (adev->mode_info.num_crtc)
	//	amdgpu_display_resume_mc_access(adev, save);
}

VOID GmcMcEarlyInit(AMD_GPU * Gpu)
{
#define AMDGPU_VM_SIZE 8
	Gpu->MemoryController.AddressMask = 0xffffffffffULL;
	Gpu->MemoryController.MaxPfn = (AMDGPU_VM_SIZE << 18);

	Gpu->MemoryController.AperBase = PciGetBarBase(Gpu->PciDev, 0);
	Gpu->MemoryController.AperSize = PciGetBarSize(Gpu->PciDev, 0);
	KDEBUG("AMD GPU:AperBase = 0x%08x, AperSize = %dMB.\n", Gpu->MemoryController.AperBase, Gpu->MemoryController.AperSize >> 20);

	Gpu->MemoryController.VramSize = ((UINT64)RREG32(mmCONFIG_MEMSIZE) << 20);
	Gpu->MemoryController.VisibleVramSize = Gpu->MemoryController.AperSize;
	UINT32 VramType = (RREG32(mmMC_SEQ_MISC0) >> 28);
	CHAR8 * VramTypeTable[] =
	{
		"Invalid type",
		"GDDR1",
		"DDR2",
		"GDDR3",
		"GDDR4",
		"GDDR5",
		"HBM"
	};

	KDEBUG("AMD GPU:VramSize = %dMB, Type:%s\n", Gpu->MemoryController.VramSize >> 20, VramTypeTable[VramType]);

	Gpu->MemoryController.GttBase = 0;
	Gpu->MemoryController.GttSize = 0x100000000;

	Gpu->MemoryController.PageTableBase = 0x3000000;
	Gpu->MemoryController.PageTableSize = 0x1000000;
}
/**
* GmcMcProgram - program the GPU memory controller
*
* @adev: amdgpu_device pointer
*
* Set the location of vram, gart, and AGP in the GPU's
* physical address space (CIK).
*/
static VOID GmcMcProgram(AMD_GPU * Gpu)
{
	int i,j;
	UINT32 Tmp;
	/* Initialize HDP */
	for (i = 0, j = 0; i < 32; i++, j += 0x6) {
		WREG32((0xb05 + j), 0x00000000);
		WREG32((0xb06 + j), 0x00000000);
		WREG32((0xb07 + j), 0x00000000);
		WREG32((0xb08 + j), 0x00000000);
		WREG32((0xb09 + j), 0x00000000);
	}
	WREG32(mmHDP_REG_COHERENCY_FLUSH_CNTL, 0);

	GmcMcStop(Gpu);
	if (GmcWaitForIdle(Gpu)) {
		KDEBUG("Wait for MC idle timedout !\n");
	}

	/* Update configuration */
	WREG32(mmMC_VM_SYSTEM_APERTURE_LOW_ADDR,
		0 >> 12);
	WREG32(mmMC_VM_SYSTEM_APERTURE_HIGH_ADDR,
		Gpu->MemoryController.VisibleVramSize >> 12);
	WREG32(mmMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR,
		0 >> 12);
	Tmp = ((Gpu->MemoryController.VisibleVramSize >> 24) & 0xFFFF) << 16;
	Tmp |= ((0 >> 24) & 0xFFFF);
	WREG32(mmMC_VM_FB_LOCATION, Tmp);
	KDEBUG("AMD GPU:System aperture low address:%08x\n", RREG32(mmMC_VM_SYSTEM_APERTURE_LOW_ADDR));
	KDEBUG("AMD GPU:System aperture hign address:%08x\n", RREG32(mmMC_VM_SYSTEM_APERTURE_HIGH_ADDR));
	KDEBUG("AMD GPU:FB Location:%08x\n", RREG32(mmMC_VM_FB_LOCATION));
	KDEBUG("AMD GPU:FB Offset:%08x\n", RREG32(mmMC_VM_FB_OFFSET));
	/* XXX double check these! */
	//WREG32(mmHDP_NONSURFACE_BASE, (0 >> 8));
	//WREG32(mmHDP_NONSURFACE_INFO, (2 << 7) | (1 << 30));
	//WREG32(mmHDP_NONSURFACE_SIZE, 0x3FFFFFFF);
	//WREG32(mmMC_VM_AGP_BASE, 0);
	//WREG32(mmMC_VM_AGP_TOP, 0x0FFFFFFF);
	//WREG32(mmMC_VM_AGP_BOT, 0x0FFFFFFF);
	if (GmcWaitForIdle(Gpu)) {
		KDEBUG("Wait for MC idle timedout !\n");
	}

	GmcMcResume(Gpu);

	extern SYSTEM_PARAMETERS *SysParam;
	KDEBUG("BIOS:Pixel Per ScanLine:%x\n", SysParam->PixelsPerScanLine);

	for (i = 0; i < 1; i++)
	{
		KDEBUG("%d:FB Address:%016x,Width = %d,Height = %d\n", i, 
			(UINT64)RREG32(mmGRPH_PRIMARY_SURFACE_ADDRESS + i) | 
			((UINT64)RREG32(mmGRPH_PRIMARY_SURFACE_ADDRESS_HIGH) << 32),
			RREG32(mmGRPH_X_END + i),
			RREG32(mmGRPH_Y_END + i));

		/* Set CRTC Address to 0.*/
		WREG32(mmGRPH_PRIMARY_SURFACE_ADDRESS + i, 0);
		WREG32(mmGRPH_PRIMARY_SURFACE_ADDRESS_HIGH + i, 0);
	}
	
	KDEBUG("AMD GPU:FB Offset:%08x\n", RREG32(mmMC_VM_FB_OFFSET));

	WREG32(mmBIF_FB_EN, BIF_FB_EN__FB_READ_EN_MASK | BIF_FB_EN__FB_WRITE_EN_MASK);

	Tmp = RREG32(mmHDP_MISC_CNTL);
	Tmp = REG_SET_FIELD(Tmp, HDP_MISC_CNTL, FLUSH_INVALIDATE_CACHE, 0);
	WREG32(mmHDP_MISC_CNTL, Tmp);

	Tmp = RREG32(mmHDP_HOST_PATH_CNTL);
	WREG32(mmHDP_HOST_PATH_CNTL, Tmp);

	//UINT32 * TestPtr = (UINT32 *)PHYS2VIRT(SysParam->FrameBufferBase);
	//for (i = 0; i < 0x100000; i++)
	//	TestPtr[i] = 0x00ff00ff;
	//while (1);
}


/*
* GART
* VMID 0 is the physical GPU addresses as used by the kernel.
* VMIDs 1-15 are used for userspace clients and are handled
* by the amdgpu vm/hsa code.
*/

/**
* gmc_v8_0_gart_flush_gpu_tlb - gart tlb flush callback
*
* @adev: amdgpu_device pointer
* @vmid: vm instance to flush
*
* Flush the TLB for the requested page table (CIK).
*/
static VOID GmcGartFlushGpuTlb(AMD_GPU * Gpu,
	UINT32 Vmid)
{
	/* flush hdp cache */
	WREG32(mmHDP_MEM_COHERENCY_FLUSH_CNTL, 0);

	/* bits 0-15 are the VM contexts0-15 */
	WREG32(mmVM_INVALIDATE_REQUEST, 1 << Vmid);
}

/**
* gmc_v8_0_gart_set_pte_pde - update the page tables using MMIO
*
* @adev: amdgpu_device pointer
* @cpu_pt_addr: cpu address of the page table
* @gpu_page_idx: entry in the page table to update
* @addr: dst addr to write into pte/pde
* @flags: access flags
*
* Update the page tables using the CPU.
*/
static VOID GmcSetPdePte(AMD_GPU * Gpu,
	UINT64 PtOffset,
	UINT64 GpuPageIndex,
	UINT64 Addr,
	UINT64 Flags)
{
	UINT64 Value;
	UINT64 PtAddrOnCpu = PHYS2VIRT(Gpu->MemoryController.AperBase + PtOffset);

	/*
	* PTE format on VI:
	* 63:40 reserved
	* 39:12 4k physical page base address
	* 11:7 fragment
	* 6 write
	* 5 read
	* 4 exe
	* 3 reserved
	* 2 snooped
	* 1 system
	* 0 valid
	*
	* PDE format on VI:
	* 63:59 block fragment size
	* 58:40 reserved
	* 39:1 physical base address of PTE
	* bits 5:1 must be 0.
	* 0 valid
	*/
	Value = Addr & 0x000000FFFFFFF000ULL;
	Value |= Flags;
	*(UINT32 *)(PtAddrOnCpu + GpuPageIndex * 8) = Value & 0xffffffff;
	*(UINT32 *)(PtAddrOnCpu + GpuPageIndex * 8 + 4) = (Value >> 32);
}

/**
* gmc_v8_0_set_fault_enable_default - update VM fault handling
*
* @adev: amdgpu_device pointer
* @value: true redirects VM faults to the default page
*/
static void gmc_v8_0_set_fault_enable_default(AMD_GPU * Gpu,
	bool value)
{
	u32 Tmp;

	Tmp = RREG32(mmVM_CONTEXT1_CNTL);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		VALID_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		READ_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL,
		EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	WREG32(mmVM_CONTEXT1_CNTL, Tmp);
}

static VOID SetTestPageTable(AMD_GPU * Gpu)
{
	UINT64 VirtAddrStart = 0;
	UINT64 AddrSize = 0x40000000;
	UINT64 PhysAddrStart = 0x200000;
	UINT64 Index;
	for (Index = 0; Index < AddrSize / 0x1000; Index++)
	{
		GmcSetPdePte(Gpu,
			Gpu->MemoryController.PageTableBase,
			Index,
			PhysAddrStart + Index * 0x1000,
			BIT0 | BIT1 | BIT5 | BIT6
		);
	}
}

/**
* GmcGartEnable - gart enable
*
* @adev: amdgpu_device pointer
*
* This sets up the TLBs, programs the page tables for VMID0,
* sets up the hw for VMIDs 1-15 which are allocated on
* demand, and sets up the global locations for the LDS, GDS,
* and GPUVM for FSA64 clients (CIK).
* Returns 0 for success, errors for failure.
*/
static void GmcGartEnable(AMD_GPU * Gpu)
{
	UINT32 r, i;
	UINT32 Tmp;

	//if (adev->gart.robj == NULL) {
	//	dev_err(adev->dev, "No VRAM object for PCIE GART.\n");
	//	return -EINVAL;
	//}
	//r = amdgpu_gart_table_vram_pin(adev);
	//if (r)
	//	return r;
	SetTestPageTable(Gpu);
	/* Setup TLB control */
	Tmp = RREG32(mmMC_VM_MX_L1_TLB_CNTL);
	Tmp = REG_SET_FIELD(Tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
	Tmp = REG_SET_FIELD(Tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_L1_FRAGMENT_PROCESSING, 1);
	Tmp = REG_SET_FIELD(Tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
	Tmp = REG_SET_FIELD(Tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 1);
	Tmp = REG_SET_FIELD(Tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
	WREG32(mmMC_VM_MX_L1_TLB_CNTL, Tmp);
	/* Setup L2 cache */
	Tmp = RREG32(mmVM_L2_CNTL);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, ENABLE_L2_CACHE, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, ENABLE_L2_PTE_CACHE_LRU_UPDATE_BY_WRITE, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, ENABLE_L2_PDE0_CACHE_LRU_UPDATE_BY_WRITE, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, EFFECTIVE_L2_QUEUE_SIZE, 7);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL, ENABLE_DEFAULT_PAGE_OUT_TO_SYSTEM_MEMORY, 1);
	WREG32(mmVM_L2_CNTL, Tmp);
	Tmp = RREG32(mmVM_L2_CNTL2);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL2, INVALIDATE_L2_CACHE, 1);
	WREG32(mmVM_L2_CNTL2, Tmp);
	Tmp = RREG32(mmVM_L2_CNTL3);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL3, L2_CACHE_BIGK_ASSOCIATIVITY, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL3, BANK_SELECT, 4);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL3, L2_CACHE_BIGK_FRAGMENT_SIZE, 4);
	WREG32(mmVM_L2_CNTL3, Tmp);
	/* XXX: set to enable PTE/PDE in system memory */
	Tmp = RREG32(mmVM_L2_CNTL4);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT0_PDE_REQUEST_PHYSICAL, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT0_PDE_REQUEST_SHARED, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT0_PDE_REQUEST_SNOOP, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT0_PTE_REQUEST_PHYSICAL, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT0_PTE_REQUEST_SHARED, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT0_PTE_REQUEST_SNOOP, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT1_PDE_REQUEST_PHYSICAL, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT1_PDE_REQUEST_SHARED, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT1_PDE_REQUEST_SNOOP, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT1_PTE_REQUEST_PHYSICAL, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT1_PTE_REQUEST_SHARED, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_L2_CNTL4, VMC_TAP_CONTEXT1_PTE_REQUEST_SNOOP, 0);
	WREG32(mmVM_L2_CNTL4, Tmp);
	/* setup context0 */
	WREG32(mmVM_CONTEXT0_PAGE_TABLE_START_ADDR, Gpu->MemoryController.GttBase >> 12);
	WREG32(mmVM_CONTEXT0_PAGE_TABLE_END_ADDR, (Gpu->MemoryController.GttBase + Gpu->MemoryController.GttSize) >> 12);
	WREG32(mmVM_CONTEXT0_PAGE_TABLE_BASE_ADDR, Gpu->MemoryController.PageTableBase >> 12);
	//WREG32(mmVM_CONTEXT0_PROTECTION_FAULT_DEFAULT_ADDR,
	//	(u32)(adev->dummy_page.addr >> 12));
	WREG32(mmVM_CONTEXT0_CNTL2, 0);
	Tmp = RREG32(mmVM_CONTEXT0_CNTL);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT0_CNTL, ENABLE_CONTEXT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT0_CNTL, PAGE_TABLE_DEPTH, 0);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT0_CNTL, RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	WREG32(mmVM_CONTEXT0_CNTL, Tmp);

	WREG32(mmVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR, 0);
	WREG32(mmVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR, 0);
	WREG32(mmVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET, 0);

	/* empty context1-15 */
	/* FIXME start with 4G, once using 2 level pt switch to full
	* vm size space
	*/
	/* set vm size, must be a multiple of 4 */
	WREG32(mmVM_CONTEXT1_PAGE_TABLE_START_ADDR, 0);
	WREG32(mmVM_CONTEXT1_PAGE_TABLE_END_ADDR, Gpu->MemoryController.MaxPfn - 1);
	for (i = 1; i < 16; i++) {
		if (i < 8)
			WREG32(mmVM_CONTEXT0_PAGE_TABLE_BASE_ADDR + i,
				Gpu->MemoryController.PageTableBase >> 12);
		else
			WREG32(mmVM_CONTEXT8_PAGE_TABLE_BASE_ADDR + i - 8,
				Gpu->MemoryController.PageTableBase >> 12);
	}

#define AMDGPU_VM_BLOCK_SIZE 9

	/* enable context1-15 */
	//WREG32(mmVM_CONTEXT1_PROTECTION_FAULT_DEFAULT_ADDR,
	//	(u32)(adev->dummy_page.addr >> 12));
	WREG32(mmVM_CONTEXT1_CNTL2, 4);
	Tmp = RREG32(mmVM_CONTEXT1_CNTL);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, ENABLE_CONTEXT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, PAGE_TABLE_DEPTH, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, VALID_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, READ_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
	Tmp = REG_SET_FIELD(Tmp, VM_CONTEXT1_CNTL, PAGE_TABLE_BLOCK_SIZE,
		AMDGPU_VM_BLOCK_SIZE - 9);
	WREG32(mmVM_CONTEXT1_CNTL, Tmp);
	//if (amdgpu_vm_fault_stop == AMDGPU_VM_FAULT_STOP_ALWAYS)
	//	gmc_v8_0_set_fault_enable_default(adev, false);
	//else
	//	gmc_v8_0_set_fault_enable_default(adev, true);

	GmcGartFlushGpuTlb(Gpu, 0);
	//DRM_INFO("PCIE GART of %uM enabled (table at 0x%016llX).\n",
	//	(unsigned)(adev->mc.gtt_size >> 20),
	//	(unsigned long long)adev->gart.table_addr);
	//adev->gart.ready = true;

	
	//return 0;
}

RETURN_STATUS GmcHwInit(AMD_GPU * Gpu)
{
	GmcMcEarlyInit(Gpu);
	GmcGoldenRegisterInit(Gpu);
	GmcMcProgram(Gpu);

	GmcGartEnable(Gpu);

	RETURN_STATUS RingBufferTestInit(AMD_GPU * Gpu);
	//RingBufferTestInit(Gpu);

	
	UINT32 * TestFault = (UINT32 *)PHYS2VIRT(PciGetBarBase(Gpu->PciDev, 0) + 0x2468);
	*TestFault = 0xdeadcafe;

	UINT64 FaultAddr = RREG32(mmVM_CONTEXT0_PROTECTION_FAULT_ADDR);
	UINT64 FaultStat = RREG32(mmVM_CONTEXT0_PROTECTION_FAULT_STATUS);
	UINT64 FaultClient = RREG32(mmVM_CONTEXT0_PROTECTION_FAULT_MCCLIENT);
	CHAR8 Client[] = { FaultClient >> 24, (FaultClient >> 16) & 0xFF, (FaultClient >> 8) & 0xFF ,(FaultClient) & 0xFF };
	KDEBUG("AMD GPU:VM Fault!Addr = %x,Status = %x, Client:%s\n", FaultAddr, FaultStat, Client);
	KDEBUG("AMD GPU:GART Enabled.\n");

	return RETURN_SUCCESS;
}