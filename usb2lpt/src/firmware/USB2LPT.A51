;EZUSB-Firmware für USB2LPT, haftmann#software 07/06
;Bis jetzt wird nur ein dptr benutzt (also kein DPS), nur Registerbank 0
;Zu übersetzen mit ASEM51; EEPROM-Datei erstellen mit
;HEX2BIX -I -V 0x16C0 -P 0x06B3
;Pro OUT-Befehl werden zwei Bytes in die OUT-Pipe geschrieben.
;Für jeden IN-Befehl wird ein Byte [Adresse] in die OUT-Pipe gesetzt,
; als Folge kommt ein Byte [Daten] in die IN-Pipe. Genaueres siehe "upv"!
;Seriennummern-Position: 1 Byte @ FFFFh (alt), 4 Byte @ FFFCh (Intel, neu)
;06xxxx	EEPROM+XRAM-Routinen etwa wie VEND_AX
;060622	Gekaufte VID+PID eingetragen
;060629	Einzelpin-Richtungsumschaltung
;060630 Open-Collector-Simulation (Daten- und Steuerpins)
;060705	Richtungsbit-Simulation korrigiert (stets 0 bei Mode 0 und 2)
;060710	EPP: allgemeine Fehlerbeseitigung, ungetestet
;060711	Schattenregister für Steuerport, Code-Erweiterbarkeit für Anwender
;060809	DirectIo, korrigiertes Open-Collector-Verhalten (Steuerport), EEPROM
;070128	Richtungsbits geändert auf 0=Eingang, 1=Ausgang (wie EZUSB und ATmega)
;Zu tun: 
;-
	$nopaging
	$nosymbols
	$nomod51
	$nolist
	$include(an2131.mcu)	;fest für AN2131SC
	$include(makros.i51)
	$list
	$genonly		;das "Innere" von Makros
	$condonly		;das "Innere" von IFxx

DateYear	equ	2007	;"Versionskennung"
DateMonth	equ	1
DateDay		equ	28
Interfaces	equ	2	;1 = ohne "USB-Druckerunterstützung"
MyID		equ	1	;0 = Cypress-ID (Debuggen mit Cypress-Tools)
Always_Renum	equ	0	;1: Firmware-Update im Gerätemanager versagt

;###############
;## Schaltung ##
;###############
;Datenport  (Basisadresse+0) = PortB (mit gestürzter Bit-Anordnung)
;Statusport (Basisadresse+1) = PortC (Bits 7-6-5-4 auf Portbits 6-7-5-4)
;			   und PortA (Bit  3	   auf Portbit  4)
;Steuerport (Basisadresse+2) = PortC (Bits 3..0 ohne "Dreher" angeschlossen)
BSY	EQU	6	;PinsC.6	(ist mit D6 verbunden)
ACK	EQU	7	;PinsC.7
PE	EQU	5	;PinsC.5	(ist mit D6 verbunden)
ONL	EQU	4	;PinsC.4	D/P	TDO
ERR	EQU	4	;PinsA.4	Ucc	Ucc

SEL	EQU	3	;OutC.3
INI	EQU	2	;OutC.2
AF	EQU	1	;OutC.1
STB	EQU	0	;OutC.0
;D4			;OutB.3		/PROG (zieht TDO auf Low)
;D3			;OutB.4		/CTRL (aktiviert 2 Ausgangstreiber)
;D2			;OutB.5		PROG	TMS
;D1			;OutB.6		CCLK	TCK
;D0			;OutB.7		DIN	TDI
LED	EQU	5	;auf PortA, HIGH-aktiv
;###############
;## Endpoints ##
;###############
;Pipe	Funktion					EZUSB	FX2
;0	OUT-Adressen, OUT-Daten, IN-Adressen		EP2Out	EP2(Out)
;1	IN-Daten					EP2In	EP6(In)
;2(=0)	USB-Druckerunterstützung Vorwärtskanal		EP4Out	EP4(Out)
;3(=1)	USB-Druckerunterstützung Rückkanal		EP4In	EP8(In)
;-----------
;## DATEN ##
;-----------
DSEG AT 20h
IntReq:		ds	1	;für USB-Interruptrequests, bitadressierbar
Configuration:	ds	1	;hier: Null (adressiert) oder Eins (konfig.)
AltSetting1:	ds	1	;Null oder Eins (nur für 2. Interface)
bits:		ds	1
LedBlink	BIT	bits.0	;1 solange die LED blinkt
FeatureChanged	BIT	bits.4	;Feature-Byte in EEPROM brennen (persistent)
Data5V		BIT	bits.6	;wie OCData && !direction
Control5V	BIT	bits.7	;wie (SPP && !TPControl) || OCControl
;===Druckerport===
DCR:		ds	1	;Device Control Register (+2)
ackIntEn	BIT	DCR.4	;1=ein, rücklesbar, aber nicht unterstützt
direction	BIT	DCR.5	;0=OUT, 1=IN, rücklesbar nur bei ECR_Bits>1
ECR:		ds	1	;Extended Control Register (ECP 402)
fifoe		BIT	ECR.0	;1 wenn FIFO leer
fifof		BIT	ECR.1	;1 wenn FIFO voll
ECR_Bits:	ds	1	;Mit immer nur einem Bit gesetzt (one-hot)
;0=SPP,1=BiDi,2=SPP-FIFO,3=ECP,4=EPP,5=?,6=Test,7=Config
EPPTimeOut:	ds	1	;Bit0=0: kein TimeOut aufgetreten
				;Bit2=0: Interrupt aufgetreten (zz. ungenutzt)
Feature:	ds	1
OCData		BIT	Feature.0 ;Offene-Senke-Simulation für Daten (+0)
TPControl	BIT	Feature.1 ;Totempfahl auch bei SPP (hier immer 1)
OCControl	BIT	Feature.2 ;Offene-Senke-Simulation für Steuerport (+2)
DirectIo	BIT	Feature.6 ;keine Invertierungen, kein Datenrichtungsbit
;===I²C===
I2C_ALen:	ds	1	;entspricht etwa wIndexH beim Vendor-Request A2
 I2C_ALen0	BIT	I2C_ALen.0	; 1 = 1 Byte, oder 2 Bytes vertauscht
 I2C_ALen1	BIT	I2C_ALen.1	; 1 = 2 Bytes (sonst 0 oder 1 Byte)
 I2C_NoStop	BIT	I2C_ALen.2
 I2C_NoStart	BIT	I2C_ALen.3
 I2C_Paging	BIT	I2C_ALen.4	; 1 = Paging aktiv (nicht wIndexH)
 I2C_Verify	BIT	I2C_ALen.5	; zusammen mit Paging immer Verify

DSEG AT 30h
FIFOSIZE	equ	16
;===FIFO===... hat hier eine Tiefe von FIFOSIZE Wörtern zu je 9 Bit
Fifo:		ds	FIFOSIZE*2	;Wegen ECP brauchen wir 9 bit Breite!
fifor:		ds	1	;Fifo-Lesezeiger
fifow:		ds	1	;Fifo-Schreibzeiger
;===allgemein===
LedTime:	ds	1	;"Nachblinkzeit" der LED in ms
BlinkF:		ds	1	;"Blinkfrequenz" in ms (halbe Periode)
Blink:		ds	1	;Blink-Zähler
UniIdx:		ds	1	;Index in OUT4BUF zum byteweisen Lesen
;=== Variablen für I²C/EEPROM ===
I2C_Addr:	ds	1	;I²C-Adresse (EEPROM)
I2C_PMask:	ds	1	;Für seiten-weises EEPROM-Schreiben,
	;00 = einzelbyteweise, 07 = 8-Byte-Seite, 1F = 32-Byte-Seite usw.
	;07 (8bit), 1F (16bit) oder (1<<High-Nibble von wIndexH)-1 (Request ED)
;Die PMask wird mit dem LOW-Teil der laufenden Adresse geODERt; bei
;Ergebnis Null wird der I²C-Transfer unterbrochen und das Brennen ausgelöst.


;--------------
;## PROGRAMM ##
;--------------
;--> Dieses Programm kommt beinahe ohne Interrupts aus! <--
;Nach dieser Fallstudie frage ich mich, wieso Anchor/Cypress dem Anwender
;einen Bären mit 'zig Interrupts aufbinden will, wobei die Beispielquellen
;nichts anderes machen als in der ISR ein Bit zu setzen und im Hauptprogramm
;abzufragen (Polling). Dazu sind doch Interrupt-Request-Bits da!
;Nur für's Resume [Fortsetzen] schien eine Mini-ISR unumgänglich zu sein.
;(Naja, Schlafmodus und Resume sind hier ganz nettes Beiwerk...)

CSEG AT 0
	ljmp	main		;am Reset-Vektor
SAVEORG 6,0
	dwi	%(DateYear-1980)*512+DateMonth*32+DateDay

SAVEORG 2Eh
Scratch:ds	1	;für EEPROM-Transfer
usr2f:	RETC	;@002F Ansprung bei Ausgabebyte > 20h, ersetzen mit AJMP
usr31:	RETC	;@0031 unbekannter Request über EP0, ersetzen mit AJMP

SAVEORG RESUME	;RESUME-ISR @33h (erforderlich, sonst geht das Wecken nicht)
	clr	EICON.4
	reti
usr36:	ret	;@0036 Zyklischer Ansprung, ersetzen mit AJMP (oder LJMP)

SAVEORG 0C00h			;3 Kilobyte für User
;Erzeugt Bit-Reversions-Tabelle wegen "gestürzter" Zuordnung
;der Datenbits zum Parallelport (um Layout zu vereinfachen)
;VR: DPTR=BitReverse+256,A=0,B=0FFh
MakeBitReverse:
	mov	dptr,#BitReverse
	clr	a
mbr:	rlc	a
	mov	B.0,C
	rlc	a
	mov	B.1,C
	rlc	a
	mov	B.2,C
	rlc	a
	mov	B.3,C
	rlc	a
	mov	B.4,C
	rlc	a
	mov	B.5,C
	rlc	a
	mov	B.6,C
	rlc	a
	mov	B.7,C	;24 Takte
	rlc	a
	xch	a,B	;jetzt in B das ursprüngliche A
	STOS		;abspeichern
	xch	a,B
	inc	a
	jnz	mbr
	ret
