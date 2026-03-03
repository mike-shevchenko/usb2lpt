/* Firmware für ATmega8 im USB2LPT 1.6, haftmann#software 11/10
 * basierend auf: »usbdrv« von Objective Development GmbH (Österreich),
 * Beispieldatei von Henning Paul
 * Lizenz: GNU GPL v2 (see License.txt)
 * Zu übersetzen und zu brennen mit zugehörigem »makefile«,
 * bspw. »make« zum Kompilieren, »make flash« zum Brennen (ggf. mit Kompilation),
 * (einmalig pro Chip) »make fuse« zum Setzen der Sicherungen (Taktquelle u.ä.)
 * Das Paket »winavr« ist erforderlich!
 * Alle Schiebeoperationen (>>, <<) sind wegen Compiler-Umständen (int-Cast)
 * in Schiebe-Zuweisungen (>>=, <<=) umgesetzt;
 * dito auch & und | bei Tests (bspw. bei »if«) mit mehr als einem Bit.
 * Das Schlüsselwort »inline« scheint unnötig, gcc hat eigenen Kopf.
 *
 * Mit dem eingesparten Quarz handelt man sich Probleme bei schwankender
 * Speisespannung ein! Also: Hiermit nur geringe Lasten treiben!
 * An manchen PCs ist die 5V so versaut, dass USB2LPT 1.6 nicht funktioniert.
 * (So die Vermutung!)
 *
*101128	Mittels Option "-nostartfiles" wird Startup-Code minimiert.
	4 KByte (für ATmega48) bleiben illusorisch.
+101213	HID-Interface. Wegen Win64 und Treiberproblem.
-110917	HID-Reportdeskriptor mit Usages versehen
	(damit der Windows HID-Parser funktioniert)
*120301	Problem erkannt: Vista/7 "USB-Verbundgerät" (usbccgp.sys) lädt nicht
	Lösung: Vertauschen der Alternate Settings.
	Evtl. Problem: Treiber geht nicht zu laden??
	Treiber sollte angepasst werden
	Noch ein Problem: Vista/7 lädt usb2lpt.sys auf das Verbundgerät,
	für Nicht-Verbundgeräte ist eine andere PID erforderlich!
+120308	Voreinstellbares ECR via EEPROM-Speicherstelle 0xFFF9
	(gemeinsam mit High-Speed)
	Zusammensetzbare Feature-Requests
+130501	Weitere Mikrocodes für Bitmanipulation (wegen DLPORTIO.DLL):
	0x21 = Bit setzen, 0x22 = Bit löschen, 0x23 = Bit kippen
	Folgebyte Bit 2:0 = Bitnummer, Bit 3 = Echt-Outport-Bit,
	Bit 7:4 = LPT-Adressoffset
 */

// Aus »winavr«
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <string.h>	// memcpy
#include <util/delay.h>	// _delay_us

// Aus »usbdrv«
#include "usbdrv.c"

// Firmware-Datum (Meldung bei A3-Request mit wData==6 und wLength==2)
#define DATEYEAR	2013
#define DATEMONTH	5
#define DATEDAY		28

