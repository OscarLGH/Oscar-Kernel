#include <types.h>
#include <kernel.h>
#include <acpi.h>
#include <string.h>

void *acpi_get_desc(char *name)
{
	u64 acpi_addr;
	u64 table_enrty_size;
	u64 index;

	acpi_addr = PHYS2VIRT(boot_parm->acpi_rsdp);
	//printk("acpi_addr = %016x\n", (u64)acpi_addr);
	if (strncmp((u8 *)acpi_addr, "RSD PTR ", 8) != 0) {
		printk("ACPI Error: ACPI signature not found.\n");
		return NULL;
	}

	struct acpi_rsdp *rsdptr = (struct acpi_rsdp *)acpi_addr;
	struct acpi_sdt_header *header_ptr = NULL, *return_ptr = NULL;

	if ((rsdptr->revision == 2)) {
		struct xsdt *xsdt_ptr = (struct xsdt *)PHYS2VIRT(rsdptr->xsdt_addr);
		//printk("xsdt_ptr = %016x\n", (u64)xsdt_ptr);
		for (index = 0; index < (xsdt_ptr->header.length - sizeof(struct acpi_sdt_header)) / 8; index++) {
			header_ptr = (struct acpi_sdt_header *)PHYS2VIRT(xsdt_ptr->entry[index]);
			//printk("header_ptr = %016x\n", (u64)header_ptr);
			if (strncmp(header_ptr->signature, name, 4) == 0) {
				return_ptr = header_ptr;
				return return_ptr;
			}
		}
	} else {
		struct rsdt *rsdt_ptr = (struct rsdt *)PHYS2VIRT(rsdptr->rsdt_addr);
		//printk("rsdt_ptr = %016x\n", (u64)rsdt_ptr);
		for (index = 0; index < (rsdt_ptr->header.length - sizeof(struct acpi_sdt_header)) / 4; index++) {
			header_ptr = (struct acpi_sdt_header *)PHYS2VIRT(rsdt_ptr->entry[index]);
			//printk("header_ptr = %016x\n", (u64)header_ptr);
			if (strncmp(header_ptr->signature, name, 4) == 0) {
				return_ptr = header_ptr;
				return return_ptr;
			}
		}
	}

	return NULL;
}
