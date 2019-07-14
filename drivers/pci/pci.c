#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>

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

int pci_find_capability(struct pci_dev *dev, u8 id)
{
	u8 next_cap;
	u8 cap_id;
	pci_read_config_byte(dev, PCI_CAPABILITY_LIST, &next_cap);

	while (next_cap) {
		pci_read_config_byte(dev, next_cap, &cap_id);
		if (cap_id == id)
			return next_cap;
		//printk("capability = %d\n", cap_id);
		pci_read_config_byte(dev, next_cap + 1, &next_cap);
	}

	return 0;
}

int pci_irq_register(struct pci_dev *dev, int cpu, u64 vector)
{
	struct pci_irq_desc *desc = kmalloc(sizeof(*desc), GFP_KERNEL);
	if (desc == NULL)
		return -ENOSPC;
	desc->cpu = cpu;
	desc->vector = vector;
	INIT_LIST_HEAD(&desc->list);
	list_add_tail(&desc->list, &dev->irq_list);
	return 0;
}

static int msi_entry_setup(struct pci_dev *dev, u64 msg_addr, u64 msg_data, u64 mask)
{
	u16 control;
	u32 addr_l = msg_addr & 0xffffffff;
	u32 addr_h = (msg_addr >> 32);
	u16 msi_data = msg_data;
	u32 msi_mask = mask;
	u8 msi_cap = pci_find_capability(dev, PCI_CAP_ID_MSI);
	if (msi_cap == 0)
		return -ENODEV;
	pci_read_config_word(dev, msi_cap + PCI_MSI_FLAGS, &control);

	pci_write_config_dword(dev, msi_cap + PCI_MSI_ADDRESS_LO, addr_l);
	if (control & PCI_MSI_FLAGS_64BIT) {
		pci_write_config_dword(dev, msi_cap + PCI_MSI_ADDRESS_HI, addr_h);
		pci_write_config_word(dev, msi_cap + PCI_MSI_DATA_64, msi_data);

		if (control & PCI_MSI_FLAGS_MASKBIT) {
			pci_write_config_dword(dev, msi_cap + PCI_MSI_MASK_64, msi_mask);
		}
	} else {
		pci_write_config_word(dev, msi_cap + PCI_MSI_DATA_32, msi_data);

		if (control & PCI_MSI_FLAGS_MASKBIT) {
			pci_write_config_dword(dev, msi_cap + PCI_MSI_MASK_32, msi_mask);
		}
	}

	return 0;
}

static int msi_irq_mask(struct pci_dev *dev, int msi_irq, int mask)
{
	u16 control;
	u32 msi_mask;
	u8 msi_cap = pci_find_capability(dev, PCI_CAP_ID_MSI);
	if (msi_cap == 0)
		return -ENODEV;

	if (mask) {
		msi_mask |= (1 << (msi_irq));
	} else {
		msi_mask &= ~(1 << (msi_irq));
	}

	pci_read_config_word(dev, msi_cap + PCI_MSI_FLAGS, &control);

	if (control & PCI_MSI_FLAGS_64BIT) {

		if (control & PCI_MSI_FLAGS_MASKBIT) {
			pci_read_config_dword(dev, msi_cap + PCI_MSI_MASK_64, &msi_mask);
			pci_write_config_dword(dev, msi_cap + PCI_MSI_MASK_64, msi_mask);
		} else {
			return -ENODEV;
		}
	} else {
		if (control & PCI_MSI_FLAGS_MASKBIT) {
			pci_read_config_dword(dev, msi_cap + PCI_MSI_MASK_32, &msi_mask);
			pci_write_config_dword(dev, msi_cap + PCI_MSI_MASK_32, msi_mask);
		} else {
			return -ENODEV;
		}
	}

	return 0;
}

