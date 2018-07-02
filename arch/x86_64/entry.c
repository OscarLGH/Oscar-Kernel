#include <types.h>
#include <lapic.h>
#include <msr.h>
#include <paging.h>

extern u64 ap_startup_vector;
extern u64 ap_startup_end;

extern u64 jmp_table[];
void arch_init()
{
	bool bsp = is_bsp();
	asm("movw $0x3f8, %dx");
	rdcr3();
	while(1) {
		if (bsp) {
			asm("movb $'B', %al");
		} else {
			asm("movb $'A', %al");
		}
		asm("outb %al, %dx");
	}
}
