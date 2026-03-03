/* Firmware für AT90USB162 im USB2LPT12, haftmann#software 1/14
 * Zu übersetzen und zu brennen mit zugehörigem »makefile«,
 * bspw. »make« zum Kompilieren, »make flash« zum Brennen (ggf. mit Kompilation),

*140117	Aller Anfang.

Hardware:

PortB = Statusport
14	PB0	(!SS)	S0		21
15	PB1	(SCLK)	S1		22
16	PB2	(MOSI)	S2		23
17	PB3	(MISO)	S3	!ERR	15
18	PB4	(T1)	S4	ONL	13
19	PB5		S5	PE	12
20	PB6		S6	!ACK	10
21	PB7	(OC0A)	S7!	BSY	11
PortC = (zumeist) Steuerport
2	PC0	XTAL2	Quarz
24	PC1	!RESET	Notnagel zum Programmieren des Bootloaders
5	PC2		!LED
-	PC3		(existiert nicht)
26	PC4		C0!	!STB	1
25	PC5	(OC1B)	C1!	!AF	14
26	PC6	(OC1A)	C2	!INI	16
22	PC7	(ICP1)	C3!	!SEL	17
PortD = Datenport
6	PD0	(OC0B)	D0		2
7	PD1	(AIN0)	D1		3
8	PD2	(RxD)	D2		4
9	PD3	(TxD)	D3		5
10	PD4		D4		6
11	PD5	(XCK)	D5		7
12	PD6	(!RTS)	D6		8
13	PD7	HWB	D7		9	Bootloader erzwingen
Übrige Pins des Mikrocontrollers:
4, 31, 32 - Betriebsspannung 5V
27 - Anschluss Stützkondensator für 3,3 V USB-Betriebsspannung
29, 30 - USB-Datenleitungen D+, D-
1, 2 - Quarz
3, 28 - Masse

Die Zuordnung zu den Portpins erfolgte nach der Maßgabe,
Portadressen nicht aufzuteilen, um bei Ausgaben mit zwei Pegelwechseln
diese Pegelwechsel exakt gleichzeitig erscheinen zu lassen.
*/

#include "usb.h"
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <string.h>	// memcpy
#include <util/delay.h>	// _delay_us

// Firmware-Datum (Meldung bei A3-Request mit wData==6 und wLength==2)
#define DATEYEAR	2014
#define DATEMONTH	2
#define DATEDAY		17

#define FATDATE (((DATEYEAR)-1980)*512+(DATEMONTH)*32+(DATEDAY))
#define BCDDATE (((DATEYEAR)-2000)/10<<12)+(((DATEYEAR)-2000)%10<<8)+((DATEMONTH)/10<<4)+(DATEMONTH)%10

/* USB */

typedef union{
 unsigned word;
 uchar bytes[2];
}usbWord_t;

struct usbRequest_t{
 uchar bmRequestType;
 uchar bRequest;
 usbWord_t wValue;
 usbWord_t wIndex;
 usbWord_t wLength;  
};

// Gesamtgröße: 176 Byte, hier 8+64+64+8+32

#define EP0_SIZE	8
#define OUT_EP		1	// USB2LPT native Mikrocode
#define OUT_EP_SIZE	64	// BULK, einfach
#define IN_EP		2	// USB2LPT native Ergebnisphase
#define IN_EP_SIZE	64	// BULK, einfach
#define HID_EP		3	// HID-Interface
#define HID_EP_SIZE	8	// INTERRUPT, einfach
#define PRN_EP		4	// USBPRN
#define PRN_EP_SIZE	16	// BULK, doppelt


#define USBPID_DATA0		0xC3
#define USBPID_DATA1		0x4B
#define USBRQ_HID_GET_REPORT	0x01
#define USBRQ_HID_SET_REPORT	0x09
#define USBDESCR_DEVICE		1
#define USBDESCR_CONFIG		2
#define USBDESCR_STRING		3
#define USBDESCR_INTERFACE	4
#define USBDESCR_ENDPOINT	5
#define USBDESCR_HID		0x21
#define USBDESCR_HID_REPORT	0x22
#define USB_NO_MSG		255	/* constant meaning "no message" */
#define USBPID_NAK		0x5A

uchar usbCurrentDataToken;
uchar usbSofCount;
uchar*usbMsgPtr; 
uchar usbTxLen;
uchar usbMsgLen;

/* Parallelport */
static uchar data_byte;
static uchar DCR;	// Device Control Register (SPP +2)
#define DIRECTION_INPUT (DCR&0x20)
static uchar ECR;	// Extended Control Register (ECP +402)
static uchar ECR_Bits;	// 1-aus-n-Code der höchsten 3 Bits aus ECR
static uchar EppTimeOut;// Bit0 = 0: kein Timeout aufgetreten
			// Bit2 = 0: Interrupt aufgetreten (zz. ungenutzt)
			// Alle anderen Bits müssen =1 sein!
static struct {
 uchar ecr;
 uchar unused;		// bei Low-Speed: osccal
 uchar Feature;		// Bit0 = Offene-Senke-Simulation für Daten (SPP +0)
			// Bit2 = Offene-Senke-Simulation für Steuerport (+2)
			// Bit4 = Seriennummer via USB-Deskriptor
			// Bit6 = DirectIO (keine Invertierungen)
 ulong SerialNumber;
} g;
/* FIFO */
#define FIFOSIZE 16	// muss laut Programmlogik Vielfaches von 2 sein
static NOINIT unsigned Fifo[FIFOSIZE];
				//wegen ECP brauchen wir 9 bit Breite
static uchar fifor, fifow;	//Indizes

/* sonstiges */
static uchar Led_T;	// "Nachblinkzeit" der LED in ms, 0 = LED blinkt nicht
static uchar Led_F;	// "Blinkfrequenz" in ms (halbe Periode)
static uchar Led_C;	// Blink-Zähler (läuft alles über SOF-Impuls)

