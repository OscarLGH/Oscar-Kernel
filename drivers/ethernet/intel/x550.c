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
	return 0;
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

	IXGBE_WRITE_REG(ixgbe, IXGBE_EIMS, 0xffffffff);
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

