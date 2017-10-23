#include "NvidiaGpu.h"
#include "NvidiaChannel.h"

RETURN_STATUS NvidiaGpuSupported(DEVICE_NODE * Dev)
{	
	PCI_DEVICE * PciDev = Dev->Device;
	if(Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;
	
	if(Dev->DeviceType != DEVICE_TYPE_DISPLAY)
		return RETURN_UNSUPPORTED;

	if(NVIDIA_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;

	KDEBUG("NVIDIA GPU driver supported on device:%02x:%02x.%02x\n", 
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	return RETURN_SUCCESS;
}

RETURN_STATUS NvidiaGpuFifoIrq(NVIDIA_GPU * Gpu);
void NvidiaGpuIrq(void * Dev)
{
	NVIDIA_GPU * Gpu = Dev;

	/*Unarm Intr.*/
	NvMmioWr32(Gpu, 0x140, 0);
	NvMmioWr32(Gpu, 0x144, 0);
	NvMmioRd32(Gpu, 0x140);

	printk("NVIDIA GPU Intr.\n");
	/*Rearm MSI.*/
	//NvMmioWr32(Gpu, 0x88000 + 0x0704, 0x00000000);
	NvMmioWr08(Gpu, 0x88000 + 0x0068, 0x000000ff);

	UINT32 Intr0 = NvMmioRd32(Gpu, 0x100);
	UINT32 Intr1 = NvMmioRd32(Gpu, 0x104);

	NvidiaGpuFifoIrq(Gpu);

	/*Rearm Intr.*/
	NvMmioWr32(Gpu, 0x140, 1);
	NvMmioWr32(Gpu, 0x144, 1);

}

extern void Gk104Constructor();
extern void Gk208Constructor();
extern void Gp100Constructor();

struct
{
	UINT64 Id;
	void(*ChipsetConstructor)(NVIDIA_GPU *);
	CHAR8 * Name;
} ChipsetArray[] = 
{
	{0x0e4, Gk104Constructor, "GK104"},
	{0x106, Gk208Constructor, "GK208"},
	{0x117, Gk104Constructor, "GM107"},
	//{0x118, Gk104Constructor, "GM107"},
	{0x132, Gp100Constructor, "GP102"},
	{0x134, Gp100Constructor, "GP104"}
};

RETURN_STATUS NvidiaFbInit(NVIDIA_GPU * Gpu);
RETURN_STATUS NvidiaMemoryCopy(NVIDIA_GPU * Gpu, UINT64 SrcOffset, UINT64 DstOffset, UINT64 Len);

RETURN_STATUS NvidiaGpuStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	UINT64 Chipset = 0;
	KDEBUG("NVIDIA GPU driver starting...\n"); 
	UINT16 PciCommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PciCommand |= (BIT1 | BIT2);	/* Enable bus master, memory. */
	//PciCommand ^= BIT10;			/* Enable Interrupt.*/
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PciCommand);
	
	Dev->PrivateData = KMalloc(sizeof(NVIDIA_GPU));
	NVIDIA_GPU * Gpu = Dev->PrivateData;
	memset(Gpu, 0, sizeof(NVIDIA_GPU));
	Gpu->PciDev = Dev->Device;

	Gpu->MmioBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(PciDev, 0));
	
	UINT32 Boot0 = NvMmioRd32(Gpu, 0);
	UINT32 Strap = NvMmioRd32(Gpu, 0x10100);
	
	Chipset = (Boot0 >> 20) & 0x1ff;
	KDEBUG("NVIDIA:Chipset = %08x\n", Chipset);
	KDEBUG("NVIDIA:Strap = %08x\n", Strap & 0x00400040);
	
	//SYSTEM_PARAMETERS *SysParam = (SYSTEM_PARAMETERS *)SYSTEMPARABASE;
	//KDEBUG("BIOS setup framebuffer base:%x\n", SysParam->FrameBufferBase);

#if 0
	UINT32 Mask = 0;
	NvMmioWr32(Gpu, 0x180, ~Mask);
	NvMmioWr32(Gpu, 0x160, Mask);
	NvMmioWr32(Gpu, 0x184, ~Mask);
	NvMmioWr32(Gpu, 0x164, Mask);

	Mask = 0x7fffffff;
	NvMmioWr32(Gpu, 0x180, ~Mask);
	NvMmioWr32(Gpu, 0x160, Mask);
	NvMmioWr32(Gpu, 0x184, ~Mask);
	NvMmioWr32(Gpu, 0x164, Mask);
#endif

	//NvMmioWr32(Gpu, 0x640, 0xffffffff);
	//NvMmioWr32(Gpu, 0x644, 0xffffffff);
	//NvMmioWr32(Gpu, 0x648, 0xffffffff);

	IrqRegister(0x40, NvidiaGpuIrq, Gpu);

	PciMsiCheck(PciDev);
	UINT64 Irqs = PciMsiGetMultipleMessageCapable(PciDev);
	KDEBUG("NVIDIA:PCI-MSI:Request %d IRQs.\n", Irqs);
	PciMsiMultipleMessageCapableEnable(PciDev, Irqs);
	MSI_CAP_STRUCTURE_64_MASK PciMsi;
	PciMsi.MessageAddr = 0xFEE00000;
	PciMsi.MessageData = 0x40;
	PciMsiSet(PciDev, &PciMsi);
	PciMsiEnable(PciDev);

	/* Rearm MSI*/
	//NvMmioWr32(Gpu, 0x88000 + 0x0704, 0x00000000);
	NvMmioWr08(Gpu, 0x88000 + 0x0068, 0xff);

	//NvMmioWr32(Gpu, 0x200, 0xfc37b16f); //GK208
	//NvMmioWr32(Gpu, 0x200, 0x5c6cf1e1); //GP104
	NvMmioWr32(Gpu, 0x200, 0xFFFFFFFF);


	NvMmioWr32(Gpu, 0x140, 0x3);
	NvMmioWr32(Gpu, 0x144, 0x3);

	KDEBUG("NVIDIA:PCI-MSI:Enabled.\n", Irqs);

	int i;
	for (i = 0; i < sizeof(ChipsetArray) / sizeof(ChipsetArray[0]); i++)
	{
		if (ChipsetArray[i].Id == Chipset)
			break;
	}

	if (i != sizeof(ChipsetArray) / sizeof(ChipsetArray[0]))
	{
		ChipsetArray[i].ChipsetConstructor(Gpu);

		if (Gpu->VBios->OneInit)
			Gpu->VBios->OneInit(Gpu->VBios);

		if (Gpu->VramManager->OneInit)
			Gpu->VramManager->OneInit(Gpu->VramManager);

		if (Gpu->Therm->OneInit)
			Gpu->Therm->OneInit(Gpu->Therm);

		if (Gpu->MemoryCopyEngine->OneInit)
			Gpu->MemoryCopyEngine->OneInit(Gpu->MemoryCopyEngine);

		if (Gpu->DisplayEngine->OneInit)
			Gpu->DisplayEngine->OneInit(Gpu->DisplayEngine);

		if (Gpu->GraphicEngine->OneInit)
			Gpu->GraphicEngine->OneInit(Gpu->GraphicEngine);

		if (Gpu->VBios->Init)
			Gpu->VBios->Init(Gpu->VBios);

		if (Gpu->VramManager->Init)
			Gpu->VramManager->Init(Gpu->VramManager);

		if (Gpu->Therm->Init)
			Gpu->Therm->Init(Gpu->Therm);

		if (Gpu->MemoryCopyEngine->Init)
			Gpu->MemoryCopyEngine->Init(Gpu->MemoryCopyEngine);

		if (Gpu->DisplayEngine->Init)
			Gpu->DisplayEngine->Init(Gpu->DisplayEngine);


		if (Gpu->GraphicEngine->Init)
			Gpu->GraphicEngine->Init(Gpu->GraphicEngine);

		NvidiaFbInit(Gpu);

	}
	else
	{
		KDEBUG("NVIDIA:Chipset 0x%x not supported.\n", Chipset);
		return RETURN_UNSUPPORTED;
	}
	//while (1);
	return RETURN_SUCCESS;
}

DRIVER_NODE NvidiaGpuDriver = 
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = NvidiaGpuSupported,
	.DriverStart = NvidiaGpuStart,
	.DriverStop = 0,
	.Operations = NULL,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void NvidiaGpuDriverInit()
{
	DriverRegister(&NvidiaGpuDriver, &DriverList[DEVICE_TYPE_DISPLAY]);
}

EARLY_MODULE_INIT(NvidiaGpuDriverInit);