// Ein WORD in FIFO schreiben - nichts tun wenn FIFO voll
static void PutFifo(unsigned w) {
 uchar idx, ecr = ECR;		// mit Registern (nicht RAM) arbeiten
 if (ecr & 2) return;		// nichts tun wenn FIFO voll
 idx = fifow;			// in Register holen
 Fifo[idx++] = w;		// einschreiben
 idx &= FIFOSIZE - 1;		// if (idx >= FIFOSIZE) idx = 0;
 ecr &= ~1;			// FIFO nicht leer
 if (idx == fifor) ecr |= 2;	// FIFO voll
 fifow = idx;			// Register rückspeichern
 ECR = ecr;
}

// ein WORD aus FIFO lesen - letztes WORD liefern wenn FIFO leer
// (Ein echtes ECP tut das auch!)
static unsigned GetFifo(void) {
 uchar idx = fifor, ecr = ECR;	// mit Registern (nicht RAM) arbeiten
 unsigned w;
 if (ecr & 1) {			// bei leerer FIFO vorhergehendes WORD liefern
  idx--;
  idx &= FIFOSIZE - 1;		// if (idx >= FIFOSIZE) idx = FIFOSIZE - 1;
  w = Fifo[idx];
 }else{
  w = Fifo[idx++];
  idx &= FIFOSIZE - 1;		// if (idx >= FIFOSIZE) idx = 0;
  ecr &= ~2;			// FIFO nicht voll
  if (fifow == idx) ecr |= 1;	// FIFO leer
  fifor = idx;			// Register rückspeichern
  ECR = ecr;
 }
 return w;
}

// Datenrichtungswechsel von DCR Bit 5 wirksam werden lassen,
// dabei Simulation der Offenen Senke (Open Collector) beachten
static void LptSetDataDir(void) {
 if (DIRECTION_INPUT) {
  DDRD  = 0x00;		// alles EINGÄNGE
  PORTD = 0xFF;		// alle Pull-Ups EIN
 }else{
  PORTD = data_byte;	// Datenbyte ausgeben
  DDRD  = g.Feature & 0x01 ? ~PORTD : 0xFF;
 }
}

// Aktivieren der Parallelschnittstelle bei USB-Aktivität
// oder Rückstellen beim Löschen des DirectIO-Bits im Feature-Register
static void LptOn(void) {
 LptSetDataDir();	// Datenport: Ausgänge (oder nur Pullups)
 DDRB  = 0x07;		// Statusport: nur S2:0 sind Ausgang
 PORTB = 0xF8;		// Statusport: fünf Pullups
 PORTC = PORTC&0x0F | (DCR^0x0B)<<4;
 DDRC  = 0xF4;		// LED-Anschluss bleibt Ausgang
}

// <strobe>-Low-Bits auf Steuerport ausgeben und max. 10 µs auf WAIT=H warten
// Liefert Daten eines READ-Zyklus'
// Ausgabedaten müssen vorher auf PORTD gelegt werden.
// Bei Fehler wird das TimeOut-Bit gesetzt
// (Gelöscht wird es durch Schreiben einer Null aufs Statusport.)
static uchar epp_io(uchar strobe) {
 uchar i, saveoe = DDRD, save = PORTC;
 if (PINB & 0x80) goto n_ok;	// BUSY
 PORTC = save & strobe;	// ASTROBE bzw. DSTROBE sowie ggf. WRITE auf LOW
 if (!(strobe & 0x04)) DDRD = 0xFF;	// Ausgabe (jetzt erst)
 i = 24; do{
  if (PINB & 0x80) goto okay;
 }while (--i);
n_ok:
 EppTimeOut |= 0x01;		// TimeOut markieren
okay:
 i = PIND;			// Daten einlesen (nur für INPUT relevant:-)
 if (!(strobe & 0x04)) DDRD = saveoe;
 PORTC = save;
 return i;
}

// === OUT auf Adresse +0 (Datenport) ===
static void out0(uchar b) {
 uchar t = ECR_Bits & 0x4C;	// Compiler macht int-Murks ohne Hilfsvariable
 if (t) PutFifo(b);		// eine FIFO-Betriebsart
 else{
  data_byte = b;
  if (!DIRECTION_INPUT) {
   PORTD = b;
   if (g.Feature & 1) DDRD = ~b;	// Simulation OC (mit Pullups)
  }
 }
}

// === OUT auf Adresse +1 (Statusport) ===
// Eine Ausgabe erfolgt nur bei DirectIO = 1 oder bei (via out405)
// auf Ausgabe geschalteten Leitungen.
// Ansonsten keine Ausgabe; Pullups bleiben eingeschaltet
static void out1(uchar b) {
 if (ECR_Bits & 0x10 && !(b & 0x01)) EppTimeOut &=~ 0x01;	// TimeOut-Bit
 if (!(g.Feature & 0x40)) {
  b ^= 0x80;		// BSY-Bit invertieren
  b |= ~DDRB;		// Ausgabe nur bei DirectIO oder vorhandenen Ausgabepins
 }
 PORTB = PORTB&0x07 | b&0xF8;
}

// === OUT auf Adresse +2 (Steuerport) ===
// Bit4=IRQ-Freigabe (nicht unterstützt, aber gespeichert)
// Bit5=Ausgabetreiber-Freigabe
static void out2(uchar b) {
 uchar t = ECR_Bits & 0x05;
 b |= 0xC0;
 if (t) b &=~ 0x20;	// Richtung auf Ausgabe fixieren
 t = b^DCR;	// geänderte Bits
 DCR = b;
 if (!(g.Feature & 0x40)) {	// kein DirectIO?
  if (t & 0x20) LptSetDataDir();// bei Richtungswechsel
  b ^= 0x0B;			// Invertierungen anpassen
 }
 b <<= 4;
 PORTC = PORTC&0x0F | b;
 if (g.Feature & 0x04) DDRC = ~b | 0x04;	// LED-DDR-Bit muss 1 bleiben
}

