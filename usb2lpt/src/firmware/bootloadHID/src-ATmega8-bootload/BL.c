/* Name: main.c
 * Project: AVR bootloader HID
 * Author: Christian Starkjohann
 * Creation Date: 2007-03-19
 * Tabsize: 8
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt)
*10????	Heavily modified by haftmann#software to fit to the USB2LPT project,
	especially for fitting into 2 KB
*1108??	replyBuffer now reports _remaining_ (available) flash size
	(which is incompatible to the V-USB original but more meaningful)
	Problem jumping to address 0 solved - see leaveBootloader()
*110906	Report descriptor with non-zero usages, Delay2ms() added
-110917	wdt_reset() moved into Delay2ms()
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <string.h>
#include <util/delay.h>
#include <avr/eeprom.h>

// this routine is placed to address 0x1800 (ATmega8, ATmega88)
void vectors(void) __attribute__((naked,noreturn,section(".vectors")));
void vectors(void) {		// this code is ATmega8 specific
 asm volatile(
"	ldi	r30,0x5F\n"	// initialize stack
"	ldi	r31,0x04\n"
"	out	0x3E,r31\n"	// SPH
"	out	0x3D,r30\n"	// SPL
"	rjmp	main\n"
"	rjmp	__vector_5\n");
}

void __do_copy_data(void) __attribute__((naked));
void __do_copy_data(void) {}	// no non-zero-initialized static data

void __do_clear_bss(void) __attribute__((naked));
void __do_clear_bss(void) {}	// no static data at all

static void leaveBootloader() __attribute__((naked,noreturn));

#include "bootloaderconfig.h"
#include "usbdrv.c"

/* ------------------------------------------------------------------------ */

#ifndef ulong
# define ulong	unsigned long
#endif
#ifndef uint
# define uint	unsigned int
#endif

#if (FLASHEND) > 0xffff	// we need long addressing
# define addr_t	ulong
#else
# define addr_t	uint
#endif

static addr_t	currentAddress;	// in bytes of flash memory (Bit0 = 0)
static uchar	offset;         // data already processed in current transfer
#if BOOTLOADER_CAN_EXIT
static uchar	exitMainloop;
#endif

PROGMEM char usbHidReportDescriptor[33] = {
 0x06, 0x00, 0xff,	// G Usage Page (Vendor defined)
 0x09, 0x01,		// L Usage (Vendor Usage 1)
 0xa1, 0x01,		// M Collection (Application)
 0x15, 0x00,		// G  Logical Minimum (0)
 0x26, 0xff, 0x00,	// G  Logical Maximum (255)
 0x75, 0x08,		// G  Report Size (8)

 0x85, 0x01,		// G  Report ID (1)
 0x95, 0x06,		// G  Report Count (6)
 0x09, 0x21,		// L  Usage (Programming Info)
 0xb2, 0x03, 0x01,	// M  Feature (Const,Data,Var,Abs,Buf)

 0x85, 0x02,		// G  Report ID (2)
 0x95, 0x83,		// G  Report Count (131)
 0x09, 0x22,		// L  Usage (Programming Data)
 0xb2, 0x02, 0x01,	// M  Feature (Data,Var,Abs,Buf)
 0xc0};			// M End Collection

// allow compatibility with avrusbboot's bootloaderconfig.h:
#ifdef BOOTLOADER_INIT
# define bootLoaderInit()	BOOTLOADER_INIT
#endif
#ifdef BOOTLOADER_CONDITION
# define bootLoaderCondition()	BOOTLOADER_CONDITION
#endif

// compatibility with ATMega88 and other new devices:
#ifndef TCCR0
#define TCCR0	TCCR0B
#endif
#ifndef GICR
#define GICR	MCUCR
#endif

static void leaveBootloader() {
 cli();
 boot_rww_enable();
 USB_INTR_ENABLE = 0;
 USB_INTR_CFG = 0;	// also reset config bits
#if F_CPU == 12800000
 TCCR0 = 0;		// reset to default values
 TCNT0 = 0;
#endif
 GICR = (1 << IVCE);	// enable change of interrupt vectors
 GICR = (0 << IVSEL);	// move interrupts to application flash section
 asm volatile("rjmp vectors-0x1800");
}

