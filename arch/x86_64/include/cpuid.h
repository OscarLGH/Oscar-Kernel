#ifndef _CPUID_H_
#define _CPUID_H_

#include <types.h>

static inline void cpuid(u32 eax, u32 ecx, u32 *out)
{
	asm volatile("movq %rdx, %r8\n\t"
			"movq %rdi, %rax\n\t"
			"movq %rsi, %rcx\n\t"
			"cpuid\n\t"
			"movl %eax, 0(%r8)\n\t"
			"movl %ebx, 4(%r8)\n\t"
			"movl %ecx, 8(%r8)\n\t"
			"movl %edx, 12(%r8)\n\t"
		);
}

#endif