// === OUT auf Adresse +3 (EPP-Adresse) ===
static void out3(uchar b) {
 PORTD = b;
 epp_io(~0x24);			// AddrStrobe und Write LOW (0xx0)
}

// === OUT auf Adresse +4 (EPP-Daten) ===
static void out4(uchar b) {
 PORTD = b;
 epp_io(~0x0C);			// DataStrobe und Write LOW (xx00)
}

// === OUT auf Adresse +400 (ECP-Daten-FIFO) ===
static void out400(uchar b) {
 uchar t = ECR_Bits & 0x4C;
 if (t) PutFifo(b | 0x100);	// Datenbyte einschreiben
}

// === OUT auf Adresse +402 (ECP-Steuerport) ===
static void out402(uchar b) {	// svw. SetECR
 uchar t;
 ECR = (b & 0xF8) | 0x05;	// FIFOs leeren
 b >>= 5;
 t = 1;
 t <<= b;
 ECR_Bits = t;			// 1-aus-n-Kode setzen
 EppTimeOut = t & 0x10 ? 0xFE : 0xFF;	// Timeout-Bit voreinstellen
 if (t & 0x05) {		// SPP oder SPP-FIFO?
  if (DIRECTION_INPUT) {
   DCR &=~ 0x20;		// Richtung fest auf AUSGABE
   LptSetDataDir();
  }
 }
 fifor = fifow = 0;
}

// === OUT auf Adresse +404 (Datenrichtung Datenport) [HINTERTÜR] ===
static void out404(uchar b) {
 DDRD = b;
}

// === OUT auf Adresse +405 (Datenrichtung Statusport) [HINTERTÜR] ===
static void out405(uchar b) {
 DDRB = b;
 if (!(g.Feature & 0x40)) {	// ohne DirectIO Pullups sicherstellen!
  PORTB |= ~b;
 }
}

// === OUT auf Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
static void out406(uchar b) {
 DDRB = b<<4 | 0x04;		// LED-Anschluss stets Ausgang
}

// === OUT auf Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
static void out407(uchar b) {
 uchar t;
 b &= 0x55;			// nur unterstützte Bits durchlassen
 t = g.Feature ^ b;
 g.Feature = b;
 if (t) {
// BrennFeature()
  if (t & 0x01 && !(b & 0x40) && !DIRECTION_INPUT) {
   DDRD = b & 0x01 ? ~PORTD : 0xFF;
  }
  if (t & 0x04 && !(b & 0x40)) {
   DDRC = b & 0x04 ? ~PORTC & 0xF0 | 0x04 : 0xF4;
  }
  if (t & 0x40 && !(b & 0x40)) {	// Portrichtungen wiederherstellen
   LptOn();
  }
 }
}
 
// Der Teufel hat ECP erfunden! Wie soll festgestellt werden, ob in der
// FIFO ein Adress- oder ein Datenbyte liegt?
// Mein "echtes" EC-Port ignoriert beim Rücklesen einfach das neunte Bit.
// === IN von Adresse +0 (Datenport) ===
static uchar in0(void) {
 uchar t = ECR_Bits & 0x4C;
 if (t) t = GetFifo();
 else t = PIND;			// ansonsten stets Portpins lesen
 return t;
}

// === IN von Adresse +1 (Statusport) ===
static uchar in1(void) {
 uchar t = PINB;
 t |= 0x07;
 if (!(g.Feature & 0x40)) t ^= 0x80;
 t &= EppTimeOut;
 return t;
}

// b==0: Echtes PIN-Rüclesen, sonst PORT-Rücklesen
static uchar internal_in2(uchar b) {
 b = b ? PORTC : PINC;
 b >>= 4;
 b &= 0x0F;
 if (!(g.Feature & 0x40)) b ^= 0x0B;
 b |= DCR & 0xF0;
 return b;
}

// === IN von Adresse +2 (Steuerport) ===
// Überraschung: Bei einem "genügend neuen" echten Parallelport sind die
// Steuerleitungen gar nicht (mehr) rücklesbar!
// Hier wird diese Einschränkung nur in den 3 FIFO-Betriebsarten nachgeahmt
static uchar in2(void) {
 return internal_in2(ECR_Bits & 0x4C);
}

// === IN von Adresse +3 (EPP-Adresse) ===
static uchar in3(void) {
 return epp_io(~0x20);		// AddrStrobe LOW (0xxx)
}

// === IN von Adresse +4 (EPP-Daten) ===
static uchar in4(void) {
 return epp_io(~0x08);		// DataStrobe LOW (xx0x)
}

// === IN von Adresse +400 (ECP-FIFO) ===
static uchar in400(void) {
 uchar t = ECR_Bits & 0xCC;
 if (!t) return 0xFF;
 if (t & 0x80) return 0x10;	// Konfigurationsregister A (konstant)
 return GetFifo();
}

// === IN von Adresse +401 (ECP-???) ===
static uchar in401(void) {
 return ECR_Bits & 0x80 ? 0 : 0xFF;	// Konfigurationsregister A (0)
}

// === IN von Adresse +402 (ECP-Steuerport) ===
static uchar in402(void) {
 return ECR;
}

// === IN von Adresse +404 (Datenrichtung Datenport) [HINTERTÜR] ===
static uchar in404(void) {
 return DDRD;
}

// === IN von Adresse +405 (Datenrichtung Statusport) [HINTERTÜR] ===
static uchar in405(void) {
 return DDRB;
}

// === IN von Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
static uchar in406(void) {
 return DDRC>>4;
}

// === IN von Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
static uchar in407(void) {
 return g.Feature;
}

