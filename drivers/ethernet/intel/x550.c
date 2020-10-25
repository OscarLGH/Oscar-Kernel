#include "x550.h"

s32 ixgbe_get_mac_addr_generic(struct ixgbe_hw *hw, u8 *mac_addr)
{
	u32 rar_high;
	u32 rar_low;
	u16 i;

	rar_high = IXGBE_READ_REG(hw, IXGBE_RAH(0));
	rar_low = IXGBE_READ_REG(hw, IXGBE_RAL(0));

	for (i = 0; i < 4; i++)
		mac_addr[i] = (u8)(rar_low >> (i*8));

	for (i = 0; i < 2; i++)
		mac_addr[i+4] = (u8)(rar_high >> (i*8));

	return 0;
}

void ixgbe_reset(struct ixgbe_hw *hw)
{
	
}

int ixgbe_intr(int irq, void *data)
{
	struct ixgbe_hw *hw = data;
	u32 intr_cause;
	u32 link_status;
	int tx_head, tx_tail, rx_head, rx_tail;
	intr_cause = IXGBE_READ_REG(hw, IXGBE_EICR);
	printk("X550 INTR:%d, cause:%x\n", irq, intr_cause);

	char *speed [] = {
				"reserved",
				"100 Mb/s",
				"1 GbE",
				"10 GbE",
				NULL
	};

	if (intr_cause & IXGBE_EICR_LSC) {
		link_status = IXGBE_READ_REG(hw, IXGBE_LINKS);
		printk("link status: %x\n", link_status);
		if (link_status & BIT30) {
			printk("LINK UP. Speed:%s\n", speed[((link_status >> 28) & 0x3)]);
		} else {
			printk("LINK DOWN\n");
		}
	}

	if (intr_cause & 0xffff) {
		tx_head = 0*IXGBE_READ_REG(hw, IXGBE_TDH(0));
		rx_tail = 0*IXGBE_READ_REG(hw, IXGBE_RDT(0));
		printk("RX DESC:%016x %016x\n",
			hw->rx_desc_ring[0][rx_tail].read.pkt_addr,
			hw->rx_desc_ring[0][rx_tail].read.hdr_addr
		);
		printk("TX DESC:%016x %08x %08x\n",
			hw->tx_desc_ring[0][tx_head].read.buffer_addr,
			hw->tx_desc_ring[0][tx_head].read.cmd_type_len,
			hw->tx_desc_ring[0][tx_head].read.olinfo_status
			);
	}
	return 0;
}

#define RING_SIZE 0x1000
int tx_init(struct ixgbe_hw *hw)
{
	int i;
	for (i = 0; i < 128; i++) {
		hw->tx_desc_ring[i] = kmalloc(RING_SIZE, GFP_KERNEL);
		memset(hw->tx_desc_ring[i], 0, RING_SIZE);
		IXGBE_WRITE_REG(hw, IXGBE_TDBAL(i), VIRT2PHYS(hw->tx_desc_ring[i]));
		IXGBE_WRITE_REG(hw, IXGBE_TDBAH(i), VIRT2PHYS(hw->tx_desc_ring[i]) >> 32);
		IXGBE_WRITE_REG(hw, IXGBE_TDLEN(i), RING_SIZE);
		IXGBE_WRITE_REG(hw, IXGBE_TDH(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_TDT(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(i), BIT25);
	}
}

int rx_init(struct ixgbe_hw *hw)
{
	int i;
	for (i = 0; i < 128; i++) {
		hw->rx_desc_ring[i] = kmalloc(0x1000, GFP_KERNEL);
		memset(hw->rx_desc_ring[i], 0, RING_SIZE);
		IXGBE_WRITE_REG(hw, IXGBE_RDBAL(i), VIRT2PHYS(hw->rx_desc_ring [i]));
		IXGBE_WRITE_REG(hw, IXGBE_RDBAH(i), VIRT2PHYS(hw->rx_desc_ring [i]) >> 32);
		IXGBE_WRITE_REG(hw, IXGBE_RDLEN(i), RING_SIZE);
		IXGBE_WRITE_REG(hw, IXGBE_RDH(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_RDT(i), 0);
	}
	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, 0x1);
}

void test_transmit(struct ixgbe_hw *hw, void *buffer)
{
	union ixgbe_adv_tx_desc *tx_desc = hw->tx_desc_ring[0];
	tx_desc[0].read.buffer_addr = VIRT2PHYS(buffer);
	tx_desc[0].read.cmd_type_len = (0xf << 24) | 0 | 0x200;
	IXGBE_WRITE_REG(hw, IXGBE_TDT(0), 1);
}

int intel_x550_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u32 *mmio_virt;
	int i = 0;
	int ret = 0;
	u8 mac_addr[6] = {0};
	struct pci_irq_desc *irq_desc;
	printk("INTEL X550 Ethernet card found.\n");
	struct ixgbe_hw *ixgbe = kmalloc(sizeof(*ixgbe), GFP_KERNEL);
	memset(ixgbe, 0, sizeof(*ixgbe));
	ixgbe->pdev = pdev;
	ixgbe->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	pci_enable_device(pdev);
	pci_set_master(pdev);

	ret = pci_enable_msix(pdev, 0, 2);
	if (ret < 0) {
		ret = pci_enable_msi(pdev);
	}
	list_for_each_entry(irq_desc, &pdev->irq_list, list) {
		request_irq_smp(get_cpu(), irq_desc->vector, ixgbe_intr, 0, "ixgbe", ixgbe);
	}
	volatile u32 *led = ixgbe->mmio_virt + 0x200 / 4;
	*led |= BIT5 | BIT7 | BIT15 | BIT23 | BIT31;
	ixgbe_get_mac_addr_generic(ixgbe, mac_addr);
	printk("MAC address:");

	for (i = 0; i < 6; i++) {
		printk("%02x ", mac_addr[i]);
	}
	printk("\n");

	rx_init(ixgbe);
	tx_init(ixgbe);

	for (i = 0; i < 64; i++) {
		IXGBE_WRITE_REG(ixgbe, IXGBE_IVAR(i), 0x80808080);
	}
	IXGBE_WRITE_REG(ixgbe, IXGBE_EIMS, 0xffffffff);

	void *test_buffer = kmalloc(0x1000, GFP_KERNEL);
	memset(test_buffer, 0xff, 0x1000);
	test_transmit(ixgbe, test_buffer);
	return 0;
}

void intel_x550_remove(struct pci_dev *pdev)
{
	
}

struct pci_device_id intel_x550_ids[64] = {
	{0x15638086, 0x020000, 0xffffffff, 0x0},
};

struct pci_driver intel_x550_driver = {
	.name = "intel x550 ethernet driver",
	.id_array = intel_x550_ids,
	.pci_probe = intel_x550_probe,
	.pci_remove = intel_x550_remove
};

void intel_X550_init()
{
	pci_register_driver(&intel_x550_driver);
}

module_init(intel_X550_init);