;================================
;== Zeichenketten-Umwurstelung ==
;================================
ReadUTF8:
;UTF8/8859-1 -> Unicode-Konvertierung (Unicode-16 für USB)
;Um das EEPROM-File kleiner und den Quelltext leichter pflegbar zu machen...
;PE: dptr=Quell-UTF8; jedes "falsche" UTF8 wird als ISO-8859-1 interpretiert!
;    (Damit kann man in Westeuropa immer mit ISO-8859-1 arbeiten.)
;    DPS.0=0!
;PA: r7:r6=Unicode-16
;    dptr vorgerückt
;VR: A,R4,R5,R6,R7,dptr,Flags
;LZ: Stark vom Zeichensatz abhängig, bei reinem ASCII 14 Takte
	LODS
	mov	r6,a		;R6 enthält fortan erstes Byte für Rückfall
	jnb	ACC.7,utf1	;normales ASCII
	jnb	ACC.6,utf1	;kein gültiges UTF8-Führungsbyte
	jnb	ACC.5,utf2	;2-Byte-UTF8
	jb	ACC.4,utf1	;kein 3-Byte-UTF8
utf2:	anl	a,#3Fh
	jz	utf1
	dec	a
	jz	utf1
	inc	a
	rr	a
	rr	a
	mov	r5,a		;geANDetes und geschobenes Führungsbyte
	LOADX
	jnb	ACC.7,utf1	;kein gültiges Trail-Byte
	jb	ACC.6,utf1	;ebenso
	clr	ACC.7
	xch	a,r5		;R5=geANDetes Folgebyte, A=höheres Byte
	mov	r4,a		;R4=höheres Byte (Bits 7:6 oben, 10:8 unten)
	anl	a,#0C0h
	orl	a,r5
	xch	a,r4		;R4=niederwertiges Byte, A=höheres Byte
	anl	a,#0Fh		;3 Datenbits 10:8 und 1 "Testbit"
	mov	r7,a		;R7 setzen (höheres Byte)
	jnb	ACC.3,okay2	;war 2-Byte-Kode
	push	DPL0
	push	DPH0		;retten zur Restaurierung (fehlt "dec dptr")
	 LOADX	NEXT		;drittes Byte
	 jb	ACC.6,utf1r	;kein Folgebyte, ganze Kette verwerfen
	 jbc	ACC.7,utf3	;und #0:R6 (»Windows-Zeichensatz«) annehmen
utf1r:
	pop	DPH0		;dptr zurückstellen
	pop	DPL0
utf1:	mov	r7,#0
	ret
utf3:
	 mov	r5,a
	 mov	a,r4		;R5=geANDetes drittes Byte, A="mittleres" Byte
	 rr	a
	 rr	a
	 mov	r4,a		;R4=rechtsgeschobenes "mittleres" Byte
	 anl	a,#0C0h		;Bit 7:6
	 orl	a,r5		;A=Bit 7:0
	 xch	a,r4		;Low-Teil fertig, in R4, A="mittleres" Byte
	 anl	a,#3Fh		;Bit 13:8
	 xch	a,r7		;nach R7, A=höchstes Byte (nur 2 Bits 15:14)
	 rr	a
	 rr	a
	 anl	a,#0C0h
	 orl	a,r7		;High-Teil fertig
	 mov	r7,a		;in R7
	 add	a,#-8
	 jnc	utf1r		;zu klein, ungültig!
	pop	ACC
	pop	ACC
okay2:	mov	a,r4
	mov	r6,a		;R6:=R4 (Low-Teil) setzen
	inc	dptr
	ret

MakeString:	;USB-String-Deskriptor (erst) bei Anforderung zusammenbauen
;PE: R2=String-Nummer (in nullterminierter Kette)
;PA: StringPuffer gefüllt mit USB-String-Deskriptor
;VR: R1=0,R2=0,R4,R5,R6,R7,dptr
;LZ: Etwa linear mit R2 zunehmend!
	mov     dptr,#StringDescriptors
	inc	r2
	sjmp	ms2
ms1:	LODS
	jnz	ms1
ms2:	djnz	r2,ms1
	push	MPAGE
	 mov	MPAGE,#HIGH(StringPuffer)
	 mov	r1,#0
	 mov	r7,#3		;"String-Deskriptor"
ms3:	 mov	a,r6
	 movx	@r1,a		;(zunächst wird eine falsche Länge gesetzt)
	 inc	r1
	 mov	a,r7
	 movx	@r1,a		;"Zeichen", High-Byte
	 inc	r1
	 acall	ReadUTF8
	 mov	a,r6
	 orl	a,r7
	 jnz	ms3
	 mov	a,r1
	 mov	r1,#0
	 movx	@r1,a		;Länge in Bytes nachtragen
	pop	MPAGE
	ret

;EEPROM-Adresse (gerade; Schreibadresse) am I²C-Bus
EADDR	equ	0A0h		;für 8-bit-EEPROM, 16-bit-EEPROMS haben EADR+2

;EEPROM-Default-"Seitengröße" zum Brennen mehrerer Bytes auf einmal
EPAGE8	equ	8		;für 8-bit-EEPROM 24LC01/02
EPAGE16	equ	32		;für 16-bit EEPROM 24LC32/64
;Andere EEPROM-Typen müssen per wIndex ausgewählt werden.
;Eine "Seitengröße" von 1 führt zum Einzelbyte-Brennen (viel langsamer)

;Die Auswahl 8/16 bit erfolgt automatisch anhand I2CS.4 wie im EZUSB-Kern

;======================
;== Byte-Ein/Ausgabe ==
;======================

i2c_wr:	;Byte auf I²C schreiben (kann blockieren!)
;PE: A=Ausgabe-Byte
;PA: CY=1 bei Fehler oder NAK, ACC.4=ID1
	STX	I2DAT
	dec	r0		;auf I2CS
i2c_w:	LDX
	mov	c,ACC.2		;BERR-Bit
	jc	i2c_err
	jnb	ACC.0,i2c_w	;DONE-Bit
	orl	c,/ACC.1	;ACK-Bit=0 -> C=1
	ret
	
i2c_rd:	;Byte von I²C lesen (kann blockieren!)
;PE: R4=Anzahl noch zu lesender Bytes (für LASTRD und STOP)
;PA: A=Eingabe-Byte
	mov	r0,#LOW(I2CS)
i2c_r:	LDX	
	mov	c,ACC.2		;BERR-Bit
	jc	i2c_err
	jnb	ACC.0,i2c_r	;DONE-Bit
i2c_d:	;Dummy-Lesezugriff (Seiteneinstieg mit A=0 und R0=I2CS)
	jb	I2C_NoStop,i2c_b
	cjne	r4,#2,i2c_a
	setb	ACC.5		;LASTRD setzen
i2c_a:	cjne	r4,#1,i2c_b
	setb	ACC.6		;STOP setzen
i2c_b:	STX			;LASTRD bzw. STOP setzen
	LDX	NEXT		;I2DAT lesen
i2c_err:ret

;==============================
;== I²C(EEPROM)-Adressierung ==
;==============================

EAdr:	;I²C starten und (Adress-)Bytes ausgeben
;PE: DPTR=Adresse (DPL allein, wenn ALEN<2)
;PA: CY=1 bei Fehler
;VR: A,R0,R2
;N: Diese Routine kann bis 10 ms blockieren, bis der EEPROM bereit ist
;   (weil er sich bis zum Fertigschreiben I²C-busseitig tot stellt)
	LDX	I2CS
	jb	ACC.6,EAdr	;Stoppsequenz abwarten
	mov	r2,#0		;max. 256 Versuche, 10 ms = 120000 CPU-Takte
eal:	jb	I2C_NoStart,ea_nostart
	STX	I2CS,80h	;(4) Startsequenz ausgeben
ea_nostart:
	mov	a,I2C_Addr	;(2) I²C-Adresse: Schreiben
	acall	i2c_wr		;(>=25)
	jnc	ea1		;(3) EEPROM könnte mit Brennen beschäftigt sein
	jb	ACC.2,eae	;(3) wenn BERR nicht noch einmal versuchen
	jnb	I2C_Paging,eae	;(3) raus mit Fehler (nicht neu versuchen)
	mov	r0,#144		;(2)
	djnz	r0,$		;(144*3) warten
	djnz	r2,eal		;(3) ... Adressierung wiederholen!
eae:	ret			;raus (ggf. mit Fehler)
ea1:	;Adressbyte-Ausgabe (Tabelle s.u.)
	jnb	I2C_ALen0,ea_x0
	mov	a,DPL0		;EEPROM-Adresse Low-Teil - oder Low-Teil zuerst
	acall	i2c_wr
	jc	i2c_err
	jnb	I2C_ALen1,eae	; kein Fehler: fertig
	mov	a,DPH0		;EEPROM-Adresse High-Teil
	jmp	i2c_wr
ea_x0:
	jnb	I2C_ALen1,eae	; kein Fehler: fertig ohne ein Adressbyte
	mov	a,DPH0		;EEPROM-Adresse High-Teil zuerst
	acall	i2c_wr
	jc	i2c_err
	mov	a,DPL0		;EEPROM-Adresse Low-Teil als zweites
	jmp	i2c_wr		;immer schreiben und Ende
;Adressbyte-Ausgabe tabellarisch
; I2C_ALen	wIndexH	!	 I²C-Ausgabe
;ALen1	ALen0	ALEN	!	1.Byte	2.Byte	
;0	0	0	!	-	-
;0	1	1	!	DPL	-		DPL = wValueL
;1	0	2	!	DPL	DPH		DPH = wValueH
;1	1	3	!	DPH	DPL

;=============================
;== I²C(EEPROM)-Ein/Ausgabe ==
;=============================

EWrite:	;(Boot-EEPROM oder beliebiges) I²C-Gerät schreiben
;PE: DPTR=(EEPROM-)Adresse (Ziel)
;    I2C_Addr = I²C-Adresse
;    I2C_ALen = I²C-Adresslänge sowie übrige Bits
;    I2C_PMask= Seiten-Maske
;    AutoPtr=Puffer-Adresse (zu schreibende Daten)
;    R4=Länge der Daten (0 = keine Daten)
;PA: CY=1 bei Fehler, dann R4=verbliebene Bytes im Puffer
;    DPTR erhöht
;VR: A,DPTR,R0,R2,R4
	acall	EAdr
	jc	ewe
	inc	r4
	sjmp	ewf