// === WARTEN (blockieren) in 4-µs-Stückelung ===
static void wait(uchar us) {
 us++;
 do{
  _delay_us(4-3E6/F_CPU);	// 4 µs minus 3 CPU-Takte
 }while (--us);		// dec R16; brnz lbl = 3 Takte
}

static uchar getbyte(uchar a) {
 a&=0xF8;
 if (a==0x00) return in0();
 if (a==0x08) return PORTD;
 if (a==0x10) return in1();
 if (a==0x18) {a=PORTC; a<<=2; a|=7; return a;}
 if (a==0x20) return in2();
 if (a==0x28) return internal_in2(1);
 a&=0xF0;
 if (a==0xA0) return in402();
 if (a==0x30) return in3();
 if ((a&0xCF)==0x40) return in4();
 if (a==0x80) return in400();
 if (a==0xC0) return in404();
 if (a==0xD0) return in405();
 if (a==0xE0) return in406();
 if (a==0xF0) return in407();
 return 0xFF;
}

static void setbyte(uchar a, uchar b) {
 a&=0xF0;
 if (a==0x00) out0(b);
 else if (a==0x10) out1(b);
 else if (a==0x20) out2(b);
 else if (a==0xA0) out402(b);
 else if (a==0x30) out3(b);
 else if ((a&0xCF)==0x40) out4(b);
 else if (a==0x80) out400(b);
 else if (a==0xC0) out404(b);
 else if (a==0xD0) out405(b);
 else if (a==0xE0) out406(b);
 else if (a==0xF0) out407(b);
}

static void setbit(uchar a) {
 uchar m = 1;
 m <<= a&7;
 setbyte(a,getbyte(a)|m);
}

static void resbit(uchar a) {
 uchar m = 1;
 m <<= a&7;
 setbyte(a,getbyte(a)&~m);
}

static void cplbit(uchar a) {
 uchar m = 1;
 m <<= a&7;
 setbyte(a,getbyte(a)^m);
}

static void waitbit(uchar a) {
 uchar t=usbSofCount;
 uchar m = 1;
 uchar x = 0;
 m <<= a&7;
 if (a&8) x--;
 while ((getbyte(a)^x)&m) if (t!=usbSofCount) break;
}

// Zyklischer Aufruf; Emulation der SPP-Ausgabefifo
static void SppXfer(void) {
 if (ECR & 1) return;		// bei leerer FIFO nichts tun
 if (PINC & 0x20) return;	//  BSY = H: nichts tun
 PORTD = GetFifo();
 PORTC &= ~0x10;		// /STB = L
 wait(1);
 PORTC |=  0x10;		// /STB = H
}

// Zyklischer Aufruf; Emulation der ECP-Ein/Ausgabefifo
static void EcpXfer(void) {
 unsigned w;
 if (DIRECTION_INPUT) {		// FIFO-Eingabe
  if (ECR & 2) return;		// bei voller FIFO nichts tun
  if (!(PORTC & 0x20)) {	//  /AF = L: Byte einlesen?
   if (PINB & 0x40) return;	// /ACK = H: kein Byte von außen vorhanden
   PORTC |= 0x20;		//  /AF = H setzen
  }				//  /AF = H: hier zweite Handshake-Phase
  if (!(PINB & 0x40)) return;	// /ACK = L: nichts tun!
  w = PIND;
  if (PINB & 0x80) w|=0x0100;	//  BSY = Command(0) / Data(1)
  PutFifo(w);
  PORTC &= ~0x20;		//  /AF = L setzen
 }else{				// FIFO-Ausgabe
  if (PORTC & 0x10) {		// /STB = H: Byte ausgeben?
   uchar t;
   if (ECR & 1) return;		// bei leerer FIFO nichts tun
   if (PINB & 0x80) return;	//  BSY = H: nichts tun!
   w = GetFifo();		// Adress- oder Datenbyte lesen
   PORTD = (uchar)w;		// Byte anlegen
   t = PORTC & 0xD0;
   if (w & 0xFF00) t |= 0x20;	// AF setzen oder nicht
   PORTC = t;
   PORTC &= ~0x10;		// /STB = L setzen
  }				// /STB = L: hier zweite Handshake-Phase
  if (!(PINB & 0x80)) return;	//  BSY = L: nichts tun!
  PORTC |= 0x10;		// /STB = H setzen
 }
}

// LED starten mit Blinken, t = Periodendauer
void Led_Start(uchar t) {
 if (!Led_T) {
  Led_C = t;
  PORTC |= 0x04;	// LED ausschalten
 }
 Led_T = Led_F = t;
}

// LED-Zustand alle 1 ms (SOF) aktualisieren
static void Led_On1ms(void) {
 uchar led_t = Led_T, led_c;
 if (!led_t) return;
 led_c = Led_C;		// in Register laden
 if (!--led_c) {
  PORTC ^= 0x04;	// LED umschalten
  led_c = Led_F;	// Zähler neu laden
 }
 if (!--led_t) PORTC &= ~0x04;	// LED einschalten
 Led_T = led_t;		// Register rückschreiben
 Led_C = led_c;
}

static void initIOPorts(void) {
 data_byte = 0x00;
 DCR = 0xCC;
}

