# Tabsize = 8, Encoding = UTF-8, LineEnds = LF
PROJECT = usb2lpt6
DEVICE  = atmega8
#DEVICE = atmega88
AVRDUDE = avrdude -c pony-stk200 -P lpt1 -p $(DEVICE) -E noreset
HIDPROG = ../bootloadHID/bootloadHID.exe -r
USBDRV  = ../usbdrv
# The hex file for the 2 KByte USB HID bootloader
BOOTHEX = ../bootloadHID/src-ATmega8-bootload/BL.hex
F_CPU = 12800000

COMPILE = avr-gcc -Wall -Os -I$(USBDRV) -I. -mmcu=$(DEVICE) -D F_CPU=$(F_CPU)
# My development system is WinAVR 20060421 using gcc 3.x,
# which produces good (i.e. dense) code anyway.
# For gcc 4.x, insert the following options to reduce resulting code size:
# -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions

OBJECTS = usbdrvasm.o $(PROJECT).o

# Symbolic targets:
all:	$(PROJECT).hex $(PROJECT).lst $(PROJECT)+BL.hex
disasm:	$(PROJECT).lst


ifeq ($(DEVICE),atmega8)
# ATmega8 fuses: (HFUSE = 0x81 if no bootloader is used with "make flash"!)
 HFUSE = 0x81
 HFUSEBL = 0x80
# Fuse high byte:
# 1 0 0 0   0 0 0 0 ←─ BOOTRST	(Boot reset vector at byte address 0x1800)
# ↑ ↑ ↑ ↑   ↑ └─┴───── BOOTSZ	(Boot-area size: 2 KByte (0x800), maximum)
# │ │ │ │   └───────── EESAVE	(Preserve EEPROM over chip erase: YES)
# │ │ │ └───────────── CKOPT	(Clock option: full output swing)
# │ │ └─────────────── SPIEN	(Serial Programming enable: YES)
# │ └───────────────── WDTON	(Watchdog Timer always on: YES)
# └─────────────────── RSTDISBL	(External Reset disable: NO)

# Fuse low byte:
ifeq ($(F_CPU),12800000)
 LFUSE = 0x84
# 1 0 0 0   0 1 0 0
# ↑ ↑ └┬┘   └──┬──┘
# │ │  │       └────── CKSEL	(Oscillator selection: RC oscillator 8 MHz)
# │ │  └────────────── SUT	(Start-up time: 0.5 µs, minimum)
# │ └───────────────── BODEN	(Brown-out detector enable: YES)
# └─────────────────── BODLEVEL (Brown-out detection level: 2.7V)
else
 LFUSE = 0xAE
# 1 0 1 0   1 1 1 0
#     └┬┘   └─┬─┘ │
#      │      └───┼─── CKSEL	(Oscillator selection: High-Speed Crystal)
#      └──────────┴─── SUT	(Start-up time: 85 µs (@ 12 MHz), minimum)
endif

else
# ATmega88 fuses:
# Fuse high byte: (The EFUSE is not changed)
 HFUSE = 0xD5
# 1 1 0 1   0 1 0 1
# ↑ ↑ ↑ ↑   ↑ └─┴─┴─── BODLEVEL	(Brown-out detection level: 2.7V)
# │ │ │ │   └───────── EESAVE	(preserve EEPROM over chip erase: YES)
# │ │ │ └───────────── WDTON	(Watchdog Timer always On: NO)
# │ │ └─────────────── SPIEN	(Serial Programming enable: YES)
# │ └───────────────── DWEN	(debugWIRE Enable: NO)
# └─────────────────── RSTDISBL	(External Reset Disable: NO)

# Fuse low byte:
ifeq ($(F_CPU),12800000)
 LFUSE = 0xC2
# 1 1 0 0   0 0 1 0
#     └┬┘   └──┬──┘
#      │       └────── CKSEL	(Oscillator selection: RC oscillator @ 8 MHz)
#      └────────────── SUT	(Start-up time: 6CK = 0.75 µs)
else
 LFUSE = 0xEF
# 1 1 1 0   1 1 1 0
# ↑ ↑ └┬┘   └─┬─┘ │
# │ │  │      └───┼─── CKSEL	(Oscillator selection: Low-Power Quartz crystal)
# │ │  └──────────┴─── SUT	(Start-up time: 85 µs (@ 12 MHz), minimum)
# │ └───────────────── CKOUT	(Clock output at PB0: NO)
# └─────────────────── CKDIV8	(Initial clock division by 8: NO)
endif
endif


# This target (“make prog”) is intended for firmware update via HID bootloader.
# The bootloader must be programmed and possibly re-activated beforehand.
# It does not re-build the “usb2lpt6.hex” file.
prog:
	$(HIDPROG) $(PROJECT).hex

PROGRAM = $(AVRDUDE) -U flash:w:$(PROJECT)+BL.hex:i -U hfuse:w:$(HFUSEBL):m -U lfuse:w:$(LFUSE):m -U lock:w:0x2F:m -U eeprom:w:0xFF:m

# This target (“make program”) is intended for first-time firmware programming,
# e.g. for mass production.
# It programs the bootloader, the firmware the fuses and lock bits in one step
# without re-compiling. Albeit the “usb2lpt6+BL.hex” file must be available.
program:
	$(PROGRAM)

# This is for 64bit Windows where avrdude won't run without tricks. My InpOut32.dll does it. (Go get it!)
# Furthermore, it runs without GiveIO installed on 32 bit systems. But incredibly slow on Win2k.
program64:
	crundll inpout32,CatchIo $(PROGRAM)

# This target ("make fuse") sets the fuses for further firmware development. (32bit only)
fuse:
	$(AVRDUDE) -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m

# This target ("make flash") is intended for firmware development
# without using HID bootloader. It rebuilds the HEX file when necessary. (32bit only)
flash: all
	$(AVRDUDE) -U flash:w:$(PROJECT).hex:i

# This target ("make clean") deletes intermediate files but no HEX files.
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
# The .data section is not needed in the resulting .hex file here, initialization is done by hand
	avr-objcopy -j .text -O ihex $< $@
	avr-size --mcu=$(DEVICE) -C $<

$(PROJECT).lst: $(PROJECT).elf Makefile
	avr-objdump -d $< > $@

$(PROJECT)+BL.hex: $(PROJECT).hex $(BOOTHEX)
	grep :......00 $< > $@
	cat $(BOOTHEX) >> $@
