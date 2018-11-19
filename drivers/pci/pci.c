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

u64 pci_get_bar_base(struct pci_dev *pdev, int bar)
{
	u32 bar_lo32 = 0;
	u32 bar_hi32 = 0;

	pci_read_config_dword(pdev, PCI_CONF_BAR0 + bar * 4, &bar_lo32);
	if (bar_lo32 & BIT2) {
		pci_read_config_dword(pdev, PCI_CONF_BAR0 + bar * 4 + 4, &bar_hi32);
	}
	
	return (bar_lo32 & (~0xf)) | ((u64)bar_hi32 << 32);
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

	pci_read_config_word(pdev, PCI_CONF_COMMAND, &ori_cmd_reg);
	new_cmd_reg = ori_cmd_reg & (~0x3);
	pci_write_config_word(pdev, PCI_CONF_COMMAND, &new_cmd_reg);

	pci_read_config_dword(pdev, PCI_CONF_BAR0 + bar * 4, &ori_bar_lo32);
	pci_write_config_dword(pdev, PCI_CONF_BAR0 + bar * 4, &all_ones);
	pci_read_config_dword(pdev, PCI_CONF_BAR0 + bar * 4, &size_lo32);
	pci_write_config_dword(pdev, PCI_CONF_BAR0 + bar * 4, &ori_bar_lo32);

	if (ori_bar_lo32 & BIT2) {
		pci_read_config_dword(pdev, PCI_CONF_BAR0 + (bar + 1) * 4, &ori_bar_hi32);
		pci_write_config_dword(pdev, PCI_CONF_BAR0 + (bar + 1) * 4, &all_ones);
		pci_read_config_dword(pdev, PCI_CONF_BAR0 + (bar + 1) * 4, &size_hi32);
		pci_write_config_dword(pdev, PCI_CONF_BAR0 + (bar + 1) * 4, &ori_bar_hi32);
	}

	pci_write_config_word(pdev, PCI_CONF_COMMAND, &ori_cmd_reg);
	size = size_lo32 | ((u64)size_hi32 << 32);
	size &= ~(0xfULL);
	size = ~size + 1;

	return size;
}

u64 pci_get_oprombar_base(struct pci_dev *pdev)
{
	u32 bar = 0;
	pci_read_config_dword(pdev, PCI_CONF_EXPANSIONROMBAR, &bar);	
	return (bar & (~0xf));
}

u64 pci_get_oprombar_size(struct pci_dev *pdev)
{
	u32 ori_cmd_reg = 0;
	u32 new_cmd_reg = 0;
	u32 ori_bar = 0;
	u32 size = 0;
	u32 all_ones = 0xffffffff;

	pci_read_config_word(pdev, PCI_CONF_COMMAND, &ori_cmd_reg);
	new_cmd_reg = ori_cmd_reg & (~0x3);
	pci_write_config_word(pdev, PCI_CONF_COMMAND, &new_cmd_reg);

	pci_read_config_dword(pdev, PCI_CONF_EXPANSIONROMBAR, &ori_bar);
	pci_write_config_dword(pdev, PCI_CONF_EXPANSIONROMBAR, &all_ones);
	pci_read_config_dword(pdev, PCI_CONF_EXPANSIONROMBAR, &size);
	pci_write_config_dword(pdev, PCI_CONF_EXPANSIONROMBAR, &ori_bar);

	pci_write_config_word(pdev, PCI_CONF_COMMAND, &ori_cmd_reg);
	size &= ~(0xfULL);
	size = ~size + 1;

	return size;
}


void pci_device_init(struct pci_dev *pdev)
{
	int i;
	u32 vid = 0;
	u32 did = 0;
	INIT_LIST_HEAD(&pdev->list);
	pci_read_config_word(pdev, PCI_CONF_VENDORID, &vid);
	pci_read_config_word(pdev, PCI_CONF_DEVICEID, &did);
	pdev->viddid = (vid << 16) | (did);
	for (i = 0; i < 6; i++) {
		pdev->resource[i].start = pci_get_bar_base(pdev, i);
		pdev->resource[i].end = pdev->resource[i].start + pci_get_bar_size(pdev, i) - 1;
	}
	pdev->resource[i].start = pci_get_oprombar_base(pdev);
	pdev->resource[i].end = pci_get_oprombar_size(pdev);
}

void register_pci_device(struct pci_dev *pdev)
{
	list_add_tail(&pdev->list, &pci_device_list);
}

void pci_device_print()
{
	struct pci_dev *pdev;
	int i;
	list_for_each_entry(pdev, &pci_device_list, list) {
		printk("%02d:%02d.%d %08x\n", pdev->bus, pdev->device, pdev->fun, pdev->viddid);
		for (i = 0; i < 6; i++) {
			if (pdev->resource[i].start != 0) {
				printk("BAR %d:0x%x-0x%x\n", i, pdev->resource[i].start, pdev->resource[i].end);
			}
		}
		if (pdev->resource[i].start != 0) {
			printk("OPROM BAR:0x%x-0x%x\n", pdev->resource[i].start, pdev->resource[i].end);
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
				pci_read_config_word(&pdev_test, PCI_CONF_VENDORID, &vendor_id);
				pci_read_config_word(&pdev_test, PCI_CONF_DEVICEID, &device_id);
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
