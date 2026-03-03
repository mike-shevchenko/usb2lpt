// This English resource must not use extended ANSI characters to display correctly on all non-English systems,
// i.e. must be plain 7-bit ASCII

// DIALOG(EX)
#define S10000	"Emulation"
#define S10001	"Emulated port &address:"
#define S10002	"LPT &enhancement mode:"
#define S10003	"Trapping method for IN and OUT instructions"
#define S10004	"&Debug register trap"
#define S10005	"(catches all flavours of IN and OUT instructions)"
#define S10006	"&Force allocation, even if previously allocated"
#define S10007	"&READ/WRITE_PORT_UCHAR/USHORT/ULONG redirect"
#define S10008	"(faster, more stable)"
#define S10009	"Performance options"
#define S10010	"Use &write-back for OUT instructions"
#define S10011	"(Concatenate multiple OUT instructions to one USB packet)"
#define S10012	"Send packet after that &timeout value:"
#define S10014	"Read-&back local copy of output registers"
#define S10015	"&More precisely..."
#define S10016	"(return last-written value on IN instruction; does not work when outputs are driven by external hardware)"

#define S10100	"Statistics"
#define S10101	"Debug register trap"
#define S10102	"&Reset to zero"
#define S10103	"OUT instructions:"
#define S10104	"IN instructions:"
#define S10105	"Emulation errors:"
#define S10106	"(unsupported IN or OUT instruction, e.g. OUTSB)"
#define S10107	"Stolen debug registers:"
#define S10108	"(An unknown driver wrote to debug registers)"
#define S10109	"Overwritten debug registers are detected by a 100 ms kernel timer."
#define S10110	"Really assigned to:"
#define S10114	"READ_PORT_UCHAR / WRITE_PORT_UCHAR redirection"
#define S10115	"Reset to &zero"
#define S10116	"Oversized:"
#define S10117	"USB2LPT Sp&ecial..."
#define S10118	"Firmware &Update..."
#define S10119	"haftmann#software, Technical University Chemnitz,\nFaculty of Electronics Engineering and Information Technology,\nProfessur für Mess- und Sensortechnik, 2006-2009"

#define S10200	"Read-back copy of output registers"
#define S10201	"&Data port (base address + 0)"
#define S10202	"&Control port (base address + 2)"
#define S10203	"&Other ports (ECP, EPP)"
#define S10204	"OK"
#define S10205	"Cancel"

#define S10302	"A wrong setting may crash your computer or eat the content of your hard disk! Using one of the three predefined addresses should be safe."
#define S10304	"The LPT enhancement modes enable trapping of further port addresses.\nThese are very seldom used; so the SPP setting is OK."
#define S10305	"These and further settings you can later make in Windows' Device Manager. Point to the USB2LPT device, and open Properties."

#define S10400	"USB2LPT Special"
#define S10401	"Feature Register"
#define S10402	"(The Feature Register will be kept persistent in EEPROM of USB2LPT device. Access via DeviceIoControl)"
#define S10403	"Open Collectors for &Data Port (+0)"
#define S10404	"Open Collectors for &Control Port (+2)"
#define S10405	"switch off &Pullups (and 5V at Pin25 if so wired)"
#define S10406	"D&irect In/Out mode (USB2LPT converts to I/O device)"
#define S10407	"(Weak 5V instead of strong 3.3V for high-level outputs)"
#define S10408	"(A grayed checkmark means: Open Collectors in SPP mode, Push-Pull otherwise)"
#define S10409	"(just energy saving)"
#define S10410	"(No more inverted control and status lines! Therefore no useful LPT emulation, for access via DeviceIoControl.\nRev.4 has 20 I/O lines, 17 otherwise.\nDirection is per-bit and per-pin selectable.)"
#define S10411	"&Set && Readback"
#define S10412	"Scan for short circui&ts"
#define S10413	"Serial Number"
#define S10414	"in &USB descriptor"
#define S10415	"Firmware Date"
#define S10416	"Dimmed &blue LED"
#define S10417	"Startup value for &ECR (operating mode):"

