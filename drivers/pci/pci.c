#include <pci.h>
#include <kernel.h>
#include <mm.h>

struct pci_host_bridge *host_bridge;

struct list_head pci_device_list;
struct list_head pci_driver_list;

int pci_register_driver(struct pci_driver *driver)
{
	struct pci_dev *pdev;
	int i;
	int ret = -1;
	list_add_tail(&driver->list, &pci_driver_list);
	list_for_each_entry(pdev, &pci_device_list, list) {
		for (i = 0; i < 64; i++) {
			if (pdev->viddid == driver->id_array[i].viddid) {
				ret = driver->pci_probe(pdev, NULL);
			}
		}
	}
	return ret;
}

void pci_unregister_driver(struct pci_driver *driver)
{
	struct pci_dev *pdev;
	int i;
	list_for_each_entry(pdev, &pci_device_list, list) {
		for (i = 0; i < 64; i++) {
			if (pdev->viddid == driver->id_array[i].viddid) {
				driver->pci_remove(pdev);
			}
		}
	}
	list_del(&driver->list);
}

int pci_read_config_byte(struct pci_dev *pdev, int offset, void *value)
{
	return host_bridge->pci_read_config(pdev, offset, value, 1);
}

int pci_read_config_word(struct pci_dev *pdev, int offset, void *value)
{
	return host_bridge->pci_read_config(pdev, offset, value, 2);
}

int pci_read_config_dword(struct pci_dev *pdev, int offset, void *value)
{
	return host_bridge->pci_read_config(pdev, offset, value, 4);
}

int pci_write_config_byte(struct pci_dev *pdev, int offset, void *value)
{
	return host_bridge->pci_write_config(pdev, offset, value, 1);
}

int pci_write_config_word(struct pci_dev *pdev, int offset, void *value)
{
	return host_bridge->pci_write_config(pdev, offset, value, 2);
}

int pci_write_config_dword(struct pci_dev *pdev, int offset, void *value)
{
	return host_bridge->pci_write_config(pdev, offset, value, 4);
}

int pci_enable_device(struct pci_dev *pdev)
{
	u32 cmd_reg = 0;

	pci_read_config_word(pdev, PCI_COMMAND, &cmd_reg);
	cmd_reg |= 0x3;
	pci_write_config_word(pdev, PCI_COMMAND, &cmd_reg);

	return 0;
}

int pci_set_master(struct pci_dev *pdev)
{
	u32 cmd_reg = 0;

	pci_read_config_word(pdev, PCI_COMMAND, &cmd_reg);
	cmd_reg |= 0x4;
	pci_write_config_word(pdev, PCI_COMMAND, &cmd_reg);

	return 0;
}

//TODO:
int pci_enable_msi(struct pci_dev *pdev)
{
	
}

u16 pci_get_bar_type(struct pci_dev *pdev, int bar)
{
	u32 bar_val = 0;
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &bar_val);
	if (bar_val & BIT1) {
		return 0xffff;
	}
	return bar_val & 0xf;
}

u64 pci_get_bar_base(struct pci_dev *pdev, int bar)
{
	u32 bar_lo32 = 0;
	u32 bar_hi32 = 0;

	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &bar_lo32);

	if (bar_lo32 & BIT1) {
		return 0;
	}

	if (bar_lo32 & BIT2) {
		pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4 + 4, &bar_hi32);
	}

	if (bar_lo32 & BIT0)
		bar_lo32 &= (~0x3);
	else
		bar_lo32 &= (~0xf);
	return bar_lo32 | ((u64)bar_hi32 << 32);
}

u64 pci_get_bar_size(struct pci_dev *pdev, int bar)
{
	u32 ori_cmd_reg = 0;
	u32 new_cmd_reg = 0;
	u32 ori_bar_lo32 = 0;
	u32 ori_bar_hi32 = 0;
	u32 size_lo32 = 0;
	u32 size_hi32 = 0xffffffff;
	u64 size = 0;
	u32 all_ones = 0xffffffff;
	u64 valid = 1;

	pci_read_config_word(pdev, PCI_COMMAND, &ori_cmd_reg);
	new_cmd_reg = ori_cmd_reg & (~0x3);
	pci_write_config_word(pdev, PCI_COMMAND, &new_cmd_reg);

	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &ori_bar_lo32);
	pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &all_ones);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &size_lo32);
	pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &ori_bar_lo32);

	if (ori_bar_lo32 & BIT1) {
		valid = 0;
	}

	if (ori_bar_lo32 & BIT2) {
		pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, &ori_bar_hi32);
		pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, &all_ones);
		pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, &size_hi32);
		pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, &ori_bar_hi32);
	}

	pci_write_config_word(pdev, PCI_COMMAND, &ori_cmd_reg);

	if (ori_bar_lo32 & BIT0)
		size_lo32 &= (~0x3);
	else
		size_lo32 &= (~0xf);

	/* IO type bar ignores upper 16 bits. */
	if (ori_bar_lo32 & BIT0)
		size_lo32 |= 0xffff0000;

	size = size_lo32 | ((u64)size_hi32 << 32);
	size &= ~(0xfULL);
	size = ~size + 1;

	if (valid == 0)
		size = 0;

	return size;
}

