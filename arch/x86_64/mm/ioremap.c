#include <types.h>
#include <kernel.h>

/* TODO: vma management. */

void *ioremap(u64 phys_addr, u64 size)
{
	return (void *)PHYS2VIRT(phys_addr);
}

void *ioremap_nocache(u64 phys_addr, u64 size)
{
	return ioremap(phys_addr, size);
}

void iounmap(void *virt_addr)
{

}
