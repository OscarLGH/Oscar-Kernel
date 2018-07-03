#include <types.h>
#include <kernel.h>
#include <lapic.h>
#include <msr.h>
#include <paging.h>
#include <spinlock.h>
#include <in_out.h>
#include <cpuid.h>

extern u64 ap_startup_vector;
extern u64 ap_startup_end;

extern u64 jmp_table[];

extern int printk(const char *fmt, ...);
int cpu_cnt = 0;
void arch_init()
{
	u8 buffer[64] = {0};
	bool bsp = is_bsp();
	cpuid(0x80000002, 0x00000000, (u32 *)&buffer[0]);
	cpuid(0x80000003, 0x00000000, (u32 *)&buffer[16]);
	cpuid(0x80000004, 0x00000000, (u32 *)&buffer[32]);

	printk("CPU %d %s\n", cpu_cnt++, buffer);
	//asm("movw $0x3f8, %dx");
	//read_cr3();
	//write_cr3(0x0);

	while(1) {
		if (bsp) {
			printk("bsp\n");
		} else {
			//asm("movb $'A', %al");
			printk("ap\n");
		}
		//asm("outb %al, %dx");
	}
}