u64 pci_get_oprombar_base(struct pci_dev *pdev)
{
	u32 bar = 0;
	if (pdev->type & 0x1 == 0) {
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS, &bar);
	} else {
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS1, &bar);
	}
	return (bar & (~0xf));
}

u64 pci_get_oprombar_size(struct pci_dev *pdev)
{
	u32 ori_cmd_reg = 0;
	u32 new_cmd_reg = 0;
	u32 ori_bar = 0;
	u32 size = 0;
	u32 all_ones = 0xfffff800;

	pci_read_config_word(pdev, PCI_COMMAND, &ori_cmd_reg);
	new_cmd_reg = ori_cmd_reg & (~0x3);
	pci_write_config_word(pdev, PCI_COMMAND, &new_cmd_reg);

	if ((pdev->type & 0x1) == 0) {
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS, &ori_bar);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS, &all_ones);
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS, &size);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS, &ori_bar);
	} else {
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS1, &ori_bar);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS1, &all_ones);
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS1, &size);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS1, &ori_bar);
	}

	pci_write_config_word(pdev, PCI_COMMAND, &ori_cmd_reg);
	size &= ~(0x8ffULL);
	size = ~size + 1;

	return size;
}


void pci_device_init(struct pci_dev *pdev)
{
	int i;
	u32 vid = 0;
	u32 did = 0;
	u8 type = 0;
	u8 class_code[3];
	INIT_LIST_HEAD(&pdev->list);
	pci_read_config_byte(pdev, PCI_HEADER_TYPE, &type);
	pci_read_config_word(pdev, PCI_VENDOR_ID, &vid);
	pci_read_config_word(pdev, PCI_DEVICE_ID, &did);
	pci_read_config_byte(pdev, PCI_CLASS_DEVICE, &class_code[0]);
	pci_read_config_byte(pdev, PCI_CLASS_PROG, &class_code[1]);
	pci_read_config_byte(pdev, PCI_CLASS_REVISION, &class_code[2]);
	pdev->class = (class_code[0] << 16) | (class_code[1] << 8) | (class_code[2]);
	pdev->viddid = (vid << 16) | (did);
	pdev->type = type;
	if ((pdev->type & 0x1) == 0) {
		for (i = 0; i < 6; i++) {
			pdev->resource[i].start = pci_get_bar_base(pdev, i);
			pdev->resource[i].end = pdev->resource[i].start + pci_get_bar_size(pdev, i) - 1;
			pdev->resource[i].flags = pci_get_bar_type(pdev, i);
		}
		pdev->resource[i].start = pci_get_oprombar_base(pdev);
		pdev->resource[i].end = pci_get_oprombar_size(pdev);
	} else {
		pdev->resource[0].start = pci_get_bar_base(pdev, 0);
		pdev->resource[0].end = pdev->resource[0].start + pci_get_bar_size(pdev, 0) - 1;
		pdev->resource[0].flags = pci_get_bar_type(pdev, 0);
		pdev->resource[1].start = pci_get_bar_base(pdev, 1);
		pdev->resource[1].end = pdev->resource[1].start + pci_get_bar_size(pdev, 1) - 1;
		pdev->resource[1].flags = pci_get_bar_type(pdev, 1);
		pdev->resource[2].start = pci_get_oprombar_base(pdev);
		pdev->resource[2].end = pci_get_oprombar_size(pdev);
	}
}

void register_pci_device(struct pci_dev *pdev)
{
	list_add_tail(&pdev->list, &pci_device_list);
}

