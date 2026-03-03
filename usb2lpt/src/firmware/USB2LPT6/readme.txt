┌─┐
│A│
└─┘
To put the new firmware into ATmega's flash ROM using SPI (not bootloadHID):

* Ensure that you have an ATmega8. ATmegaX8 currently will not work.

* When you use PonyProg2000:
  - ensure that programmer settings are correct
    (Avr ISP I/O, no inversions), check connection with “Probe”
  - connect to target device and power the ATmega chip, e.g. by USB cable
    (the computer will detect a non-functional USB device)
  - load “usb2lpt6+BL.hex” as FLASH (not EEPROM) file
  - set the fuses as shown in “PonyProg Fuses.png” picture
    NEVER TWIDDLE WITH THE FUSES IF YOU DON'T KNOW THEIR BEHAVIOUR!
  - connect to target device, erase and burn all (the fuses and the flash)
  - re-plug the USB cable, the h#s low-speed USB2LPT adapter should occur

* When you use avrdude (usually when you have WinAVR installed):
  - adopt the “Makefile” to match your programmer, if you're not using STK200
  - connect to target device and power the ATmega chip
  - run “make program”
  - re-plug USB cable, the h#s low-speed USB2LPT adapter should occur

* The firmware has an adjanced USB bootloader so later firmware updates
  don't require a parallel port with adapter or programming device.

Note that you don't need a crystal and you don't see any oscillations
on pin PB6 and PB7.

┌─┐
│B│
└─┘
To update the ATmega8's firmware without changing the bootloader:

* Ensure that you have a USB2LPT device with activated bootloader.
  You can activate the bootloader in the Device Manager's property sheet page
  for the USB2LPT device.
  An activated bootloader leads that the USB2LPT device is non-functional
  in sense of parallel port emulation but appears as a USB HID.

* When you don't have WinAVR:
  - run “../bootloadHID/bootloadHID.exe -r usb2lpt6.hex”

* When you have WinAVR installed:
  - run “make prog”

h#s 110819
