#include "intel-iommu.h"
#include "acpi.h"
#include "init.h"

void intel_iommu_init()
{
	struct acpi_dmar *dmar_ptr = acpi_get_desc("DMAR");
	u8 *sub_struc_ptr;
	struct drhd_structure *drhd_ptr;
	struct reserved_memory_region_structure *rmrr_ptr;
	struct root_port_ast_structure *atsr_ptr;
	struct remapping_hw_static_affinity_structure *hw_sas_ptr;
	int i;
	int type, length;
	u64 *reg_base;

	if (dmar_ptr != NULL) {
		sub_struc_ptr = (void *)((u8 *)dmar_ptr + sizeof(*dmar_ptr));
		printk("IOMMU found.\n");
		printk("host address width:%d, flags = %x\n", dmar_ptr->host_addr_width, dmar_ptr->flags);

		for (i = 0; i < dmar_ptr->header.length - sizeof(*dmar_ptr);) {
			switch (sub_struc_ptr[0]) {
				case DRHD:
					drhd_ptr = (void *)sub_struc_ptr;
					printk("DRHD:\n");
					printk("Segment Number:%d:\n", drhd_ptr->segment_number);
					printk("Register Base Address:0x%x:\n", drhd_ptr->register_base);
					reg_base = (u64 *)PHYS2VIRT(drhd_ptr->register_base);
					break;
				case RMRR:
					rmrr_ptr = (void *)sub_struc_ptr;
					printk("RMRR:\n");
					printk("Segment Number:%d Region Base:0x%x Limit:0x%x\n",
						rmrr_ptr->segment_number,
						rmrr_ptr->reserved_memory_region_base,
						rmrr_ptr->reserved_memory_region_limit);
					break;
				case ATSR:
					atsr_ptr = (void *)sub_struc_ptr;
					printk("ATSR:\n");
					printk("flags = %x, Segment Number = %d\n",
						atsr_ptr->flags,
						atsr_ptr->segment_number
					);
					break;
				case RHSA:
					hw_sas_ptr = (void *)sub_struc_ptr;
					printk("RHSA:\n");
					printk("Register Base = 0x%x, Domain = %d\n",
						hw_sas_ptr->register_base,
						hw_sas_ptr->proximity_domain
					);
					break;
				default:
					break;
			}
			i += sub_struc_ptr[2];
			sub_struc_ptr += sub_struc_ptr[2];
		}

		printk("Version:%x\n", reg_base[DMAR_VER_REG / 8]);
		printk("Capbality:0x%x\n", reg_base[DMAR_CAP_REG / 8]);
		printk("Ext Capbality:0x%x\n", reg_base[DMAR_ECAP_REG / 8]);
	}
}


module_init(intel_iommu_init);