/************************
 * Hardware:
 *
 * PortB = (zumeist) Steuerport
 *[12] ICP   0 = USB D- (sowie Interrupt und Pullup 10k nach 5P)
 *[13] OC1   1 = USB D+
 *[14] SS    2 = /STB - /C0 (1)
 *[15] MOSI  3 = /AF  - /C1 (14)
 *[16] MISO  4 = /INI -  C2 (16)
 *[17] SCK   5 = /SEL - /C3 (17)
 * [8] XTAL2 6 = Quarz oder frei (wird per internem Pullup high gezogen...)
 * [7] XTAL1 7 = Quarz oder frei (...um erhöhter Stromaufnahme vorzubeugen)
 * Schade, jetzt hätte man ein zweites 8-bit-Port wie beim USB2LPT 1.4 und 1.7
 * herausführen können (Verzicht auf LED /oder/ RESET).
 * Aber die 500 Leiterplatten sind gefertigt...
 *
 * PortC = (zumeist) Statusport
 *[23] ADC0  0 = /LED (low-aktiver Ausgang)
 *[24] ADC1  1 = /ERR -  S3 (15)
 *[25] ADC2  2 =  ONL -  S4 (13)
 *[26] ADC3  3 =  PE  -  S5 (12)
 *[27] ADC4  4 = /ACK -  S6 (10)
 *[28] ADC5  5 =  BSY - /S7 (11)
 *[29] RESET 6 = /Reset (kein E/A-Anschluss) - Lötbrücke nach ONL S4 (13)
 *
 * PortD = Datenport
 *[30] RxD   0 = D0 (2)
 *[31] TxD   1 = D1 (3)
 *[32] INT0  2 = D2 (4)
 * [1] INT1  3 = D3 (5)
 * [2] T0    4 = D4 (6)
 * [9] T1    5 = D5 (7)
 *[10] AIN0  6 = D6 (8)
 *[11] AIN1  7 = D7 (9)
 *
 * In eckigen Klammern: ATmega8-Pinnummer (TQFP-Gehäuse)
 * In runden Klammern: Pinnummer SubD25-Buchse
 * Übrige Pins:
 *[4][6][18] - Betriebsspannung (5V, nicht 3,3V, wegen Ausgabe auf SubD)
 *[3][5][21] - Masse
 *[19][20][22] - zusätzliche A/D-Wandler-Eingänge; A/D-Referenzspannung
 *(18)..(24) - Masse
 *(25) - Masse oder umlötbar 5V
 *
 * Die Zuordnung zu den Portpins erfolgte nach der Maßgabe,
 * Portadressen nicht aufzuteilen, um bei Ausgaben mit zwei Pegelwechseln
 * diese Pegelwechsel exakt gleichzeitig erscheinen zu lassen.
 * Zur Ausrichtung von Ein-Ausgabedaten genügen Schiebebefehle.
 * Damit war PortD als Datenport zwangsweise festgelegt, und INT0 (INT1)
 * nicht für USB nutzbar.
 ************************/

#define FATDATE (((DATEYEAR)-1980)*512+(DATEMONTH)*32+(DATEDAY))
#define BCDDATE (((DATEYEAR)-2000)/10<<12)+(((DATEYEAR)-2000)%10<<8)+((DATEMONTH)/10<<4)+(DATEMONTH)%10

inline __attribute((naked,section(".vectors"))) void __vectors() {
 asm volatile(
"	ldi	r30,0x5F\n"	// initialize stack
"	ldi	r31,0x04\n"
"	rjmp	main\n"
#ifdef PCINT0_vect
"	rjmp	__vector_3\n"::);	// das FAT-Datum wird simuliert
#else
"	.byte	%0,%1\n"	// hier liegt das FAT-Datum
"	.byte	'h','s'\n"	// 2 ungenutzte Bytes
"	rjmp	__vector_5\n"
	:
	:"M" (FATDATE&0xFF),
	 "M" ((FATDATE>>8)&0xFF)
	);
#endif
}
void __do_copy_data(void) __attribute__((naked));
void __do_copy_data(void) {}	// no non-zero-initialized static data

void __do_clear_bss(void) __attribute__((naked));
void __do_clear_bss(void) {}	// no static data at all
int main(void) __attribute__((noreturn,naked));
 
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
 uchar osccal;
 uchar Feature;		// Bit0 = Offene-Senke-Simulation für Daten (SPP +0)
			// Bit2 = Offene-Senke-Simulation für Steuerport (+2)
			// Bit4 = Seriennummer via USB-Deskriptor
			// Bit6 = DirectIO (keine Invertierungen)
			// Bit7 = Bulk-statt-Interrupt
 unsigned long SerialNumber;
} g;
/* FIFO */
#define FIFOSIZE 16	// muss laut Programmlogik Vielfaches von 2 sein
static unsigned Fifo[FIFOSIZE] __attribute((section(".noinit")));
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
 uchar t;
 LptSetDataDir();	// Datenport: Ausgänge (oder nur Pullups)
 DDRC  = 0x01;		// Statusport: nur die LED ist Ausgang
 PORTC = 0x3E;		// Statusport: fünf Pullups
 t = DCR;
 t ^= 0x0B;
 t <<= 2;
 PORTB = t | 0xC0;
 DDRB  = 0x3C;		// USB-Anschlüsse bleiben Eingänge
}

