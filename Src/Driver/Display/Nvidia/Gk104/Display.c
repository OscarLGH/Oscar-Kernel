#include "Gk104.h"

RETURN_STATUS Gk104DisplayInit(DISPLAY_ENGINE * Engine)
{
	NVIDIA_GPU * Gpu = Engine->Parent;

	UINT8 Head = 0;
	UINT8 Ver = 0;
	UINT8 Cnt = 0;
	UINT8 Len = 0;

	DcbTable(Gpu->VBios, &Ver, &Head, &Cnt, &Len);

	KDEBUG("Dcb:Ver = %x, Head = %x, Cnt = %x, Len = %x\n", Ver, Head, Cnt, Len);

	NvMmioWr32(Gpu, 0x645080, 0);
	NvMmioWr32(Gpu, 0x645084, 0);

	/* initialise channel for dma command submission */
	//NvMmioWr32(Gpu, 0x610494, (UINT32)Engine->CoreChannel->RunList[0]->Addr);
	NvMmioWr32(Gpu, 0x610498, 0x00010000);
	NvMmioWr32(Gpu, 0x61049c, 0x00000001);
	NvMmioMask(Gpu, 0x610490, 0x00000010, 0x00000010);
	NvMmioWr32(Gpu, 0x640000, 0x00000000);
	NvMmioWr32(Gpu, 0x610490, 0x01000013);
	int i = 0;

	Head = NvMmioRd32(Gpu, 0x22448);
	KDEBUG("NVIDIA:CRTCS = %d\n", Head);
	
	/* steal display away from vbios, or something like that */
	if (NvMmioRd32(Gpu, 0x6100ac) & 0x00000100)
	{
		NvMmioWr32(Gpu, 0x6100ac, 0x00000100);
		NvMmioMask(Gpu, 0x6194e8, 0x00000001, 0x00000000);
	}

	/* point at display engine memory area (hash table, objects) */
	//NvMmioWr32(Gpu, 0x610010, (Engine->CoreChannel->RunList[0]->Addr >> 8) | 9);

	/* enable supervisor interrupts, disable everything else */
	NvMmioWr32(Gpu, 0x610090, 0x00000000);
	NvMmioWr32(Gpu, 0x6100a0, 0x00000000);
	NvMmioWr32(Gpu, 0x6100b0, 0x00000307);

	/*vblank_init*/
	NvMmioMask(Gpu, 0x6100c0 + 0x800, 0x00000001, 0x00000001);

	Head = 1;
	for (i = 0; i < Head; i++)
	{
		UINT64 Total = NvMmioRd32(Gpu, 0x640414 + i * 0x300);
		UINT64 Blanke = NvMmioRd32(Gpu, 0x64041c + i * 0x300);
		UINT64 Blanks = NvMmioRd32(Gpu, 0x640420 + i * 0x300);
		UINT64 VLine = NvMmioRd32(Gpu, 0x616340 + i * 0x800);
		UINT64 HLine = NvMmioRd32(Gpu, 0x616344 + i * 0x800);
		//KDEBUG("Total = %x, Blanke = %x, Blanks = %x, VLine = %x, HLine = %x\n", Total, Blanke, Blanks, VLine, HLine);
	}

	/*PIOC init*/
	//NvMmioWr32(Gpu, 0x610200, 0x00002000);
	//MicroSecondDelay(2000);
	//NvMmioWr32(Gpu, 0x610200, 0x00000001);
}
