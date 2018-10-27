/* bootmem allocator,used before buddy system.
 * First Fit allocation.
 * bootmem allocator can only manage up to 64GB physical memory.
 * bootmem CANNOT free any memory currently.
 */

#include <mm.h>
#include <kernel.h>
#include <math.h>
#include <spinlock.h>
#include <string.h>

u8 bootmem_bitmap[0x200000] = {0};

struct bootmem bootmem;

static void bss_init()
{
	extern int __bss_start, __bss_end;
	int *p = &__bss_start;
	for (; p < &__bss_end; p++)
		*p = 0;
}


static void set_bootmem_bitmap(u64 paddr)
{
	u64 index = (paddr >> (12 + 3));
	u64 bit = (paddr >> 12) & 0x7;
	bootmem.bitmap[index] |= (1 << bit);
}

static void clear_bootmem_bitmap(u64 paddr)
{
	u64 index = (paddr >> (12 + 3));
	u64 bit = (paddr >> 12) & 0x7;
	bootmem.bitmap[index] &= ~(1 << bit);
}

static u64 get_bootmem_bitmap(u64 paddr)
{
	u64 index = (paddr >> (12 + 3));
	u64 bit = (paddr >> 12) & 0x7;
	return (bootmem.bitmap[index] & (1 << bit));
}

static void set_bootmem_bitmap_area(u64 paddr, u64 size)
{
	u64 addr = paddr;
	while (addr < paddr + size) {
		set_bootmem_bitmap(addr);
		addr += 0x1000;
	}
}

static void clear_bootmem_bitmap_area(u64 paddr, u64 size)
{
	u64 addr = paddr;
	while (addr < paddr + size) {
		clear_bootmem_bitmap(addr);
		addr += 0x1000;
	}
}


static u64 find_free_bitmap(u64 size)
{
	u64 addr1, addr2;
	for (addr1 = 0; addr1 < 64 * 0x40000000ULL; addr1 += 0x1000) {
		if (get_bootmem_bitmap(addr1))
			continue;

		for (addr2 = addr1; addr2 < 64 * 0x40000000ULL; addr2 += 0x1000) {
			if (get_bootmem_bitmap(addr2))
				break;

			if (addr2 - addr1 >= size)
				return addr1;
		}
	}

	return -1;
}

void *bootmem_alloc(u64 size)
{
	u64 paddr;
	void *vaddr = NULL;
	spin_lock(&bootmem.spin_lock);
	if (size >= 0x1000) {
		bootmem.last_alloc_offset = 0;
		paddr = find_free_bitmap(size);

		if (paddr != -1) {
			set_bootmem_bitmap_area(paddr, size);
		}
	} else {
		if (bootmem.last_alloc_offset + size > 0x1000) {
			if (!get_bootmem_bitmap(bootmem.last_alloc_pfn * 0x1000 + size)) {
				set_bootmem_bitmap(bootmem.last_alloc_pfn * 0x1000 + size);
				paddr = bootmem.last_alloc_pfn * 0x1000 + bootmem.last_alloc_offset;
				bootmem.last_alloc_offset = roundup((bootmem.last_alloc_offset + size) % 0x1000, 64);
				bootmem.last_alloc_pfn += 1;
			} else {
				paddr = find_free_bitmap(0x1000);
				if (paddr != -1) {
					bootmem.last_alloc_offset = 0;
					bootmem.last_alloc_pfn = paddr / 0x1000;
					set_bootmem_bitmap_area(paddr, size);
				}
			}
		} else {
			if (bootmem.last_alloc_offset != 0) {
				paddr = bootmem.last_alloc_pfn * 0x1000 + bootmem.last_alloc_offset;
				bootmem.last_alloc_offset = roundup((bootmem.last_alloc_offset + size) % 0x1000, 64);
			} else {
				paddr = find_free_bitmap(0x1000);
				if (paddr != -1) {
					bootmem.last_alloc_offset = roundup((bootmem.last_alloc_offset + size) % 0x1000, 64);
					bootmem.last_alloc_pfn = paddr / 0x1000;
					set_bootmem_bitmap_area(paddr, size);
				}
			}
		}
	}
	spin_unlock(&bootmem.spin_lock);
	vaddr = (void *)PHYS2VIRT(paddr);
	return vaddr;
}

/* Unsupported operation. */
void bootmem_free(void *vaddr)
{
	u64 paddr = VIRT2PHYS(vaddr);
	clear_bootmem_bitmap(paddr);
}

void bootmem_init()
{
	int i;
	bss_init();
	bootmem.bitmap = bootmem_bitmap;
	bootmem.last_alloc_pfn = 1;
	bootmem.last_alloc_offset = 0;
	bootmem.spin_lock = 0;
	memset(bootmem_bitmap, 0xff, 0x200000);

	struct bootloader_parm_block *boot_parm = (void *)SYSTEM_PARM_BASE;
	for (int i = 0; i < boot_parm->ardc_cnt; i++) {
		if (boot_parm->ardc_array[i].type == 1) {
			clear_bootmem_bitmap_area(boot_parm->ardc_array[i].base, boot_parm->ardc_array[i].length);
		}
	}

	/* Reserve 16MB memory for kernel code & data. */
	set_bootmem_bitmap_area(0, 0x1000000);
}