ewl:	
	LDX1
	acall	i2c_wr		;1 Byte schreiben (noch nicht brennen)
	jc	ewe
	inc	dptr		;EEPROM-Adresse mitzählen
	jnb	I2C_Paging,ewf
	mov	a,I2C_PMask
	anl	a,DPL0
	jz	ewp		;Seite zu Ende!
ewf:	djnz	r4,ewl		;in Seite, nächstes Byte
	jb	I2C_NoStop,ewr
ewe:	STX	I2CS,40h	;Stoppsequenz
ewr:	ret
ewp:	STX	I2CS,40h	;Stoppsequenz, damit brennen
	djnz	r4,EWrite	;weitermachen, nächster Block
	ret

EE_W:	jnb	I2C_Verify,EWrite	;sofort zum Schreiben gehen!
	push	DPL0		;Adresse zwecks Vergleichen retten
	push	DPH0
	push	AR4
	 acall	EWrite
	pop	AR4
	pop	DPH0
	pop	DPL0		;Adresse zurückstellen
	STXW	AutoPtr,Out0Buf	;auch Vergleichspuffer zurück
	setb	F0
	jnc	ERead		;Vergleichen
	ret

ERead:	;Boot-EEPROM lesen oder überprüfen
;PE: DPTR=EEPROM-Adresse (Quelle)
;    I2C_Addr = I²C-Adresse
;    I2C_ALen = I²C-Adresslänge sowie NoStart und NoStop-Bits
;    AutoPtr=Puffer-Adresse (Lese-Puffer), R1=XAutoDat
;    R4=Länge Daten/Puffer (0 = nicht erlaubt!)
;    F0=0: lesen, F0=1: prüfen/vergleichen (Verify)
;PA: CY=1 bei Fehler, dann R4=verbliebene Bytes im Puffer
;    DPTR erhöht
;VR: A,DPTR,R0,R2,R4
	jb	I2C_NoStart,erns	;Hier: Auch keine Adressbytes ausgeben!
	acall	EAdr
	jc	ere
	STX	I2CS,80h	;Noch eine Startsequenz ausgeben
erns:	mov	a,I2C_Addr	;I²C-Adresse
	inc	a		;Leseadresse
	acall	i2c_wr
	jc	ere
	inc	r4		;Problem: Klappt nicht mit R4=0
	acall	i2c_d		;Dummy-Lesezugriff von I2DAT
	dec	r4
erl:
	acall	i2c_rd		;Byte lesen
	jc	ere
	jb	F0,erv		;Vergleichen
	STX1
	sjmp	er1
erv:	mov	r0,a
	LDX1
	xrl	a,r0		;A=0 wenn gleich
	jnz	eru		;raus mit Fehler wenn ungleich
er1:	inc	dptr		;EEPROM-Adresse mitzählen
	djnz	r4,erl
	jb	I2C_NoStop,er9	;Stopp-Unterdrückung nur bei regulärem Ende!
	db	0E5h		;mov a,xx
eru:	setb	c
ere:	STX	I2CS,40h	;Stoppsequenz
er9:	ret

;===================================
;== Arbeitszellen-Initialisierung ==
;===================================

EReqInit:	;A2-Request auswerten und drei I²C-Parameter zusammenstellen
;PE: wIndex = USB-Parameter (ACHTUNG bei EZUSB Control Panel [Rindfleisch])
;VR: A,R0,I2C_Addr,I2C_ALen,I2C_PMask
	LDX	SetupDat+4	;wIndexL
	jnz	eri_nBootRom
EReqInitBoot:	;Seiteneinstieg für Startup
	LDX	I2CS		;bei wIndexL=0 Boot-ROM automatisch wählen
	mov	c,ACC.4		;ID1-Bit = 1 bei 16-bit-EEPROM
	mov	a,#EADDR
	mov	ACC.1,c		;ggf. aus A0 ein A2 machen
	mov	I2C_Addr,a
	clr	a
	addc	a,#1		;Adresslänge 1 oder 2 Bytes
	mov	I2C_ALen,a
	sjmp	eri_Auto
eri_nBootRom:
	clr	ACC.0		;Hier: Schreibadresse!
	mov	I2C_Addr,a
	LDX	NEXT		;wIndexH
	anl	a,#0Fh		;Low-Nibble
	mov	I2C_ALen,a	;übernehmen, wie es ist
	LDX			;noch einmal wIndexH
	swap	a
	anl	a,#0Fh		;High-Nibble
	jnz	eri_nAuto
eri_Auto:			;bei HINIBBLE(wIndexH)=0 Paginierung automatisch
	mov	a,I2C_Addr
	anl	a,#0F0h
	xrl	a,#0A0h		;Irgendein serieller EEPROM-Typ (Axh)?
	jnz	eri_e		;nein, keine (automatische) Paginierung
	mov	a,I2C_ALen
	cjne	a,#1,eri_n1	;Adress-Länge 1? (Und kein NoStop/NoStart?)
	mov	a,#EPAGE8-1	;Maske setzen
	sjmp	eri_s
eri_n1:	cjne	a,#2,eri_e	;Adress-Länge 2? Alles andere: ohne Auto-Paging!
	mov	a,#EPAGE16-1	;Maske setzen
	sjmp	eri_s
eri_nAuto:
	mov	r0,a		;Bitmaske erzeugen: 1->00, 2->01, 3->03
	clr	a		;4->07, 5->0F, 6->1F, 7->3F, 8->7F, 9->FF
	sjmp	eri_f
eri_l:	setb	c
	rlc	a		;nach links schieben, Einsen einfügen
eri_f:	djnz	r0,eri_l
eri_s:	mov	I2C_PMask,a
	setb	I2C_Paging	;Auto-Paging aktivieren
	setb	I2C_Verify	;Verify aktivieren (vorerst gemeinsam)
eri_e:	RETNC			;(womöglich später auch beim Lesen, wenn erf.)

;=============================
;== EEPROM- und RAM-Zugriff ==
;=============================
MemR:	LODS			;lesen
	STX1
	djnz	r4,MemR
	ret
MemW:	LDX1
	STOS			;schreiben
	djnz	r4,MemW
	ret
	
Partial_64:
;Subtrahiert 64 (bzw. R4) von der Gesamtlänge; R7=0 wenn letzter Block
	mov	r4,#64
Partial_r4:
	mov	a,r6
	subb	a,r4
	jnc	pa1
	clr	c
	djnz	r7,pa1	;Letzte "Runde" mit R7=0
	LD	r4,a,r6
	inc	r4
	ret
pa1:	mov	r6,a
	ret
	
LongEP0:
;Diese Routine funktioniert "absichtlich" nicht bei Längen >=FF00h,
;Control-Transfers sind ohnehin auf 1000h (4KB) beschränkt.
;Behandlung aller "langen" EP0-Transfers, mit selbstmodifizierendem Code
;dptr=Zeiger Leseroutine (R1=AutoPtr-EP0Buf, DPTR=wValue, R4=Länge)
;R7:R6=Zeiger Schreibroutine (dito, für OUT-Transfers)
;Alle diese Routinen bekommen DPTR=wValue, R4=Transferlänge, R1=AutoDat,
;und dürfen R6, R7 nicht verändern. (R7=0 für letzten Teiltransfer)
;F0 ist (zunächst) gelöscht
;R4=bmRequestType
	clr	F0
	mov	a,DPH0
	mov	r5,DPL0	;retten
	mov	dptr,#PatchR+1
	STORX
	mov	a,r5
	STORX	NEXT
	mov	a,r7
	mov	dptr,#PatchW+1
	STORX			;High-Teil
	mov	a,r6
	STORX	NEXT		;Low-Teil
	LDX	SetupDat+2	;wValueL
	mov	DPL0,a
	LDX	NEXT		;SetupDat+3
	mov	DPH0,a
	LDX	SetupDat+6	;wLengthL
	add	a,#0FFh
	mov	r6,a
	LDX	NEXT		;wLengthH
	addc	a,#0		;erhöhen für Funktion von Partial_xx
	mov	r7,a
	cjne	r4,#01000000b,a3_no	;bmRequestType
	jz	LEP0E		;Keine Daten!
LEP0W:	WAIT_EP0_OUT
	mov	r4,a
	call	Partial_R4
	STXW1	AutoPtr,Out0Buf
	inc	r1		;AutoData
PatchW:	lcall	0
	jc	LEP0E
	mov	a,r7
	jnz	LEP0W		;nächste Runde, sowie CY=0
	ret
	
a3_no:	cjne	r4,#11000000b,err2	;bmRequestType
	jz	LEP0E		;Keine Daten!
LEP0R:	call	Partial_64
	WAIT_EP0_IN
	push	AR4		;retten für In0BC
	 STXW1	AutoPtr,In0Buf
	 inc	r1		;AutoData
PatchR:	 lcall	0
	pop	ACC
	jc	LEP0E
	STX	In0BC
	mov	a,r7
	jnz	LEP0R		;nächste Runde, sowie CY=0
LEP0E:	ret
err2:	RETC

;=========================
;== Routinen für Ende 0 ==	;PE: R0=SetupDat, R4=SDAT[0], R5=SDAT[2]
;=========================	                  R6=SDAT[4], R7=SDAT[5]
IF MyID
 ClassOut	equ	00100001b	;Klassenrequest (Drucker)
 ClassIn	equ	10100001b
ELSE
 ClassOut	equ	01000000b	;VendorRequest zum Test im EzMr
 ClassIn	equ	11000000b
ENDIF

epid2r0:
;Rechnet Endpoint-ID in INxCS/OUTxCS-Adresse um
;PE: R6=[SetupDat+4]=Enden-Adresse
;PA: R0=Zeiger auf EPxCS (Low-Teil)
;VR: A,R0,C
	mov	a,r6
	rlc	a		;IN-Bit ausschieben
	cpl	c		;IN-Bit invertieren, also OUT-Bit
	mov	ACC.4,c		;OUT-Bit einsetzen
	anl	a,#1Eh		;Übrige Bits weg
	add	a,#LOW(EP0CS)
	mov	r0,a
	ret

GetStatus:
	cjne	r4,#ClassIn,gs_nid	;GET_DEVICE_ID? (Nur Interface 1)
	jmp	GetDeviceId
gs_nid:
	cjne	r4,#80h,gs_no_dev	;GS_DEVICE