uchar usbFunctionSetup(uchar data[8]) {
 usbRequest_t *rq = (void *)data;
 static PROGMEM struct __attribute__((packed)){
  uchar reportID;
  unsigned spm_pagesize;
  unsigned long flashend;
 }replyBuffer = {1, SPM_PAGESIZE, 0x1800};

 if (rq->bRequest == USBRQ_HID_SET_REPORT) {
  if (rq->wValue.bytes[0] == 2) {
   offset = 0;
#if F_CPU == 12800000	// dim LED and stop tuning while writing to flash
   DDRB |= 1<<2;	// Too lengthy cli() sections?
#endif			// But this helps against communication losses! 110825
   return USB_NO_MSG;
  }
#if BOOTLOADER_CAN_EXIT
  else{
   eeprom_write_byte(0,0xFF);
   exitMainloop = 1;
  }
#endif
 }else if (rq->bRequest == USBRQ_HID_GET_REPORT) {
  usbMsgFlags = USB_FLG_MSGPTR_IS_ROM; 
  usbMsgPtr = (uchar*)&replyBuffer;
  return 7;
 }
 return 0;
}

static void Delay2ms() {
 wdt_reset();
 _delay_ms(2);	// This additional wait ensures that data is written correctly.
}		// With no delay, a verify will fail for about 50 % (110825)

uchar usbFunctionWrite(uchar *data, uchar len) {
 union{
  addr_t  l;
  uint s[sizeof(addr_t)/2];
  uchar c[sizeof(addr_t)];
 }address;
 uchar isLast;

 address.l = currentAddress;
 if (!offset) {
  address.c[0] = data[1];
  address.c[1] = data[2];
#if (FLASHEND) > 0xffff	// we need long addressing
  address.c[2] = data[3];
  address.c[3] = 0;
#endif
  data += 4;
  len -= 4;
 }
 offset += len;
 isLast = offset & 0x80;	// != 0 if last block received
 do{
  addr_t prevAddr;
  if (!(address.s[0] & (SPM_PAGESIZE-1))){	// if page start: erase
#ifndef TEST_MODE
   cli();
   boot_page_erase(address.l);     // erase page
   sei();
   boot_spm_busy_wait();           // wait until page is erased
   Delay2ms();	// additional wait compensating the too-high OSCCAL value
#endif
  }
  cli();
  boot_page_fill(address.l, *(short *)data);
  sei();
  prevAddr = address.l;
  address.l += 2;
  data += 2;
// write page when we cross page boundary
  if (!(address.s[0] & (SPM_PAGESIZE-1))) {
#ifndef TEST_MODE
   cli();
   boot_page_write(prevAddr);
   sei();
   boot_spm_busy_wait();
   Delay2ms();	// additional wait compensating the too-high OSCCAL value
#endif
  }
  len -= 2;
 }while(len);
 currentAddress = address.l;
 return isLast;
}

static void initForUsbConnectivity(void) {
 uchar i = 0;
#if F_CPU == 12800000
 OSCCAL = 0xCF;		// boost a bit
 TCCR0 = 3;		// 1/64 prescaler
#endif
 usbInit();
// enforce USB re-enumerate, can be omitted if code space is tight
 usbDeviceDisconnect();  // do this while interrupts are disabled
 do Delay2ms();		// fake USB disconnect for > 250 ms
 while(--i);
 usbDeviceConnect();
 sei();
}

int main(void) __attribute__((naked,noreturn));
int main(void) {
// continue initialization that does not fit into interrupt vector table
 asm volatile(
"	clr	r1\n"
"l1:	st	-Z,r1\n"	// clear entire BSS
"	cpi	r30,0x60\n"	// this code is ATmega8 specific
"	cpc	r31,r1\n"
"	brne	l1");
// initialize two bytes of data declared inside USBDRV.C,
// because there is no data copy routine (this here is shorter)
 usbTxLen = USBPID_NAK;	
 usbMsgLen = USB_NO_MSG;
// initialize hardware
 bootLoaderInit();
// jump to application if EEPROM value is not set
 if (bootLoaderCondition()) {
#ifndef TEST_MODE
  GICR = (1 << IVCE);	// enable change of interrupt vectors
  GICR = (1 << IVSEL);	// move interrupts to boot flash section
#endif
  initForUsbConnectivity();
  do{	// main event loop
   wdt_reset();
   usbPoll();
#if BOOTLOADER_CAN_EXIT
   if (exitMainloop && eeprom_is_ready()) {
//#if F_CPU != 12800000	// memory is tight at 12.8 MHz, save luxury stuff
    static uchar i;
    if (!--i)	// delay 256 iterations to allow for USB reply to exit command
//#endif
    break;
   }
#endif
  }while(bootLoaderCondition());
 }
 leaveBootloader();
}