static int msi_irq_pending(struct pci_dev *dev, int msi_irq)
{
	u16 control;
	u32 msi_pending;
	u8 msi_cap = pci_find_capability(dev, PCI_CAP_ID_MSI);
	if (msi_cap == 0)
		return -ENODEV;

	pci_read_config_word(dev, msi_cap + PCI_MSI_FLAGS, &control);

	if (control & PCI_MSI_FLAGS_64BIT) {

		if (control & PCI_MSI_FLAGS_MASKBIT) {
			pci_read_config_dword(dev, msi_cap + PCI_MSI_PENDING_64, &msi_pending);
		} else {
			return -ENODEV;
		}
	} else {
		if (control & PCI_MSI_FLAGS_MASKBIT) {
			pci_read_config_dword(dev, msi_cap + PCI_MSI_PENDING_32, &msi_pending);
		} else {
			return -ENODEV;
		}
	}

	return !!(msi_pending & (1 << msi_irq));
}


static int msi_capability_init(struct pci_dev *dev, int nvec,
			       const struct irq_affinity *affd)

{
	int msi_cap_offset;
	u16 msi_ctrl;
	int ret;
	int start_irq;
	int cpu;
	int i;
	struct msi_data msi_data;

	pci_msi_set_enable(dev, 0);
	start_irq = alloc_irqs(&cpu, nvec, nvec);
	if (start_irq == -1)
		return -ENOSPC;

	for (i = start_irq; i < start_irq + nvec; i++) {
		pci_irq_register(dev, cpu, i);
	}
	arch_pci_build_msi_entry(&msi_data, start_irq, cpu);
	msi_entry_setup(dev, msi_data.addr, msi_data.data, 0);
	pci_msi_multiple_enable(dev, nvec);
	
	pci_msi_set_enable(dev, 1);

	return 0;
}

/* return order 2 of nr_vectors. */
int pci_msi_vec_count(struct pci_dev *dev)
{
	int ret;
	u16 msgctl;	
	u16 control;
	u8 msi_cap = pci_find_capability(dev, PCI_CAP_ID_MSI);
	if (msi_cap == 0)
		return -ENODEV;

	pci_read_config_word(dev, msi_cap + PCI_MSI_FLAGS, &msgctl);
	ret = 1 << ((msgctl & PCI_MSI_FLAGS_QMASK) >> 1);

	return ret;
}

#define msix_table_size(flags)	((flags & PCI_MSIX_FLAGS_QSIZE) + 1)

static int msix_entry_setup(struct pci_dev *dev, u64 index, u64 msg_addr, u64 msg_data, u64 mask)
{
	u16 control;
	u32 addr_l = msg_addr & 0xffffffff;
	u32 addr_h = (msg_addr >> 32);
	u16 msi_data = msg_data;
	u32 msi_mask = mask;
	struct msi_x_table *table_base;
	u64 phys_addr;
	u32 table_offset;
	u8 bir;
	u32 nr_entries;

	u8 msix_cap = pci_find_capability(dev, PCI_CAP_ID_MSIX);
	if (msix_cap == 0)
		return -ENODEV;

	pci_read_config_word(dev, msix_cap + PCI_MSIX_FLAGS, &control);
	nr_entries = msix_table_size(control);

	pci_read_config_dword(dev, msix_cap + PCI_MSIX_TABLE,
			      &table_offset);
	bir = (u8)(table_offset & PCI_MSIX_TABLE_BIR);

	table_offset &= PCI_MSIX_TABLE_OFFSET;
	phys_addr = pci_resource_start(dev, bir) + table_offset;

	table_base = ioremap_nocache(phys_addr, nr_entries * PCI_MSIX_ENTRY_SIZE);

	table_base[index].addr_l = msg_addr;
	table_base[index].addr_h = msg_addr >> 32;
	table_base[index].data = msg_data;
	table_base[index].vector_ctrl = (mask & 0x1);

	iounmap(table_base);
	return 0;
}

