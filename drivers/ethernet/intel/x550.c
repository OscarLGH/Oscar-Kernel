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
	u32 ctrl, i;
	ctrl = IXGBE_CTRL_RST;
	ctrl |= IXGBE_READ_REG(hw, IXGBE_CTRL);
	IXGBE_WRITE_REG(hw, IXGBE_CTRL, ctrl);
	IXGBE_WRITE_FLUSH(hw);

	/* Poll for reset bit to self-clear indicating reset is complete */
	for (i = 0; i < 10; i++) {
		ctrl = IXGBE_READ_REG(hw, IXGBE_CTRL);
		if (!(ctrl & IXGBE_CTRL_RST_MASK))
			break;
		udelay(1);
	}

	for (i = 0; i < 4; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_FCTTV(i), 0);
	}
	for (i = 0; i < 8; i++) {
		IXGBE_WRITE_REG(hw, IXGBE_FCRTL(i), 0);
		IXGBE_WRITE_REG(hw, IXGBE_FCRTH(i), 0);
	}
	IXGBE_WRITE_REG(hw, IXGBE_FCRTV, 0);
	IXGBE_WRITE_REG(hw, IXGBE_FCCFG, 0);

	for (;;) {
		ctrl = IXGBE_READ_REG(hw, IXGBE_EEC_X550);
		if ((ctrl & IXGBE_EEC_ARD))
			break;
		udelay(1);
	}

	for (;;) {
		ctrl = IXGBE_READ_REG(hw, IXGBE_EEMNGCTL);
		if (1 || ((ctrl & BIT18 & BIT19) == (BIT18 | BIT19)))
			break;
		udelay(1);
	}

	for (;;) {
		ctrl = IXGBE_READ_REG(hw, IXGBE_RDRXCTL);
		if (ctrl & IXGBE_RDRXCTL_DMAIDONE)
			break;
		udelay(1);
	}

	IXGBE_WRITE_REG(hw, IXGBE_FCBUFF, 0);
	IXGBE_WRITE_REG(hw, IXGBE_FCFLT, 0);

	IXGBE_WRITE_REG(hw, IXGBE_IPSTXIDX, 0);
}