// Ausgabedaten bearbeiten und Ergebnisbytes in <buf> aufsammeln,
// liefert Anzahl der nach <buf> geschriebenen Bytes.
// Behandelt auch über Puffergrenzen hinauslaufende 2-Byte-Kommandos
static uchar ProcessInOut(const uchar *data, uchar len, uchar *buf) {
 static uchar PendingByte;
 uchar *InData = buf, command, value;
 if (!len) return 0;
 Led_Start(100);	// schnelles Blinken (5 Hz)
 do{
  command = PendingByte;
  if (command) {
   command &= ~0x10;	// eh' nur 2-Byte-Ausgabekommando
   PendingByte = 0;
   len++;
  }else command = *data++;
  if (command & 0x10) {	// IN-Kommandos
   value = 0xFF;
   if (command==0x10) value = in0();
   else if (command==0x11) value = in1();
   else if (command==0x12) value = in2();
   else if (command==0x13) value = in3();
   else if (command>=0x14
          && command<0x18) value = in4();
   else if (command==0x18) value = in400();
   else if (command==0x19) value = in401();
   else if (command==0x1A) value = in402();
   else if (command==0x1C) value = in404();
   else if (command==0x1D) value = in405();
   else if (command==0x1E) value = in406();
   else if (command==0x1F) value = in407();
   *InData++ = value;	// IN-Kommandos erzeugen Antwort-Bytes
  }else{		// OUT-Kommandos mit Folgebyte
   if (!--len) {	// Wird erst beim nächsten ProcessInOut verarbeitet
    command |= 0x10;
    PendingByte = command;	// merken für Fortsetzung
    break;		// mit len==0 aus Schleife ausbrechen
   }
   value = *data++;
   if      (command==0x00) out0(value);
   else if (command==0x01) out1(value);
   else if (command==0x02) out2(value);
   else if (command==0x0A) out402(value);
   else if (command==0x03) out3(value);
   else if (command>=0x04
          && command<0x07) out4(value);
   else if (command==0x08) out400(value);
   else if (command==0x0C) out404(value);
   else if (command==0x0D) out405(value);
   else if (command==0x0E) out406(value);
   else if (command==0x0F) out407(value);
   else if (command==0x20) wait(value);
   else if (command==0x21) setbit(value);
   else if (command==0x22) resbit(value);
   else if (command==0x23) cplbit(value);
   else if (command==0x24) waitbit(value);
  }
 }while (--len);
 return InData - buf;	// Geschriebene Bytes liefern
}

// Zeiger zum Lesen (nur für gestückelte HID-Feature-Requests) und Schreiben
static uchar ReplyIdxR, ReplyIdxW;
// Antwort-Puffer (nicht überlauf-geschützt!)
// Für alle Arten von Setup-Transfers
static NOINIT uchar ReplyBuf[128];

// Häufige Art der Mikrocode-Verarbeitung
static void HidProcessInOut(const uchar*data, uchar len) {
 ReplyIdxW += ProcessInOut(data, len, ReplyBuf+2+ReplyIdxW);
}

static uchar gbRequest;		// Kopie vom letzten SETUP-Paket
static NOINIT size_t gAdr, gLen;


static NOINIT uchar replyBuf[128];

uchar usbFunctionSetup(uchar data[8]) {
 gbRequest = data[1];		// SETUPDAT merken (wie Cypress-Controller)
 gAdr = ((usbRequest_t*)data)->wValue.word;
 gLen = ((usbRequest_t*)data)->wLength.word;

// Rudiment von Henning Paul, für seinen Linux-Treiber
 usbMsgPtr = replyBuf;
// SetFeature -> Stall -> EP1OUT: Data-Toggle rücksetzen
 if (data[0]==0x02 && data[1]==0x03 && data[2]==0x00 && data[4]==0x01) {
  ExpectEP1OutToken=USBPID_DATA0;
 }else if (data[0]==0x21 && data[1]==USBRQ_HID_SET_REPORT && data[4]==0x01) {
  return 0xFF;		// usbFunctionWrite() kümmert sich 
 }else if (data[0]==0xA1 && data[1]==USBRQ_HID_GET_REPORT && data[4]==0x01
   && ReplyIdxW) {
// Die ersten zwei Bytes werden nicht für die Antwortbytes benutzt.
  usbMsgPtr = ReplyBuf+ReplyIdxR+1;	// Report-ID = Länge, danach Datenbytes
  *(uchar*)usbMsgPtr = data[2];		// gewünschte Report-ID = Länge eintragen
  ReplyIdxR += data[2];
  if (ReplyIdxR>=ReplyIdxW) ReplyIdxR=ReplyIdxW=0;	// alles leer
  return data[6];		// Anzahl Bytes 
 }else if (data[0] == 0xC0) {	// IN, Vendor, Device
// Ehemalige BULK-Transfers
  if (data[1] >= 0x90 && data[1] <= 0x94) {
// bis zu 4 Bytes von SETUPDAT als OUT-Daten verarbeiten
// (Speed! - Für übliche geringe Ausgabemengen kein extra SETUP-Transfer)
   HidProcessInOut(data+2, data[1]-0x90);
   gbRequest = 0xA1;		// vom RAM lesen (lassen)
   gAdr = (size_t)(ReplyBuf+2);	// feste Speicheradresse
   if (gLen > ReplyIdxW) gLen = ReplyIdxW;
   ReplyIdxR=ReplyIdxW=0;	// für's nächste ProcessInOut sei ReplyBuf leer
// Cypress' EZUSB-kompatible Routinen (VendAx.hex)
  }else if (data[1] >= 0xA1 && data[1] <= 0xA3) {	// lesen
// im Sonderfall Adresse=6 und Länge=2 wird das Datums-WORD (FAT) geliefert
   if (data[1] == 0xA3 && gAdr == 6 && gLen == 2) {
    ((usbWord_t*)replyBuf)->word = FATDATE;
    return 2;
   }
   return 0xFF;			// usbFunctionRead() kümmert sich
  }
 }else if (data[0] == 0x40) {	// OUT, Vendor, Device
  if (data[1] >= 0x90 && data[1] <= 0x94) {
// bis zu 4 Bytes von SETUPDAT als OUT-Daten verarbeiten (Bandbreite sparen)
   HidProcessInOut(data+2, data[1]-0x90);
   gbRequest = 0x90;	// alle weiteren OUT-Daten in usbFunctionWrite abarbeiten lassen
  }else if (data[1] == 0xA1 || data[1] == 0xA2) {	// RAM oder EEPROM schreiben
   return 0xFF;		// usbFunctionWrite() kümmert sich
  }
#if 1
 }else	// Ballast!! Welchen Wert hat bmRequestType?
     if(data[1] == 0){       /* ECHO */
        replyBuf[0] = data[2];
        replyBuf[1] = data[3];
        return 2;
    }else if(data[1] == 1){       /* READ_REG */
	if (data[2] == 0){
            replyBuf[0] = in0();
            return 1;
	} 
	else if (data[2] == 1){
            replyBuf[0] = in1();
            return 1;
	}
	else if (data[2] == 2){
	    replyBuf[0] = in2();
            return 1;
	}
    }else if(data[1] == 2){       /* WRITE_REG */
	if (data[2] == 0){
		out0(data[4]);
        }
	else if (data[2] == 2){
		out2(data[4]);
        }
#endif
 }
 return 0;
}

