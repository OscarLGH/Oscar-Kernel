#include <mm.h>

u8 boot_mem_pool[0x100000] = {0};
u64 allocate_index = 0;

void *boot_mem_alloc(u64 size)
{
	void *ptr = &boot_mem_pool[allocate_index];
	allocate_index += size;
	return ptr;
}

