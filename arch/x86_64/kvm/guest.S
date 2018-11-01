.global test_guest
.global test_guest_end


.text
.code16
test_guest:
	mov $0, %ax

	inb $0x70, %al
	orb $0x80, %al
	outb %al, $0x79

	cpuid

	jmp test_guest
test_guest_end:
	nop
	
