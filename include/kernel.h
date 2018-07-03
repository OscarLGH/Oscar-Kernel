#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <types.h>

extern int KERNEL_SPACE;

#define PHYS2VIRT(x) (((u64)(x))+(u64)&KERNEL_SPACE)
#define VIRT2PHYS(x) (((u64)(x))-(u64)&KERNEL_SPACE)

#endif