int ixgbe_intr(int irq, void *data)
{
	struct ixgbe_hw *hw = data;
	u32 intr_cause;
	u32 link_status;
	int index;
	int i;
	char mac_string[18];

	intr_cause = IXGBE_READ_REG(hw, IXGBE_EICR);
	printk("X550 INTR:%d, cause:%x\n", irq, intr_cause);

	char *speed [] = {
				"reserved",
				"100 Mb/s",
				"1 GbE",
				"10 GbE",
				NULL
	};

	sprintf(mac_string,
		"%02x:%02x:%02x:%02x:%02x:%02x", 
		hw->mac_addr[0],
		hw->mac_addr[1],
		hw->mac_addr[2],
		hw->mac_addr[3],
		hw->mac_addr[4],
		hw->mac_addr[5]);

	if (intr_cause & IXGBE_EICR_LSC) {
		link_status = IXGBE_READ_REG(hw, IXGBE_LINKS);
		//printk("link status: %x\n", link_status);
		if (link_status & BIT30) {
			printk("[%s] LINK UP. Speed:%s\n", mac_string, speed[((link_status >> 28) & 0x3)]);
		} else {
			printk("[%s] LINK DOWN\n", mac_string);
		}
	}

	if (intr_cause & 0xffff) {

		if ((intr_cause & 0xffff) > 2) {
			printk("X550 INTR:%d, cause:%x\n", irq, intr_cause);
		}
		
		hw->statistic.tx_packets += IXGBE_READ_REG(hw, IXGBE_TXDGPC);
		hw->statistic.tx_bytes += 
			(IXGBE_READ_REG(hw, IXGBE_TXDGBCL) + ((u64)IXGBE_READ_REG(hw, IXGBE_TXDGBCH) << 32));
		hw->statistic.rx_packets += IXGBE_READ_REG(hw, IXGBE_RXDGPC);
		hw->statistic.rx_bytes += 
			(IXGBE_READ_REG(hw, IXGBE_RXDGBCL) + ((u64)IXGBE_READ_REG(hw, IXGBE_RXDGBCH) << 32));
		hw->tx_desc_ring[0]->head = IXGBE_READ_REG(hw, IXGBE_TDH(0));
		hw->rx_desc_ring[0]->head = IXGBE_READ_REG(hw, IXGBE_RDH(0));
		hw->tx_desc_ring[0]->tail = IXGBE_READ_REG(hw, IXGBE_TDT(0));
		hw->rx_desc_ring[0]->tail = IXGBE_READ_REG(hw, IXGBE_RDT(0));
		if (hw->rx_desc_ring[0]->head == IXGBE_READ_REG(hw, IXGBE_RDT(0))) {
			//printk("rx ring empty.\n");
			hw->rx_desc_ring[0]->tail = hw->rx_desc_ring[0]->head + hw->rx_desc_ring[0]->size / 16;
			IXGBE_WRITE_REG(hw, IXGBE_RDT(0), hw->rx_desc_ring[0]->tail);
		}

		if ((hw->statistic.rx_packets != 0 && hw->statistic.rx_packets % 10000 == 0) ||
			(hw->statistic.tx_packets != 0 && hw->statistic.tx_packets % 10000 == 0)
			) {
			printk("[%s] RX packets:%d (%d bytes) TX packets:%d (%d bytes)\n",
				mac_string,
				hw->statistic.rx_packets,
				hw->statistic.rx_bytes,
				hw->statistic.tx_packets,
				hw->statistic.tx_bytes
			);
		}
		//printk("TX HEAD:%d, RX HEAD:%d\n", hw->tx_desc_ring[0]->head, hw->rx_desc_ring[0]->head);

/*
		if (hw->rx_desc_ring[0]->head) {
			printk("RX DESC:%016x %016x\n",
	 			hw->rx_desc_ring[0]->rx_desc_ring[hw->rx_desc_ring[0]->head - 1].read.pkt_addr,
	 			hw->rx_desc_ring[0]->rx_desc_ring[hw->rx_desc_ring[0]->head - 1].read.hdr_addr
			);
			hex_dump(
				(void *)PHYS2VIRT(hw->rx_desc_ring[0]->rx_desc_ring[hw->rx_desc_ring[0]->head - 1].read.pkt_addr), 
				hw->rx_desc_ring[0]->rx_desc_ring[hw->rx_desc_ring[0]->head - 1].wb.upper.length
				);
		}
		if (hw->tx_desc_ring[0]->tail) {
			printk("TX DESC:%016x %08x %08x\n",
				hw->tx_desc_ring[0]->tx_desc_ring[hw->tx_desc_ring[0]->tail - 1].read.buffer_addr,
				hw->tx_desc_ring[0]->tx_desc_ring[hw->tx_desc_ring[0]->tail - 1].read.cmd_type_len,
				hw->tx_desc_ring[0]->tx_desc_ring[hw->tx_desc_ring[0]->tail - 1].read.olinfo_status
				);
		}
*/
	}
	return 0;
}

#define RING_SIZE 0x1000
int tx_init(struct ixgbe_hw *hw)
{
	int i;
	u32 ctrl;

	ctrl = IXGBE_READ_REG(hw, IXGBE_HLREG0);
	IXGBE_WRITE_REG(hw, IXGBE_HLREG0, ctrl);
	for (i = 0; i < 128; i++) {
		hw->tx_desc_ring[i] = kmalloc(sizeof(*hw->tx_desc_ring[i]), GFP_KERNEL);
		hw->tx_desc_ring[i]->size = RING_SIZE;
		hw->tx_desc_ring[i]->head = 0;
		hw->tx_desc_ring[i]->tail = 0;
		hw->tx_desc_ring[i]->tx_desc_ring = kmalloc(hw->tx_desc_ring[i]->size, GFP_KERNEL);
		memset(hw->tx_desc_ring[i]->tx_desc_ring, 0, hw->tx_desc_ring[i]->size);
		IXGBE_WRITE_REG(hw, IXGBE_TDBAL(i), VIRT2PHYS(hw->tx_desc_ring[i]->tx_desc_ring));
		IXGBE_WRITE_REG(hw, IXGBE_TDBAH(i), VIRT2PHYS(hw->tx_desc_ring[i]->tx_desc_ring) >> 32);
		IXGBE_WRITE_REG(hw, IXGBE_TDLEN(i), hw->tx_desc_ring[i]->size);
		IXGBE_WRITE_REG(hw, IXGBE_TDH(i), hw->tx_desc_ring[i]->head);
		IXGBE_WRITE_REG(hw, IXGBE_TXDCTL(i), IXGBE_TXDCTL_ENABLE);
		IXGBE_WRITE_REG(hw, IXGBE_TDT(i), hw->tx_desc_ring[i]->tail);
	}

	ctrl = IXGBE_READ_REG(hw, IXGBE_DMATXCTL);
	IXGBE_WRITE_REG(hw, IXGBE_DMATXCTL, ctrl | IXGBE_DMATXCTL_TE);
}

