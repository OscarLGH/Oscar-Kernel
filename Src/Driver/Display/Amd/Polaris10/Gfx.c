#include "Gfx.h"

#define	PACKET_TYPE3	3
#define PACKET3(op, n)	((PACKET_TYPE3 << 30) |				\
			 (((op) & 0xFF) << 8) |				\
			 ((n) & 0x3FFF) << 16)

#define	PACKET3_SET_UCONFIG_REG				0x79
#define		PACKET3_SET_UCONFIG_REG_START			0x0000c000
#define STRACH 0xc040

static VOID GfxEnable(AMD_GPU * Gpu, bool enable)
{
	int i;
	u32 Tmp = RREG32(mmCP_ME_CNTL);

	if (enable) {
		Tmp = REG_SET_FIELD(Tmp, CP_ME_CNTL, ME_HALT, 0);
		Tmp = REG_SET_FIELD(Tmp, CP_ME_CNTL, PFP_HALT, 0);
		Tmp = REG_SET_FIELD(Tmp, CP_ME_CNTL, CE_HALT, 0);
	}
	else {
		Tmp = REG_SET_FIELD(Tmp, CP_ME_CNTL, ME_HALT, 1);
		Tmp = REG_SET_FIELD(Tmp, CP_ME_CNTL, PFP_HALT, 1);
		Tmp = REG_SET_FIELD(Tmp, CP_ME_CNTL, CE_HALT, 1);
		//for (i = 0; i < adev->gfx.num_gfx_rings; i++)
		//	adev->gfx.gfx_ring[i].ready = false;
	}
	WREG32(mmCP_ME_CNTL, Tmp);
	MicroSecondDelay(50);
}
RETURN_STATUS RingBufferTestInit(AMD_GPU * Gpu)
{
	UINT32 Tmp;
	UINT32 RingBufferSize;
	RING_BUFFER * Ring;
	UINT64 RingBufferGpuAddr = 0;
	Gpu->Ring = KMalloc(sizeof(*(Gpu->Ring)));
	Ring = Gpu->Ring;
	Ring->RingSize = 64 * 1024;
	Ring->RingBuffer = KMalloc(Gpu->Ring->RingSize);
	Ring->ReadPtr = 0;
	Ring->WritePtr = 0;
	/* Set the write pointer delay */
	WREG32(mmCP_RB_WPTR_DELAY, 0);

	/* set the RB to use vmid 0 */
	WREG32(mmCP_RB_VMID, 0);

	/* Set ring buffer size */
	
	RingBufferSize = OrderBase2(Gpu->Ring->RingSize / 8);
	Tmp = REG_SET_FIELD(0, CP_RB0_CNTL, RB_BUFSZ, RingBufferSize);
	Tmp = REG_SET_FIELD(Tmp, CP_RB0_CNTL, RB_BLKSZ, RingBufferSize - 2);
	Tmp = REG_SET_FIELD(Tmp, CP_RB0_CNTL, MTYPE, 3);
	Tmp = REG_SET_FIELD(Tmp, CP_RB0_CNTL, MIN_IB_AVAILSZ, 1);
#ifdef __BIG_ENDIAN
	Tmp = REG_SET_FIELD(Tmp, CP_RB0_CNTL, BUF_SWAP, 1);
#endif
	WREG32(mmCP_RB0_CNTL, Tmp);

	/* Initialize the ring buffer's read and write pointers */
	WREG32(mmCP_RB0_CNTL, Tmp | CP_RB0_CNTL__RB_RPTR_WR_ENA_MASK);
	//ring->wptr = 0;
	WREG32(mmCP_RB0_WPTR, Ring->WritePtr);

	/* set the wb address wether it's enabled or not */
	//rptr_addr = adev->wb.gpu_addr + (ring->rptr_offs * 4);
	//WREG32(mmCP_RB0_RPTR_ADDR, lower_32_bits(rptr_addr));
	//WREG32(mmCP_RB0_RPTR_ADDR_HI, upper_32_bits(rptr_addr) & 0xFF);

	MicroSecondDelay(1);
	WREG32(mmCP_RB0_CNTL, Tmp);

	Ring->GpuAddress = VIRT2PHYS(Ring->RingBuffer);
	KDEBUG("AMD GPU:Ring Buffer host address:%016x\n", Ring->RingBuffer);
	RingBufferGpuAddr = Ring->GpuAddress >> 8;
	WREG32(mmCP_RB0_BASE, RingBufferGpuAddr);
	WREG32(mmCP_RB0_BASE_HI, RingBufferGpuAddr >> 32);


	/* init the CP */
	WREG32(mmCP_MAX_CONTEXT, 16 - 1);
	WREG32(mmCP_ENDIAN_SWAP, 0);
	WREG32(mmCP_DEVICE_ID, 1);

	GfxEnable(Gpu, 1);


	Ring->RingBuffer[0] = PACKET3(PACKET3_SET_UCONFIG_REG, 1);
	Ring->RingBuffer[1] = STRACH - PACKET3_SET_UCONFIG_REG_START;
	Ring->RingBuffer[2] = 0xDEADBEEF;
	

	WREG32(mmCP_RB0_WPTR, 16);
	(void)RREG32(mmCP_RB0_WPTR);

	KDEBUG("AMD GPU:Ring test start.\n");
	UINT64 TimeOut = 20;
	while (--TimeOut)
	{
		UINT32 Reg = RREG32(STRACH);
		
		if (Reg == 0xDEADBEEF)
			break;

		MilliSecondDelay(1);
	}
	KDEBUG("AMD GPU:Ring test %s.\n", TimeOut ? "succeed":"failed");
}