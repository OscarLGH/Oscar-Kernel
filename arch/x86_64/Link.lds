KERNEL_SPACE = 0xFFFF800000000000;
KERNEL_ADDRESS = 0x200000;
KERNEL_BSS_ADDRESS = 0x500000;

/*
LD won't use _start as entry point when using lds.
So specifying ENTRY is necessary.
*/

ENTRY(bsp_entry_64)

SECTIONS 
{
    . = KERNEL_ADDRESS + KERNEL_SPACE;
    .text 	ALIGN(8) : AT(.)
	{
		*(.text) 
	}
	.initcall.init ALIGN(8) : AT((LOADADDR(.text)+SIZEOF(.text)+7)&~(0x07))
	{
		__initcall_start = .;
		__initcall1_start = .;
		*(.initcall1.init)
		__initcall1_end = .;

		__initcall2_start = .;
		*(.initcall2.init)
		__initcall2_end = .;

		__initcall3_start = .;
		*(.initcall3.init)
		__initcall3_end = .;

		__initcall4_start = .;
		*(.initcall4.init)
		__initcall4_end = .;

		__initcall5_start = .;
		*(.initcall5.init)
		__initcall5_end = .;

		__initcall6_start = .;
		*(.initcall6.init)
		__initcall6_end = .;

		__initcall7_start = .;
		*(.initcall7.init)
		__initcall7_end = .;

		__initcall8_start = .;
		*(.initcall8.init)
		__initcall8_end = .;

		__initcall_end = .;
	}
	
    .rodata ALIGN(8) : AT((LOADADDR(.initcall.init)+SIZEOF(.initcall.init)+7)&~(0x07))
	{
		*(.rodata*)
	}
    .data ALIGN(8)   : AT((LOADADDR(.rodata)+SIZEOF(.rodata)+7)&~(0x07)) 
	{
		*(.data) 
	}
	.eh_frame : 
	{
		*(.eh_frame)
	}
    __bss_start = .;
    .bss ALIGN(8)  : AT(.)
	{
		*(.bss)  
		*(COMMON) 
	}
    __bss_end = .;
}