// Datenübertragungsfunktionen für Cypress-kompatible "lange" Setup-Transfers
// sowie für ehemals Bulk-Daten
uchar usbFunctionRead(uchar *data, uchar len) {
 void *adr = (void*)gAdr;	// im Register halten
 if (len > gLen) len = gLen;	// begrenzen auf Restlänge
 if (gbRequest == 0xA1) {	// RAM (oder speziell ReplyBuf) lesen
  memcpy(data, adr, len);
 }else if (gbRequest == 0xA2) {	// EEPROM lesen
  eeprom_read_block(data, adr, len);
 }else if (gbRequest == 0xA3) {	// Flash lesen
  memcpy_P(data, (PGM_P)adr, len);
 }else return 0xFF;		// Fehler: nichts zu liefern! (STALL EP0)
 gAdr = (size_t)adr + len;
 gLen -= len;
 return len;
}

uchar usbFunctionWrite(uchar *data, uchar len) {
 uchar *adr = (uchar*)gAdr;	// im Register halten
 if (len > gLen) len = gLen;	// begrenzen auf Restlänge
 if (gbRequest == 0xA1) {	// RAM schreiben
  memcpy(adr, data, len);
  adr += len;
 }else if (gbRequest == 0xA2) {	// EEPROM schreiben
  uchar ll;
  for (ll = 0; ll < len; ll++) {
   eeprom_write_byte(adr, *data++);	// statt eeprom_write_block()
   wdt_reset();			// nach jedem Byte Watchdog beruhigen
   adr++;
  }
 }else if (gbRequest == 0x90) {	// OUT-Daten über EP0 (Vista/Linux-Kompatibilität)
  HidProcessInOut(data, len);
 }else if (gbRequest == USBRQ_HID_SET_REPORT) {
  HidProcessInOut(data+1, data[0]);
 }else return 0xFF;		// Fehler: unerwartete Daten (STALL EP0)
 gAdr = (size_t)adr;
 gLen -= len;
 return len;
}

void usbSetInterrupt(uchar*, uchar);

void usbFunctionWriteOut(uchar *data, uchar len) {
 uchar tmp=ExpectEP1OutToken;
 if (usbCurrentDataToken==tmp) {
  ExpectEP1OutToken=tmp^USBPID_DATA0^USBPID_DATA1;
  uchar InDataLen = ProcessInOut(data, len, ReplyBuf);
  if (InDataLen) usbSetInterrupt(ReplyBuf, InDataLen);	// Antwort zum Host
 }
}

#include <stdlib.h>

#define FETCH_BYTE(src) pgm_read_byte(src)
#define FETCH_BLOCK(d,s,l) memcpy_P(d,s,l);
// Solange der Flash noch Platz hat, kommen die Deskriptoren in den Flash.
// Erst wenn's knapp wird, kommen diese in den EEPROM, wie bei usb2lpt5.
// Den kann aber der (derzeitige) Bootloader nicht ansprechen …

// Generiert USB-String-Deskriptor (mit UTF-16), hier nur 11-Bit-UTF-16
static uchar BuildStringFromUTF8(const char *src) {
 wchar_t *d=(wchar_t*)(replyBuf+2);
 uchar len;
 for (;;) {
  wchar_t c=FETCH_BYTE(src++);
  if (!c) break;
  if (c&1<<7) {
   c=c<<6&0xFFF | FETCH_BYTE(src++)&0x3F;
// if (c&1<<11) c=c<<6|FETCH_BYTE(src++)&0x3F;
  }
  *d++=c;
 }
 len=(uchar)((uchar*)d-replyBuf);
 replyBuf[0]=len;
 replyBuf[1]=USBDESCR_STRING;		// Länge und ID für String-Deskriptor
 return len;
}

// itoa() ist zu fett! Daher diese Ersatzroutine.
static uchar hexDigit(uchar nibble) {
 if (nibble>=10) nibble+=7;
 nibble+='0';
 return nibble;
}

static uchar BuildStringFromSerial() {
 int i;
 uchar *d=replyBuf;
 *d++=18;
 *d++=USBDESCR_STRING;		// Länge und ID für String-Deskriptor
 for (i=3; i>=0; i--) {
  uchar c=((uchar*)&g.SerialNumber)[i];
  *d++=hexDigit(c>>4);
  *d++=0;
  *d++=hexDigit(c&0x0F);
  *d++=0;
 }
 return 18;
}

#define W(x) (x)&0xFF,(x)>>8
#define D(x) W((x)&0xFFFF),W((x)>>16)

PROGMEM const char device_desc[] = {
 18,		//bLength
 1,		// descriptor type	1=DEVICE
 W(0x0110),	// USB version supported
 0,		// USB_CFG_DEVICE_CLASS,
 0,		// USB_CFG_DEVICE_SUBCLASS,
 0,		// protocol
 8,		// max packet size
 W(0x16c0),	// vendor ID = VOTI, Teilbereich siphec, Teilbereich h#s
 W(0x06B6),	// product ID = USB2LPT mit 3 Interfaces
 W(BCDDATE),	// version (ganze Monate)
 1,		// manufacturer string index
 2,		// product string index
 0,		// serial number string index		PATCH offset 16
 1};		// number of configurations