void pci_device_print()
{
	struct pci_dev *pdev;
	u8 primary_bus, secondary_bus, subordinate_bus;
	u64 memory_base, memory_limit;
	u64 prefetchable_memory_base, prefetchable_memory_limit;
	u64 io_base, io_limit;
	u8 tmp0, tmp1;
	u16 tmp2, tmp3;
	u32 tmp4, tmp5;

	int i;
	list_for_each_entry(pdev, &pci_device_list, list) {
		printk("%02d:%02d.%d %08x type:%d class:%08x\n", 
			pdev->bus, 
			pdev->device, 
			pdev->fun, 
			pdev->viddid, 
			pdev->type,
			pdev->class
			);
		if ((pdev->type & 0x1) == 0) {
			for (i = 0; i < 6; i++) {
				if (pdev->resource[i].start != 0) {
					printk("    BAR %d [%s]:0x%x-0x%x\n", i, pdev->resource[i].flags == 0x1 ? "I/O" : "MEM",
						pdev->resource[i].start, pdev->resource[i].end);
				}
			}
			if (pdev->resource[i].start != 0) {
				printk("    OPROM BAR:0x%x-0x%x\n", pdev->resource[i].start, pdev->resource[i].end);
			}
		} else {
			for (i = 0; i < 2; i++) {
				if (pdev->resource[i].start != 0) {
					printk("    BAR %d [%s]:0x%x-0x%x\n", i, pdev->resource[i].flags == 0x1 ? "I/O" : "MEM",
						pdev->resource[i].start, pdev->resource[i].end);
				}
			}

			if (pdev->resource[i].start != 0) {
				printk("    OPROM BAR:0x%x-0x%x\n", pdev->resource[i].start, pdev->resource[i].end);
			}
			pci_read_config_byte(pdev, PCI_PRIMARY_BUS, &primary_bus);
			pci_read_config_byte(pdev, PCI_SECONDARY_BUS, &secondary_bus);
			pci_read_config_byte(pdev, PCI_SUBORDINATE_BUS, &subordinate_bus);
			printk("    Primary bus:%d Secondary bus:%d Subordinate bus:%d\n",
				primary_bus,
				secondary_bus,
				subordinate_bus
			);
			pci_read_config_byte(pdev, PCI_IO_BASE, &tmp0);
			pci_read_config_word(pdev, PCI_IO_BASE_UPPER16, &tmp2);
			io_base = ((tmp0 >> 4) << 12) + (tmp2 << 16);
			pci_read_config_byte(pdev, PCI_IO_LIMIT, &tmp0);
			pci_read_config_word(pdev, PCI_IO_LIMIT_UPPER16, &tmp2);
			io_limit = ((tmp0 >> 4) << 12) + (tmp2 << 16) + 0xfff;

			pci_read_config_word(pdev, PCI_MEMORY_BASE, &tmp2);
			pci_read_config_word(pdev, PCI_MEMORY_LIMIT, &tmp3);
			memory_base = (((u64)tmp2 >> 4) << 20);
			memory_limit = (((u64)tmp3 >> 4) << 20) + 0xfffff;

			pci_read_config_word(pdev, PCI_PREF_MEMORY_BASE, &tmp2);
			pci_read_config_dword(pdev, PCI_PREF_BASE_UPPER32, &tmp4);
			prefetchable_memory_base = (((u64)tmp2 >> 4) << 20) + ((u64)tmp4 << 32);
			pci_read_config_word(pdev, PCI_PREF_MEMORY_LIMIT, &tmp2);
			pci_read_config_dword(pdev, PCI_PREF_LIMIT_UPPER32, &tmp4);
			prefetchable_memory_limit = (((u64)tmp2 >> 4) << 20) + ((u64)tmp4 << 32) + 0xfffff;
			printk("    io base:0x%x, limit:0x%x\n", io_base, io_limit);
			printk("    memory base:0x%x, limit:0x%x\n", memory_base, memory_limit);
			printk("    prefetchable memory base:0x%x, limit:0x%x\n", prefetchable_memory_base, prefetchable_memory_limit);
			
		}
	}	
}

void pci_scan()
{
	int b,d,f;
	int i;
	u16 vendor_id = 0, device_id = 0;
	u64 bar_size = 0, bar_base = 0;
	struct pci_dev pdev_test;
	struct pci_dev *pdev;
	INIT_LIST_HEAD(&pci_device_list);
	INIT_LIST_HEAD(&pci_driver_list);

	for (b = 0; b < host_bridge->end_bus_number; b++)
		for (d = 0; d < 32; d++)
			for (f = 0; f < 8; f++) {
				pdev_test.bus = b;
				pdev_test.device = d;
				pdev_test.fun = f;
				pci_read_config_word(&pdev_test, PCI_VENDOR_ID, &vendor_id);
				pci_read_config_word(&pdev_test, PCI_DEVICE_ID, &device_id);
				if (vendor_id != 0xffff && device_id != 0xffff) {
					pdev = bootmem_alloc(sizeof(*pdev));
					pdev->bus = b;
					pdev->device = d;
					pdev->fun = f;
					//printk("%02d:%02d.%d vendor:%04x, device:%04x\n", b, d, f, vendor_id, device_id);
					pci_device_init(pdev);
					list_add_tail(&pdev->list, &pci_device_list);
				}
			}
	pci_device_print();
}

int pci_scan_alloc_resource()
{
	
}