nullin:	clr	a		;kein WakeUp(Bit1), kein SelfPower(Bit0)
wordin:	STX	IN0BUF
	STX	NEXT,0
	STX	IN0BC,2
	ret
gs_no_dev:
	cjne	r4,#81h,gs_no_if	;GS_INTERFACE
	jmp	nullin		;auch zwei Nullen melden
gs_no_if:
	cjne	r4,#82h,ep0stall	;GS_ENDPOINT
	acall	epid2r0
	LDX			;STALL-Bit angeln
	anl	a,#01h
	sjmp	wordin

ClearFeature:
	cjne	r4,#ClassIn,cf_nps	;Klassenrequest
;hier: GetPortStatus
	;cjne	r6,#1,ep0stall
	call	in1
	anl	a,#00111000b
	ajmp	gi		;Info-Byte senden
cf_nps:
	cjne	r4,#0,cf_no_dev	;FT_DEVICE
	cjne	r5,#1,ep0stall	;einziges Feature: Remote Wakeup
	;clr	Rwuen		;Disable Remote Wakeup
	ret
cf_no_dev:
	cjne	r4,#02h,ep0stall
	cjne	r5,#0,ep0stall	;einziges Feature: STALL
	acall	epid2r0
	STX	,0		;STALL entfernen
	mov	a,r6
	mov	c,ACC.7
	mov	ACC.4,c
	anl	a,#00010111b
restog:	STX	TOGCTL
	setb	ACC.5		;WRITEDELAY
	STX			;Togglebit zurücksetzen
	ret

SoftReset:
	cjne	r4,#ClassOut,ep0stall	;nur klassenspezifisch
	;cjne	r6,#1,ep0stall	;nur Interface 1
	STX	OutC,0111b
	clr	ACC.INI
	STX			;RESET-Impuls ausgeben
	setb	ACC.INI
	STX
	ret

SetFeature:
	cjne	r4,#0,sf_no_dev	;FT_DEVICE, nur Rem.Wakeup, nicht unterstützt
	cjne	r5,#1,ep0stall	;einziges Feature: Remote Wakeup
	;setb	Rwuen		;Enable Remote Wakeup
	ret
sf_no_dev:
	cjne	r4,#2,ep0stall	;FT_ENDPOINT
	cjne	r5,#0,ep0stall	;einziges Feature: STALL
	acall	epid2r0
	STX	,01h		;STALL setzen
	ret

GetDescriptor:
	cjne	r4,#80h,ep0stall	;nur DEVICE_IN
	LDX	SetupDat+3
	cjne	a,#01h,gd_no_dev	;GD_DEVICE
	mov	dptr,#DeviceDescriptor
gd_sto:	mov	a,DPH0
	STX	SUDPTRH
	mov	a,DPL0
	STX	NEXT
	ret
ep0stall:
	RETC

gd_no_dev:
	cjne	a,#02h,gd_no_conf	;GD_CONFIGURATION
	mov	dptr,#ConfigDescriptor
	sjmp	gd_sto
gd_no_conf:
	cjne	a,#03h,ep0stall		;GD_STRING
	mov	a,r5
	mov	r2,a
IF Interfaces = 2
	add	a,#-7			;Nur 0..6 zulassen
ELSE
	add	a,#-5			;Nur 0..4 zulassen
ENDIF
	jc	ep0stall
	call	MakeString
	mov	dptr,#StringPuffer
	sjmp	gd_sto

GetConfiguration:
	cjne	r4,#80h,ep0stall	;nur DEVICE_IN
	mov	a,Configuration	 ;Entweder konfiguriert oder unkonfiguriert
gi:	STX	IN0BUF+0
	STX	IN0BC,1
	ret

SetConfiguration:
	cjne	r4,#0,ep0stall		;nur DEVICE_OUT
	mov	a,r5
	add	a,#-2
	jc	ep0stall	;nur Konfiguration 0 oder 1
	mov	Configuration,r5
	ret

GetAltSet:
	cjne	r4,#81h,ep0stall	;nur INTERFACE IN
	cjne	r6,#1,as_n1
	mov	a,AltSetting1
	jmp	gi
as_n1:	jnc	ep0stall	;wenn Interface > 1
	clr	c
	clr	a
	jmp	gi

SetAltSet:
	cjne	r4,#1,ep0stall		;nur INTERFACE OUT
	cjne	r6,#0,as_n0	;Interface 0: h#s Parallelport
ResTog2:	;Data Toggle rücksetzen (EP2)
	mov	a,#02h		;OUT2 rücksetzen
	acall	restog
	mov	a,#12h		;IN2 rücksetzen
	acall	restog
;alle IN-Enden frei machen
	LDX	IN2CS		;BUSY (und STALL) lesen
	STX			;BUSY durch "1" schreiben rücksetzen
;alle OUT-Enden bereit machen
	STX	OUT2BC		;OUT2 scharfmachen
	ret
as_n0:
	cjne	r6,#1,ep0stall	;Interface 1: USB-Druckerunterstützung
	mov	a,r5
	add	a,#-2
	jc	ep0stall	;Nur Alternative 0 oder 1 zulassen
	mov	AltSetting1,r5
ResTog4:
	mov	a,#04h
	acall	restog
	mov	a,#14h
	acall	restog
	LDX	IN4CS
	STX
	STX	OUT4BC
sude:	ret

SUD_Tab:
	ajmp	GetStatus	;0 - auch: GET_DEVICE_ID
	ajmp	ClearFeature	;1 - auch: GET_PORT_STATUS
	ajmp	SoftReset	;2 (normalerweise ep0stall)
	ajmp	SetFeature	;3
err1:	RETC			;4
	RETC	;SetAddress	;5, sollte nicht vorkommen
	ajmp	GetDescriptor	;6
	RETC	;SetDescriptor	;7
	ajmp	GetConfiguration;8
	ajmp	SetConfiguration;9
	ajmp	GetAltSet	;10
	ajmp	SetAltSet	;11
	;RETC	;SyncFrame	;12

HandleSUD:			;liefert CY=1 für EP0Stall
	LDX	SetupDat	;bmRequestType -> R4
	mov	r4,a
	LDX	NEXT		;SetupDat+1: bRequest -> A
	cjne	a,#0A2h,hs1	;EEPROM-Zugriff?
	call	EReqInit	;wIndex heranziehen
	mov	dptr,#ERead
	MOVR	r6,r7,EE_W
	jmp	LongEP0
hs1:	cjne	a,#0A3h,hs2	;XRAM-Zugriff?
	mov	dptr,#MemR
	MOVR	r6,r7,MemW
	jmp	LongEP0
hs2:	mov	r7,a
	add	a,#-12		;nur 0..11 zulassen
	jc	err1
	LDX	NEXT		;SetupDat+2: wValueL -> R5
	mov	r5,a
	LDX	SetupDat+4	;wIndexL -> R6
	mov	r6,a
	LDX	NEXT		;SetupDat+5: wIndexH -> R7
	xch	a,r7
	call	usr31		;darf bei CY=1 kein Register ändern!
	jnc	sude		;bei CY=0 ist's des Users Eigenverantwortung
	JMPTBL	SUD_Tab

;==============================
;== Routinen für Druckerport ==
;==============================
SetECR:	;ECR-Byte setzen, FIFOs leeren
	anl	a,#0F8h
	orl	a,#5		;FIFO leer setzen
	mov	ECR,a
	swap	a
	rr	a
	anl	a,#7
	inc	a
	mov	r3,a
	clr	a
	setb	c
se1:	rlc	a
	djnz	r3,se1		;Bit draus machen, testet sich besser
	mov	ECR_Bits,a

	mov	c,ECR_Bits.4	;EPP-Bit
	mov	a,#0FFh
	subb	a,#0
	mov	EppTimeout,a	;beim Einschalten von EPP auf Null, sonst 1

	mov	a,ECR_Bits
	anl	a,#00000101b	;Bei SPP oder SPP-FIFO auf Ausgabe schalten!
	jz	se3		;kein Schalten am Richtungsbit!
	clr	direction
	call	DirChanged	;Ausgabetreiber stets aktiv - je nach OC-Simul.
se3:	mov	fifor,#Fifo
	mov	fifow,#Fifo
	ret

;===SPP-FIFO===========================
;Die Emulation der SPP-FIFO-Betriebsart
;-> Funktioniert nur mit ausgeschalteter OC-Simulation des Steuerports (+2)
SppXfer:
	jb	fifoe,exf	;wenn FIFO leer ist nichts zu tun
	LDX	PinsC
	jb	ACC.BSY,exf	;wenn beschäftigt dann geht's nicht
	mov	r0,fifor
	mov	AR4,@r0		;Datenbyte->R4
	acall	out0i		;Byte (R4) anlegen, Treiber muss aktiv sein
	LDX	OutC
	clr	ACC.STB
	STX			;Strobe aktivieren
	acall	IncFifoR	;Lesezeiger erhöhen (und Zeit verbrauchen)
	LDX	OutC
	setb	ACC.STB
	STX			;Strobe zurücknehmen
exf:	ret

;===EPP===========================
;-> EPP-Funktionen funktionieren hier nur ohne OC-Simulation.
wait_epp:
;PORTC mit R3 ANDen ausgeben und max. 10 µs auf WAIT=H warten
;sowie Datenrichtung temporär (auf Ausgabe) schalten
;PA: EppTimeOut.0 bei TimeOut, A=abgetastete Portpins
	LDX	OutC		;Zustand lesen
	push	ACC
	 anl	a,r3
	 STX			;Addr/DataStrobe LOW
	 mov	a,r3		;WRITE-Bit (Strobe)
	 rrc	a
	 jc	sw0
	 LDX	OEB		;bei Schreibzugriff: Zustand retten,...
	 push	ACC
	 STX	,0FFh		;temporär alles Ausgabe
sw0:	;max. 10 µs warten bis WAIT=H
	 mov	r3,#7		;7 Runden à knapp 2 µs
	 mov	r0,#LOW(PinsC)
sw1:	 LDX			;(2)
	 jb	ACC.BSY,sw2	;(3) WAIT endlich HIGH
	 djnz	r3,sw1		;(3)
	 setb	EppTimeOut.0	;TimeOut aufgetreten
sw2:	 jc	sw3		;Noch einmal CY auswerten!
	 pop	ACC		;falls Schreibzugriff: OE-Zustand zurück
	 STX	OEB
	 sjmp	sw4