// <strobe>-Low-Bits auf Steuerport ausgeben und max. 10 µs auf WAIT=H warten
// Liefert Daten eines READ-Zyklus'
// Ausgabedaten müssen vorher auf PORTD gelegt werden.
// Bei Fehler wird das TimeOut-Bit gesetzt
// (Gelöscht wird es durch Schreiben einer Null aufs Statusport.)
static uchar epp_io(uchar strobe) {
 uchar i, saveoe = DDRD, save = PORTB;
 if (PINC & 0x20) goto n_ok;	// BUSY
 PORTB = save & strobe;	// ASTROBE bzw. DSTROBE sowie ggf. WRITE auf LOW
 if (!(strobe & 0x04)) DDRD = 0xFF;	// Ausgabe (jetzt erst)
 i = 24; do{
  if (PINC & 0x20) goto okay;	// sbic PINC,5; rjmp okay (2)
 }while (--i);			// dec r18; brne l1 (3)
n_ok:
 EppTimeOut |= 0x01;		// TimeOut markieren
okay:
 i = PIND;			// Daten einlesen (nur für INPUT relevant:-)
 if (!(strobe & 0x04)) DDRD = saveoe;
 PORTB = save;
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
 b >>= 2;
 if (!(g.Feature & 0x40)) {
  b ^= 0x20;		// BSY-Bit invertieren
  b |= ~DDRC;		// Ausgabe nur bei DirectIO oder vorhandenen Ausgabepins
 }
 b &= 0x3E;
 PORTC = (PORTC & 1) | b;
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
 b <<= 2;
 PORTB = b | 0xC0;
 if (g.Feature & 0x04) DDRB = ~b & 0x3C;	// USB-DDR-Bits müssen 0 bleiben
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
 b >>= 2;
 b |= 0xC1;
 DDRC = b;			// LED-Ausgang immer EIN
 if (!(g.Feature & 0x40)) {	// ohne DirectIO Pullups sicherstellen!
  PORTC |= ~b;
 }
}

// === OUT auf Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
static void out406(uchar b) {
 b <<= 2;
 DDRB = b & 0x3C;		// USB-Anschlüsse stets Eingang
}