#define S10500	"Monitor"
#define S10501	"Data &Port (+0)"
#define S10502	"Read"
#define S10503	"&Status Port (+1)"
#define S10504	"&Control Port (+2)"
#define S10506	"&Read Byte"
#define S10507	"&Address"
#define S10508	"&Data"
#define S10509	"&Write Byte"
#define S10512	"FIFO full"
#define S10513	"FIFO empty"
#define S10514	"Direction register (USB2LPT specific)"


// STRINGTABLE
#define S16 "Do you really, really want to use this non-standard port address?\n\nYou may crash your computer!"
#define S17 "This port address is not a valid hexadecimal number!"
#define S18 "This port address is wrong: too less, too big, or not divisable by 4!"
#define S19 "On port addresses not dividable by 8, EPP emulation is not possible!"
#define S20 "Incorrect number as Timeout value! (Must not be larger than 1000 ms.)"
#define S21 "Assignment of emulated port address"
#define S22 "This port address must match to the address used by your application software."
#define S23 "This firmware doesn't support the USB2LPT specific feature register!"
#define S24 "none"	// serial number
#define S25 "Context &help"
#define S26 "Bul&k transfer (faster; good for XP and earlier)"
#define S27 "Interrupt transfer otherwise"
#define S28 "Activating bulk transfer renders USB2LPT defective under Windows Vista and newer!\nRemember to reset this bit before upgrading your system.\n\nProceed?"
#define S29 "The debug register trap hook will be detected by PatchGuard some minutes later, which results in BSoD 0x109 when no countermeasure was taken.\n\nProceed?"

#define S32 "The firmware is written to the EEPROM successfully.\nCycle power to device to start the new firmware."
#define S33 "Unknown error while writing firmware to EEPROM. Please do not disconnect device and try again. The USB2LPT device may not work anymore!"
#define S34 "File ""%s"" is not accepted as firmware for that USB2LPT device!"
#define S35 "Date of actual (running) firmware: %s\n\nDate of firmware to be written: %s\n\nProceed?"
#define S36 "unknown"		// firmware date (early releases have no date inside)
#define S37 "Firmware\0*.iic"	// filter for open file standard dialog
#define S38 "Perfect function."
#define S39 "Incorrect function detected! Run Kurzschlusstest.EXE for details."
#define S40 "Do you measure HIGH level on all 17 lines?"
#define S41 "The firmware for Low-Speed USB2LPT device with ATmega controller cannot be updated here, but you can set a flag for activating a built-in boot loader. After power cycling you must update using ""bootloadHID.EXE"".\n\nProceed?"
#define S42 "This USB2LPT doesn't support firmware update via USB.\n\n(First EEPROM byte = 0x%02X)"

#define S48 "SPP (Nibble)\0Bidirectional\0AutoStrobe\0ECP\0EPP\0reserved\0test mode\0Configuration"
#define S49 "additional\0reserved\0Interrupt Request\0Error\0Online\0Paper End\0Acknowledge\0Busy"
#define S50 "Strobe\0AutoFeed\0Init\0Select Input\0Interrupt Enable\0Data direction, 1=input\0not implemented"
#define S51 "C&ommand\0&Address"		// Alternating text for radio button 105/122
#define S52 "normally\0Nibble Mode\0Byte Mode\0ECP Mode\0EPP Mode"

// VERSIONINFO
#define I002	9,1				// LANGUAGE
#define S003	"Property Sheet Provider"	// FileDescription
#define S005	"Public Domain"			// LegalCopyright
#define S006	"h#s USB to LPT converter"	// ProductName
#define S007	"Kernel-mode driver"		// FileDescription for .SYS driver

/* INF file strings (processed by makefile)
InstDisk="h#s USB to LPT converter driver diskette"
DevDesc ="h#s USB to LPT converter"
SvcDesc ="h#s driver for USB to LPT converter (usb2lpt.sys)"
*/