static int msix_irq_mask(struct pci_dev *dev, int msix_irq, int mask)
{
	u16 control;
	struct msi_x_table *table_base;
	u64 phys_addr;
	u32 table_offset;
	u8 bir;
	u32 nr_entries;

	u8 msix_cap = pci_find_capability(dev, PCI_CAP_ID_MSIX);
	if (msix_cap == 0)
		return -ENODEV;

	pci_read_config_word(dev, msix_cap + PCI_MSIX_FLAGS, &control);
	nr_entries = msix_table_size(control);

	pci_read_config_dword(dev, msix_cap + PCI_MSIX_TABLE,
			      &table_offset);
	bir = (u8)(table_offset & PCI_MSIX_TABLE_BIR);

	table_offset &= PCI_MSIX_TABLE_OFFSET;
	phys_addr = pci_resource_start(dev, bir) + table_offset;

	table_base = ioremap_nocache(phys_addr, nr_entries * PCI_MSIX_ENTRY_SIZE);

	table_base[msix_irq].vector_ctrl = (mask & 0x1);
	iounmap(table_base);
	return 0;
}

static int msix_irq_pending(struct pci_dev *dev, int msix_irq)
{
	u16 control;
	u64 *pba_base;
	u64 phys_addr;
	u32 pba_offset;
	u8 bir;
	u32 nr_entries;
	u64 pending;

	u8 msix_cap = pci_find_capability(dev, PCI_CAP_ID_MSIX);
	if (msix_cap == 0)
		return -ENODEV;

	pci_read_config_word(dev, msix_cap + PCI_MSIX_FLAGS, &control);
	nr_entries = msix_table_size(control);

	pci_read_config_dword(dev, msix_cap + PCI_MSIX_PBA,
			      &pba_offset);
	bir = (u8)(pba_offset & PCI_MSIX_PBA_BIR);

	pba_offset &= PCI_MSIX_PBA_OFFSET;
	phys_addr = pci_resource_start(dev, bir) + pba_offset;

	pba_base = ioremap_nocache(phys_addr, nr_entries * PCI_MSIX_ENTRY_SIZE);

	pending = pba_base[msix_irq / 64] & (1 << (msix_irq % 64));
	iounmap(pba_base);
	return !!pending;
}

static int msix_capability_init(struct pci_dev *dev, int nvec,
			       const struct irq_affinity *affd)

{
	int msi_cap_offset;
	u16 msi_ctrl;
	int ret;
	int start_irq;
	int cpu;
	int irq;
	int i;
	struct msi_data msi_data;

	/* Ensure MSI-X is disabled while it is set up */
	pci_msix_clear_and_set_ctrl(dev, PCI_MSIX_FLAGS_ENABLE, 0);

	for (i = 0; i < nvec; i++) {
		irq = alloc_irq_from_smp(&cpu);
		pci_irq_register(dev, cpu, i);
		arch_pci_build_msi_entry(&msi_data, start_irq, cpu);
		msix_entry_setup(dev, i, msi_data.addr, msi_data.data, 0);
	}
	
	pci_msix_clear_and_set_ctrl(dev, PCI_MSIX_FLAGS_MASKALL, 0);

	return 0;
}


int pci_msix_vec_count(struct pci_dev *dev)
{
	u16 control;
	u8 msix_cap = pci_find_capability(dev, PCI_CAP_ID_MSIX);
	if (msix_cap == 0)
		return -ENODEV;

	pci_read_config_word(dev, msix_cap + PCI_MSIX_FLAGS, &control);
	return msix_table_size(control);
}

int pci_enable_msix(struct pci_dev *dev, struct msix_entry *entries, int nvec)
{
	int ret;
	int nvec0;

	nvec0 = pci_msix_vec_count(dev);
	if (nvec0 < 0)
		return nvec0;

	printk("%02x:%02x.%01x pci msix:request %d vectors.\n", dev->bus, dev->device, dev->fun, nvec0);

	//ret = msix_capability_init(dev);
	return ret;
}