#define CONFIG_DESC_SIZE (9+9+7+7 +9+9+7 +9+7)
#define HID_DESC2_OFFSET  (9+9+7+7 +9)

PROGMEM const char config_desc[CONFIG_DESC_SIZE]={
 9,		//bLength
 2,		//bDescriptorType	2=CONFIG
 W(CONFIG_DESC_SIZE),	//wTotalLength
 2,		//bNumInterfaces	h#s USB2LPT + HID
 1,		//bConfigurationValue (willkürliche Nummer dieser K.)
 0,		//iConfiguration	ohne Text
 0x80,		//bmAttributes (Busversorgt, kein Aufwecken)
 100/2,		//MaxPower (in 2 Milliampere)	100 mA
//Natives USB2LPT-Interface:
 9,		//bLength
 4,		//bDescriptorType	INTERFACE
 0,		//bInterfaceNumber
 0,		//bAlternateSetting
 2,		//bNumEndpoints
 -1,		//bInterfaceClass	hersteller-spezifisch
 0,		//bInterfaceSubClass	(passt in keine Klasse)
 0,		//bInterfaceProtocol
 0,		//iInterface		ohne Text (TODO wenn Platz)
//Enden-Beschreiber C1I0A0:Bulk EP1OUT
 7,		//bLength
 5,		//bDescriptorType	ENDPOINT
 OUT_EP,	//bEndpointAddress	EP1OUT
 2,		//bmAttributes		BULK
 W(OUT_EP_SIZE),//wMaxPacketSize
 0,		//bInterval
//Enden-Beschreiber C1I0A0:Interrupt EP1IN
 7,		//bLength
 5,		//bDescriptorType	ENDPOINT
 IN_EP|0x80,	//bEndpointAddress	EP2IN
 2,		//bmAttributes		BULK
 W(IN_EP_SIZE),	//wMaxPacketSize
 0,		//bInterval
//HID-Interface:
 9,		//bLength
 4,		//bDescriptorType	INTERFACE
 1,		//bInterfaceNumber
 0,		//bAlternateSetting
 1,		//bNumEndpoints
 3,		//bInterfaceClass	HID
 0,		//bInterfaceSubClass	(ohne Spezifizierung)
 0,		//bInterfaceProtocol
 0,		//iInterface		ohne Text (TODO wenn Platz)
//HID-Beschreiber (an Offset 41)
 9,		// bLength
 0x21,		// bDescriptorType	HID
 W(0x0101),	// BCD representation of HID version
 0,		// target country code
 1,		// number of HID Report Descriptor infos to follow
 0x22,		// descriptor type: report
 W(14+7*9+1),	// total length of report descriptor
//Enden-Beschreiber C1I1A0:Interrupt EP3IN
 7,		//bLength
 5,		//bDescriptorType	ENDPOINT
 HID_EP|0x80,	//bEndpointAddress	EP3IN
 3,		//bmAttributes		INTERRUPT
 W(HID_EP_SIZE),//wMaxPacketSize
 1,		//bInterval		1 ms Abfrageintervall (min. zulässig)
//USBPRN-Interface:
 9,		//bLength
 4,		//bDescriptorType	4=INTERFACE
 2,		//bInterfaceNumber
 0,		//bAlternateSetting
 1,		//bNumEndpoints
 7,		//bInterfaceClass	Drucker
 1,		//bInterfaceSubClass	Drucker
 1,		//bInterfaceProtocol	Unidirektional
 0,		//iInterface
//Enden-Beschreiber C0I2A0:Bulk EP4OUT
 7,		//bLength
 5,		//bDescriptorType	5=ENDPOINT
 PRN_EP,	//bEndpointAddress	EP4OUT
 2,		//bmAttributes		2=BULK
 W(PRN_EP_SIZE),//wMaxPacketSize
 0,		//bInterval		keine Bedeutung bei BULK
};

// Für sinnvoll-maximale USB-Performance ist der Report
// nicht größer als die FIFO (8 Bytes) gewählt - inkl. Report-ID
// Die Report-ID gibt die Anzahl der relevanten
// Bytes an, bleiben 1..7 Bytes Nutzdaten
// (bspw. 3 OUT-Befehle und 1 IN-Befehl).
PROGMEM const char usbDescriptorHidReportT[] = {
 0x06, W(0xFF00),	// G Usage Page (Vendor specific)
 0x09, 0x01,		// L Usage (Vendor Usage 1)
 0xA1, 0x01,		// M Collection (Application)
 0x15, 0x00,		// G  Logical Minimum (0)
 0x26, W(0x00FF),	// G  Logical Maximum (255)
 0x75, 0x08,		// G  Report Size (8 bits)

 0x85, 0x01,		// G  Report ID (1)		PATCH: Offset 15
 0x95, 0x01,		// G  Report Count (1 Byte)	PATCH: Offset 17
 0x09, 0x01,		// L  Usage (1)			PATCH: Offset 19
 0xB2, W(0x0102),	// M  Feature (Data,Var,Abs,Buf)
// lange Reports (für Full Speed und Hi-Speed vorgesehen)
 0x85, 0x3E,		// G  Report ID (62)
 0x95, 0x01,		// G  Report Count (1 Byte)
 0x0B, D(0x0001003B),	// L  Usage (Generic Desktop : Byte Count)
 0xB1, 0x02,		// M  Feature (Data,Var,Abs)
 0x95, 0x3E,		// G  Report Count (62 Byte)
 0x09, 0x3E,		// L  Usage (3E)
 0xB2, W(0x0102),	// M  Feature (Data,Var,Abs,Buf)
 0xC0};			// M End Collection