int rx_init(struct ixgbe_hw *hw)
{
	int i, j;
	u32 ctrl;
	void *buffer;
	IXGBE_WRITE_REG(hw, IXGBE_CTRL_EXT, 0);

	IXGBE_WRITE_REG(hw, IXGBE_FCTRL, IXGBE_FCTRL_SBP | IXGBE_FCTRL_MPE | IXGBE_FCTRL_UPE | IXGBE_FCTRL_BAM);

	ctrl = IXGBE_READ_REG(hw, IXGBE_VLNCTRL);
	IXGBE_WRITE_REG(hw, IXGBE_VLNCTRL, ctrl ^ IXGBE_VLNCTRL_VFE);

	for (i = 0; i < 1; i++) {
		//IXGBE_WRITE_REG(hw, IXGBE_VFTA(i), 0);
		//IXGBE_WRITE_REG(hw, IXGBE_MPSAR_LO(i), 0);
		//IXGBE_WRITE_REG(hw, IXGBE_MPSAR_HI(i), 0);
		hw->rx_desc_ring[i] = kmalloc(sizeof(*hw->rx_desc_ring[i]), GFP_KERNEL);
		hw->rx_desc_ring[i]->size = RING_SIZE;
		hw->rx_desc_ring[i]->rx_desc_ring = kmalloc(hw->rx_desc_ring[i]->size, GFP_KERNEL);
		hw->rx_desc_ring[i]->head = 0;
		hw->rx_desc_ring[i]->tail = hw->rx_desc_ring[i]->size / sizeof(*hw->rx_desc_ring[i]->rx_desc_ring) - 1;
		memset(hw->rx_desc_ring[i]->rx_desc_ring, 0, hw->rx_desc_ring[i]->size);
		for (j = 0; j < hw->rx_desc_ring[i]->size / sizeof(*hw->rx_desc_ring[i]->rx_desc_ring); j++) {
			buffer = kmalloc(hw->rx_desc_ring[i]->size, GFP_KERNEL);
			hw->rx_desc_ring[i]->rx_desc_ring[j].read.pkt_addr = VIRT2PHYS(buffer);
			memset(buffer, 0, hw->rx_desc_ring[i]->size);
		}
		IXGBE_WRITE_REG(hw, IXGBE_RDBAL(i), VIRT2PHYS(hw->rx_desc_ring[i]->rx_desc_ring));
		IXGBE_WRITE_REG(hw, IXGBE_RDBAH(i), VIRT2PHYS(hw->rx_desc_ring[i]->rx_desc_ring) >> 32);
		IXGBE_WRITE_REG(hw, IXGBE_RDLEN(i), hw->rx_desc_ring[i]->size);
		IXGBE_WRITE_REG(hw, IXGBE_RDH(i), hw->rx_desc_ring[i]->head);
		IXGBE_WRITE_REG(hw, IXGBE_RXDCTL(i), IXGBE_RXDCTL_ENABLE);
		IXGBE_WRITE_REG(hw, IXGBE_RDT(i), hw->rx_desc_ring[i]->tail);
	}
	IXGBE_WRITE_REG(hw, IXGBE_RXCTRL, IXGBE_RXCTRL_RXEN);
}

void packet_transmit(struct ixgbe_hw *hw, void *buffer, int len)
{
	int i;
	int free_queue = -1;
	union ixgbe_adv_tx_desc *tx_desc;
/*
	while (1) {
		for (i = 0; i < 128; i++) {
			if (hw->tx_desc_ring[i]->tail == IXGBE_READ_REG(hw, IXGBE_TDH(i))) {
				free_queue = i;
				goto done;
			}
				
		}
	}
	*/
	free_queue = 63;
done:
	tx_desc = hw->tx_desc_ring[free_queue]->tx_desc_ring;
	tx_desc[hw->tx_desc_ring[free_queue]->tail].read.buffer_addr = VIRT2PHYS(buffer);
	tx_desc[hw->tx_desc_ring[free_queue]->tail].read.cmd_type_len = (0xb << 24) | (36 << 16) | len;
	tx_desc[hw->tx_desc_ring[free_queue]->tail].read.olinfo_status = (0 << 16);
	hw->tx_desc_ring[free_queue]->tail = (hw->tx_desc_ring[free_queue]->tail + 1) % (hw->tx_desc_ring[free_queue]->size / sizeof(*tx_desc));

	IXGBE_WRITE_REG(hw, IXGBE_TDT(free_queue), hw->tx_desc_ring[free_queue]->tail);
}

