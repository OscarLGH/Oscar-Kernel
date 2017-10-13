#include "AmdGpu.h"

RETURN_STATUS AmdGpuSupported(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	if (Dev->BusType != BUS_TYPE_PCI)
		return RETURN_UNSUPPORTED;

	if (Dev->DeviceType != DEVICE_TYPE_DISPLAY)
		return RETURN_UNSUPPORTED;

	if (AMD_VENDOR_ID != PciCfgRead16(PciDev, PCI_CONF_VENDORID_OFF_16))
		return RETURN_UNSUPPORTED;

	KDEBUG("AMD GPU driver supported on device:%02x:%02x.%02x\n",
		PCI_GET_BUS(Dev->Position),
		PCI_GET_DEVICE(Dev->Position),
		PCI_GET_FUNCTION(Dev->Position));

	return RETURN_SUCCESS;
}


void AmdGpuIrq(void * Dev)
{
	//NVIDIA_GPU * Gpu = Dev;

	KDEBUG("AMD GPU:GPU Interrupt.\n");
	
}

RETURN_STATUS GmcHwInit(AMD_GPU * Gpu);

RETURN_STATUS AmdGpuStart(DEVICE_NODE * Dev)
{
	PCI_DEVICE * PciDev = Dev->Device;
	AMD_GPU * Gpu = KMalloc(sizeof(*Gpu));
	Gpu->PciDev = PciDev;
	UINT64 Chipset = 0;
	KDEBUG("AMD GPU driver starting...\n");
	KDEBUG("AMD GPU:Device ID:%04x\n", PciCfgRead16(PciDev, PCI_CONF_DEVICEID_OFF_16));
	UINT16 PciCommand = PciCfgRead16(PciDev, PCI_CONF_COMMAND_OFF_16);
	PciCommand |= (BIT1 | BIT2);	/* Enable bus master, memory. */
	PciCommand ^= BIT10;			/* Enable Interrupt.*/
	PciCfgWrite16(PciDev, PCI_CONF_COMMAND_OFF_16, PciCommand);

	IrqRegister(0x48, AmdGpuIrq, NULL);

	PciMsiCheck(PciDev);
	UINT64 Irqs = PciMsiGetMultipleMessageCapable(PciDev);
	KDEBUG("AMD GPU:PCI-MSI:Request %d IRQs.\n", Irqs);
	PciMsiMultipleMessageCapableEnable(PciDev, Irqs);
	MSI_CAP_STRUCTURE_64_MASK PciMsi;
	PciMsi.MessageAddr = 0xFEE00000;
	PciMsi.MessageData = 0x48;
	PciMsiSet(PciDev, &PciMsi);
	PciMsiEnable(PciDev);

	Gpu->MmioBase = (UINT32 *)PHYS2VIRT(PciGetBarBase(Gpu->PciDev, 5));
	Gpu->MmioSize = PciGetBarSize(Gpu->PciDev, 5);

	GmcHwInit(Gpu);
	//extern SYSTEM_PARAMETERS *SysParam;
	//KDEBUG("Pixel Per ScanLine:%x\n", SysParam->PixelsPerScanLine);

	

	return RETURN_SUCCESS;
}

/**
* amdgpu_program_register_sequence - program an array of registers.
*
* @adev: amdgpu_device pointer
* @registers: pointer to the register array
* @array_size: size of the register array
*
* Programs an array or registers with and and or masks.
* This is a helper for setting golden registers.
*/
VOID AmdGpuProgramRegisterSequence(AMD_GPU * Gpu, UINT32 *Registers, UINT32 ArraySize)
{
	UINT32 Tmp, Reg, AndMask, OrMask;
	UINT32 i;

	if (ArraySize % 3)
		return;

	for (i = 0; i < ArraySize; i += 3) {
		Reg = Registers[i + 0];
		AndMask = Registers[i + 1];
		OrMask = Registers[i + 2];

		if (AndMask == 0xffffffff) {
			Tmp = OrMask;
		}
		else {
			Tmp = RREG32(Reg);
			Tmp &= ~AndMask;
			Tmp |= OrMask;
		}
		WREG32(Reg, Tmp);
	}
}

DRIVER_NODE AmdGpuDriver =
{
	.BusType = BUS_TYPE_PCI,
	.DriverSupported = AmdGpuSupported,
	.DriverStart = AmdGpuStart,
	.DriverStop = 0,
	.Operations = NULL,
	.PrevDriver = 0,
	.NextDriver = 0
};

static void AmdGpuDriverInit()
{
	DriverRegister(&AmdGpuDriver, &DriverList[DEVICE_TYPE_DISPLAY]);
}

EARLY_MODULE_INIT(AmdGpuDriverInit);
