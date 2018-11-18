#include <pci.h>
#include <kernel.h>
#include <mm.h>

struct pci_host_bridge *host_bridge;

struct list_head pci_device_list;

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


void pci_scan()
{
	int b,d,f;
	int i;
	u16 vendor_id = 0, device_id = 0;
	u64 bar_size = 0, bar_base = 0;
	struct pci_dev pdev_test;
	struct pci_dev *pdev;
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
					printk("%02d:%02d.%d vendor:%04x, device:%04x\n", b, d, f, vendor_id, device_id);

					for (i = 0; i < 6; i++) {
						bar_base = pci_get_bar_base(&pdev_test, i);
						bar_size = pci_get_bar_size(&pdev_test, i);
						if (bar_base != 0 && bar_size !=0) {
							printk("BAR%d: Base:0x%x, Size:0x%x\n", i, bar_base, bar_size);
						}
					}
				}
			}
}

int pci_scan_alloc_resource()
{
	
}
