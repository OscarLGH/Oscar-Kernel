void arch_init()
{
	while(1) {
		asm("movw $0x3f8, %dx");
		asm("movb $'A', %al");
		asm("outb %al, %dx");
	}
}