static uchar BuildHidReportDesc() {
 uchar *d=replyBuf;
 uchar id;
 FETCH_BLOCK(d,usbDescriptorHidReportT,14+9);
 d+=14+9;
 for (id=2; id<8; id++) {
  FETCH_BLOCK(d,usbDescriptorHidReportT+14,9+1);
  d[1]=d[3]=d[5]=id;
  d+=9;
 }
 return 14+7*9+1;
}

int usbFunctionDescriptor(usbRequest_t *rq) {
 uchar ret=0;
 switch (rq->wValue.bytes[1]) {
  case USBDESCR_DEVICE: {	//1
   FETCH_BLOCK(replyBuf,usbDescriptorDeviceT,sizeof usbDescriptorDeviceT);
   if (g.Feature&0x10 && g.SerialNumber && g.SerialNumber!=0xFFFFFFFF) replyBuf[16]=3;
   ret=sizeof usbDescriptorDeviceT;
  }break;
  case USBDESCR_CONFIG: {	//2
   FETCH_BLOCK(replyBuf,usbDescriptorConfigurationT,sizeof usbDescriptorConfigurationT);
   if (g.Feature&0x80) replyBuf[21]=replyBuf[28]=2;	//BULK
   ret=sizeof usbDescriptorConfigurationT;
  }break;
  case USBDESCR_STRING: switch (rq->wValue.bytes[0]) {	//3
   case 0: ret=BuildStringFromUTF8(PSTR("\xD0\x87\xD0\x89")); break;	// generiert 0x407 und 0x409
   case 1: ret=BuildStringFromUTF8(PSTR("haftmann#software")); break;
   case 2: ret=BuildStringFromUTF8(rq->wIndex.bytes[0]==7
		?PSTR("USB-zu-LPT-Umsetzer, Full Speed")
		:PSTR("USB2LPT full speed adapter")); break;
   case 3: ret=BuildStringFromSerial(); break;
  }break;
  case USBDESCR_HID: {		//0x21
   FETCH_BLOCK(replyBuf,usbDescriptorConfigurationT+41,9);
   ret=9;
  }break;
  case USBDESCR_HID_REPORT: {	//0x22
   ret=BuildHidReportDesc();
  }break;
 }
 usbMsgPtr=replyBuf;
 return ret;
}

static void usbInit() {
 HW_CONFIG();
 USB_FREEZE();				// enable USB
 PLL_CONFIG();				// config PLL
 while (!(PLLCSR & (1<<PLOCK))) ;	// wait for PLL lock
 USB_CONFIG();				// start USB clock
 UDCON = 0;				// enable attach resistor
 usbCfg=0;
}

static void usbPoll() {
// USB-Interrupts behandeln
 if (UDINT&1<<EORSTI) {// Ende USB-Reset
  UENUM = 0;		// EP0 einrichten
  UECONX = 1;
  UECFG0X = EP_TYPE_CONTROL;
  UECFG1X = EP_SIZE(EP0_SIZE) | EP_SINGLE_BUFFER;
  UEINTX=4;		// Kill Bank IN??
  usbCfg=0;		// unkonfiguriert
  UDINT=~(1<<EORSTI);
 }
 if (UDINT&1<<SUSPI) {
  TIMSK0=0;		// keine Interrupts
  EIMSK=0;
  USBCON=0xA0;		// USB-Takt anhalten
  PLLCSR=0x04;		// PLL anhalten
  CKSEL0|=1<<RCE;	// 0x0D
  while (!(CKSTA&1<<RCON));	// warte bis RC-Oszillator läuft
  CKSEL0&=~(1<<CLKS);	// 0x0C // umschalten
  CKSEL0&=~(1<<EXTE);	// 0x08
// TODO: Resistive Verbraucher (LEDs, PWM) sowie ISR abschalten
  UDINT=~(1<<SUSPI);
 }
 if (UDINT&1<<WAKEUPI) {
  CKSEL0|=1<<EXTE;	// 0x0C
  while (!(CKSTA&1<<EXTON));	// warte bis Quarzoszillator läuft
  CKSEL0|=1<<CLKS;	// 0x0D	// umschalten
  PLLCSR=0x06;		// PLL starten
  CKSEL0&=~(1<<RCE);	// 0x05
  while (!(PLLCSR&1<<PLOCK));	// warte bis PLL eingerastet
  USBCON=0x80;		// USB-Takt anlegen
  UDINT=~(1<<WAKEUPI);
 }
 usbPollEP0();
}

int main(void) {
 usbTxLen = USBPID_NAK;	
 usbMsgLen = USB_NO_MSG; 
 ExpectEP1OutToken=USBPID_DATA0;

 uchar SofCmp = 0;
 ACSR |= 0x80;		// Analogvergleicher ausschalten - 70 µA Strom sparen
 sleep_enable();	// normaler Schlafmodus
 usbInit();
 sei();
 eeprom_read_block(&g,(uchar*)0xFFF9,7);
 if (g.Feature == 0xFF) g.Feature = 0;	// ungebrannt als 0 annehmen
 initIOPorts();
 LptOn();
 if (g.ecr&0x1F) g.ecr=0x20;// bidirektionalen PS/2-Modus voreinstellen
 out402(g.ecr);

 for(;;) {		// Hauptschleife
  uchar t = ECR_Bits;
  if (t & 0x04) SppXfer();		// SPP-FIFO-Transfer prüfen
  else if (t & 0x08) EcpXfer();	// ECP-FIFO-Transfer prüfen
  t = usbSofCount;
  if (SofCmp != t) wdt_reset();	// alle 1 ms (oder öfter bei SE0) Watchdog beruhigen
  usbPoll();
  if (SofCmp != t) {	// SOF eingetroffen?
   SofCmp = t;
   Led_On1ms();		// LED blinken lassen
   if (eeprom_is_ready()) {	// Feature-Byte im EEPROM nachführen
    if (eeprom_read_byte((uchar*)0xFFFB) != g.Feature)
      eeprom_write_byte((uchar*)0xFFFB, g.Feature);
   }
  }
 }
}