sw3:	 acall	in0		;falls Lesezugriff: Portpins abtasten
	 mov	r3,a
sw4:	pop	ACC
	STX	OutC		;Zustand wiederherstellen
	mov	a,r3		;Im Falle von Schreibzugriff undefiniert!
	ret

out_epp:
;OUT-Befehl Byte R4 mit R3=Addr/DataStrobe, WRITE=0
;Steuerleitungen werden wie im Original nur bei Bedarf nach Low gezogen
;Datenrichtung wird (temporär) umgeschaltet. (Bei "richtigem" Port beobachtet.)
	acall	out0i		;Daten ausgeben (immer - vorher!)
	LDX	PinsC		;EPP 1.9: WAIT muss LOW sein!
	jb	ACC.BSY,no_epp
	jmp	wait_epp

in_epp:
;IN-Befehl mit r3=Addr/DataStrobe
;PA: A=gelesenes Byte
	LDX	PinsC		;EPP 1.9: WAIT muss LOW sein!
	jb	ACC.BSY,no_epp_read_PINSB
	jmp	wait_epp

no_epp_read_PINSB:
	acall	in0		;PinsB schnappen
no_epp:	setb	EppTimeOut.0	;TimeOut-Bit setzen
	ret			;nichts ausgeben!

;===ECP===========================
;-> ECP-Funktionen funktionieren nur, wenn die Hintertüren geschlossen wurden,
;   d.h. alle ungewöhnlichen Datenrichtungen sowie OC-Simulationen
;   ausgeschaltet sind.

IncFifoPtr_Compare:
;FIFO-Zeiger erhöhen und mit anderem Zeiger vergleichen
;PE: R0=Zeiger auf einen der beiden FIFO-Zeiger
;PA: A=0 wenn nach Erhöhung beide Zeiger gleich, sonst <>0
	mov	a,@r0
	inc	a
	inc	a
	cjne	a,#Fifo+FIFOSIZE*2,ic1
	mov	a,#Fifo
ic1:	mov	@r0,a
	mov	a,fifor
	xrl	a,fifow
	ret

IncFifoR:
;FIFO-Lese-Zeiger erhöhen, "FIFO voll" löschen, "FIFO leer" ggf. setzen
;PE: -
;PA: A=0 wenn FIFO leer
;VR: R0,A
	mov	r0,#fifor
	acall	IncFifoPtr_Compare
	jnz	ifr1
	setb	fifoe		;FIFO leer
ifr1:	clr	fifof		;FIFO ist keinesfalls voll
	ret

IncFifoW:
;FIFO-Schreib-Zeiger erhöhen, "FIFO leer" löschen, "FIFO voll" ggf. setzen
;PE: -
;PA: A=0 wenn FIFO voll
;VR: R0,A
	mov	r0,#fifow
	acall	IncFifoPtr_Compare
	jnz	ifw1
	setb	fifof		;FIFO voll
ifw1:	clr	fifoe		;FIFO ist keinesfalls leer
	ret

EcpXfer:
;Auf Transfer von/in FIFO im Hintergrund prüfen
;Diese Routine wird, sofern ECP aktiv, zyklisch aufgerufen
;Keine Parameter, VR: A,R0,R3,R4
	jb	direction,EcpInXfer
	LDX	OutC
	jnb	ACC.STB,oxf2	;2. Phase der Byte-Übertragung zz. aktiv
	jb	fifoe,no_Xfer	;Kann kein Byte rausschicken
	LDX	PinsC
	jb	ACC.BSY,no_Xfer	;Gegenstelle ist beschäftigt
	mov	r0,fifor
	mov	AR4,@r0		;Datenbyte->R4
	inc	r0
	mov	AR3,@r0		;Command/Data->R3
	acall	IncFifoR	;Lesezeiger erhöhen
	acall	out0i		;Byte (R4) anlegen
	STX	OEB,0FFh	;Treiber aktivieren
	mov	a,r3
	rrc	a		;Command(0) oder Data(1) ausschieben
	LDX	OutC
	mov	ACC.AF,C	;HostAck(AF,1) setzen
	setb	ACC.INI		;nReverseRequest auf HIGH (Init) (unnötig?)
	STX
	clr	ACC.STB		;HostClk(STB) auf LOW
	STX
oxf2:	LDX	PinsC		;PeriphAck abfragen
	jnb	ACC.BSY,no_Xfer	;Gegenstelle ist beschäftigt
	LDX	OutC
	setb	ACC.STB		;HostClk(STB) auf HIGH
	STX
	ret
EcpInXfer:
	jb	fifof,no_Xfer	;Kann kein Byte einlesen
	LDX	PinsC
	xch	a,r3
	LDX	OutC
	jb	ACC.AF,ixf2	;HostAck=H, 2. Phase des Byte-Lesens...
	xch	a,r3
	jb	ACC.ACK,no_Xfer	;Gegenstelle meldet (noch) keinen Bedarf
	xch	a,r3
	setb	ACC.AF		;HostAck=H
	STX
ixf2:	LDX	PinsC
	jnb	ACC.ACK,no_Xfer	;PeriphClk=L = noch nicht bereit
	mov	c,ACC.BSY	;Command(0) / Data(1)
	acall	in0		;Datenbyte abholen
	mov	r0,fifow
	mov	@r0,a		;abspeichern
	inc	r0
	clr	a
	mov	ACC.0,c		;Bit einsetzen
	mov	@r0,a
	acall	IncFifoW
	LDX	OutC
	clr	ACC.AF		;HostAck=L
	STX
no_Xfer:ret

DirChanged2:
;Aufzurufen, wenn sich die Steuerportrichtung ändert...
;Neues Control5V berechnen:
	mov	c,ECR_Bits.0	;SPP-Modus?
	anl	c,/TPControl
	orl	c,OCControl
	jc	dc_occontrol1
	jbc	Control5V,dc_occontrol0
	ret
dc_occontrol0:	;OpenCollector-Status löschen
	LDX	OEC
	orl	a,#00001111b	;Alle Ausgänge aktivieren
	STX	OEC
dc_oce:	ret
dc_occontrol1:
	jb	Control5V,dc_oce	;Bereits aktiviert: nichts tun!
	setb	Control5V	;Aktivierung vermerken
	LDX	OutC
DoOcControl:	;A=Datenbyte: Ausgabetreiber nachführen
	cpl	a
	anl	a,#00001111b
	mov	r3,a
	LDX	OEC
	anl	a,#11110000b
	orl	a,r3
	STX
	ret
	
DirChanged0:
;Aufzurufen, wenn sich die Datenportrichtung ändert...
;Neues Data5V berechnen:
	mov	c,OCData	;Feature-Register: aktiv?
	anl	c,/direction	;ECR-Register: Ausgabe?
	jc	dc_ocdata
	jbc	Data5V,DoSetOeb	;OpenCollector-Status löschen
	ret
DoSetOeb:
	clr	a
	jb	direction,dc_tb
	dec	a
dc_tb:	STX	OEB		;Treiberstatus setzen
	ret
dc_ocdata:
	jb	Data5V,dc_oce	;Ist schon an!
	setb	Data5V
	LDX	OutB
;DoOcData:	;A=Datenbyte: Ausgabetreiber nachführen (zu simpel für UP!)
	cpl	a
	STX	OEB		;High-ausgebende Leitungen hochohming
	ret


;===Portzugriffe=========================

;OUT-Unterprogramme bekommen in R4 das Argument,
;IN-Unterprogramme müssen das Byte in A liefern

;=== OUT auf Adresse +0 (Datenport) ===
out0:	mov	a,ECR_Bits
	anl	a,#10110011b	;0 = FIFO-Betriebsarten?
	jz	out0ecp		;mit A=0 in die FIFO
out0i:	mov	a,r4
out0a:	;Innerer Einsprung zur Datenbyte-Ausgabe
	mov	dptr,#BitReverse
	movc	a,@a+dptr
	STX	OutB
	jnb	Data5V,out0e	;Open-Collector-Simulation?
	cpl	a
	STX	OEB
out0e:	ret

;=== OUT auf Adresse +1 (Statusport) ===
out1:	LDX	OutA	;LED-Bit holen
	anl	a,#11101111b
	mov	r3,a
	mov	a,r4
	rl	a		;ERR auf richtige Bit-Position
	anl	a,#00010000b
	orl	a,r3	;übrige Bits einsetzen
	STX
	LDX	OutC	;Steuerbits holen
	anl	a,#00001111b
	mov	r3,a
	mov	a,r4
	rlc	a	;BSY ausschieben
	rl	a	;ACK ans Bit 0
	jb	DirectIo,out1ni	;Nicht invertieren
	cpl	c	;BSY invertieren
out1ni:	rrc	a	;BSY einschieben (künftig Bit 6), ACK ausschieben
	rrc	a	;ACK einschieben (Bit 7)
	mov	c,ACC.0	;Bit0 retten (wegen EPP)
	anl	a,#11110000b
	orl	a,r3
	STX
	anl	c,ECR_Bits.4	;Aufforderung, das TimeOut-Bit zu löschen?
	jnc	out1nepp
	clr	EppTimeOut.0	;TimeOut-Bit löschen
out1nepp:
	ret

;=== OUT auf Adresse +2 (Steuerport) ===
;Bit4=IRQ-Freigabe (nicht unterstützt, aber gespeichert)
;Bit5=Ausgabetreiber-Freigabe
out2:	mov	DCR,r4		;(2)
	mov	a,ECR_Bits	;(2)
	anl	a,#00000101b	;(2)
	jz	bidi1		;(3) Modus nicht SPP oder AutoStrobe
	clr	direction	;(2) DCR.5 klebt auf 0
bidi1:	LDX	OutC	;Damit die o.g. Hintertür klappt!
	anl	a,#11110000b
	mov	r3,a
	mov	a,r4
	jb	DirectIo,out2ni	;Nicht invertieren
	xrl	a,#00001011b
out2ni:	anl	a,#00001111b
	orl	a,r3
	STX
	jnb	Control5V,DirChanged
	acall	DoOcControl	;OC simulieren
DirChanged:
	jb	DirectIo,out2e	;Nicht die Datenportrichtung beeinflussen!
	acall	DirChanged0	;Ggf. OC-Simulation umschalten
	jnb	Data5V,DoSetOeb	;Ohne OC-Simulation Tristate-Treiber schalten
out2e:	ret
	
