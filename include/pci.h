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

#define PCI_CONF_CACHELINE_SIZE 0xC
#define PCI_CONF_LATENCY_TIMER	0xD
#define PCI_CONF_HEADER_TYPE	0xE
#define PCI_CONF_BIST			0xF

#define PCI_CONF_BAR0			0x10
#define PCI_CONF_BAR1			0x14
#define PCI_CONF_TYPE0_BAR2		0x18

#define PCI_CONF_TYPE1_PRIMARY_BUS_NUM 			0x18
#define PCI_CONF_TYPE1_SECONDARY_BUS_NUM 		0x19
#define PCI_CONF_TYPE1_SUBORDINATE_BUS_NUM 		0x1A
#define PCI_CONF_TYPE1_SeCONDARY_LATENCY_TIMER 	0x1B

#define PCI_CONF_TYPE0_BAR3		0x1C

#define PCI_CONF_TYPE1_IO_BASE	0x1C
#define PCI_CONF_TYPE1_IO_LIMIT 0x1D
#define PCI_CONF_TYPE1_SECONDARY_STATUS 0x1E

#define PCI_CONF_TYPE0_BAR4		0x20

#define PCI_CONF_TYPE1_MEMORY_BASE 0x20
#define PCI_CONF_TYPE1_MEMORY_LIMIT 0x22

#define PCI_CONF_TYPE0_BAR5		0x24

#define PCI_CONF_TYPE1_PREFETCHABLE_MEMORY_BASE 	0x24
#define PCI_CONF_TYPE1_PREFETCHABLE_MEMORY_LIMIT 	0x26

#define PCI_CONF_CARDBUS_CIS_POINTER				0x28

#define PCI_CONF_TYPE1_PREFETCHABLE_BASE_UPPER32 	0x28

#define PCI_CONF_TYPE0_SUBSYSTEM_VENDORID			0x2C

#define PCI_CONF_TYPE1_PREFETCHABLE_LIMIT_UPPER32	0x2C

#define PCI_CONF_TYPE0_SUBSYSTEM_ID					0x2E
#define PCI_CONF_TYPE0_EXPANSION_ROM_BAR			0x30

#define PCI_CONF_TYPE1_IO_BASE_UPPER16 				0x30
#define PCI_CONF_TYPE1_IO_LIMIT_UPPER16				0x32

#define PCI_CONF_CAPABILITIES_POINTER				0x34

#define PCI_CONF_TYPE1_EXPANSION_ROM_BAR 			0x38

#define PCI_CONF_INTERRUPT_LINE				0x3C
#define PCI_CONF_INTERRUPT_PIN				0x3D
#define PCI_CONF_MIN_GNT					0x3E
#define PCI_CONF_MAX_LAT					0x3F

#define PCI_RESOURCE_MEM 0
#define PCI_RESOURCE_IO 1
#define PCI_RESOURCE_PREFETCH 2
struct pci_dev {
	u8 seg;
	u8 bus;
	u8 device;
	u8 fun;

	u32 viddid;
	u64 status;

	/* Bridge or device ? */
	u64 type;

	/* class code. */
	u64 class;

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

struct pci_device_id {
	u32 viddid;
	u32 class;
	u32 class_mask;
	u32 reserved;
};
#define PCI_DEVICE ((a),(b)) (((a)<<16)|(b))
struct pci_driver {
	char *name;
	struct pci_device_id id_array[64];
	int (*pci_probe)(struct pci_dev *pdev, struct pci_device_id *pent);
	void (*pci_remove)(struct pci_dev *pdev);
	struct list_head list;
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
