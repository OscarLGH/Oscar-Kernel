#include <types.h>
#include <msr.h>
#include <lapic.h>

bool is_bsp()
{
	u64 value = rdmsr(MSR_IA32_APICBASE);
	return (value >> 8) & 0x1; 
}

