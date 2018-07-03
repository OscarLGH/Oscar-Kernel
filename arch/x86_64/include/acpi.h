#ifndef _ACPI_H_
#define _ACPI_H_

#include <types.h>

#pragma pack(1)
struct acpi_rsdp {
	u8 signature[8];
	u8 checksum;
	u8 oem_id[6];
	u8 revision;
	u32 rsdt_addr;
	u32 length;
	u64 xsdt_addr;
	u8 ext_checksum;
	u8 reserved[3];
};

struct acpi_sdt_header {
	u8 signature[4];
	u32 length;
	u8 revision;
	u8 chacksum;
	u8 oem_id[6];
	u64 oem_table_id;
	u32 oem_revision;
	u32 creator_id;
	u32 creator_revision;
};

struct rsdt {
	struct acpi_sdt_header header;
	u32 entry[32];
};

struct xsdt {
	struct acpi_sdt_header header;
	u64 entry[32];
};

enum apic_struc_types
{
	PROCESSOR_LOCAL_APCI = 0,
	IO_APIC,
	INTERRUPT_SOURCE_OVERRIDE,
	NMI_INTERRUPT_SOURCE,
	LOCAL_APIC_NMI,
	LOCAL_APIC_ADDRESS_OVERRIDE,
	IO_SAPIC,
	LOCAL_SAPIC,
	PLATFORM_INTERRUPT_SOURCES,
	PROCESSOR_LOCAL_X2APIC,
	LOCAL_X2APIC_NMI,
	GIC,
	GICD,
	RESERVED,
};

struct processor_lapic_structure {
	u8 type;
	u8 length;
	u8 acpi_processor_id;
	u8 apic_id;
	u32 flags;
};

struct io_apic_structure {
	u8 type;
	u8 length;
	u8 io_apic_id;
	u8 reserved;
	u32 io_apic_addr;
	u32 global_sys_intr_base;
};

struct acpi_madt {
	acpi_sdt_header header;
	u32 lapic_addr;
	u32 flags;
};

struct mmcs_bar {
	u64 base_addr;
	u16 pci_segment_group_number;
	u8 start_bus_number;
	u8 end_bus_number;
	u32 reserved;
};

struct acpi_mcfg {
	acpi_sdt_header header;
	u64 reserved;
	struct mmcs_bar confgure_arr[32];
};


enum remapping_structure_types
{
	DRHD = 0,/* DMA Remapping Hardware Unit Definition Reporting*/
	RMRR,	 /* Reserved Memory Region Reporting Reporting */
	ATSR,	 /* Root Port ATS Capability Reporting */
};

struct device_scope_structure {
	u8 type;
	u8 length;
	u16 reserved;
	u8 enumeration_id;
	u8 start_bus_number;
	/* path */
};

struct reserved_memory_region_structure {
	u16 type;
	u16 length;
	u16 reserved;
	u16 segment_number;
	u64 reserved_memory_region_base;
	u64 reserved_memory_region_limit;
	/* device_scope_structure */
};

struct root_port_ast_structure {
	u16 type;
	u16 length;
	u8 flags;
	u8 reserved;
	u16 segment_number;
	/* device_scope_structure */
};

struct drhd_structure {
	u16 type;
	u16 length;
	u8 flags;
	u8 reserved;
	u16 segment_number;
	u64 register_base;
	/* device_scope_structure */
};

struct acpi_dmar {
	struct acpi_sdt_header header;
	u8 host_addr_width;
	u8 flags;
	u8 reserved[10];
	/* remapping structures */
};

struct acpi_hpet {
	struct acpi_sdt_header;
	u32 event_timer_block_id;
	u8 address_space_id;
	u8 register_bit_width;
	u8 register_bit_offset;
	u8 reserved;
	u64 base_addr;
	u8 hpet_number;
	u16 main_counter_min_clock;
	u8 attribure;
};

#pragma pack()

#endif
