ASM		= nasm
CC		= gcc
LD		= @ld
ASMBFLAGS	= -I Boot/Legacy/include/
ASMFLAGS	= -f elf64
CFLAGS		= -c -fno-builtin -fno-stack-protector -fPIC -Werror -g -mno-red-zone -I ../../Include

LDFLAGS		= -T Link.lds --nmagic

# This Program
OSCARSBOOT	= ../../../bin/X64/Boot.bin ../../../bin/X64/Loader.bin
OSCARSKERNEL	= ../../../Bin/X64/Kernel.bin

SRCSC		= $(shell find ../.. -name "*.c")
SRCSA		= $(shell find ../.. -name "*.asm")
OBJSC		= $(patsubst %.c,%.c.o,$(SRCSC))
OBJSA		= $(patsubst %.asm,%.asm.o,$(SRCSA))
OBJS		= $(OBJSA) 
OBJS		+= $(OBJSC)

# All Phony Targets
.PHONY : everything clean

# Default starting position
everything : $(OSCARSKERNEL)

clean :
	rm -f $(OBJS)

$(OSCARSKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(OSCARSKERNEL) $(OBJS)

$(OBJSA) : %.asm.o : %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

$(OBJSC) : %.c.o : %.c
	$(CC) $(CFLAGS) -o $@ $<





