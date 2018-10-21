#ifndef PCI_H_
#define PCI_H_

#include <types.h>
#include <list.h>

struct pci_dev {
	u8 seg;
	u8 bus;
	u8 device;
	u8 fun;

	u64 status;

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

#endif