int intel_x550_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u32 *mmio_virt;
	int i = 0;
	int ret = 0;
	u8 mac_addr[6] = {0};
	u32 ivar;

	struct pci_irq_desc *irq_desc;
	printk("INTEL X550 Ethernet card found.\n");
	struct ixgbe_hw *ixgbe = kmalloc(sizeof(*ixgbe), GFP_KERNEL);
	memset(ixgbe, 0, sizeof(*ixgbe));
	ixgbe->pdev = pdev;
	ixgbe->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));
	pci_enable_device(pdev);

	ixgbe_reset(ixgbe);
	pci_set_master(pdev);

	ret = pci_enable_msix(pdev, 0, 32);
	if (ret < 0) {
		ret = pci_enable_msi(pdev);
	}
	list_for_each_entry(irq_desc, &pdev->irq_list, list) {
		request_irq_smp(get_cpu(), irq_desc->vector, ixgbe_intr, 0, "ixgbe", ixgbe);
	}

	ixgbe_get_mac_addr_generic(ixgbe, ixgbe->mac_addr);

	printk("MAC address:");
	for (i = 0; i < 5; i++) {
		printk("%02x:", ixgbe->mac_addr[i]);
	}
	printk("%02x\n", ixgbe->mac_addr[i]);

	/* clear counters */
	IXGBE_READ_REG(ixgbe, IXGBE_GPRC);
	IXGBE_READ_REG(ixgbe, IXGBE_GPTC);

	rx_init(ixgbe);
	tx_init(ixgbe);

	//IXGBE_WRITE_REG(ixgbe, IXGBE_EICS, 0x7fffffff);
	IXGBE_WRITE_REG(ixgbe, IXGBE_EIMS, 0x7fffffff);
	//IXGBE_WRITE_REG(ixgbe, IXGBE_EICS_EX(0), 0xffffffff);
	//IXGBE_WRITE_REG(ixgbe, IXGBE_EICS_EX(1), 0xffffffff);
	IXGBE_WRITE_REG(ixgbe, IXGBE_EIMS_EX(0), 0xffffffff);
	IXGBE_WRITE_REG(ixgbe, IXGBE_EIMS_EX(1), 0xffffffff);

	for (i = 0; i < 64; i++) {
		ivar = (i * 4) % 64;
		ivar = 0x80808080 | (ivar | ((ivar + 1) << 8) | ((ivar + 2) << 16) | ((ivar + 3) << 24));
		IXGBE_WRITE_REG(ixgbe, IXGBE_IVAR(i), ivar);
	}

	IXGBE_WRITE_REG(ixgbe, IXGBE_GPIE, IXGBE_GPIE_MSIX_MODE | IXGBE_GPIE_EIAME);

	u8 *test_buffer = kmalloc(0x1000, GFP_KERNEL);
	memset(test_buffer, 0xff, 0x1000);
	u8 arp_packet[] = {
		 0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x50,0x56,0xc0,0x00,0x01,0x08,0x06,0x00,0x01,
		 0x08,0x00,0x06,0x04,0x00,0x01,0x00,0x50,0x56,0xc0,0x00,0x01,0xc0,0xa8,0x00,0x01,
		 0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xa8,0x00,0x02
	};

	struct mac_frame {
		u8 dest_mac[6];
		u8 src_mac[6];
		u16 type;
	} mac_frame = {
		{0xff,0xff,0xff,0xff,0xff,0xff},
		{0xa0,0x36,0x9f,0xb9,0x8a,0x3a},
		0x0008
	};
	memcpy(&mac_frame.src_mac, ixgbe->mac_addr, 6);

	memcpy(test_buffer, &mac_frame, sizeof(mac_frame));
	memcpy(test_buffer + sizeof(mac_frame), arp_packet, sizeof(arp_packet));

	for (;;) {
		packet_transmit(ixgbe, test_buffer, sizeof(mac_frame) + sizeof(arp_packet) + 1000);
	}

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

