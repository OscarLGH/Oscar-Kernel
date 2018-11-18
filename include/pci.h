#ifndef PCI_H_
#define PCI_H_

#include <types.h>
#include <list.h>

//PCI Configuration Space Offsets
#define PCI_CONF_VENDORID 		0x0
#define PCI_CONF_DEVICEID 		0x2
#define PCI_CONF_COMMAND	 	0x4
#define PCI_CONF_STATUS   		0x6
#define PCI_CONF_REVISIONID 	0x8

#define PCI_CONF_INTERFACECODE	0x9
#define PCI_CONF_SUBCLASSCODE	0xA
#define PCI_CONF_BASECLASSCODE	0xB

#define PCI_CONF_CACHELINESIZE 	0xC
#define PCI_CONF_LATENCYTIMER	0xD
#define PCI_CONF_HEADERTYPE		0xE
#define PCI_CONF_BIST_OFF_8		0xF

#define PCI_CONF_BAR0			0x10
#define PCI_CONF_BAR1			0x14
#define PCI_CONF_BAR2			0x18
#define PCI_CONF_BAR3			0x1C
#define PCI_CONF_BAR4			0x20
#define PCI_CONF_BAR5			0x24

#define PCI_CONF_CARDBUSCISPOINTER			0x28
#define PCI_CONF_SUBSYSTEMVENDORID			0x2C
#define PCI_CONF_SUBSYSTEMID				0x2E

#define PCI_CONF_EXPANSIONROMBAR			0x30
#define PCI_CONF_CAPABILITIESPOINTER		0x34

#define PCI_CONF_INTERRUPTLINE				0x3C
#define PCI_CONF_INTERRUPTPIN				0x3D
#define PCI_CONF_MINGNT						0x3E
#define PCI_CONF_MAXLAT						0x3F

struct pci_dev {
	u8 seg;
	u8 bus;
	u8 device;
	u8 fun;

	u64 status;

	/* Bridge or device ? */
	u64 type;

	struct list_head list;
	struct pci_dev *parent;

	/* for legacy irq or msi. */
	u64 irq;

	struct {
		u64 start;
		u64 end;
		u64 flags;
		u64 reserved;
	} resource[7];
};

/*
 * These helpers provide future and backwards compatibility
 * for accessing popular PCI BAR info
 */
#define pci_resource_start(dev, bar)	((dev)->resource[(bar)].start)
#define pci_resource_end(dev, bar)	((dev)->resource[(bar)].end)
#define pci_resource_flags(dev, bar)	((dev)->resource[(bar)].flags)
#define pci_resource_len(dev,bar) \
	((pci_resource_start((dev), (bar)) == 0 &&	\
	  pci_resource_end((dev), (bar)) ==		\
	  pci_resource_start((dev), (bar))) ? 0 :	\
							\
	 (pci_resource_end((dev), (bar)) -		\
	  pci_resource_start((dev), (bar)) + 1))

/* arch specific. */
struct pci_host_bridge {
	u32 segment_number;
	u32 start_bus_number;
	u32 end_bus_number;
	u32 reserved;

	u64 rc_mmio_base;

	int (*pci_read_config)(struct pci_dev *pdev, int offset, void *value, int len);
	int (*pci_write_config)(struct pci_dev *pdev, int offset, void *value, int len);
	int (*pci_allocate_irq)(struct pci_dev *pdev);
	int (*pci_free_irq)(struct pci_dev *pdev, int irq);
	int (*pci_arm_msi)(struct pci_dev *pdev, int irq);
	int (*pci_pm_func)(struct pci_dev *pdev);
};

#endif
