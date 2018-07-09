#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <types.h>

extern u64 KERNEL_SPACE;

#define PHYS2VIRT(x) (((u64)(x))+(u64)&KERNEL_SPACE)
#define VIRT2PHYS(x) (((u64)(x))-(u64)&KERNEL_SPACE)

#define SYSTEM_PARM_BASE PHYS2VIRT(0x10000)
#define MAX_CPUS 128

struct ards {
	u64 base;
	u64 length;
	u64 type;
	u64 reserved;
};

struct bootloader_parm_block {
	u64 acpi_rsdp;
	u64 screen_width;
	u64 screen_height;
	u64 pixels_per_scanline;
	u64 frame_buffer_base;
	u64 processor_cnt;
	u64 cpuid_array[512];
	u64 ardc_cnt;
	struct ards ardc_array[256];
};

extern struct bootloader_parm_block *boot_parm;
extern int printk(const char *fmt, ...);


#endif