static int __pci_enable_msi_range(struct pci_dev *dev, int minvec, int maxvec,
				  const struct irq_affinity *affd)
{
	int nvec;
	int rc;

	nvec = pci_msi_vec_count(dev);
	printk("%02x:%02x.%01x pci msi:request %d vectors.\n", dev->bus, dev->device, dev->fun, nvec);
	if (nvec < 0)
		return nvec;
	if (nvec < minvec)
		return -ENOSPC;

	if (nvec > maxvec)
		nvec = maxvec;

	for (;;) {
		//if (affd) {
			//nvec = irq_calc_affinity_vectors(minvec, nvec, affd);
			//if (nvec < minvec)
			//	return -ENOSPC;
		//}

		rc = msi_capability_init(dev, nvec, affd);
		if (rc == 0)
			return nvec;

		if (rc < 0)
			return rc;
		if (rc < minvec)
			return -ENOSPC;

		nvec = rc;
	}
}

/* deprecated, don't use */
int pci_enable_msi(struct pci_dev *dev)
{
	int rc = __pci_enable_msi_range(dev, 1, 32, NULL);
	if (rc < 0)
		return rc;
	return 0;
}


static int __pci_enable_msix_range(struct pci_dev *dev,
				   struct msix_entry *entries, int minvec,
				   int maxvec, const struct irq_affinity *affd)
{
	int rc, nvec = maxvec;

	if (maxvec < minvec)
		return -ERANGE;

	for (;;) {
		if (affd) {
			//nvec = irq_calc_affinity_vectors(minvec, nvec, affd);
			if (nvec < minvec)
				return -ENOSPC;
		}

		msix_capability_init(dev, nvec, affd);
		//rc = __pci_enable_msix(dev, entries, nvec, affd);
		//if (rc == 0)
		//	return nvec;

		if (rc < 0)
			return rc;
		if (rc < minvec)
			return -ENOSPC;

		nvec = rc;
	}
}

/**
 * pci_enable_msix_range - configure device's MSI-X capability structure
 * @dev: pointer to the pci_dev data structure of MSI-X device function
 * @entries: pointer to an array of MSI-X entries
 * @minvec: minimum number of MSI-X irqs requested
 * @maxvec: maximum number of MSI-X irqs requested
 *
 * Setup the MSI-X capability structure of device function with a maximum
 * possible number of interrupts in the range between @minvec and @maxvec
 * upon its software driver call to request for MSI-X mode enabled on its
 * hardware device function. It returns a negative errno if an error occurs.
 * If it succeeds, it returns the actual number of interrupts allocated and
 * indicates the successful configuration of MSI-X capability structure
 * with new allocated MSI-X interrupts.
 **/
int pci_enable_msix_range(struct pci_dev *dev, struct msix_entry *entries,
		int minvec, int maxvec)
{
	return __pci_enable_msix_range(dev, entries, minvec, maxvec, NULL);
}

/**
 * pci_alloc_irq_vectors_affinity - allocate multiple IRQs for a device
 * @dev:		PCI device to operate on
 * @min_vecs:		minimum number of vectors required (must be >= 1)
 * @max_vecs:		maximum (desired) number of vectors
 * @flags:		flags or quirks for the allocation
 * @affd:		optional description of the affinity requirements
 *
 * Allocate up to @max_vecs interrupt vectors for @dev, using MSI-X or MSI
 * vectors if available, and fall back to a single legacy vector
 * if neither is available.  Return the number of vectors allocated,
 * (which might be smaller than @max_vecs) if successful, or a negative
 * error code on error. If less than @min_vecs interrupt vectors are
 * available for @dev the function will fail with -ENOSPC.
 *
 * To get the Linux IRQ number used for a vector that can be passed to
 * request_irq() use the pci_irq_vector() helper.
 */

/**
 * pci_alloc_irq_vectors_affinity - allocate multiple IRQs for a device
 * @dev:		PCI device to operate on
 * @min_vecs:		minimum number of vectors required (must be >= 1)
 * @max_vecs:		maximum (desired) number of vectors
 * @flags:		flags or quirks for the allocation
 * @affd:		optional description of the affinity requirements
 *
 * Allocate up to @max_vecs interrupt vectors for @dev, using MSI-X or MSI
 * vectors if available, and fall back to a single legacy vector
 * if neither is available.  Return the number of vectors allocated,
 * (which might be smaller than @max_vecs) if successful, or a negative
 * error code on error. If less than @min_vecs interrupt vectors are
 * available for @dev the function will fail with -ENOSPC.
 *
 * To get the Linux IRQ number used for a vector that can be passed to
 * request_irq() use the pci_irq_vector() helper.
 */
