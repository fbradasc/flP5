INCLUDE=-I/usr/local/share/sdcc/include/pic
PROCESSOR_FLAG=-mpic14 -p16f877
CC=sdcc
AS=gpasm
CFLAGS=-S

HEX_FILES=test.hex

all: $(HEX_FILES)

%.hex: %.c
	$(CC) $(INCLUDE) $(PROCESSOR_FLAG) $(CFLAGS) $<
	$(AS) $*.asm

clean:
	$(RM) *.asm *.cod *.lst *.hex *.p *.d *.lnk *~