// === OUT auf Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
static void out407(uchar b) {
 uchar t;
 b &= 0xD5;			// nur unterstützte Bits durchlassen
 t = g.Feature ^ b;
 g.Feature = b;
 if (t) {
// BrennFeature()
  if (t & 0x01 && !(b & 0x40) && !DIRECTION_INPUT) {
   DDRD = b & 0x01 ? ~PORTD : 0xFF;
  }
  if (t & 0x04 && !(b & 0x40)) {
   DDRB = b & 0x04 ? ~PORTB & 0x3C : 0x3C;
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
 uchar t = PINC;
 t <<= 2;
 t |= 0x07;
 if (!(g.Feature & 0x40)) t ^= 0x80;
 t &= EppTimeOut;
 return t;
}

// b==0: Echtes PIN-Rüclesen, sonst PORT-Rücklesen
static uchar internal_in2(uchar b) {
 b = b ? PORTB : PINB;
 b >>= 2;
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
 uchar t = DDRC;
 t <<= 2;
 t &= 0xF8;
 return t;
}

// === IN von Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
static uchar in406(void) {
 uchar t = DDRB;
 t >>= 2;
 t &= 0x0F;
 return t;
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
 PORTB &= ~0x04;		// /STB = L
 wait(1);
 PORTB |=  0x04;		// /STB = H
}

// Zyklischer Aufruf; Emulation der ECP-Ein/Ausgabefifo
static void EcpXfer(void) {
 unsigned w;
 if (DIRECTION_INPUT) {		// FIFO-Eingabe
  if (ECR & 2) return;		// bei voller FIFO nichts tun
  if (!(PORTB & 0x08)) {	//  /AF = L: Byte einlesen?
   if (PINC & 0x10) return;	// /ACK = H: kein Byte von außen vorhanden
   PORTB |= 0x08;		//  /AF = H setzen
  }				//  /AF = H: hier zweite Handshake-Phase
  if (!(PINC & 0x10)) return;	// /ACK = L: nichts tun!
  w = PIND;
  if (PINC & 0x20) w|=0x0100;	//  BSY = Command(0) / Data(1)
  PutFifo(w);
  PORTB &= ~0x08;		//  /AF = L setzen
 }else{				// FIFO-Ausgabe
  if (PORTB & 0x04) {		// /STB = H: Byte ausgeben?
   uchar t;
   if (ECR & 1) return;		// bei leerer FIFO nichts tun
   if (PINC & 0x20) return;	//  BSY = H: nichts tun!
   w = GetFifo();		// Adress- oder Datenbyte lesen
   PORTD = (uchar)w;		// Byte anlegen
   t = PORTB & 0x34;
   if (w & 0xFF00) t |= 0x08;	// AF setzen oder nicht
   PORTB = t;
   PORTB &= ~0x04;		// /STB = L setzen
  }				// /STB = L: hier zweite Handshake-Phase
  if (!(PINC & 0x20)) return;	//  BSY = L: nichts tun!
  PORTB |= 0x04;		// /STB = H setzen
 }
}

// LED starten mit Blinken, t = Periodendauer
void Led_Start(uchar t) {
 if (!Led_T) {
  Led_C = t;
  PORTC |= 0x01;	// LED ausschalten
 }
 Led_T = Led_F = t;
}

// LED-Zustand alle 1 ms (SOF) aktualisieren
static void Led_On1ms(void) {
 uchar led_t = Led_T, led_c;
 if (!led_t) return;
 led_c = Led_C;		// in Register laden
 if (!--led_c) {
  PORTC ^= 0x01;	// LED umschalten
  led_c = Led_F;	// Zähler neu laden
 }
 if (!--led_t) PORTC &= ~0x01;	// LED einschalten
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
static uchar ReplyBuf[128] __attribute((section(".noinit")));

// Häufige Art der Mikrocode-Verarbeitung
static void HidProcessInOut(const uchar*data, uchar len) {
 ReplyIdxW += ProcessInOut(data, len, ReplyBuf+2+ReplyIdxW);
}

static uchar gbRequest;		// Kopie vom letzten SETUP-Paket
static size_t gAdr, gLen __attribute((section(".noinit")));

//-090624: Ohne diese Maßnahme gelangen Daten „in den falschen Hals“
static uchar ExpectEP1OutToken/*=USBPID_DATA0*/;

static uchar replyBuf[128] __attribute((section(".noinit")));

USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
 gbRequest = data[1];		// SETUPDAT merken (wie Cypress-Controller)
 gAdr = ((usbRequest_t*)data)->wValue.word;
 gLen = ((usbRequest_t*)data)->wLength.word;

// Rudiment von Henning Paul, für seinen Linux-Treiber
 usbMsgPtr = (usbMsgPtr_t)replyBuf;
// SetFeature -> Stall -> EP1OUT: Data-Toggle rücksetzen
 if (data[0]==0x02 && data[1]==0x03 && data[2]==0x00 && data[4]==0x01) {
  ExpectEP1OutToken=USBPID_DATA0;
 }else if (data[0]==0x21 && data[1]==USBRQ_HID_SET_REPORT && data[4]==0x01) {
  return 0xFF;		// usbFunctionWrite() kümmert sich 
 }else if (data[0]==0xA1 && data[1]==USBRQ_HID_GET_REPORT && data[4]==0x01
   && ReplyIdxW) {
// Die ersten zwei Bytes werden nicht für die Antwortbytes benutzt.
  usbMsgPtr = (usbMsgPtr_t)(ReplyBuf+ReplyIdxR+1);	// Report-ID = Länge, danach Datenbytes
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
#ifdef PCINT0_vect
// im Sonderfall Adresse=6 und Länge=2 wird das Datums-WORD (FAT) geliefert
   if (data[1] == 0xA3 && gAdr == 6 && gLen == 2) {
    ((usbWord_t*)replyBuf)->word = FATDATE;
    return 2;
   }
#endif
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
USB_PUBLIC uchar usbFunctionRead(uchar *data, uchar len) {
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

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
 void *adr = (void*)gAdr;	// im Register halten
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

USB_PUBLIC void usbFunctionWriteOut(uchar *data, uchar len) {
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
static usbMsgLen_t BuildStringFromUTF8(const char *src) {
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

static usbMsgLen_t BuildStringFromSerial() {
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

PROGMEM const char usbDescriptorDeviceT[] = {
 18,		//bLength
 USBDESCR_DEVICE,        // descriptor type	1
 W(0x0110),	// USB version supported
 0,		// USB_CFG_DEVICE_CLASS,
 0,		// USB_CFG_DEVICE_SUBCLASS,
 0,		// protocol
 8,		// max packet size
 W(0x16c0),	// vendor ID = VOTI, Teilbereich siphec, Teilbereich h#s
 W(0x06B4),	// product ID = USB2LPT mit 2 Interfaces
 W(BCDDATE),	// version (ganze Monate)
 1,		// manufacturer string index
 2,		// product string index
 0,		// serial number string index		PATCH offset 16
 1};		// number of configurations


PROGMEM const char usbDescriptorConfigurationT[57]={
 9,	//bLength
 2,	//bDescriptorType	2=CONFIG
 W(57),	//wTotalLength
 2,	//bNumInterfaces	h#s USB2LPT + HID
 1,	//bConfigurationValue (willkürliche Nummer dieser K.)
 0,	//iConfiguration	ohne Text
 0x80,	//bmAttributes (Busversorgt, kein Aufwecken)
 100/2,	//MaxPower (in 2 Milliampere)	100 mA
//Interface-Beschreiber 0, Alternative 0:
 9,	//bLength
 4,	//bDescriptorType	INTERFACE
 0,	//bInterfaceNumber
 0,	//bAlternateSetting
 2,	//bNumEndpoints
 -1,	//bInterfaceClass	hersteller-spezifisch
 0,	//bInterfaceSubClass	(passt in keine Klasse)
 0,	//bInterfaceProtocol
 0,	//iInterface		ohne Text (TODO wenn Platz)
//Enden-Beschreiber C0I0A1:Interrupt EP1OUT
 7,	//bLength
 5,	//bDescriptorType	ENDPOINT
 1,	//bEndpointAddress	EP1OUT
 3,	//bmAttributes		INTERRUPT	PATCH: Offset 21
 W(8),	//wMaxPacketSize
 10,	//bInterval		10 ms Abfrageintervall (min. zulässig)
//Enden-Beschreiber C0I0A1:Interrupt EP1IN
 7,	//bLength
 5,	//bDescriptorType	ENDPOINT
 0x81,	//bEndpointAddress	EP1IN
 3,	//bmAttributes		INTERRUPT	PATCH: Offset 28
 W(8),	//wMaxPacketSize
 10,	//bInterval		10 ms Abfrageintervall (min. zulässig)
//Interface-Beschreiber 1, Alternative 0:
 9,	//bLength
 4,	//bDescriptorType	INTERFACE
 1,	//bInterfaceNumber
 0,	//bAlternateSetting
 1,	//bNumEndpoints
 3,	//bInterfaceClass	HID
 0,	//bInterfaceSubClass	(ohne Spezifizierung)
 0,	//bInterfaceProtocol
 0,	//iInterface		ohne Text (TODO wenn Platz)
//HID-Beschreiber (an Offset 41)
 9,	// bLength
 0x21,	// bDescriptorType	HID
 W(0x0101),	// BCD representation of HID version
 0,	// target country code
 1,	// number of HID Report Descriptor infos to follow
 0x22,	// descriptor type: report
 W(14+7*9+1),	// total length of report descriptor
//Enden-Beschreiber C0I1A0:Interrupt EP3IN (Dummy)
 7,	//bLength
 5,	//bDescriptorType	ENDPOINT
 0x83,	//bEndpointAddress	EP3IN
 3,	//bmAttributes		INTERRUPT
 W(8),	//wMaxPacketSize
 10,	//bInterval		10 ms Abfrageintervall (min. zulässig)
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
/* lange Reports (für Full Speed und Hi-Speed vorgesehen)
 0x85, 0x3E,		// G  Report ID (62)
 0x95, 0x01,		// G  Report Count (1 Byte)
 0x0B, D(0x0001003B),	// L  Usage (Generic Desktop : Byte Count)
 0xB1, 0x02,		// M  Feature (Data,Var,Abs)
 0x95, 0x3E,		// G  Report Count (62 Byte)
 0x09, 0x3E,		// L  Usage (3E)
 0xB2, W(0x0102),	// M  Feature (Data,Var,Abs,Buf)
 */
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

USB_PUBLIC usbMsgLen_t usbFunctionDescriptor(struct usbRequest *rq) {
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
		?PSTR("USB-zu-LPT-Umsetzer, Low-Speed")
		:PSTR("USB2LPT low-speed adapter")); break;
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
 usbMsgPtr=(usbMsgPtr_t)replyBuf;
 return ret;
}

#if F_CPU == 12800000
volatile uchar timer0Snapshot;
#define TIMER0_PRESCALING           64 /* must match the configuration for TIMER0 in main */
#define TOLERATED_DEVIATION_PPT     10 /* max clock deviation before we tune in 1/10 % */
/* derived constants: */
#define EXPECTED_TIMER0_INCREMENT   ((F_CPU / (1000L * TIMER0_PRESCALING)) & 0xff)
#define TOLERATED_DEVIATION         (TOLERATED_DEVIATION_PPT * F_CPU / (1000000 * TIMER0_PRESCALING))
static void tuneOsccal(void) {
 static uchar lastTimer0Value;
 uchar t=timer0Snapshot;
 char d=t-lastTimer0Value;
 lastTimer0Value=t;
 d-=EXPECTED_TIMER0_INCREMENT;
 t=OSCCAL;
 if (d>=TOLERATED_DEVIATION+1) t--;
 if (d<-TOLERATED_DEVIATION) t++;
 OSCCAL=t;
}
#endif
//extern uchar usbTxLen;
extern uchar usbMsgLen;

int main(void) {
// continue initialization that does not fit into interrupt vector table
 asm volatile(
"	out	0x3E,r31\n"	// SPH
"	out	0x3D,r30\n"	// SPL
"	clr	r1\n"
"1:	st	-Z,r1\n"	// clear entire BSS
"	cpi	r30,0x60\n"	// this code is ATmega8 specific
"	cpc	r31,r1\n"
"	brne	1b");
// initialize two bytes of data declared inside USBDRV.C,
// because there is no data copy routine (this here is shorter)
 usbTxLen = USBPID_NAK;	
 usbMsgLen = USB_NO_MSG; 
 ExpectEP1OutToken=USBPID_DATA0;

 uchar SofCmp = 0;
#ifdef PCINT0_vect	// hier genutztes wesentliches Merkmal der ATmegaX8
 MCUSR = 0;
 wdt_disable();		// Watchdog-Zustand überlebt Reset! Ausschalten!
#if F_CPU == 12800000
 PRR = 0xCF;		// getaktete Peripherie außer Timer0 totlegen
 TCCR0B = 0x03;		// Vorteiler 64 (für Synchronisation benötigt)
#else
 PRR = 0xEF;		// Komplette Peripherie totlegen
#endif
 ACSR |= 0x80;		// Analogvergleicher ausschalten - 70 µA Strom sparen
 usbInit();		// Pegelwechsel-Interrupt aktivieren
 sei();
 set_sleep_mode(SLEEP_MODE_PWR_DOWN);
 sleep_enable();
 if (USBIN & USBMASK) {	// kein SE0?
  sleep_cpu();		// Ohne Oszillator schlafen bis zum Pegelwechsel
 }			// warten bis SE0
 set_sleep_mode(SLEEP_MODE_STANDBY);	// kein ...IDLE weil keine Peripherie
 wdt_enable(WDTO_15MS);	// Watchdog ist Strom sparender als ein Zeitgeber,
			// ein 3-ms-Zeitgeber wäre aber die USB-konforme Lösung
#else		// ATmega8-Kode: Polling per Watchdog-Timer (WDT-Fuse gesetzt)
 uchar mcucsr = MCUCSR;
 MCUCSR = 0;
 ACSR |= 0x80;		// Analogvergleicher ausschalten - 70 µA Strom sparen
#if F_CPU == 12800000
 TCCR0 = 0x03;		// Vorteiler 64 (für Synchronisation benötigt)
#endif
 sleep_enable();	// normaler Schlafmodus
 if (mcucsr & (1 << WDRF)) {
  if (USBIN & USBMASK) {// kein SE0?
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_cpu();		// Schlafen ohne sei() - bis zum nächsten Watchdog
  }			// Gemessene mittlere Stromaufnahme: 500 µA (hurra!)
 }
 usbInit();
 sei();
#endif
 eeprom_read_block(&g,(uchar*)0xFFF9,7);
 if (g.Feature == 0xFF) g.Feature = 0;	// ungebrannt als 0 annehmen
 if (g.osccal && g.osccal!=0xFF) OSCCAL = g.osccal;
	// beschleunigt auf 12,8 MHz ziehen, außer bei erster Inbetriebnahme
 initIOPorts();
 LptOn();
 if (g.ecr&0x1F) g.ecr=0x20;// bidirektionalen PS/2-Modus voreinstellen
 out402(g.ecr);

 for(;;) {		// Hauptschleife
  uchar t = ECR_Bits;
  if (t & 0x04) SppXfer();		// SPP-FIFO-Transfer prüfen
  else if (t & 0x08) EcpXfer();		// ECP-FIFO-Transfer prüfen
  else if (USBIN & USBMASK) sleep_cpu();// Schlafen, außer bei SE0
  t = usbSofCount;
  if (SofCmp != t || !(USBIN & USBMASK)) {
   wdt_reset();		// alle 1 ms (oder öfter bei SE0) Watchdog beruhigen
  }
  usbPoll();
  if (SofCmp != t) {	// SOF eingetroffen?
#if F_CPU == 12800000
   if ((uchar)(t-SofCmp)==1) tuneOsccal();
#endif
   SofCmp = t;
   Led_On1ms();		// LED blinken lassen
   if (eeprom_is_ready()) {	// Feature-Byte im EEPROM nachführen
    if (eeprom_read_byte((uchar*)0xFFFB) != g.Feature)
      eeprom_write_byte((uchar*)0xFFFB, g.Feature);
    else if (eeprom_read_byte((uchar*)0xFFFA) != OSCCAL)
      eeprom_write_byte((uchar*)0xFFFA, OSCCAL);	// OSCCAL nachführen
   }
  }
 }
}

/*** Untersuchung Stromverbrauch USB-Standby (und Normalbetrieb) ***
 * ATmega8, 12,8 MHz:
 Wie erwartet läuft alle 15 ms der RC-Oszillator kurz an, um den Pegel
 bei ICP zu prüfen. Die Länge des USB-Resets (SE0) ist hier unkritisch,
 weil der interne Oszillator schnell hochläuft.
 Der mittlere Stromverbrauch liegt bei < 500 µA, also < 300 µA des ATmega8
 und 200 µA des Pullups.
 Im Normalbetrieb beträgt die Stromaufnahme datenblattgerechte 7 mA.
 * ATmegaX8:
 ATmega48 nicht untersucht; die Firmware ist bereits größer als 4 KB.
*/