int pci_alloc_irq_vectors_affinity(struct pci_dev *dev, unsigned int min_vecs,
				   unsigned int max_vecs, unsigned int flags,
				   const struct irq_affinity *affd)
{
	static const struct irq_affinity msi_default_affd;
	int vecs = -ENOSPC;

	if (flags & PCI_IRQ_AFFINITY) {
		if (!affd)
			affd = &msi_default_affd;
	} else {
			affd = NULL;
	}

	if (flags & PCI_IRQ_MSIX) {
		vecs = __pci_enable_msix_range(dev, NULL, min_vecs, max_vecs,
				affd);
		if (vecs > 0)
			return vecs;
	}

	if (flags & PCI_IRQ_MSI) {
		vecs = __pci_enable_msi_range(dev, min_vecs, max_vecs, affd);
		if (vecs > 0)
			return vecs;
	}

	/* use legacy irq if allowed */
	if (flags & PCI_IRQ_LEGACY) {
		//if (min_vecs == 1 && dev->irq) {
			//pci_intx(dev, 1);
		//	return 1;
		//}
	}

	return vecs;
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
	pci_write_config_word(pdev, PCI_COMMAND, new_cmd_reg);

	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &ori_bar_lo32);
	pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, all_ones);
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, &size_lo32);
	pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar * 4, ori_bar_lo32);

	if (ori_bar_lo32 & BIT2) {
		pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, &ori_bar_hi32);
		pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, all_ones);
		pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, &size_hi32);
		pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + (bar + 1) * 4, ori_bar_hi32);
	}

	pci_write_config_word(pdev, PCI_COMMAND, ori_cmd_reg);

	if (ori_bar_lo32 & PCI_BASE_ADDRESS_SPACE_IO) {
		size_lo32 &= (~0x3);
		/* IO type bar ignores upper 16 bits. */
		size_lo32 |= 0xffff0000;
		valid = 0;
	} else {
		size_lo32 &= (~0xf);
	}

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
	pci_write_config_word(pdev, PCI_COMMAND, new_cmd_reg);

	if ((pdev->type & 0x1) == 0) {
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS, &ori_bar);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS, all_ones);
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS, &size);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS, ori_bar);
	} else {
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS1, &ori_bar);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS1, all_ones);
		pci_read_config_dword(pdev, PCI_ROM_ADDRESS1, &size);
		pci_write_config_dword(pdev, PCI_ROM_ADDRESS1, ori_bar);
	}

	pci_write_config_word(pdev, PCI_COMMAND, ori_cmd_reg);
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
	INIT_LIST_HEAD(&pdev->irq_list);
	pci_read_config_byte(pdev, PCI_HEADER_TYPE, &type);
	pci_read_config_word(pdev, PCI_VENDOR_ID, &vid);
	pci_read_config_word(pdev, PCI_DEVICE_ID, &did);
	pci_read_config_byte(pdev, PCI_CLASS_DEVICE, &class_code[0]);
	pci_read_config_byte(pdev, PCI_CLASS_PROG, &class_code[1]);
	pci_read_config_byte(pdev, PCI_CLASS_REVISION, &class_code[2]);
	pdev->class = (class_code[0] << 16) | (class_code[1] << 8) | (class_code[2]);
	pdev->viddid = (did << 16) | (vid);
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

			pci_find_capability(pdev, 0);
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
					pdev = kmalloc(sizeof(*pdev), GFP_KERNEL);
					pdev->bus = b;
					pdev->device = d;
					pdev->fun = f;
					INIT_LIST_HEAD(&pdev->irq_list);
					//printk("%02d:%02d.%d vendor:%04x, device:%04x\n", b, d, f, vendor_id, device_id);
					pci_device_init(pdev);
					list_add_tail(&pdev->list, &pci_device_list);
				}
			}
	//pci_device_print();
}

int pci_scan_alloc_resource()
{
	
}

subsys_init(pci_scan);
