#include <pci.h>
#include <acpi.h>
#include <kernel.h>
#include <in_out.h>
#include <lapic.h>
#include <irq.h>
#include <init.h>

struct pci_host_bridge x86_host_bridge;
extern struct pci_host_bridge *host_bridge;
extern void pci_scan(void);

#define X86_PCI_PIO_ADDR(b,d,f,o) \
	(0x80000000 | ((b) << 16) | ((d) << 11) | ((f) << 8) | ((o) & ~(0x3)))

int x86_pci_read_config_pio(struct pci_dev *pdev, int offset, void *value, int len)
{
	u32 pci_pio_addr = X86_PCI_PIO_ADDR(pdev->bus, pdev->device, pdev->fun, offset);
	out32(0xcf8, pci_pio_addr);
	if (len == 4)
		*(u32 *)value = in32(0xcfc);
	else if (len == 2)
		*(u16 *)value = in16(0xcfc);
	else
		*(u8 *)value = in8(0xcfc);

	return 0;
}

int x86_pci_write_config_pio(struct pci_dev *pdev, int offset, u32 value, int len)
{
	u32 pci_pio_addr = X86_PCI_PIO_ADDR(pdev->bus, pdev->device, pdev->fun, offset);
	out32(0xcf8, pci_pio_addr);
	if (len == 4)
		out32(0xcfc, value);
	else if (len == 2)
		out16(0xcfc + offset & 0x2, value);
	else
		out8(0xcfc + offset & 0x3, value);

	return 0;
}

#define X86_PCI_MMIO_ADDR(b,d,f,o) \
	(PHYS2VIRT(x86_host_bridge.rc_mmio_base) + (((b) << 20) | ((d) << 15) | ((f) << 12) | o))

int x86_pci_read_config_mmio(struct pci_dev *pdev, int offset, void *value, int len)
{
	void *addr = (void *)X86_PCI_MMIO_ADDR(pdev->bus, pdev->device, pdev->fun, offset);
	if (len == 4)
		*(u32 *)value = *(u32 *)addr;
	else if (len == 2)
		*(u16 *)value = *(u16 *)addr;
	else
		*(u8 *)value = *(u8 *)addr;
	return 0;
}

int x86_pci_write_config_mmio(struct pci_dev *pdev, int offset, u32 value, int len)
{
	void *addr = (void *)X86_PCI_MMIO_ADDR(pdev->bus, pdev->device, pdev->fun, offset);
	if (len == 4)
		*(u32 *)addr = value;
	else if (len == 2)
		*(u16 *)addr = value;
	else
		*(u8 *)addr = value;
	return 0;
}

int x86_pci_msi_entry_build(struct msi_data *msi, int irq, int cpu)
{
	msi->addr = cpu | 
		MSI_ADDR_BASE_LO |
		MSI_ADDR_DEST_MODE_PHYSICAL;
	msi->data = (irq + 32) | 
		MSI_DATA_TRIGGER_EDGE |
		MSI_DATA_LEVEL_ASSERT |
		MSI_DATA_DELIVERY_FIXED;
	return 0;
}

void x86_pci_hostbridge_init()
{
	struct acpi_mcfg *mcfg = acpi_get_desc("MCFG");
	if (mcfg == NULL) {
		printk("PCI: No 'MCFG' structure found in ACPI.\n");
		x86_host_bridge.segment_number = 0;
		x86_host_bridge.start_bus_number = 0;
		x86_host_bridge.end_bus_number = 5;
		x86_host_bridge.pci_read_config = x86_pci_read_config_pio;
		x86_host_bridge.pci_write_config = x86_pci_write_config_pio;
		x86_host_bridge.pci_msi_entry_build = x86_pci_msi_entry_build;
	} else {
		x86_host_bridge.segment_number = mcfg->confgure_arr[0].pci_segment_group_number;
		x86_host_bridge.start_bus_number = mcfg->confgure_arr[0].start_bus_number;
		x86_host_bridge.end_bus_number = mcfg->confgure_arr[0].end_bus_number;
		x86_host_bridge.rc_mmio_base = mcfg->confgure_arr[0].base_addr;
		x86_host_bridge.pci_read_config = x86_pci_read_config_mmio;
		x86_host_bridge.pci_write_config = x86_pci_write_config_mmio;
		x86_host_bridge.pci_msi_entry_build = x86_pci_msi_entry_build;
	}

	host_bridge = &x86_host_bridge;
}

arch_init(x86_pci_hostbridge_init);
