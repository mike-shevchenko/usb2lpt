# Tabsize = 8, Encoding = UTF-8, LineEnds = LF
PROJECT = ul12
DEVICE  = at90usb162
AVRDUDE = avrdude -c pony-stk200 -P lpt1 -p $(DEVICE) -E noreset
PROG = dfu-programmer
F_CPU = 16000000

COMPILE = avr-gcc -Wall -Wno-parentheses -Os -I$(USBDRV) -I. -mmcu=$(DEVICE) -D F_CPU=$(F_CPU)
# My development system is WinAVR 20060421 using gcc 3.x,
# which produces good (i.e. dense) code anyway.
# For gcc 4.x, insert the following options to reduce resulting code size:
# -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions

# Symbolic targets:
all:	$(PROJECT).hex $(PROJECT).lst


# This target (“make prog”) is intended for firmware update via bootloader.
# It does not re-build the “ul12.hex” file.
prog:
	$(PROG) $(PROJECT).hex

# This target ("make flash") is intended for firmware development.
# It rebuilds the HEX file when necessary.
flash: all
	$(PROG) $(PROJECT).hex

# This target ("make clean") deletes intermediate files but no HEX files.
# Good for zipping (archiving) and distribution.
clean:
	rm -f $(PROJECT).lst $(PROJECT).elf

# File targets:
$(PROJECT).elf: $(PROJECT).c Makefile
	$(COMPILE) $< -o $@

$(PROJECT).hex: $(PROJECT).elf Makefile
	rm -f $@
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size --mcu=$(DEVICE) -C $<

$(PROJECT).lst: $(PROJECT).elf Makefile
	avr-objdump -d $< > $@