;=== OUT auf Adresse +3 (EPP-Adresse) ===
out3:	mov	r3,#0110b	;AddrStrobe (17) LOW
	jmp	out_epp

;=== OUT auf Adresse +4 (EPP-Daten) ===
out4:	mov	r3,#1100b	;DataStrobe (14) LOW
	jmp	out_epp

;=== OUT auf Adresse +400 (ECP-Daten-FIFO) ===
out400:	mov	a,ECR_Bits
	anl	a,#01001100b	;Konfigurationen mit FIFO
	jz	nix
	mov	a,#1		;Daten (=1)
out0ecp:
	jb	fifof,nix	;Nichts tun, wenn FIFO voll!
	mov	r0,fifow
	mov	@r0,AR4		;abspeichern
	inc	r0
	mov	@r0,a
	jmp	IncFifoW

;=== OUT auf Adresse +402 (ECP-Steuerport) ===
out402:	mov	a,r4
	call	SetECR
	jmp	DirChanged2	;Änderung OC-Status Steuerport?

;=== OUT auf Adresse +404 (Datenrichtung Datenport) [HINTERTÜR] ===
out404:	mov	a,r4
	mov	dptr,#BitReverse
	movc	a,@a+dptr
	STX	OEB
	ret

;=== OUT auf Adresse +405 (Datenrichtung Statusport) [HINTERTÜR] ===
out405:	mov	a,r4
	rl	a		;ERR auf richtige Bit-Position
	anl	a,#00010000b
	orl	a,#00100000b	;LED bleibt Ausgabe
	STX	OEA
	LDX	OEC	;Treiber der Steuerbits holen
	anl	a,#00001111b
	mov	r3,a
	mov	a,r4
	cpl	a
	rlc	a	;BSY und ACK vertauschen...
	rl	a
	rrc	a
	rrc	a
	anl	a,#11110000b
	orl	a,r3
	STX
	ret

;=== OUT auf Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
out406:	LDX	OEC	;Treiber der Statusbits holen
	anl	a,#11110000b
	mov	r3,a
	mov	a,r4
	anl	a,#00001111b	;Nur Bits 0..3 benutzen
	orl	a,r3
	STX
sf1:	ret

;=== OUT auf Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
out407:	mov	a,r4
SetFea:	anl	a,#01000101b	;Nur "bekannte" Bits durchlassen
	orl	a,#00000010b	;TPControl stets EIN (keine Pullups vorhanden)
	xch	a,Feature
	xrl	a,Feature	;Veränderungen?
	jz	sf1		;nein, gar nichts tun
	setb	FeatureChanged	;in EEPROM brennen (verzögert)
	jnb	ACC.6,setf6
	LDX	OutC
	xrl	a,#01001011b	;sofort entsprechende Bits drehen!
	STX
setf6:	acall	DirChanged0	;OC-Status Datenport nachführen
	ajmp	DirChanged2	;OC-Status Steuerport nachführen

;=== IN von Adresse +0 (Datenport) ===
in0:	mov	a,ECR_Bits
	anl	a,#10110011b	;0 = eine FIFO-Betriebsart?
	jz	infifo		;Datenport nicht rücklesbar!
	LDX	PinsB
	mov	dptr,#BitReverse
	movc	a,@a+dptr
nix:	ret

;=== IN von Adresse +1 (Statusport) ===
in1:			;(3-4) Aufruf VR: R3!
	LDX	PinsA	;(4)
	orl	a,#0EFh ;(2)	;ERR-Bit invers ausmaskieren
	mov	r3,a	;(1)
	LDX	PinsC	;(4)
	orl	a,#0Fh	;(2)
	rlc	a	;(1)	;ACK ausschieben nach C
	anl	a,r3	;(1)	;ERR ist gerade an passender Bitposition!
	rl	a	;(1)	;BSY nun auf Bit 0, C bleibt
	rrc	a	;(1)	;ACK einschieben, jetzt BSY in C
	jb	DirectIo,in1ni	;(4)
	cpl	c	;(1)	;BSY invertieren
in1ni:	rrc	a	;(1)	;Nun sind Bit 6 und 7 vertauscht!
	anl	a,EppTimeOut	;(2)Bit 0 löschen, wenn kein EPP-TimeOut
	RETNC		;(5) = (31..32)

;=== IN von Adresse +2 (Steuerport) ===
in2:	mov	a,ECR_Bits
	anl	a,#10110011b	;0 = FIFO-Betriebsart
	jz	in2o
	mov	a,DCR
	orl	a,#11001111b	;Interrupt- und Richtungs-Bit
	mov	r3,a
	LDX	PinsC
	jb	DirectIo,in2ni
	xrl	a,#00001011b
in2ni:	orl	a,#11110000b	;Ungenutzte Bits sind stets 1
	anl	a,r3
;NEU: Auch im Standard-Modus ist das Richtungsbit immer 0
	ret
in2o:	;Rücklesen des Ausgaberegisters; erspart Kopfzerbrechen mit ECP
	mov	a,DCR
	orl	a,#11000000b	;obere Bits lesen immer 1
	ret
retff:	mov	a,#0FFh
retin:	ret

;=== IN von Adresse +3 (EPP-Adresse) ===
in3:	mov	r3,#0111b	;mit AddrStrobe (17) LOW
	ajmp	in_epp

;=== IN von Adresse +4 (EPP-Daten) ===
in4:	mov	r3,#1101b	;mt DataStrobe (14) LOW
	ajmp	in_epp

;=== IN von Adresse +400 (ECP-FIFO) ===
in400:	mov	a,ECR_Bits
	anl	a,#11001100b
	jz	retff		;ohne ECP gibts kein Port+400h
	mov	a,#00010000b	;Konfigurationsregister (Konstante) A
	jb	ECR_Bits.7,retin
infifo:	mov	r0,fifor
	mov	a,@r0		;FIFO lesen
	jb	fifoe,retin	;Letztes FIFO-Byte liefern wenn FIFO leer
	mov	r3,a		;Logische Schwäche bei BeyondLogic:
				;Wohin mit dem "gelesenen" PeriphAck?
	acall	IncFifoR
	mov	a,r3
	ret

;=== IN von Adresse +401 (ECP-???) ===
in401:	jnb	ECR_Bits.7,retff
	clr	a		;Konfigurationsregister (Konstante) B
	ret

;=== IN von Adresse +402 (ECP-Steuerport) ===
in402:	mov	a,ECR
	ret

;=== IN von Adresse +404 (Datenrichtung Datenport) [HINTERTÜR] ===
in404:	LDX	OEB
	mov	dptr,#BitReverse
	movc	a,@a+dptr
	ret

;=== IN von Adresse +405 (Datenrichtung Statusport) [HINTERTÜR] ===
in405:	LDX	OEA
	anl	a,#00010000b
	rr	a		;ERR auf richtige Bit-Position
	mov	r3,a
	LDX	OEC
	rlc	a	;BSY und ACK vertauschen...
	rl	a
	rrc	a
	rrc	a
	anl	a,#11110000b	;4 Bits
	orl	a,r3		;und ein fünftes
	ret

;=== IN von Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
in406:	LDX	OEC
	anl	a,#00001111b	;Nur Bits 0..3, übrige Bits sagen "Eingabe"
	ret

;=== IN von Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
in407:	mov	a,Feature
	ret

;=== Warte-Hilfsbefehl, Argument R4 = Wartezeit in 4 µs ===
wait:	inc	r4		;Wir brauchen 4x6=24 Takte pro Durchlauf
w1:	inc	dptr		;kurzer Befehl mit langer Dauer!
	inc	dptr
	inc	dptr
	inc	dptr
	inc	dptr
	inc	dptr
	inc	dptr
	djnz	r4,w1		;3 Takte
	ret

;=== Sprungtabelle für IN- und OUT-Befehle ===
;					Abgefangener|EP2 Out EP2 Out|EP2 In
;	=========> USB-Pipe-Belegung:	"ASM-Befehl"|1. Byte 2. Byte|Antwort
;					------------------------------------
upv:	ajmp	out0	;		OUT +0,al   |	00h	al  |	-
	ajmp	out1	;		OUT +1,al	01h	al	-
	ajmp	out2	;		OUT +2,al	02h	al	-
	ajmp	out3	;EPP-Adresse	OUT +3,al	03h	al	-
	ajmp	out4	;EPP-Daten	OUT +4,al	04h	al	-
	ajmp	out4	;		OUT +5,al	05h	al	-
	ajmp	out4	;		OUT +6,al	06h	al	-
	ajmp	out4	;		OUT +7,al	07h	al	-
	ajmp	out400	;nur ECP	OUT +400h,al	08h	al	-
	ajmp	nix	;		OUT +401h,al	09h	al	-
	ajmp	out402	;		OUT +402h,al	0Ah	al	-
	ajmp	nix	;		OUT +403h,al	0Bh	al	-
	ajmp	out404	;HINTERTÜR	OUT +404h,al	0Ch	al	-
	ajmp	out405	;HINTERTÜR	OUT +405h,al	0Dh	al	-
	ajmp	out406	;HINTERTÜR	OUT +406h,al	0Eh	al	-
	ajmp	out407	;Feature	OUT +407h,al	0Fh	al	-
	ajmp	in0	;		al = IN +0	10h	-	al
	ajmp	in1	;		al = IN +1	11h	-	al
	ajmp	in2	;		al = IN +2	12h	-	al
	ajmp	in3	;EPP-Adresse	al = IN +3	13h	-	al
	ajmp	in4	;EPP-Daten	al = IN +4	14h	-	al
	ajmp	in4	;		al = IN +5	15h	-	al
	ajmp	in4	;		al = IN +6	16h	-	al
	ajmp	in4	;		al = IN +7	17h	-	al
	ajmp	in400	;nur ECP	al = IN +400h	18h	-	al
	ajmp	in401	;		al = IN +401h	19h	-	al
	ajmp	in402	;		al = IN +402h	1Ah	-	al
	ajmp	retff	;		al = IN +403h	1Bh	-	al
	ajmp	in404	;HINTERTÜR	al = IN +404h	1Ch	-	al
	ajmp	in405	;HINTERTÜR	al = IN +405h	1Dh	-	al
	ajmp	in406	;HINTERTÜR	al = IN +406h	1Eh	-	al
	ajmp	in407	;Feature	al = IN +407h	1Fh	-	al
	ajmp	wait	;Code zum Warten (kein)		20h   x * 4 µs	-
