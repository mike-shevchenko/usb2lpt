# Project: bootloadHID
# Tabsize: 8, LineEnds: LF, CodePage: UTF-8

# Program the device with
#     make program # program all
#     make fuse    # to set the clock generator, boot section size etc.
#     make flash   # to load the boot loader into flash
#     make lock    # to protect the boot loader from overwriting

DEVICE = atmega8
F_CPU = 12800000
# Fuse high byte:
HFUSE = 0x90
# 1 0 0 1  0 0 0 0 ←─ BOOTRST	(Boot reset vector at byte address 0x1800)
# ↑ ↑ ↑ ↑  ↑ └─┴───── BOOTSZ	(Boot-area size: 2 KByte (0x800), maximum)
# │ │ │ │  └───────── EESAVE	(Preserve EEPROM over chip erase: YES)
# │ │ │ └──────────── CKOPT	(Clock option: not used)
# │ │ └────────────── SPIEN	(Serial Programming enable: YES)
# │ └──────────────── WDTON	(Watchdog Timer always on: YES)
# └────────────────── RSTDISBL	(External Reset disable: NO)
# Fuse low byte:
LFUSE = 0x84
# 1 0 0 0  0 1 0 0
# ↑ ↑ └┬┘  └──┬──┘
# │ │  │      └────── CKSEL	(Oscillator selection: RC oscillator 8 MHz)
# │ │  └───────────── SUT	(Start-up time: 0.5 µs, minimum)
# │ └──────────────── BODEN	(Brown-out detector enable: YES)
# └────────────────── BODLEVEL	(Brown-out detection level: 2.7 V)

PROJECT = BL
USBDRV  = ../../usbdrv
AVRDUDE = avrdude -c pony-stk200 -P lpt1 -p $(DEVICE) -E noreset

CFLAGS  += -Wall -Os -I$(USBDRV) -I. -mmcu=$(DEVICE) -DF_CPU=$(F_CPU)
LDFLAGS += -nostartfiles -Ttext 0x1800

OBJECTS = $(PROJECT).o usbdrvasm.o

# symbolic targets:
all:	$(PROJECT).hex $(PROJECT).lst

flash:	all
	$(AVRDUDE) -U flash:w:$(PROJECT).hex:i -U lock:w:0x2F:m

readflash:
	$(AVRDUDE) -U flash:r:read.hex:i

fuse:
	$(AVRDUDE) -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m

lock:
	$(AVRDUDE) -U lock:w:0x2F:m
	
# all three programming options in one invocation of avrdude
program:
	$(AVRDUDE) -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m -U flash:w:$(PROJECT).hex:i -U lock:w:0x2F:m

clean:
	rm -f $(PROJECT).lst $(PROJECT).elf $(OBJECTS)

$(PROJECT).o: $(PROJECT).c makefile bootloaderconfig.h usbconfig.h
# Ignore warnings saying “`noreturn' function does return”
	avr-gcc -c $< -o $@ $(CFLAGS) -Wno-unused-function

usbdrvasm.o: $(USBDRV)/usbdrvasm.S makefile usbconfig.h
	avr-gcc -c $< -o $@ $(CFLAGS)

$(PROJECT).elf:	$(OBJECTS)
	avr-gcc -o $@ $(OBJECTS) $(LDFLAGS)

$(PROJECT).hex: $(PROJECT).elf
	rm -f $@
# Do not copy .data section here. There is no initializion routine that uses it.
	avr-objcopy -j .text -O ihex $< $@
	avr-size $<
# With avr-gcc 3.4.6 (2006), the code size is tuned to 2048 bytes.
# avr-gcc 4.1.2 bloats the code to much more than 2048 bytes!

$(PROJECT).lst:	$(PROJECT).elf
	avr-objdump -d $< > $@
