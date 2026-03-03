These files are modified files out of actual bootloadHID project
that supports 12.8 MHz RC oscillator and fits into 2 KB boot space.
It's specifically for the USB2LPT project that connects
USB D- to B0 (interrupt source = Input Capture) and USB D+ to B1.

[The reason for not using usual D2=INT0 and D3=INT1 was to have
a true 8-bit parallel port “D” for LPT data port simulation.
Now, with no need for a crystal, it's kept for PCB compatibility
while another full 8-bit port “B” is now available.]

Reasons for changing:
Makefile	Fuse setting changed
		Clock frequency changed
		Local object files and complete dependencies

BL.c		Inlined interrupt table with initialization code
		Function main() attribute "noreturn" set
		Continued initialization inside main()
		Minimum code to jump to address 0

usbconfig.h	See "Optional MCU Description" section

bootloaderconfig.h	regulary to be changed by design

The directory "usbdrv" and not-changed files are omitted here; refer to
directory up two levels.