;					------------------------------------
;Viele OUT-Befehle tun nichts, fast genauso viele IN-Befehle liefern nur FFh.
;Die Warte-Routine bietet die Möglichkeit, Port-Zugriffe zeitlich
;auseinanderzuziehen. (Hoffentlich braucht das niemand!)
;Die Bulk-Daten dürfen beliebig viele OUT- und IN-Befehle zusammenfassen.
;Alle nicht gelisteten 1. Bytes (also >20h) sowie fehlende Folge-Bytes
;im jeweiligen Bulk-Datenblock führen zum Abbruch der Bearbeitung des Blocks.

Aufruf:			;R3=UP-Nummer
	mov	a,r3
	JMPTBL	upv

;=== IEEE1284 ===
Request:
;Datenbyte liegt bereits an PortB, Steuerleitungen im Defaultzustand
	clr	c
	STX	OutC,1101b	;SEL=high, AF=low
	mov	r2,#7		;max. 10 ms warten
	mov	r4,#0
rl1:	acall	in1		;(27)
	anl	a,#01111000b	;(2)	ACK=LOW, PE=HIGH, SEL=HIGH, ERR=HIGH?
	xrl	a,#00111000b	;(2)
	jz	ry1		;(3)
	djnz	r4,rl1		;(3) * 256 = 9216T = 1536µs
	djnz	r2,rl1
	RETC
ry1:	STX	OutC,1100b	;STB=low
	mov	r4,#2		;1 µs
rl3:	djnz	r4,rl3		;(3)
	STX	,1111b		;AF=high, STB=high
	mov	r2,#7		;max. 10 ms warten
	;mov	r4,#0
rl2:	acall	in1		;(27)
	anl	a,#01111000b	;(2)	ACK=HIGH, PE=LOW, SEL=HIGH, ERR=LOW?
	xrl	a,#01010000b	;(2)
	jz	ry2		;(3)
	djnz	r4,rl2		;(3) * 256 = 9216T = 1536µs
	djnz	r2,rl2
	setb	c
ry2:	ret	
	
Nibble:
;PA: Nibble in A (sowie R5) im High-Teil!
;VR: R0,R2,R3
	clr	c
	STX	OutC,1101b	;AF low
	mov	r3,#3		;max. 1 ms warten
	mov	r2,#0
	mov	r0,#LOW(PinsC)
nl1:	LDX			;(2)
	jnb	ACC.ACK,ny1	;(4)
	djnz	r2,nl1		;(3) * 256 = 2304T = 400µs
	djnz	r3,nl1
	RETC
ny1:	rl	a		;Diese Schaltung ist nibble-mode-freundlich!
	anl	a,#11100000b	;Bits schon richtig
	mov	r2,a
	LDX	PinsA
	anl	a,#00010000b
	orl	a,r2
	push	ACC
	 STX	OutC,1111b	;AF high
	 mov	r3,#3		;max. 1 ms warten
	 mov	r2,#0
	 mov	r0,#LOW(PinsC)
nl2:	 LDX			;(2)
	 jb	ACC.ACK,ny2	;(4)
	 djnz	r2,nl2		;(3) * 256 = 2304T = 400µs
	 djnz	r3,nl2
	 setb	c		;TimeOut
ny2:	pop	ACC
	ret

TwoNibbles:
;VR: R0,R2,R3,R5
	acall	Nibble	;Low-Nibble
	jc	tne
	swap	a
	mov	r5,a
	acall	Nibble	;High-Nibble
	orl	a,r5
tne:	ret

GetDeviceId:
;Beschafft Drucker-ID im Nibble-Modus
;PA: CY=1: Fehler
;    CY=0: Daten über EP0 (64-Byte-weise) übertragen, kein STALL
;VR: R0..R7
	STX	OutC,0111b	;SEL=low, INI=high, AF=high, STB=high
	STX	PREV,20h	;"Get Device ID using Nibble Mode" (bitrevers)
	acall	Request
	jc	gdie
	mov	dptr,#GetDevId	;Kernfunktion für Universalroutine
	mov	r4,#11000000b	;"Vendor-Request", IN
	call	LongEP0		;Universalroutine arbeiten lassen
	STX	OutC,0111b	;Standard-Modus zurück
gdie:	ret
	
GetDevId:
;Callback-Routine mit R4=Transferlänge und R1=AutoDat
	jb	F0,GetDevIdCont
	acall	TwoNibbles
	jc	gdie
	STX1
	djnz	r4,gdi1
	ret
gdi1:	mov	DPH0,a	;High-Teil retten
	acall	TwoNibbles
	jc	gdie
	STX1
	;DPTR := 2 - Länge (also im Regelfall eine negative 16-bit-Zahl)
	mov	r5,a
	mov	a,#2
	subb	a,r5
	mov	DPL0,a	;Low-Teil
	clr	a
	subb	a,DPH0
	mov	DPH0,a	;High-Teil
	clr	c
	setb	F0	;Fortsetzungs-Bit
	sjmp	gdi3
GetDevIdCont:
	mov	a,DPH0
	jz	gdi4	;Rest mit Nullen füllen
	acall	TwoNibbles
	jc	gdie
	inc	dptr	;solange negativ kommen Bytes vom Drucker
gdi4:	STX1
gdi3:	djnz	r4,GetDevIdCont
	ret

;== Drucker-Klassen-Simulation ==
UniXfer:	;Aufruf wenn Daten in OUT4BUF vorhanden sind
	LDX	PinsC
	jb	ACC.BSY,ux1	;BUSY prüfen, nichts tun wenn beschäftigt
	mov	a,UniIdx	;Index in OUT4BC holen
	add	a,#LOW(OUT4BUF)
	mov	DPL0,a
	mov	DPH0,#HIGH(OUT4BUF)
	movx	a,@dptr		;Datenbyte aus Puffer holen
	acall	out0a		;anlegen
	mov	a,#0111b	;SEL=low für Standard-Parallelport-Betrieb
	STX	OutC
	clr	ACC.STB
	STX			;Strobe-Signal erzeugen (STB=low)
	inc	UniIdx		;Index erhöhen
	setb	ACC.STB
	STX			;6 Zyklen später (1 µs) STB=HIGH
	mov	a,#166
	acall	StartLed
	LDX	OUT4BC
	xrl	a,UniIdx	;Puffer ausgelesen?
	jnz	ux1		;nein
	STX			;ja, Puffer an USB übergeben, scharf machen
	mov	UniIdx,a	;Index auf Null stellen
ux1:	ret

;=== LED-Spielerei ===

StartLed:
;Start des LED-Blinkens mit a=Blinkperiodendauer, 0=Maximum
;Blinkt die LED bereits, wird nur die künftige Blinkperiode gesetzt
;VR: A,R0=LOW(OUTA)
	mov	LedTime,a	;Zeitzähler (Monoflop) neu starten
	mov	BlinkF,a	;Blinkfrequenz setzen
	jb	LedBlink,nost
	mov	Blink,a
	setb	LedBlink
	LDX	OutA
	clr	ACC.LED
	STX	OutA		;LED ausschalten (Zugriff beginnt)
nost:	ret

HandleUsbSleep:
;Alles auf Energiesparen schalten; kehrt mit dem Wakeup des Prozessors zurück
	LDX	OEA
	push	ACC
	STX	,0		;alles hochohmig und Energie sparend
	LDX	NEXT		;OEB
	push	ACC
	STX	,0
	LDX	NEXT		;OEC - mit der Hintertür als Eingang möglich
	push	ACC
	STX	,0
	 mov	PCON,#31h	;Tiefschlaf hier
	pop	ACC
	STX			;OEC
	pop	ACC
	STX	PREV		;OEB
	pop	ACC
	STX	PREV		;OEA
	ret
	
SaveFeature:
	clr	FeatureChanged
	mov	a,Feature
	STORX	Scratch
	call	EReqInitBoot
	STXW1	AutoPtr,Scratch
	inc	r1		;XAutoDat
	mov	r4,#1
	mov	dptr,#0FFFBh	;Letztes Byte vor Seriennummer
	jmp	EWrite		;Daten schreiben (ohne Verify)

;=====================
;== Initialisierung ==
;=====================
main:
	clr	a
	mov	CKCON,a		;Handbremse für "movx" lösen
	mov	r0,#7Fh
	mov	MPAGE,r0	;die meisten XRAM-Register
	mov	SP,r0
clr_ram:mov	@r0,a
	djnz	r0,clr_ram
	call	MakeBitReverse
;Persistente Konfiguration lesen und übernehmen
	call	EReqInitBoot
	STXW1	AutoPtr,Scratch
	inc	r1		;XAutoDat
	mov	r4,#1
	mov	dptr,#0FFFBh	;Letztes BYTE vor Seriennummer
	call	ERead		;persistente Daten lesen
	LOADX	Scratch
	inc	a
	jz	w3		;FFh als 00h annehmen
	dec	a
w3:	call	SetFea		;setzen
	clr	FeatureChanged
;Ports initialisieren
	mov	a,#20h		;PS/2-Modus (zweckmäßiger?)
	call	SetECR

	STX	CPUCS,0		;Wir brauchen keinen CLK24-Ausgang

	STX	OutA,20h	;LED einschalten (bleibt an, inverse Op.)
	STX	NEXT,0		;0 gibt das BIOS standardmäßig aus (?)
	STX	NEXT,07h	;AF, INI und STB auf High, SEL auf Low

	STX	OEA, 20h	;Ausgabe für LED auf PortA
	jb	DirectIo,w4	;Bei DirectIo bleibt alles Eingabeports
	STX	NEXT,0FFh	;alles Ausgabe auf PortB
	STX	NEXT,0Fh	;Ausgabe PortC Bit 0..3
w4:
	setb	EICON.5		;Resume-Interrupt (auch bei EA=0)

	STX	USBPAIR,011011b	;OUT4,OUT2,IN4,IN2 doppelt gepuffert
				;(macht der EZUSB beinahe gratis!)
	STX	NEXT,00010101b	;nur EP4 und EP2 ist OK
	STX	NEXT
	STX	NEXT,0		;keine isochronen Transfers
	STX	NEXT
	LDX	USBCS
IF Always_Renum=0
	jb	ACC.1,mainloop	;ReNum überspringen, falls vom EEPROM geladen
	clr	ACC.2		;DiscOE
	STX
