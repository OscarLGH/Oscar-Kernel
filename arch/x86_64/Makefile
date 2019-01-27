ASM		= as
CC		= gcc
LD		= @ld

ASMFLAGS	= --64
CFLAGS		= -c -fno-builtin -fno-stack-protector -fPIC -fno-pie -Werror -g -mno-red-zone -mcmodel=large -I ../../include -I include

LDFLAGS		= -T Link.lds --nmagic

# This Program
OSCARSKERNEL	= bin/kernel.bin

SRCSC		= $(shell find ../.. -name "*.c")
SRCSA		= $(shell find ../.. -name "*.S")
OBJSC		= $(patsubst %.c,%.c.o,$(SRCSC))
OBJSA		= $(patsubst %.S,%.S.o,$(SRCSA))
OBJS		= $(OBJSA) 
OBJS		+= $(OBJSC)

# All Phony Targets
.PHONY : everything clean

# Default starting position
everything : $(OSCARSKERNEL)

clean :
	rm -f $(OBJS) $(OSCARSKERNEL)

$(OSCARSKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(OSCARSKERNEL) $(OBJS)

$(OBJSA) : %.S.o : %.S
	$(ASM) $(ASMFLAGS) -o $@ $<

$(OBJSC) : %.c.o : %.c
	$(CC) $(CFLAGS) -o $@ $<





