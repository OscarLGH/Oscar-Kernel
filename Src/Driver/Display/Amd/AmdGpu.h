#ifndef _AMD_GPU_H
#define _AMD_GPU_H

#include "Display.h"
#include "Pci.h"

#define DEBUG
#include "Debug.h"

#define AMD_VENDOR_ID 0x1002

typedef struct _RING_BUFFER
{
	UINT32 * RingBuffer;
	UINT64 GpuAddress;
	UINT64 RingSize;
	UINT64 ReadPtr;
	UINT64 WritePtr;
	UINT64 Reserved;

}RING_BUFFER;

typedef struct _AMD_GPU
{
	PCI_DEVICE * PciDev;
	volatile UINT32 * MmioBase;
	UINT64 MmioSize;

	struct
	{
		UINT64 AddressMask;
		UINT64 MaxPfn;

		UINT64 AperBase;
		UINT64 AperSize;

		UINT64 VramSize;
		UINT64 VisibleVramSize;

		UINT64 GttBase;
		UINT64 GttSize;
		
		UINT64 PageTableBase;
		UINT64 PageTableSize;

	}MemoryController;

	RING_BUFFER * Ring;
}AMD_GPU;



static inline UINT32 AmdMmioRd32(AMD_GPU * Gpu, UINT64 Offset);
static inline VOID AmdMmioWr32(AMD_GPU * Gpu, UINT64 Offset, UINT32 Value);
static inline VOID AmdMmioMask(AMD_GPU * Gpu, UINT64 Offset, UINT64 Mask, UINT64 Value);

#define RREG32(reg) AmdMmioRd32(Gpu, ((reg)<<2))
#define WREG32(reg, v) AmdMmioWr32(Gpu, ((reg)<<2), (v))

#define REG_FIELD_SHIFT(reg, field) reg##__##field##__SHIFT
#define REG_FIELD_MASK(reg, field) reg##__##field##_MASK

#define REG_SET_FIELD(orig_val, reg, field, field_val)			\
	(((orig_val) & ~REG_FIELD_MASK(reg, field)) |			\
	 (REG_FIELD_MASK(reg, field) & ((field_val) << REG_FIELD_SHIFT(reg, field))))

#define REG_GET_FIELD(value, reg, field)				\
	(((value) & REG_FIELD_MASK(reg, field)) >> REG_FIELD_SHIFT(reg, field))

#define WREG32_FIELD(reg, field, val)	\
	WREG32(mm##reg, (RREG32(mm##reg) & ~REG_FIELD_MASK(reg, field)) | (val) << REG_FIELD_SHIFT(reg, field))

VOID AmdGpuProgramRegisterSequence(AMD_GPU * Gpu, UINT32 *Registers, UINT32 ArraySize);

static inline UINT32 AmdMmioRd32(AMD_GPU * Gpu, UINT64 Offset)
{
	if(Offset < Gpu->MmioSize)
		return Gpu->MmioBase[Offset >> 2];
	else
	{
		UINT32 RegValue;
		SPIN_LOCK SpinLock;
		SpinLockIrqSave(&SpinLock);
		Gpu->MmioBase[0] = Offset;		//MMIO[0]:Index Reg
		RegValue = Gpu->MmioBase[1];	//MMIO[1]:Data Reg
		SpinUnlockIrqRestore(&SpinLock);

		return RegValue;
	}
}

static inline VOID AmdMmioWr32(AMD_GPU * Gpu, UINT64 Offset, UINT32 Value)
{
	if (Offset < Gpu->MmioSize)
	{
		Gpu->MmioBase[Offset >> 2] = Value;
		return;
	}
	else
	{
		SPIN_LOCK SpinLock;
		SpinLockIrqSave(&SpinLock);
		Gpu->MmioBase[0] = Offset;		//MMIO[0]:Index Reg
		Gpu->MmioBase[1] = Value;		//MMIO[1]:Data Reg
		SpinUnlockIrqRestore(&SpinLock);

		return;
	}
}

static inline VOID AmdMmioMask(AMD_GPU * Gpu, UINT64 Offset, UINT64 Mask, UINT64 Value)
{
	UINT32 Temp = AmdMmioRd32(Gpu, Offset);
	AmdMmioWr32(Gpu, Offset, (Temp & ~Mask) | Value);
}

#endif