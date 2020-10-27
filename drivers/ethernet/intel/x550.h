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

struct ixgbe_tx_queue {
	union ixgbe_adv_tx_desc *tx_desc_ring;
	u64 size;
	u64 tail;
	u64 head;
};

struct ixgbe_rx_queue {
	union ixgbe_adv_rx_desc *rx_desc_ring;
	u64 size;
	u64 tail;
	u64 head;
};


struct ixgbe_hw {
	struct pci_dev *pdev;
	volatile u32 *mmio_virt;
	struct ixgbe_tx_queue *tx_desc_ring[128];
	struct ixgbe_rx_queue *rx_desc_ring[128];
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

#define IXGBE_WRITE_FLUSH(a) ixgbe_read_reg((a), IXGBE_STATUS)