ENDIF
	setb	ACC.3		;DisCon
	STX
	setb	ACC.1		;ReNum
	STX
	mov	r5,#6		;1,5 Sekunden warten
	mov	r3,#0
w2:	acall	w1		;verputzt R4=0
	djnz	r3,w2
	djnz	r5,w2
	xch	a,r4		;A=0
	dec	a		;A=0FFh
	STX	IN07IRQ		;alle alten Interruptanforderungen löschen
	STX	NEXT		;OUT07IRQ
	STX	NEXT		;USBIRQ
	xch	a,r4
	clr	ACC.3		;DisCon
	STX	USBCS
	setb	ACC.2		;DiscOE
	STX
;===================
;== Hauptschleife ==
;===================
mainloop:
	call	usr36		;Zyklischer User-Aufruf
	jnb	FeatureChanged,skipSF
	call	SaveFeature
;Zuerst "USB-Interrupt-Polling"
skipSF:	jnb	ECR_Bits.2,skip0;SPP-FIFO-Transfer prüfen
	call	SppXfer
skip0:	jnb	ECR_Bits.3,skip1;ECP-FIFO-Transfer prüfen
	acall	EcpXfer
skip1:	LDX	USBIRQ		;USB-Interrupts prüfen
	jz	nu4		;Kurzschluss-Sprung
	STX			;alle Anforderungen löschen
	mov	IntReq,a
	jnb	IntReq.0,nu0	;SUDAV-Anforderung?
	clr	a		;lang (Blinken mit 2 Hz)
	acall	StartLed
	call	HandleSUD	;liefert CY=1 für EP0Stall
	mov	a,#1		;HSNAK
	rlc	a		;EP0Stall
	STX	EP0CS
nu0:
	jnb	IntReq.1,nu1	;SOF-Anforderung?
	jnb	LedBlink,nu1	;LED blinkt?
	LDX	OutA
	djnz	Blink,nbl
	cpl	ACC.LED
	STX
	mov	Blink,BlinkF
nbl:	djnz	LedTime,nu1
	setb	ACC.LED
	STX			;Nach paar Millisekunden bleibt das Licht an
	clr	LedBlink
nu1:
	jnb	IntReq.3,nu3	;Einschlaf-Anforderung?
	;Irgendwie kommt es beim Busaufzählen zu 2 Einschlaf-Aufforderungen,
	;die zum "Defekt" des USB-Gerätes führen.
	;Ob das Abwarten von SET CONFIURATION wirklich ausreicht? Ja!
	jnb	Configuration.0,nu3
	call	HandleUsbSleep
nu3:
	jnb	IntReq.4,nu4	;USB-Reset?
	;ohne Behandlung folgt Absturz beim Booten mit angestecktem Gerät?
	mov	Configuration,#0
	call	ResTog2		;etwa wie SetInterface (=SetAltSetting)
	call	ResTog4
nu4:
	LDX	OUT4CS
	jb	ACC.1,nep4
	acall	UniXfer
nep4:
;dann Auswertung des OUT-Bulk-Transfers
;AutoPtr: OUT2BUF-Zeiger, r2=Länge
;7E:R1: IN2BUF-Zeiger, Länge ergibt sich am Ende durch R1
;R3=Befehlskode
;R4=Daten (bei IN-Befehl A)
	LDX	OUT2CS
	jb	ACC.1,mainloop
	LDX	NEXT
	mov	r2,a		;Anzahl der Bytes des Bulk-Transfers

	mov	a,#100		;kurz (Blinken mit 5 Hz)
	acall	StartLed

	STX	AUTOPTRH,HIGH(OUT2BUF)
	STX	NEXT,LOW(OUT2BUF)
	mov	r1,#LOW(IN2BUF)

l1:	jnb	ECR_Bits.2,skip2;SPP-FIFO-Transfer prüfen
	call	SppXfer
skip2:	jnb	ECR_Bits.3,skip3;ECP-FIFO-Transfer prüfen
	acall	EcpXfer
skip3:	LDX	AUTODATA	;"Befehls"-Byte vom OUT2-Puffer lesen
	CMP	a,#21h
	jc	l2
	call	usr2f		;zur Extension springen
	jc	raus		;Bulk-Block-Rest bei Fehler verwerfen
	sjmp	bo1
l2:
	mov	r3,a
	jb	ACC.4,InBefehl
	dec	r2
	mov	a,r2
	jz	raus		;Fehler, wenn kein Folge-Byte folgt
	LDX			;weiteres AUTODATA-Byte lesen
	mov	r4,a
	acall	Aufruf
	sjmp	bo1
InBefehl:
	LDX	IN2CS		;kann (=will) Befehl nicht verarbeiten,
	jb	ACC.1,raus	;wenn Daten noch nicht abgeholt wurden
	acall	Aufruf
	dec	MPAGE
	movx	@r1,a		;IN-Byte in IN2BUF ablegen
	inc	MPAGE
	inc	r1
bo1:	djnz	r2,l1
raus:
	mov	a,r1
	;add	a,#-LOW(IN2BUF)	;ist glücklicherweise Null
	jz	no_in
	STX	IN2BC		;Bulk-In "anschubsen"
no_in:	STX	OUT2BC		;Bulk-Out scharfmachen
	jmp	mainloop

;USB-Deskriptoren
;********************************************
;** Antwort auf Get Descriptor: Device (1) **
;********************************************
;Geräte-Beschreiber
DeviceDescriptor:
	db	18	;bLength
	db	1	;bDescriptorType, 1=DEVICE
	DWI	101h	;bcdUSB 1.0
	db	0	;bDeviceClass		Multifunktion
	db	0	;bDeviceSubClass
	db	0	;bDeviceProtocol
	db	64	;bMaxPacketSize0
IF MyID
	DWI	16C0h	;idVendor		Voti
	DWI	06B3h	;idProduct		h#s Sub-ID
	DWI	461Fh+Interfaces ;bcdDevice	USB2LPT, USB2LPT2
ELSE
	DWI	0547h	;idVendor		Cypress
	DWI	2131h	;idProduct		EZ-USB (1!)
	DWI	4621h	;bcdDevice		irgendetwas
ENDIF
	db	1	;iManufacturer (1)
	db	2	;iProduct (2)
	db	0	;iSerialNumber		keine Seriennummer!
	db	1	;bNumConfigurations

;********************************************
;** Antwort auf Get Descriptor: Config (2) **
;********************************************
ConfigDescriptor:
;Konfigurations-Beschreiber 0
	db	9	;bLength
	db	2	;bDescriptorType	2=CONFIG
	DWI	CfgE-ConfigDescriptor	;wTotalLength
	db	Interfaces	;bNumInterfaces
	db	1	;bConfigurationValue (willkürliche Nummer dieser K.)
	db	3	;iConfiguration (3)
	db	80h	;bmAttributes (Busversorgt, kein Aufwecken)
	db	100/2	;MaxPower (in 2 Milliampere)
;Interface-Beschreiber 0, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	0	;bInterfaceNumber
	db	0	;bAlternateSetting
	db	2	;bNumEndpoints
	db	-1	;bInterfaceClass	hersteller-spezifisch
	db	-1	;bInterfaceSubClass
	db	-1	;bInterfaceProtocol
	db	4	;iInterface (4)
;Enden-Beschreiber C0I0A0:Bulk EP2OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	2	;bEndpointAddress	EP2OUT
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
;Enden-Beschreiber C0I0A0:Bulk EP2IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	82h	;bEndpointAddress	EP2IN
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
IF Interfaces = 2
;Interface-Beschreiber 1, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	1	;bInterfaceNumber
	db	0	;bAlternateSetting
	db	1	;bNumEndpoints
	db	7	;bInterfaceClass	Drucker
	db	1	;bInterfaceSubClass	Drucker
	db	1	;bInterfaceProtocol	unidirektional
	db	5	;iInterface (5)
;Enden-Beschreiber C0I1A0:Bulk EP4OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	4	;bEndpointAddress	EP4OUT
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
;Interface-Beschreiber 1, Alternative 1:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	1	;bInterfaceNumber
	db	1	;bAlternateSetting
	db	2	;bNumEndpoints
	db	7	;bInterfaceClass	Drucker
	db	1	;bInterfaceSubClass	Drucker
	db	2	;bInterfaceProtocol	bidirektional
	db	6	;iInterface (6)
;Enden-Beschreiber C0I1A1:Bulk EP1OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	4	;bEndpointAddress	EP4OUT
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
;Enden-Beschreiber C0I1A1:Bulk EP1IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	84h	;bEndpointAddress	EP4IN
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
ENDIF
CfgE:
;***=======******************************************
;** Etwaige Antwort auf Get Descriptor: String (3) **
;***=======******************************************
StringDescriptors:
;Wer Umlaute oder Chinesisch mag, muss für die folgenden Strings einen Editor
;benutzen, welcher entweder ISO-Latin1 oder UTF-8 speichert!
	db	0D0h,087h,0		;Deutsch (0407h in UTF8)
	db	"haftmann#software",0
	db	"h#s USB2LPT - der Umsetzer, der wirklich funktioniert!",0
	;db	"h#s USB2LPT - the converter that really works!",0
	db	"Busversorgt, kein Aufwecken",0
	;db	"Power supply by USB wire, no remote wakeup",0
	db	"USB2LPT - direkt steuerbares Parallelport",0
	;db	"USB2LPT - directly controlable parallel port",0
IF Interfaces = 2
	db	"Unidirektionales Drucker-Interface wie PL-2305",0
	;db	"Unidirectional printer interface, like PL-2305",0
	db	"Bidirektionales Drucker-Interface wie PL-2305",0
	;db	"Bidirectional printer interface, like PL-2305",0
ENDIF

ALIGN 256
BitReverse: ds 100h	;Hier Zugriff mit movc a,@a+dptr möglich
StringPuffer: ds 100h	;Dieser Puffer darf Seitengrenze nicht überlappen!

END

;"Richtige" Interrupt-Simulation (ACK) erfordert Schaltungsänderung!
;PortB und PortC müssten getauscht werden, weil nur INT4 oder INT6 geht
;Und ist schwierig am PC zu simulieren (will sagen, ich weiß nicht, wie)
;Ganz zu schweigen von ECP-DMA!
;USB-mäßig wäre es natürlich ein simpler 1-Byte-USB-INT-Transfer...

