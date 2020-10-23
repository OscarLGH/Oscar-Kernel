#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>
#include <bitmap.h>
#include <string.h>
#include <math.h>
#include <fb.h>
#include <irq.h>
#include <cpu.h>
#include "ixgbe_type.h"

struct ixgbe_hw {
	struct pci_dev *pdev;
	volatile u32 *mmio_virt;
};

u32 ixgbe_read_reg(struct ixgbe_hw *hw, u32 reg);

u32 ixgbe_read_reg(struct ixgbe_hw *hw, u32 reg)
{
	return *(u32 *)(hw->mmio_virt + reg / 4);
}

#define IXGBE_READ_REG(a, reg) ixgbe_read_reg((a), (reg))

static inline void ixgbe_write_reg(struct ixgbe_hw *hw, u32 reg, u32 value)
{
	*(u32 *)(hw->mmio_virt + reg / 4) = value;
}
#define IXGBE_WRITE_REG(a, reg, value) ixgbe_write_reg((a), (reg), (value))



