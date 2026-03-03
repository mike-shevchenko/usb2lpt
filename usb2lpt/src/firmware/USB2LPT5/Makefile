# Tabsize = 8, Encoding = UTF-8, LineEnds = LF
PROJECT = usb2lpt5
DEVICE = atmega48
AVRDUDE = avrdude -c pony-stk200 -P lpt1 -p $(DEVICE) -E noreset
USBDRV = ../usbdrv
F_CPU = 12000000

COMPILE = avr-gcc -Wall -Os -I$(USBDRV) -I. -DF_CPU=$(F_CPU) -mmcu=$(DEVICE)
# My development system is WinAVR 20060421 using gcc 3.x,
# which produces good (i.e. dense) code anyway.
# For gcc 4.x, insert the following options to reduce resulting code size:
# -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions

OBJECTS = usbdrvasm.o $(PROJECT).o

# Symbolic targets:
all:	$(PROJECT).hex $(PROJECT).eep $(PROJECT).lst
disasm:	$(PROJECT).lst

HFUSE = 0xD5
# 1 1 0 1   0 1 0 1
# │ │ │ │   │ └─┴─┴─── BODLEVEL	(Brown-out detection level: 2.7V)
# │ │ │ │   └───────── EESAVE	(preserve EEPROM over chip erase: YES)
# │ │ │ └───────────── WDTON	(Watchdog Timer always On: NO)
# │ │ └─────────────── SPIEN	(Serial Programming enable: YES)
# │ └───────────────── DWEN	(debugWIRE Enable: NO)
# └─────────────────── RSTDISBL	(External Reset Disable: NO)

LFUSE = 0xEF
# 1 1 1 0   1 1 1 1
# │ │ └┬┘   └─┬─┘ │
# │ │  │      └───┼─── CKSEL	(Oscillator selection: Low-Power Quartz crystal)
# │ │  └──────────┴─── SUT	(Start-up time: 85 µs (@ 12 MHz), minimum)
# │ └───────────────── CKOUT	(Clock output at PB0: NO)
# └─────────────────── CKDIV8	(Initial clock division by 8: NO)
 
# This target (“make program”) is intended for first-time firmware programming,
# e.g. for mass production.
# It programs the firmware in flash and EEPROM space, fuses and lock bits in one step
# without re-compiling. Albeit the “usb2lpt5.hex” and “usb2lpt5.eep” files must be available.
program:
	$(AVRDUDE) -U flash:w:$(PROJECT).hex:i -U eeprom:w:$(PROJECT).eep:i -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m

# This target ("make fuse") sets the fuses for further firmware development.
fuse:
	$(AVRDUDE) -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m

# This target ("make flash") is intended for firmware development.
# It rebuilds the HEX/EEP files when necessary.
flash: all
	$(AVRDUDE) -U flash:w:$(PROJECT).hex:i -U eeprom:w:$(PROJECT).eep:i

# This target ("make clean") deletes intermediate files but no HEX/EEP files.
# Good for zipping (archiving) and distribution.
clean:
	rm -f $(PROJECT).lst $(PROJECT).elf $(OBJECTS)

# File targets:
$(PROJECT).o: $(PROJECT).c usbconfig.h Makefile
	$(COMPILE) -c $< -o $@

usbdrvasm.o: $(USBDRV)/usbdrvasm.S usbconfig.h Makefile
	$(COMPILE) -c $< -o $@

$(PROJECT).elf: $(OBJECTS) Makefile
	$(COMPILE) -o $@ $(OBJECTS) -nostartfiles

$(PROJECT).hex: $(PROJECT).elf Makefile
	rm -f $@
# The .data section is not needed in the resulting .hex file here, all initialization is done by hand
	avr-objcopy -j .text -O ihex $< $@
	avr-size --mcu=$(DEVICE) -C $<

$(PROJECT).eep: $(PROJECT).elf Makefile
	avr-objcopy -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

$(PROJECT).lst: $(PROJECT).elf Makefile
	avr-objdump -d $< > $@
