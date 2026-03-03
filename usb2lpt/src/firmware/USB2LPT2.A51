;FX2-Firmware für USB2LPT2, haftmann#software 08/05
;Bis jetzt wird nur ein dptr benutzt (also kein DPS), nur Registerbank 0
;Zu übersetzen mit ASEM51; EEPROM-Datei erstellen mit
;HEX2BIX -F 0xC2 -I -V 0x16C0 -P 0x06B3
;Pro OUT-Befehl werden zwei Bytes in die OUT-Pipe geschrieben.
;Für jeden IN-Befehl wird ein Byte [Adresse] in die OUT-Pipe gesetzt,
; als Folge kommt ein Byte [Daten] in die IN-Pipe. Genaueres siehe "upv"!
;Seriennummern-Position: 1 Byte @ FFFFh (alt), 4 Byte @ FFFCh (Intel, neu)
;06xxxx	EEPROM+XRAM-Routinen
;060622	Gekaufte VID+PID eingetragen
;060629	Einzelpin-Richtungsumschaltung
;060630 Open-Collector-Simulation (Daten- und Steuerpins)
;060706	Richtungsbit-Simulation korrigiert (stets 0 bei Mode 0 und 2)
;060710	EPP: allgemeine Fehlerbeseitigung, getestet
;060711	Schattenregister für Steuerport, Code-Erweiterbarkeit für Anwender
;060809	DirectIo, korrigiertes Open-Collector-Verhalten (Steuerport), EEPROM
;060811	Drei ungenutzte PortD-Leitungen auf drei Massepins geroutet
;070128	Richtungsbits geändert auf 0=Eingang, 1=Ausgang (wie EZUSB und ATmega)
;070128	Steuerport-Richtungsfestlegung über High-Nibble von GpifIdleCfg
;070209	Achtelung der Helligkeit allzu heller blauer LEDs mit Schaltbit
;101212	Drittes HID-Interface für User-Mode-Zugriff ohne Kerneltreiber
;110922	HID-Interface mit differenzierten Usages (immer noch ungetestet)
;Zu tun:

	$nopaging
	$nosymbols
	$nomod51
	$nolist
	$include(ezusbfx2.mcu)
	$include(makros.i51)
	$list
	$genonly		;das "Innere" von Makros
	$condonly		;das "Innere" von IFxx

DateYear	equ	2011	;"Versionskennung"
DateMonth	equ	9
DateDay		equ	22
Interfaces	equ	3	;1 = ohne "USB-Druckerunterstützung"
				;(so lässt es sich besser debuggen)
MyId		equ	1	;0 = Cypress-ID (Debuggen mit Cypress-Tools)
Always_Renum	equ	0	;1: Firmware-Update im Gerätemanager versagt
Gpif		equ	0	;Benutze GPIF - oder Handarbeit fürs Drucken

;###############
;## Schaltung ##
;###############
;Datenport  (Basisadresse+0) = PortB (mit korrekter Bit-Anordnung, = FIFO-Port)
;Statusport (Basisadresse+1) = PortA (Bits 5-4-3 auf Portbits 5-6-1)
;			   und RDYx  (Bits 7-6 auf Portbits 0-1)
;Rev.3:			     = PortD (Bits 7-3 auf Portbits 7-3), RDYx gleich
;Rev.4:			     zusätzlich PortD (Bits 2-0 auf Extra-Bits 2-0)
;Steuerport (Basisadresse+2) = PortA (Bit 2 auf Portbit 2)
;			   und CTLx  (Bits 0-1-3 auf Portbits 0-1-2)
;Rev.3:			     = PortA (Bits 3-0 auf Portbits 3-0), CTLx gleich
;Die Zuweisung auf CTLx und RDYx erfolgte für Implementierbarkeit von
;SPP, ECP und EPP mithilfe von GPIF (also sehr schnell!),
;die übrige Zuweisung auf PortA nach günstigster Leiterbahnführung.
;Problem: Steuerport ist nicht rücklesbar! Korrektur in Revision 3,
;	  durch Routing der CTLx-Leitungen auf PortD
;Die Symbole PE,ONL,ERR,INI werden nur für Rev.2 verwendet.						JTAG		Rev.3
 BSY	EQU	0	;RDY1	(/7)	(ist mit D6 verbunden)	IOD.7
 ACK	EQU	1	;RDY0	( 6)				IOD.6
 PE	EQU	5	;IOA.5	( 5)	(ist mit D6 verbunden)	IOD.5
 ONL	EQU	6	;IOA.6	( 4)	D/P	TDO		IOD.4
 ERR	EQU	1	;IOA.1	( 3)	Ucc	Ucc		IOD.3
 
 SEL	EQU	2	;CTL2	(/3)				IOA.3
 INI	EQU	2	;IOA.2	( 2)				IOA.2
 AF	EQU	1	;CTL1	(/1)				IOA.1
 STB	EQU	0	;CTL0	(/0)				IOA.0
;			;D4		/PROG (zieht TDO auf Low)
;			;D3		/CTRL (aktiviert 2 Ausgangstreiber)
;			;D2		PROG	TMS
;			;D1		CCLK	TCK
;			;D0		DIN	TDI
 Ledr2	equ	IOA.0	;LOW-aktiv				IOA.4
 Led2r2	equ	IOA.7	;LOW-aktiv				IOA.5
Led	equ	IOA.4
Led2	equ	IOA.5


DefIOA	equ	0FFh	;alles High, auch LEDs
 DefOEAr2 equ	10011101b
;IOA.1 und IOA.3 sind noch frei, evtl. Steuerung der Pull-UpWiderstände
;	in Rev. 3 zur Energieeinsparung bei Idle, IOA.1 für ACK-Interrupt
DefOEA equ	10110100b
;###############				
;## Endpoints ##		USB2LPT-Rev.	0,1	2-4,7	5,6
;###############		Speed		Full	High	Low
;Pipe	Funktion				EZUSB	FX2	V-USB
;0	OUT-Adressen, OUT-Daten, IN-Adressen	EP2Out*	EP2Out*	EP1Out
;1	IN-Daten				EP2In*	EP6In*	EP1In
;2(=0)	USB-Druckerunterstützung Vorwärtskanal	EP4Out*	EP4Out*	-
;3(=1)	USB-Druckerunterstützung Rückkanal	EP4In*	EP8In*	-
;4(=0)	HID Out					EP1Out	EP1Out	-
;5(=1)	HID In					EP1In	EP1In	EP3In
;* = doppelt gepuffert
;-----------
;## DATEN ##	Der Datenbereich wird beim Firmware-Start mit Nullen gelöscht
;-----------
DSEG AT 20h	;bitadressierbar
IntReq:		ds	1	;für USB-Interruptrequests, bitadressierbar
Configuration:	ds	1	;hier: Null (adressiert) oder Eins (konfig.)
AltSetting1:	ds	1	;Null, Eins oder Zwei (nur für 2. Interface)
bits:		ds	1	;diverse Bits
HighSpeed	BIT	bits.0	;Kopie von UsbIrq.HsGrant
Led_B		BIT	bits.1	;Solange gelbe LED blinkt
Led2_B		BIT	bits.2	;Solange blaue (High-Speed-)LED blinkt
Rev2		BIT	bits.3	;Zur Verzweigung bei dennoch gleicher Firmware
FeatureChanged	BIT	bits.4	;Feature-Byte in EEPROM brennen (persistent)
Led2State	BIT	bits.5	;Eigentlicher Zustand der blauen LED
Data5V		BIT	bits.6	;wie OCData && !direction
Control5V	BIT	bits.7	;wie (SPP && !TPControl) || OCControl
;===Druckerport===
DCR:		ds	1	;Device Control Register (+2)
ackIntEn	BIT	DCR.4	;1=ein, rücklesbar, aber nicht unterstützt
direction	BIT	DCR.5	;0=OUT, 1=IN, klebt auf 0 bei Mode=0 oder =2
ECR:		ds	1	;Extended Control Register (ECP 402)
fifoe		BIT	ECR.0	;1 wenn FIFO leer
fifof		BIT	ECR.1	;1 wenn FIFO voll
ECR_Bits:	ds	1	;Mit immer nur einem Bit gesetzt
EPPTimeOut:	ds	1	;Bit0=0: kein TimeOut aufgetreten
				;Bit2=0: Interrupt aufgetreten (zz. ungenutzt)
				;Alle anderen Bits müssen =1 sein!
Feature:	ds	1
OCData		BIT	Feature.0 ;Offene-Senke-Simulation für Daten (+0)
TPControl	BIT	Feature.1 ;Totempfahl auch bei SPP (bei Rev.2 immer 1)
OCControl	BIT	Feature.2 ;Offene-Senke-Simulation für Steuerport (+2)
DarkBlue	BIT	Feature.5 ;dunklere blaue LED
DirectIo	BIT	Feature.6 ;keine Invertierungen, kein Datenrichtungsbit
PullUps		BIT	Feature.7 ;Deaktivierung der externen Pullups ab Rev.3
;===I²C===
I2C_ALen:	ds	1	;entspricht etwa wIndexH beim Vendor-Request A2
 I2C_ALen0	BIT	I2C_ALen.0	; 1 = 1 Byte, oder 2 Bytes vertauscht
 I2C_ALen1	BIT	I2C_ALen.1	; 1 = 2 Bytes (sonst 0 oder 1 Byte)
 I2C_NoStop	BIT	I2C_ALen.2
 I2C_NoStart	BIT	I2C_ALen.3
 I2C_Paging	BIT	I2C_ALen.4	; 1 = Paging aktiv (nicht wIndexH)
 I2C_Verify	BIT	I2C_ALen.5	; zusammen mit Paging immer Verify

DSEG AT 30h	;nicht bitadressierbar
FIFOSIZE	equ	16
;===FIFO===... hat hier eine Tiefe von FIFOSIZE Wörtern zu je 9 Bit
Fifo:		ds	FIFOSIZE*2	;Wegen ECP brauchen wir 9 bit Breite!
fifor:		ds	1	;Fifo-Lesezeiger
fifow:		ds	1	;Fifo-Schreibzeiger
;===allgemein===
Led_T:		ds	1	;"Nachblinkzeit" der LED in ms
Led_F:		ds	1	;"Blinkfrequenz" in ms (halbe Periode)
Led_C:		ds	1	;Blink-Zähler
Led2_T:		ds	1
Led2_F:		ds	1
Led2_C:		ds	1
FrameCnt:	ds	1	;zum Mitzählen "ganzer" USB-Rahmen (HighSpeed)
PendingByte:	ds	1
UniIdxL:	ds	1	;Index in OUT4BUF zum byteweisen Lesen
UniIdxH:	ds	1
ep2ll:		ds	1	;Längenzähler für EP2-Verarbeitung
ep2lh:		ds	1
;=== Variablen für I²C/EEPROM ===
I2C_Addr:	ds	1	;I²C-Adresse (EEPROM)
I2C_PMask:	ds	1	;Für seiten-weises EEPROM-Schreiben,
	;00 = einzelbyteweise, 07 = 8-Byte-Seite, 1F = 32-Byte-Seite usw.
	;07 (8bit), 1F (16bit) oder (1<<High-Nibble von wIndexH)-1 (Request ED)
;Die PMask wird mit dem LOW-Teil der laufenden Adresse geODERt; bei
;Ergebnis Null wird der I²C-Transfer unterbrochen und das Brennen ausgelöst.

IF Gpif
 OutB	equ	GpifSglDatLX
 TRISB MACRO v
	STX	GpifIdleCS,v
 ENDM
ELSE
 OutB	equ	IOB
 TRISB MACRO v
  IFNB <v>
	mov	OEB,#v
  ELSE
  	mov	OEB,a
  ENDIF
 ENDM
ENDIF

;--------------
;## PROGRAMM ##
;--------------
CSEG AT 0
	ljmp	main	;am Reset-Vektor
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
	

ORG 1000h			;4 Kilobyte für User
;--------------------------------------------------------------------
ResetFifos:
	STX	GpifAbort
	SyncDelay
	STX	FifoReset,80h
	SyncDelay
	STX	,2
	SyncDelay
	STX	,4
	SyncDelay
	STX	,6
	SyncDelay
	STX	,8
	SyncDelay
	STX	,0
	SyncDelay
	STX	OutPktEnd,82h	;EP2Out-Block in USB-Besitz bringen
	SyncDelay
	STX			;beide Blöcke (Doppel-Pufferung)
	SyncDelay
	STX	OutPktEnd,84h	;EP4Out-Block in USB-Besitz bringen
	SyncDelay
	STX			;beide Blöcke (Doppel-Pufferung)
	SyncDelay
	STX	EP1OutCfg,0B0h	;Interrupt-Modus (nicht Bulk)
	STX	NEXT		;EP1InCfg
	SyncDelay
	STX	EP1OutBC	;EP1Out-Block in USB-Besitz bringen
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
	call	i2c_wr		;(>=25)
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
	call	i2c_wr
	jc	i2c_err
	jnb	I2C_ALen1,eae	; kein Fehler: fertig
	mov	a,DPH0		;EEPROM-Adresse High-Teil
	jmp	i2c_wr
ea_x0:
	jnb	I2C_ALen1,eae	; kein Fehler: fertig ohne ein Adressbyte
	mov	a,DPH0		;EEPROM-Adresse High-Teil zuerst
	call	i2c_wr
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
	call	EAdr
	jc	ewe
	inc	r4
	sjmp	ewf
ewl:	
	LDX1
	call	i2c_wr		;1 Byte schreiben (noch nicht brennen)
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
	 call	EWrite
	pop	AR4
	pop	DPH0
	pop	DPL0		;Adresse zurückstellen
	MOVW	AutoPtr1,Out0Buf;auch Vergleichspuffer zurück
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
	call	EAdr
	jc	ere
	STX	I2CS,80h	;Noch eine Startsequenz ausgeben
erns:	mov	a,I2C_Addr	;I²C-Adresse
	inc	a		;Leseadresse
	call	i2c_wr
	jc	ere
	inc	r4		;Problem: Klappt nicht mit R4=0
	call	i2c_d		;Dummy-Lesezugriff von I2DAT
	dec	r4
erl:
	call	i2c_rd		;Byte lesen
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
MemR:	LODS		;lesen (Zweiter Autopointer versagt
	STX1		;sowieso wegen Bug im FX2!!)
	djnz	r4,MemR
	ret
MemW:	LDX1
	STOS		;schreiben
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
	MOVW	AutoPtr1,Out0Buf
	mov	r1,#LOW(XAutoDat1)
PatchW:	lcall	0
	jc	LEP0E
	mov	a,r7
	jnz	LEP0W		;nächste Runde, sowie CY=0
	ret
	
a3_no:	cjne	r4,#11000000b,err2	;bmRequestType
	jz	LEP0E
LEP0R:	call	Partial_64
	WAIT_EP0_IN
	push	AR4		;retten für In0BC
	 MOVW	AutoPtr1,In0Buf
	 mov	r1,#LOW(XAutoDat1)
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
;CountString: String Nr. R5-1 aus Descriptor-Liste auswählen
;PE: DPTR=String-Beschreiber-Liste (gerade Adresse!)
;    DPS=0 (sonst geht's nicht!)
;    R5=String-Nummer+1
;PA: CY=1 wenn String-Liste "vorfristig" zu Ende
;    DPTR=String-Beschreiber (stets gerade Adresse)
;VR: dptr,r5,a
cs_l:	LOADX
	jz	cs_ep0stall
	add	a,dpl0
	mov	dpl0,a
	jnc	CountString
	 inc	dph0
CountString:
	djnz	r5,cs_l
	RETNC
cs_ep0stall:	RETC

ep_cs:
;Rechnet Endpoint-ID in INxCS/OUTxCS-Adresse um
;PE: R6=[SetupDat+4]=Enden-Adresse
;PA: R0=Zeiger auf EPxCS (Low-Teil)
;VR: A,R0,C
	mov	a,r6
	;Zehn mögliche Fälle:	 01 81 02 82 04 84 06 86 08 88
	cjne	a,#1,ep_cs1	;01
	mov	a,#0FFh		;FF
	sjmp	ep_cs2
ep_cs1:	rr	a		;   40 01 41 02 42 03 43 04 44
	anl	a,#7		;   00 01 01 02 02 03 03 04 04
ep_cs2:	add	a,#LOW(EP1InCS)
	mov	r0,a
	ret
ResTog:	;Togglebit von R6=Endpoint rücksetzen
	mov	a,r6
	mov	c,ACC.7
	mov	ACC.4,c
	anl	a,#1Fh
ResTo:	STX	TOGCTL		;Toggle-Bit adressieren
	setb	ACC.5
	STX			;Togglebit zurücksetzen
	ret
ResTogAll:	;Alle Togglebits rücksetzen, VR: R5
	mov	r5,#1Fh
ResTo0:	mov	a,r5
	call	ResTo		;alle außer EP0Out (der geht immer)
	djnz	r5,ResTo0
	ret

GetStatus:	;bRequest=0, numerisch gleich: GetDeviceId (Drucker-String)
	cjne	r4,#ClassIn,gs_nid	;GET_DEVICE_ID? (Nur Interface 1)
;hier: GetDeviceId
	;cjne	r6,#1,err1	;Interface 1: Drucker (klappt nicht!!)
	ajmp	GetDeviceId
gs_nid:
	cjne	r4,#80h,gs_no_dev	;GS_DEVICE
nullin:	clr	a		;kein WakeUp(Bit1), kein SelfPower(Bit0)
wordin:	STORX	In0Buf
	STORX	NEXT,0
	STX	In0BC,2
	ret
gs_no_dev:
	cjne	r4,#81h,gs_no_if	;GS_INTERFACE
	jmp	nullin		;auch zwei Nullen melden
gs_no_if:
	cjne	r4,#82h,err1	;GS_ENDPOINT
	call	ep_cs
	LDX			;STALL-Bit angeln
	anl	a,#1
	jmp	wordin

ClearFeature:	;bRequest=1, numerisch gleich: GetPortStatus; GetReport
	cjne	r4,#ClassIn,cf_nps	;Klassenrequest
;hier: GetReport (wValueL=Report-ID, wValueH=3=Feature-Report, wIndexL=Interface)
	cjne	r6,#2,cf_ngr	;Interface, muss 2 sein für HID
	ajmp	HidGetReport
cf_ngr:
;hier: GetPortStatus
	;cjne	r6,#1,err1	;Interface 1: Drucker (klappt nicht!!)
	acall	in1
	anl	a,#00111000b
	ajmp	OneByteIn	;Info-Byte senden
cf_nps:
	cjne	r4,#2,err1
	cjne	r5,#0,err1	;einziges Feature: STALL
	call	ep_cs		;Endpoint zur Control+Status-Adresse umrechnen
	STX	,0		;STALL entfernen
	jmp	ResTog

SoftReset:
	cjne	r4,#ClassOut,err1
	;cjne	r6,#1,err1	;nur Interface 1 (klappt nicht!!)
	clr	IOA.INI		;RESET-Impuls ausgeben
	LDX	GpifIdleCtl
	anl	a,#70h
	orl	a,#011b		;SEL=L, AF=H, STB=H
	STX
	setb	IOA.INI
	ret

SetFeature:
	cjne	r4,#2,err1	;FT_ENDPOINT
	cjne	r5,#0,err1	;einziges Feature: STALL
	call	ep_cs		;Endpoint zur Control+Status-Adresse umrechnen
	STX	,1		;STALL setzen
	ret
err1:	RETC

GetDescriptor:			;bRequest=6
	LDX	SetupDat+3		;wValueH
	cjne	r4,#80h,gd_no_devin	;bmRequestType (In/Std/Device)
	cjne	a,#01h,gd_no_dev	;GD_DEVICE
	mov	dptr,#DeviceDescriptor
gd_sto:	mov	a,DPH0
	STX	SudPtrH
	mov	a,DPL0
	STX	NEXT
	ret
gd_no_dev:
	cjne	a,#02h,gd_no_conf	;GD_CONFIGURATION
	orl	c,HighSpeed
gd_cd:	mov	dptr,#ConfigDescFS
	jnc	gd_sto
	mov	dptr,#ConfigDescHS
	clr	c
	jmp	gd_sto
gd_no_conf:
	cjne	a,#03h,gd_no_str	;GD_STRING
	mov	dptr,#StringDesc
	inc	r5			;wValueL (String-Index)
	call	CountString
	jc	err1
	jmp	gd_sto
gd_no_str:
	cjne	a,#06h,gd_no_qual	;GD_QUALIFIER
	mov	dptr,#DeviceQualifier
	jmp	gd_sto
gd_no_qual:
	cjne	a,#07h,err1		;GD_OTHERSPEED
	orl	c,/HighSpeed
	jmp	gd_cd
gd_no_devin:
	cjne	r4,#081h,err1		;bmRequestType (In/Std/Interface)
	cjne	a,#21h,gd_no_hid
	mov	dptr,#HidDesc
	jmp	gd_sto
gd_no_hid:
	cjne	a,#22h,err1
IF 0
	STORX	SudPtrCtl,0
	;SyncDelay
	STORX	EP0BCH
	;SyncDelay
	mov	a,HRDE-HidReportDesc
	STORX	EP0BCL
	mov	dptr,#HidReportDesc
	jmp	gd_sto
ELSE
	;Deskriptor <dptr> Länge <r6> via EP0 senden
	;Die Umschaltung von SUDPTRCTL funktioniert einfach nicht!!
	STORX	PatchR+1,HIGH(MemR)
	STORX	NEXT,LOW(MemR)
	mov	dptr,#HidReportDesc
	mov	r6,#HRDE-HidReportDesc
	LDX	SetupDat+7	;wLengthH
	jnz	nolim
	LDX	PREV		;wLengthL
	CMP	a,AR6		;W2k,XP will 64 Bytes mehr lesen, wieso?
	jnc	nolim
	mov	r6,a
nolim:	dec	r6
	mov	r7,#1
	jmp	LEP0R
ENDIF

GetConfiguration:
	cjne	r4,#80h,err3		;nur DEVICE_IN
	mov	a,Configuration	 ;Entweder konfiguriert oder unkonfiguriert
OneByteIn:	
	STORX	In0Buf
	STX	In0BC,1
	ret

SetConfiguration:
	cjne	r4,#ClassOut,sc1
	cjne	r6,#2,err3		;Interface, muss 2 sein für HID
	ajmp	HidSetReport
sc1:	cjne	r4,#0,err3		;nur DEVICE_OUT
	mov	a,r5
	add	a,#-2
	jc	err3			;nur Konfiguration 0 oder 1
	mov	Configuration,r5
	ret

GetAltSet:
	cjne	r4,#81h,err3		;nur INTERFACE IN
	cjne	r6,#1,as_n1
	mov	a,AltSetting1
	jmp	OneByteIn
as_n1:	jnc	err3		;wenn Interface > 1
	clr	c
	clr	a
	jmp	OneByteIn

SetAltSet:
	cjne	r4,#1,err3		;nur INTERFACE OUT
;	cjne	r6,#0,as_n0	;Interface 0: h#s Parallelport
;	ret
as_n0:
	cjne	r6,#1,err3	;Interface 1: USB-Druckerunterstützung
	mov	a,r5
	add	a,#-3
	jc	err3		;Nur Alternative 0, 1 oder 2 zulassen
	mov	AltSetting1,r5
;	STX	OutPktEnd,84h	;EP4 (doppelt gepuffert) scharfmachen
;	STX
sude:	ret

SUD_Tab:
	ajmp	GetStatus	;0 - auch: GET_DEVICE_ID
	ajmp	ClearFeature	;1 - auch: GET_PORT_STATUS - HID_GET_REPORT
	ajmp	SoftReset	;2 (normalerweise err1)
	ajmp	SetFeature	;3
	RETC			;4
err3:	RETC	;SetAddress	;5, sollte nicht vorkommen
	ajmp	GetDescriptor	;6
	RETC	;SetDescriptor	;7
	ajmp	GetConfiguration;8
	ajmp	SetConfiguration;9 - auch: HID_SET_REPORT
	ajmp	GetAltSet	;10
	ajmp	SetAltSet	;11
	;RETC	;SyncFrame	;12

HandleSUD:			;liefert CY=1 für err1
	LDX	SetupDat+0	;bmRequestType -> R4
	mov	r4,a
	LDX	NEXT		;SetupDat+1: bRequest -> A
	cjne	a,#0A2h,hs1	;EEPROM-Zugriff?
	call	EReqInit	;wIndex heranziehen
	mov	dptr,#ERead
	MOVR	r6,r7,EE_W
	jmp	LongEP0
hs1:	cjne	a,#0A3h,hs2	;XRAM-Zugriff?
memep0:	mov	dptr,#MemR
	MOVR	r6,r7,MemW
	jmp	LongEP0
hs2:	mov	r7,a
	add	a,#-12		;nur 0..11 zulassen
	jc	err3
	LDX	NEXT		;SetupDat+2: wValueL -> R5
	mov	r5,a
	LDX	SetupDat+4	;wIndexL -> R6 (oft: Interface)
	mov	r6,a
	LDX	NEXT		;SetupDat+5: wIndexH -> R7
	xch	a,r7
	call	usr31		;darf bei CY=1 kein Register ändern!
	jnc	sude		;bei CY=0 ist's des Users Eigenverantwortung
	JMPTBL	SUD_Tab

;=========
;== HID ==
;=========
HidGetReport:
	MOVW	AutoPtr1,AnswerBuf
	MOVW	AutoPtr2,In0Buf
	mov	r0,#LOW(XAutoDat1)
	mov	r1,#LOW(XAutoDat2)
	LDX			;1. Byte = Längen-Byte = Report-ID (1..8)
	jz	hgee		;Keine Daten wenn Null
	mov	r2,a
	mov	r4,a
hge1:	STX1
	LDX
	djnz	r4,hge1
	mov	a,r2
	STX	In0BC		;Datenblock liefern
hgee:	ret

HidSetReport:
	WAIT_EP0_OUT		;Eingehende Daten abwarten (in einem Stück!)
	MOVW	AutoPtr1,Out0Buf
	MOVW	AutoPtr2,AnswerBuf+1	;doppelt benutzt!
	LDX	XAutoDat1
	mov	ep2ll,a		;Report-ID = Länge
	mov	ep2lh,#0
	call	ProcessInOut
	mov	a,AutoPtrL2
	subb	a,#LOW(AnswerBuf)
	mov	dptr,#AnswerBuf
	STORX			;Anzahl Antwortbytes abspeichern
	ret

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
	acall	DirChanged	;Ausgabetreiber stets aktiv - je nach OC-Simul.
se3:	mov	fifor,#Fifo
	mov	fifow,#Fifo
	ret

;===SPP-FIFO===========================
;Die Emulation der SPP-FIFO-Betriebsart
;Ein "echtes" Parallelport legt stets den FIFO-Kopf aufs Datenport...
SppXfer:
	jb	fifoe,exf	;wenn FIFO leer ist nichts zu tun
	LDX	GpifReadyStat
	jb	ACC.BSY,exf	;wenn beschäftigt dann geht's nicht
	mov	r0,fifor
	mov	OutB,@r0	;Datenbyte anlegen, Treiber muss aktiv sein
	LDX	GpifIdleCtl
	clr	ACC.STB
	STX			;Strobe aktivieren
	acall	IncFifoR	;Lesezeiger erhöhen (und Zeit verbrauchen)
	LDX	GpifIdleCtl
	setb	ACC.STB
	STX			;Strobe zurücknehmen
exf:	ret

;===EPP===========================

;A auf IdleCtl ausgeben und max. 10 µs auf WAIT=H warten
;PE: R0=LOW(GpifIdleCtl), A=GpifIdleCtl-Byte, R3=ASTB/DSTB-Maske
;PA: EppTimeOut.0, A=abgetastete Portpins
;VR: R0=LOW(GpifReadyStat),R1=A,R3
wait_epp:
	LDX	GpifIdleCtl
	mov	r1,a		;retten
	anl	a,r3		;ASTROBE/DSTROBE (ggf. WRITE)low
	STX
	mov	a,r3
	rrc	a		;Bit0 (Strobe = WRITE) extrahieren
	jc	sw0
	push	OEB		;bei Schreibzugriff: Zustand retten,...
	mov	OEB,#0FFh	;temporär alles Ausgabe
sw0:	;max. 10 µs warten bis WAIT=H
	mov	r3,#15		;15 Runden à 8 Takte (2/3 µs)
	mov	r0,#LOW(GpifReadyStat)
sw1:	LDX			;(2)
	jb	ACC.BSY,sw2	;(3) WAIT endlich HIGH
	djnz	r3,sw1		;(3)
	setb	EppTimeout.0	;TimeOut-Bit setzen
sw2:	jc	sw3		;noch einmal CY auswerten!
	pop	OEB		;bei Schreibzugriff: OEB restaurieren
sw3:	mov	r3,IOB		;abtasten (auch bei Schreibzugriff: egal!)
	mov	a,r1		;restaurieren
	STX	GpifIdleCtl	;ASTROBE/DSTROBE high
	mov	a,r3
	ret

out_epp:
;OUT-Befehl Byte R4 mit R3=Addr/DataStrobe, WRITE=0
;Steuerleitungen werden wie im Original nur bei Bedarf nach Low gezogen
;Datenrichtung wird (temporär) umgeschaltet. (Bei "richtigem" Port beobachtet.)
	mov	OutB,r4		;Daten ausgeben (immer - vorher!)
	LDX	GpifReadyStat	;EPP 1.9: WAIT muss LOW sein!
	jb	ACC.BSY,no_epp
	jmp	wait_epp

in_epp:
;IN-Befehl mit r3=Addr/DataStrobe
;PA: A=gelesenes Byte
	LDX	GpifReadyStat	;EPP 1.9: WAIT muss LOW sein!
	jb	ACC.BSY,no_epp_read_PINSB
	jmp	wait_epp

no_epp_read_PINSB:
	mov	a,IOB		;PortB lesen
no_epp:	setb	EppTimeOut.0	;TimeOut-Bit setzen
	ret			;nichts ausgeben!

;===ECP===========================

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
	call	IncFifoPtr_Compare
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
	call	IncFifoPtr_Compare
	jnz	ifw1
	setb	fifof		;FIFO voll
ifw1:	clr	fifoe		;FIFO ist keinesfalls leer
	ret

EcpXfer:
;Auf Transfer von/in FIFO im Hintergrund prüfen
;Diese Routine wird, sofern ECP aktiv, zyklisch aufgerufen
;Keine Parameter, VR: A,R0,R3,R4
	jb	direction,EcpInXfer
	LDX	GpifIdleCtl
	jnb	ACC.STB,oxf2	;2. Phase der Byte-Übertragung zz. aktiv
	jb	fifoe,no_Xfer	;Kann kein Byte rausschicken
	LDX	GpifReadyStat
	jb	ACC.BSY,no_Xfer	;Gegenstelle ist beschäftigt
	mov	r0,fifor
	mov	AR4,@r0		;Datenbyte->R4
	inc	r0
	mov	AR3,@r0		;Command/Data->R3
	call	IncFifoR	;Lesezeiger erhöhen
	mov	OutB,r4		;Byte (R4) anlegen
	TRISB	0FFh		;Treiber aktivieren
	mov	a,r3
	rrc	a		;Command(0) oder Data(1) ausschieben
	LDX	GpifIdleCtl
	mov	ACC.AF,C	;HostAck(AF,1) setzen
	setb	ACC.INI		;nReverseRequest auf HIGH (Init) (unnötig?)
	STX
	clr	ACC.STB		;HostClk(STB) auf LOW
	STX
oxf2:	LDX	GpifReadyStat	;PeriphAck abfragen
	jnb	ACC.BSY,no_Xfer	;Gegenstelle ist beschäftigt
	LDX	GpifIdleCtl
	setb	ACC.STB		;HostClk(STB) auf HIGH
	STX
	ret
EcpInXfer:
	jb	fifof,no_Xfer	;Kann kein Byte einlesen
	LDX	GpifReadyStat
	xch	a,r3
	LDX	GpifIdleCtl
	jb	ACC.AF,ixf2	;HostAck=H, 2. Phase des Byte-Lesens...
	xch	a,r3
	jb	ACC.ACK,no_Xfer	;Gegenstelle meldet (noch) keinen Bedarf
	xch	a,r3
	setb	ACC.AF		;HostAck=H
	STX
ixf2:	LDX	GpifReadyStat
	jnb	ACC.ACK,no_Xfer	;PeriphClk=L = noch nicht bereit
	mov	c,ACC.BSY	;Command(0) / Data(1)
	mov	r0,fifow
	mov	@r0,IOB		;Datenbyte abholen und abspeichern
	inc	r0
	clr	a
	mov	ACC.0,c		;Bit einsetzen
	mov	@r0,a
	call	IncFifoW
	LDX	GpifIdleCtl
	clr	ACC.AF		;HostAck=L
	STX
no_Xfer:ret


DirChanged2:
;Aufzurufen, wenn sich die Steuerport-Steuerungbits ändern...
;PE: -
;Neues Control5V berechnen:
	mov	c,ECR_Bits.0	;SPP-Modus?
	anl	c,/TPControl
	orl	c,OCControl
	jc	dc_occontrol1	;C=Offene Kollektor-Simulation fürs Steuerport
	jbc	Control5V,dc_occontrol0
	ret
dc_occontrol0:	;OpenCollector-Status löschen!
	orl	OEA,#00000100b	;INI als Ausgang aktivieren
	STX	GpifCtlCfg,10000000b	;Push-Pull setzen
	ret
dc_occontrol1:	;OpenCollector-Status setzen?
	jb	Control5V,dc_oce	;Bereits aktiviert: nichts tun!
	setb	Control5V	;Aktivierung vermerken
	STX	GpifCtlCfg,10000111b	;OpenCollector setzen
DoOcControl:	;Ausgabetreiber für INI nachführen zur OC-Simulation
;PE: DCR
;VR: A
	mov	c,DCR.INI	;RAM-Kopie lesen, weil Flipflop nicht lesbar
	cpl	c
	mov	a,OEA
	mov	ACC.INI,c	;in beiden Revisionen gleiches Bit
	mov	OEA,a
dc_oce:	ret
	
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
dc_tb:	TRISB		;Treiberstatus setzen
	ret
dc_ocdata:
	jb	Data5V,dc_oce	;Ist schon an!
	setb	Data5V
	mov	a,IOB		;Problem: Flipflops nicht rücklesbar
;DoOcData:	;A=Datenbyte: Ausgabetreiber nachführen (zu simpel für UP!)
	cpl	a
	TRISB		;High-ausgebende Leitungen hochohming
	ret

;===Portzugriffe=========================

;OUT-Unterprogramme bekommen in R4 das Argument,
;IN-Unterprogramme müssen das Byte in A liefern

;=== OUT auf Adresse +0 (Datenport) ===
out0:	mov	a,ECR_Bits
	anl	a,#10110011b	;0 = FIFO-Betriebsarten?
	jz	out0ecp		;mit A=0 in die FIFO
	mov	OutB,r4
	jnb	Data5V,out0e	;Open-Collector-Simulation?
	mov	a,r4
	cpl	a
	TRISB
out0e:	ret

;=== OUT auf Adresse +1 (Statusport) ===
out1:	mov	a,r4
	jb	DirectIo,out1ni	;Nicht invertieren, Extra-Bits durchlassen
	anl	a,#11111000b
	xrl	a,#10000000b
out1ni:	mov	IOD,a		;bei einem Eingabeport ohne Wirkung
	jnb	ECR_Bits.4,out1nepp
	mov	a,r4
	jnb	ACC.0,out1nepp
	clr	EppTimeOut.0	;TimeOut-Bit löschen
out1nepp:
	jnb	Rev2,out1e
	mov	c,ACC.5		;evtl. Problem mit fehlender Rücklesbarkeit!
	mov	IOA.PE,c
	mov	c,ACC.4
	mov	IOA.ONL,c
	mov	c,ACC.3
	mov	IOA.ERR,c	
out1e:	ret
	
;=== OUT auf Adresse +2 (Steuerport) ===
;Bit4=IRQ-Freigabe (nicht unterstützt, aber gespeichert)
;Bit5=Ausgabetreiber-Freigabe
;Randproblem: Keine zeitgleiche Steuerung von INI und den drei anderen
;Steuerleitungen möglich, wenn man GPIF benutzt
out2:	mov	DCR,r4		;(2)
	mov	a,ECR_Bits	;(2)
	anl	a,#00000101b	;(2)
	jz	bidi1		;(3) Modus nicht SPP oder AutoStrobe
	clr	direction	;(2) DCR.5 klebt auf 0
bidi1:	mov	c,DCR.2		;(2)	;INIT
	mov	IOA.INI,c	;(2)
	mov	a,DCR		;(2)
	mov	c,ACC.3		;(2)	;SEL
	mov	ACC.SEL,c	;(2)	;zu Bit2 schaffen
	jb	DirectIo,out2ni	;(4)	;Nicht invertieren
	cpl	a		;(1)	;diese drei Bits sind invertiert
out2ni:	anl	a,#7		;(2)
	mov	r4,a		;(1)	;retten
	LDX	GpifIdleCtl	;(4)
	anl	a,#70h		;(2)	;Richtungsbits unverändert lassen
	orl	a,r4		;(1)
	STX			;(2)
	jnb	Control5V,DirChanged;(4)
	acall	DoOcControl	;OC für INI simulieren
DirChanged:
	jb	DirectIo,out2e	;(4) Nicht die Datenportrichtung beeinflussen!
	acall	DirChanged0	;Ggf. OC-Simulation umschalten
	jnb	Data5V,DoSetOeb	;Ohne OC-Simulation Tristate-Treiber schalten
out2e:	ret			;(4)

;=== OUT auf Adresse +3 (EPP-Adresse) ===
out3:	mov	r3,#NOT ((1 SHL SEL) OR (1 SHL STB))	;AddrStrobe (17) LOW
	jmp	out_epp

;=== OUT auf Adresse +4 (EPP-Daten) ===
out4:	mov	r3,#NOT ((1 SHL AF) OR (1 SHL STB))	;DataStrobe (14) LOW
	jmp	out_epp

;=== OUT auf Adresse +400 (ECP-Daten-FIFO) ===
out400:	mov	a,ECR_Bits
	anl	a,#01001100b	;Konfigurationen mit FIFO
	jz	out2e
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
	jmp	SetECR

;=== OUT auf Adresse +404 (Datenrichtung Datenport) [HINTERTÜR] ===
out404:	;1 = Ausgabe, 0 = Eingabe
	mov	a,r4
	TRISB
	ret
;=== OUT auf Adresse +405 (Datenrichtung Statusport) [HINTERTÜR] ===
out405:	;1 = Ausgabe, 0 = Eingabe, Standard
	;Bei Rev.2 nicht für BSY und ACK realisierbar
	mov	a,r4
	mov	OED,a		; Rev.2: Falls jemand Brücken legt...
	jnb	Rev2,out405e
	mov	B,OEA		;OEA bitadressierbar machen
	mov	c,ACC.5
	mov	B.PE,c
	mov	c,ACC.4
	mov	B.ONL,c
	mov	c,ACC.3
	mov	B.ERR,c
	mov	OEA,B
out405e:ret
;=== OUT auf Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
out406:	;Eingabe mittels High-Nibble von GpifIdleCfg (070128)
	mov	a,r4		;(1)
	jnb	ACC.2,initi
	orl	OEA,#(1 SHL INI)
	sjmp	inito
initi:	anl	OEA,#not (1 SHL INI)
inito:	mov	c,ACC.3		;(2)	;SEL
	mov	ACC.SEL,c	;(2)
	swap	a
	anl	a,#070h		;High-Nibble
	mov	r4,a
	LDX	GpifIdleCtl
	anl	a,#7		;CTL0..CTL2 (56beiniges Gehäuse) behalten
	orl	a,r4
	STX
sf1:	ret
;=== OUT auf Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
out407:	mov	a,r4
SetFea:	anl	a,#11100111b	;Nur "bekannte" Bits durchlassen
	jnb	Rev2,sf2
	orl	a,#00000010b	;TPControl EIN bei Rev.2
	anl	a,#01111111b	;Pullups stets EIN (kein Schalttransistor)
sf2:	xch	a,Feature
	xrl	a,Feature	;Veränderungen?
	jz	sf1		;nein, gar nichts tun
	setb	FeatureChanged	;in EEPROM brennen (verzögert)
	jnb	ACC.6,setf6
	LDX	GpifIdleCtl
	xrl	a,#7		;sofort entsprechende Bits drehen!
	STX
	xrl	IOD,#10000000b	;... in der Annahme der Rücklesbarkeit:-)
setf6:	jb	Rev2,nopu
	mov	c,Pullups	;Feature.7: Pullups schalten
	mov	IOA.7,c		;"schleichend" verändertes Bit korrigieren
	cpl	c
	mov	a,OEA
	mov	ACC.7,c
	mov	OEA,a		;Z = Pullups AUS (H geht nicht!)
nopu:	acall	DirChanged0	;OC-Status Datenport nachführen
	ajmp	DirChanged2	;OC-Status Steuerport nachführen

;Der Teufel hat ECP erfunden! Wie soll festgestellt werden, ob in der
;FIFO ein Adress- oder ein Datenbyte liegt?
;Mein "echtes" EC-Port ignoriert beim Rücklesen einfach das neunte Bit.
;=== IN von Adresse +0 (Datenport) ===
in0:	mov	a,ECR_Bits
	anl	a,#10110011b	;0 = eine FIFO-Betriebsart?
	jz	infifo		;Datenport nicht rücklesbar!
	mov	a,IOB
nix:	ret

;=== IN von Adresse +1 (Statusport) ===
in1:			;(3-4) Aufruf
	jb	Rev2,in1r2	;(4)
	mov	a,IOD	;(2)
	jb	DirectIo,in1x
	xrl	a,#80h	;(2)	;BSY invertieren
	sjmp	in1e	;(3)
in1r2:	LDX	GpifReadyStat	;(4)
	rrc	a	;(1)	;BSY nach C, ACK nach ACC.0
	rr	a	;(1)	;ACK nach ACC.7
	jb	DirectIo,in1ni	;(4)
	cpl	c	;(1)	;BSY invertieren
in1ni:	rrc	a	;(1)	;BSY nach ACC.7, ACK nach ACC.6, OK
	mov	c,IOA.PE;(2)	;Die zu chaotische Zuordnung mit Bit-Transfer
	mov	ACC.5,c	;(2)
	mov	c,IOA.ONL;(2)
	mov	ACC.4,c	;(2)
	mov	c,IOA.ERR;(2)
	mov	ACC.3,c	;(2)
in1e:	orl	a,#7	;(2)	;restliche Bits = 1
	anl	a,EppTimeOut	;Bit 0 löschen, wenn kein EPP-TimeOut
in1x:	RETNC		;(5) = (26-27)
	
;=== IN von Adresse +2 (Steuerport) ===
;Überraschung: Bei einem "genügend neuen" echten Parallelport sind die
;Steuerleitungen nicht (mehr) rücklesbar!
;Hier wird diese Einschränkung nur in den 3 FIFO-Betriebsarten nachgeahmt
in2:	mov	a,ECR_Bits
	anl	a,#10110011b	;0 = FIFO-Betriebsart
	jz	in2o
	mov	a,DCR
	orl	a,#11001111b	;nur Interrupt- und Richtungs-Bit (4 & 5)
	mov	r3,a
	jb	Rev2,in2r2
	;Rev.3+: richtiges Lesen von Istzuständen
	mov	a,IOA	;(2)
	jb	DirectIo,in2e	;Nicht invertieren
	xrl	a,#1011b;(2)	;Alles außer INI invertieren
	sjmp	in2e	;(3)
in2r2:	;Rev.2: kein echtes Lesen von Leitungszuständen möglich
	LDX	GpifIdleCtl
	jb	DirectIo,in2ni	;Nicht invertieren
	cpl	a
in2ni:	mov	c,ACC.SEL
	mov	ACC.3,c
	mov	c,IOA.2		;INI-Bit lesen (Rev.2: als einziges echtes Bit)
	mov	ACC.2,c
in2e:	orl	a,#0F0h
	anl	a,r3		;Interrupt- und Richtungsbit aus DCR einsetzen
	ret
in2o:	;Rücklesen des Ausgaberegisters; erspart Kopfzerbrechen mit ECP
	mov	a,DCR
	orl	a,#11000000b	;obere Bits lesen immer 1
	ret

retff:	mov	a,#0FFh
retin:	ret

;=== IN von Adresse +3 (EPP-Adresse) ===
in3:	mov	r3,#NOT (1 SHL SEL)	;AddrStrobe LOW
	jmp	in_epp

;=== IN von Adresse +4 (EPP-Daten) ===
in4:	mov	r3,#NOT (1 SHL AF)	;DataStrobe LOW
	jmp	in_epp

;=== IN von Adresse +400 (ECP-FIFO) ===
in400:	mov	a,ECR_Bits
	anl	a,#11001100b	;FIFO-Betriebsarten + Konfiguration
	jz	retff		;ohne ECP gibts kein Port+400h
	mov	a,#00010000b	;Konfigurationsregister (Konstante) A
	jb	ECR_Bits.7,retin
infifo:	mov	r0,fifor
	mov	a,@r0		;FIFO lesen
	jb	fifoe,retin	;Letztes FIFO-Byte liefern wenn FIFO leer
	mov	r3,a		;Logische Schwäche bei BeyondLogic:
				;Wohin mit dem "gelesenen" PeriphAck?
	call	IncFifoR
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
in404:	mov	a,OEB
	ret
;=== IN von Adresse +405 (Datenrichtung Statusport) [HINTERTÜR] ===
in405:	mov	a,OED
	jnb	Rev2,in405e
	anl	a,#00111111b	;BSY und ACK sind immer Eingang
in405e:	ret
;=== IN von Adresse +406 (Datenrichtung Steuerport) [HINTERTÜR] ===
in406:	LDX	GpifIdleCtl
	swap	a		;Richtungsbits für SEL, AF, STB
	mov	c,ACC.SEL
	mov	ACC.3,c
	mov	r3,a		;retten
	mov	a,OEA
	mov	c,ACC.2		;INI-Datenrichtungsbit
	mov	a,r3		;zurück
	mov	ACC.2,c
	anl	a,#0Fh
	ret
;=== IN von Adresse +407 (USB2LPT-Feature-Register) [HINTERTÜR] ===
in407:	mov	a,Feature
	ret

;=== Warte-Hilfsbefehl, Argument R4 = Wartezeit in 1 µs ===
wait:	inc	r4		;Wir brauchen 4x6=24 Takte pro Durchlauf
w1:	inc	dptr		;kurzer Befehl mit langer Dauer!
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
;Es sei denn, es ist "benutzerdefinierter Kode" heruntergeladen.

Aufruf:			;R3=UP-Nummer
	mov	a,r3
	JMPTBL	upv
	
;=== IEEE1284 ===
Request:
;Datenbyte liegt bereits an PortB, Steuerleitungen im Defaultzustand
	clr	c
	STX	GpifIdleCtl,01110101b	;SEL=high, AF=low
	mov	r2,#13		;max. 10 ms warten
	mov	r4,#0
rl1:	call	in1		;(26)
	anl	a,#01111000b	;(2)	ACK=LOW, PE=HIGH, SEL=HIGH, ERR=HIGH?
	xrl	a,#00111000b	;(2)
	jz	ry1		;(3)
	djnz	r4,rl1		;(3) * 256 = 9216T = 768µs
	djnz	r2,rl1
	RETC
ry1:	STX	GpifIdleCtl,01110100b	;STB=low
	mov	r4,#10		;1 µs
rl3:	djnz	r4,rl3		;(3)
	STX	,01110111b	;AF=high, STB=high
	mov	r3,#13		;max. 10 ms warten
	;mov	r4,#0
	mov	r0,#LOW(GpifReadyStat)
rl2:	call	in1		;(26)
	anl	a,#01111000b	;(2)	ACK=HIGH, PE=LOW, SEL=HIGH, ERR=LOW?
	xrl	a,#01010000b	;(2)
	jz	ry2		;(3)
	djnz	r4,rl2		;(3) * 256 = 9216T = 768µs
	djnz	r2,rl2
	setb	c
ry2:	ret	
	
Nibble:
;PA: Nibble in A (sowie R5)
;VR: R0,R3,R4,R5
	STX	GpifIdleCtl,01110101b	;AF low
	mov	r3,#6		;max. 1 ms warten
	mov	r2,#0
	mov	r0,#LOW(GpifReadyStat)
nl1:	LDX			;(2)
	jnb	ACC.ACK,ny1	;(4)
	djnz	r2,nl1		;(3) * 256 = 2304T = 192µs
	djnz	r3,nl1
	RETC
ny1:	mov	c,ACC.BSY
	clr	a
	rlc	a
	mov	c,IOA.PE
	rlc	a
	mov	c,IOA.ONL
	rlc	a
	mov	c,IOA.ERR
	rlc	a		;u.a. CY=0
	push	ACC
	 STX	GpifIdleCtl,01110111b	;AF high
	 mov	r3,#6		;max. 1 ms warten
	 mov	r2,#0
	 mov	r0,#LOW(GpifReadyStat)
nl2:	 LDX			;(2)
	 jb	ACC.ACK,ny2	;(4)
	 djnz	r2,nl2		;(3) * 256 = 2304T = 192µs
	 djnz	r3,nl2
	 setb	c		;TimeOut
ny2:	pop	ACC
	ret

TwoNibbles:
;VR: R0,R2,R3,R5
	call	Nibble	;Low-Nibble
	jc	tne
	mov	r5,a
	call	Nibble	;High-Nibble
	swap	a
	orl	a,r5
tne:	ret

GetDeviceId:
;Beschafft Drucker-ID im Nibble-Modus
;PA: CY=1: Fehler
;    CY=0: Daten über EP0 (64-Byte-weise) übertragen, kein STALL
;VR: R0..R7
	setb	c
	LDX	GpifIdleCtl	;muss x111xxxx sein! Sonst Fehler!
	cpl	a
	anl	a,#70h
	jnz	gdie
	STX	,01110011b	;SEL=low, (INI=high,) AF=high, STB=high
	mov	OutB,#4		;"Get Device ID using Nibble Mode"
	jnb	Data5V,gd1	;Open-Collector-Simulation? (Datenport)
	mov	OEB,#not 4
gd1:	call	Request
	jc	gdie
	mov	dptr,#GetDevId	;Kernfunktion für Universalroutine
	mov	r4,#11000000b	;"Vendor-Request", IN
	call	LongEP0		;Universalroutine arbeiten lassen
	STX	GpifIdleCtl,01110011b;Standard-Modus zurück
gdie:	ret
	
GetDevId:
;Callback-Routine mit R4=Transferlänge und R1=AutoDat
	jb	F0,GetDevIdCont
	call	TwoNibbles
	jc	gdie
	STX1
	djnz	r4,gdi1
	ret
gdi1:	mov	DPH0,a	;High-Teil retten
	call	TwoNibbles
	jc	gdie
	STX1
	;DPTR := 2 - Länge (also im Regelfall eine negative 16-bit-Zahl)
	mov	r2,a
	mov	a,#2
	subb	a,r2
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
	call	TwoNibbles
	jc	gdie
	inc	dptr	;solange negativ kommen Bytes vom Drucker
gdi4:	STX1
gdi3:	djnz	r4,GetDevIdCont
	ret

IF Gpif=0
;== Drucker-Klassen-Simulation ==
UniXfer:	;Aufruf wenn Daten in OUT4BUF vorhanden sind
	LDX	GpifReadyStat
	jb	ACC.BSY,ux1	;BUSY prüfen, nichts tun wenn beschäftigt
	mov	DPL0,UniIdxL	;Index in OUT4BC holen
	mov	a,UniIdxH
	add	a,#HIGH(EP4FifoBuf)
	mov	DPH0,a
	LOADX			;Datenbyte aus Puffer holen
	mov	OutB,a		;anlegen
	jnb	Data5V,ux3	;Open-Collector-Simulation?
	cpl	a
	TRISB
ux3:	setb	IOA.INI		;INIT inaktiv (INI=high)
	STX	GpifIdleCtl,01110011b;SEL=low für Standard-Parallelport-Betrieb
	STX	,01110010b	;Strobe-Signal erzeugen (STB=low)
	inc	UniIdxL		;Index erhöhen
	mov	a,UniIdxL
	jnz	ux2
	inc	UniIdxH
ux2:	mov	a,#66
	call	StartLed2
	STX	,01110011b	;einige Zyklen später (1 µs) STB=HIGH
	LDX	EP4BCH
	xrl	a,UniIdxH	;Puffer ausgelesen?
	jnz	ux1		;nein
	LDX	EP4BCL
	xrl	a,UniIdxL
	jnz	ux1
	mov	UniIdxL,a	;Index auf Null stellen
	mov	UniIdxH,a
	STX	OutPktEnd,84h	;ja, Puffer an USB übergeben, scharf machen
ux1:	ret
ENDIF
;=== LED-Spielerei ===

StartLed:
;Start des LED-Blinkens mit a=Blinkperiodendauer, 0=Maximum
;Blinkt die LED bereits, wird nur die künftige Blinkperiode gesetzt
;VR: A
	mov	Led_T,a		;Zeitzähler (Monoflop) neu starten
	mov	Led_F,a		;Blinkfrequenz setzen
	jb	Led_B,nost
	mov	Led_C,a
	setb	Led_B
p0Led:	setb	Led		;LED ausschalten (Zugriff beginnt)
nost:	ret

StartLed2:
	mov	Led2_T,a	;Zeitzähler (Monoflop) neu starten
	mov	Led2_F,a	;Blinkfrequenz setzen
	jb	Led2_B,nost2
	mov	Led2_C,a
	setb	Led2_B
	mov	c,HighSpeed
	mov	Led2State,c		;LED umschalten
nost2:	ret

On1ms:	;Aufruf alle 1 Millisekunde: LED-Blinken
	jnb	Led2_B,n2
	djnz	Led2_C,nbl2
	cpl	Led2State	;blaue LED umschalten
	mov	Led2_C,Led2_F
nbl2:	djnz	Led2_T,n2
	clr	c
	orl	c,/HighSpeed
	mov	Led2State,c	;LED in Normalzustand zurück
	clr	Led2_B
n2:
	jnb	Led_B,n1	;LED blinkt?
	djnz	Led_C,nbl
p1Led:	cpl	Led
	mov	Led_C,Led_F
nbl:	djnz	Led_T,n1
p2Led:	clr	Led		;Nach paar Millisekunden bleibt das Licht an
	clr	Led_B
n1:	;blaue LED so schalten wie Led2State
	mov	c,Led2State
p0Led2:	mov	Led2,c
	ret
IF 0
;Doppel-Pufferung via EP1 und Puffer @C0..FF
	mov	IntReq,EP01Stat
	jb	IntReq.1,nep1
	MOVW	AutoPtr1,EP1OutBuf
	LDX	EP1OutBC
	jz	ncop
	mov	r2,a
	mov	r0,#LOW(XAutoDat1)
	mov	r1,#0C0h
cop:	LDX			;(2)
	mov	@r1,a		;(1)
	inc	r1		;(1)
	djnz	r2,cop		;(3)
	
ncop:	jb	IntReq.2,nep1
nep1:
ENDIF

HandleUsbSleep:
;Alles auf Energiesparen schalten; kehrt mit dem Wakeup des Prozessors zurück
	clr	HighSpeed
	setb	Led2State
	LDX	GpifIdleCtl
	mov	r4,a		;retten
	clr	a
	push	OEA
	push	OEB
	push	OED
	 STX			;CTLx-Ausgänge hochohmig
	 mov	OEA,a		;alles hochohmig und Energie sparend
	 mov	OEB,a
	 mov	OED,a
	 mov	PCON,#31h	;Tiefschlaf hier (schon mal Strom gemessen?)
	pop	OED
	pop	OEB
	pop	OEA		;Tiefschlaf beendet
	mov	a,r4
	STX			;CTLx-Ausgänge auf alten Zustand
	ret

SaveFeature:
	clr	FeatureChanged
	mov	a,Feature
	STORX	Scratch
	call	EReqInitBoot
	MOVW	AutoPtr1,Scratch
	mov	r1,#LOW(XAutoDat1)
	mov	r4,#1
	mov	dptr,#0FFFBh	;Letztes Byte vor Seriennummer
	jmp	EWrite		;Daten schreiben (ohne Verify)

;===========================
;== Mikrocode-Interpreter ==
;===========================
;2-Byte-OUT- und 1-Byte-IN-Kommandos abarbeiten
;(auf verschiedenen Wegen, nämlich USB2LPT Bulk+Control, HID Control+Interrupt)
;PE: AutoPtr1=Kommandopuffer
;    ep2l=Kommandopuffer-Länge
;    PendingByte=unvollständiges Kommandobyte
;    AutoPtr2=Ergebnispuffer
;PA: AutoPtr2=Ende der Ergebnisse
;    PendingByte=unvollständiges Kommandobyte
;VR: alle, ep2h=ep2l=0
ProcessInOut:
	mov	a,#100		;kurz (Blinken mit 5 Hz)
	call	StartLed
	mov	a,ep2ll
	add	a,#0FFh
	mov	a,ep2lh
	addc	a,#0
	mov	ep2lh,a		;High-Teil für Doppelschleife gestalten
	jz	pioe
	clr	a
	xch	a,PendingByte
	jz	pio1
	clr	ACC.4
	sjmp	pio2
pio1:
	LDX	XAutoDat1	;"Befehls"-Byte vom OUT2-Puffer lesen
pio2:	CMP	a,#21h
	jc	pio3
	call	usr2f		;zur Extension springen
	jc	pioe		;Bulk-Block-Rest bei Fehler verwerfen
	sjmp	pio5
pio3:
	mov	r3,a
	jb	ACC.4,pioIn
	djnz	ep2ll,pio4	;len--
	djnz	ep2lh,pio4
	setb	ACC.4
	mov	PendingByte,a	;Fehler, wenn kein Folge-Byte folgt
	sjmp	pioe
pio4:	LDX			;weiteres AUTODATA-Byte lesen
	mov	r4,a
	call	Aufruf
	sjmp	pio5
pioIn:
	call	Aufruf
	STX	XAutoDat2	;IN-Byte in Ep6FifoBuf ablegen
pio5:	djnz	ep2ll,pio1
	djnz	ep2lh,pio1
pioe:	clr	c
	ret

;=====================
;== Initialisierung ==
;=====================
main:
	clr	a
	mov	IOD,a
	mov	OED,#00000111b	;Massepins (Rev.4) simulieren
	mov	CKCON,a		;Handbremse für "movx" lösen
	mov	MPAGE,#HIGH(CPUCS)	;die meisten XRAM-Register
	mov	r0,#7Fh
	mov	sp,r0
clr_ram:mov	@r0,a
	djnz	r0,clr_ram
	STX	CPUCS,10h	;kein CLK-Ausgang, 48 MHz
;Persistente Konfiguration lesen und übernehmen
	call	EReqInitBoot
	MOVW	AutoPtr1,Scratch
	mov	r1,#LOW(XAutoDat1)
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
;RevCtl erfordert im Gegensatz zur Dokumentation ein SyncDelay!
	STX	RevCtl,11b	;Bit0 und Bit1 setzen
	mov	IOB,#0		;0 gibt das BIOS standardmäßig aus (?)
	jb	DirectIo,w4	;bei DirectIo bleibt Datenport Eingang
	mov	OEB,#0FFh	;alles Ausgabe auf PortB
;Feststellung der Brücke zwischen IOA.6 und IOA.7 bei Revision 3
w4:	mov	OEA,#80h
	setb	IOA.7		;Pull-Up-Treiber aus
	jnb	IOA.6,isrev2
	clr	IOA.7		;Pull-Up-Treiber ein
	jb	IOA.6,isrev2
	mov	IOA,#00100100b	;Pull-Up ein, gelb ein, blau aus, SEL low
	mov	OEA,#DefOEA
	sjmp	isrev3
isrev2:
	call	PatchCode
	mov	IOA,#10000100b	;gelbe LED ein, blaue LED aus, INI high
	mov	OEA,#DefOEAr2	;Ausgabe für LEDs und INI auf PortA
isrev3:
	STX	GpifCtlCfg,10000000b	;Push-Pull-Betrieb
	STX	PREV,01110011b	;GpifIdleCtl: AF und STB auf High, SEL auf Low
	jnb	DirectIo,w5	;bei DirectIo auch Steuerleitungen Eingänge
	STX	,00000011b	;GpifIdleCtl: Eingänge
	anl	OEA,#11111011b	;INI als Eingang
w5:	setb	EICON.5		;Resume-Interrupt (auch bei EA=0)

	LDX	USBCS
IF Always_Renum=0
	jnb	ACC.7,nohs
	setb	HighSpeed
nohs:	jb	ACC.1,noren	;ReNum überspringen, falls vom EEPROM geladen
ENDIF
	setb	ACC.3		;DisCon
	STX
	mov	r5,#48		;1,5 Sekunden warten
	mov	r3,#0
w2:	call	w1		;verputzt R4=0
	djnz	r3,w2
	djnz	r5,w2
	xch	a,r4		;A=0
	dec	a		;A=0FFh
	STX	UsbIrq		;alle alten Interruptanforderungen löschen
	STX	EpIrq
	STX	UsbErrIrq
	xch	a,r4
	setb	ACC.1		;ReNum
	clr	ACC.3		;DisCon
;	clr	Led2
	STX	USBCS
noren:
	call	ResetFifos
IF Gpif
	STX	IFConfig,11000010b
	STX	EP2FifoCfg,0	;PortD frei
	STX	NEXT,10h		;EP4FifoCfg: AutoOut(?)
	STX	NEXT,0		;EP6FifoCfg
	STX	NEXT		;EP8FifoCfg
	MOVW	AutoPtr1,SPP_W
	MOVW	AutoPtr2,%(WaveData+32)
	mov	r4,#32
	call	memcpy
	STORX	WF3_Branch,077o
	STORX	WF3_Opcode,111b
	STORX	WF3_Output,011b
	STX	GpifIdleCS,1
	mov	GpifSglDatLX,#0
	STX	EP4GpifFlgSel,1
ENDIF
p3Led:	clr	Led		;gelbe LED einschalten
;	call	EReqInitBoot
;===================
;== Hauptschleife ==
;===================
mainloop:
	call	usr36		;Zyklischer User-Aufruf
	jnb	FeatureChanged,skipSF
	call	SaveFeature
;Software-FIFOs (SPP- und ECP-Simulation) abarbeiten
skipSF:	jnb	ECR_Bits.2,skip0;SPP-FIFO-Transfer prüfen
	call	SppXfer
skip0:	jnb	ECR_Bits.3,skip1;ECP-FIFO-Transfer prüfen
	call	EcpXfer
;USB-Interrupts abarbeiten
skip1:	LDX	UsbIrq		;USB-Interrupts prüfen
	jz	nu5		;Kurzschluss-Sprung
	STX			;alle Anforderungen löschen
	mov	IntReq,a	;bitadressierbar machen
	jnb	IntReq.0,nu0	;SUDAV-Anforderung?
;	STX	SudPtrCtl,1
	clr	a		;lang (Blinken mit 2 Hz)
	call	StartLed
	call	HandleSUD	;liefert CY=1 für Ep0Stall
u31ok:	mov	a,#40h		;HSNAK setzen
	rlc	a		;EP0STALL einsetzen
	STX	EP0CS
nu0:
	jnb	IntReq.1,nu1	;SOF-Anforderung?
	jnb	DarkBlue,nu1a
p2Led2:	setb	Led2		;bei jedem (Mikro)Frame blaue LED (erst mal) AUS
nu1a:	LDX	UsbFrameL
	CJE	a,FrameCnt,nu1	;Nur ganze Rahmen mitzählen!
	mov	FrameCnt,a
	call	On1ms
nu1:
	jnb	IntReq.3,nu3	;Einschlaf-Anforderung?
	;Irgendwie kommt es beim Busaufzählen zu 2 Einschlaf-Aufforderungen,
	;die zum "Defekt" des USB-Gerätes führen.
	;Das Abwarten von SET CONFIURATION reicht.
	jnb	Configuration.0,nu3	;Ignorieren solange unkonfiguriert
	call	HandleUsbSleep
nu3:
	jnb	IntReq.4,nu4	;USB-Reset?
	;ohne Behandlung folgt Absturz beim Booten mit angestecktem Gerät?
	mov	Configuration,#0
p4Led:	setb	Led		;gelbe LED aus
	setb	Led2State	;blaue LED aus
	call	ResTogAll
nu4:
	jnb	IntReq.5,nu5
	setb	HighSpeed
	clr	Led2State	;blaue LED ein
nu5:
;Endpoints abarbeiten
	mov	IntReq,Ep2468Stat	;bitadressierbar machen
;Auswertung des OUT-Bulk-Transfers
;AutoPtr1: Ep2FifoBuf-Zeiger, ep2lh:ep2ll=Länge (DJNZ-aufbereitet)
;AutoPtr2: Ep6FifoBuf-Zeiger
;R3=Befehlskode
;R4=Daten (bei IN-Befehl A)
	jb	IntReq.0,nep2	;EP2E: Wenn EP2 nicht leer...
	jb	IntReq.5,nep2	;EP6F: Wenn EP6 nicht voll...
	LDX	EP2BCH		;Anzahl der Bytes des Bulk-Transfers
	mov	ep2lh,a
	LDX	NEXT		;EP2BCL
	mov	ep2ll,a
	MOVW	AutoPtr1,Ep2FifoBuf
	MOVW	AutoPtr2,Ep6FifoBuf
	call	ProcessInOut
	mov	a,AutoPtrL2
	subb	a,#LOW(EP6FifoBuf)
	mov	r1,a
	mov	a,AutoPtrH2
	subb	a,#HIGH(EP6FifoBuf)
	mov	r0,a
	orl	a,r1
	jz	NoIn
	mov	a,r0
	STX	EP6BCH
	mov	a,r1
	SyncDelay
	STX	NEXT		;EP6BCL: Bulk-In "anschubsen"
	SyncDelay
NoIn:	STX	OutPktEnd,82h	;Bulk-Out-Puffer zurück zur SIE (nicht zum GPIF)
nep2:
IF Gpif=0
	jb	IntReq.2,nep4	;EP4E
	call	UniXfer		;(Nur) ein Byte zum Drucker übertragen
nep4:
ELSE
	mov	IntReq,EP24FifoFlgs
	jb	IntReq.5,nf4	;EP4EF
	mov	a,GpifTrig
	jnb	ACC.7,nf4	;kann nicht starten
	;STX	IFConfig,11000010b	;30 MHz reichen dicke
	call	StartLed2
	STX	GpifTcB0,1	;GPIF freigeben bei jeder Art Blockade
	mov	GpifTrig,#1
	;SyncDelay
f4:	;mov	a,GpifTrig
	;jnb	ACC.7,f4	;kann nicht stoppen
	;STX	IFConfig,11000000b
nf4:
ENDIF
	jmp	mainloop

PatchCode:
;Für Revision 2 Patches bei LED-Zugriffen vornehmen
	setb	Rev2
	STORX	p0Led+1,Ledr2	;Bitadresse
	STORX	p1Led+1
	STORX	p2Led+1
	STORX	p3Led+1
	STORX	p4Led+1
	STORX	p0Led2+1,Led2r2
	STORX	p2Led2+1
	ret
	
;USB-Deskriptoren
EVEN
;********************************************
;** Antwort auf Get Descriptor: Device (1) **
;********************************************
;Geräte-Beschreiber
DeviceDescriptor:
	db	18	;bLength
	db	1	;bDescriptorType, 1=DEVICE
	DWI	200h	;bcdUSB 2.0
	db	0	;bDeviceClass		Multifunktion
	db	0	;bDeviceSubClass
	db	0	;bDeviceProtocol
	db	64	;bMaxPacketSize0
IF MyID
	DWI	16C0h	;idVendor		Voti
	DWI	06B3h	;idProduct		h#s Sub-ID
 IF Interfaces > 1
	DWI	4621h	;bcdDevice		USB2LPT, USB2LPT2
 ELSE
	DWI	4620h	;bcdDevice
 ENDIF
ELSE
	DWI	04B4h
	DWI	8613h	;idProduct		FX2
	DWI	4621h	;bcdDevice
ENDIF
	db	1	;iManufacturer (1)
	db	2	;iProduct (2)
	db	0	;iSerialNumber		keine Seriennummer!
	db	1	;bNumConfigurations	keine Wahl von Stromaufnahmen
;Geräte-Qualifizierer (gleich für beide Geschwindigkeiten)
EVEN
DeviceQualifier:
	db	10	;bLength
	db	6	;bDescriptorType	Qualifizierer
	DWI	200h	;bcdUSB			USB 2.0
	db	0	;bDeviceClass		Multifunktion
	db	0	;bDeviceSubClass
	db	0	;bDeviceProtocol
	db	64	;bMaxPacketSize0
	db	1	;bNumConfigurations	keine Wahl von Stromaufnahmen
	db	0	;bReserved

IF Interfaces > 2
EVEN
HidReportDesc:
;Der erste Teil ist kompatibel zu Low-Speed-USB2LPT
	db	6,0,0FFh	;G Usage Page (Vendor defined)
	db	9,1		;L Usage (Vendor Usage 1)
	db	0A1h,1		;M Collection (Application)
	db	15h,0		;G  Logical Minimum (0)
	db	26h,0FFh,0	;G  Logical Maximum (255)
	db	75h,8		;G  Report Size (8 bits)
 _nb_ SET 0
 REPT 7
 _nb_ SET _nb_+1
	db	85h,_nb_	;G  Report ID (1..7)
	db	95h,_nb_	;G  Report Count (1..7 Byte)
	db	9,_nb_		;L  Usage (1..7) = Microcode or Result
	db	0b2h,2,1	;M  Feature (Var,Buf)
 ENDM
;Diesen Teil mit den 64-Byte-Reports gibt es nur beim Full/High-Speed-USB2LPT
	db	85h,8		;G  Report ID (8)
	db	5,1		;G  Usage Page (Generic Desktop)
	db	9,3Ah		;L  Usage (Counted Buffer)
	db	0A1h,2		;M  Collection (Logical)
	db	95h,1		;G   Report Count (1 Byte)
	db	9,3Bh		;L   Usage (Byte Count)
	db	91h,2		;M   Output (Var)
	db	9,3Bh		;L   Usage (Byte Count)
	db	81h,2		;M   Input (Var)
	db	95h,62		;G   Report Count (62 Byte)
	db	6,0,0FFh	;G   Usage Page (Vendor defined)
	db	9,8		;L   Usage (8) = Microcode
	db	92h,2,1		;M   Output (Var,Buf)
	db	9,9		;L   Usage (9) = Result
	db	82h,2,1		;M   Input (Var,Buf)
	db	0C0h		;M  End Collection
	db	0C0h		;M End Collection
HRDE:
ENDIF

EVEN
;********************************************
;** Antwort auf Get Descriptor: Config (2) **
;********************************************
ConfigDescFS:	;12-Mbit/s-Version
;Konfigurations-Beschreiber 0
	db	9	;bLength
	db	2	;bDescriptorType	2=CONFIG
	DWI	CfgEFS-ConfigDescFS	;wTotalLength
	db	Interfaces	;bNumInterfaces
	db	1	;bConfigurationValue (willkürliche Nummer dieser K.)
	db	0	;iConfiguration (3)
	db	80h	;bmAttributes (Busversorgt, kein Aufwecken)
	db	100/2	;MaxPower (in 2 Milliampere)	100 mA
;Interface-Beschreiber 0, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	0	;bInterfaceNumber
	db	0	;bAlternateSetting
IF Interfaces > 1
	db	2	;bNumEndpoints
ELSE
	db	3	;bNumEndpoints
ENDIF
	db	-1	;bInterfaceClass	hersteller-spezifisch
	db	0	;bInterfaceSubClass	(passt in keine Klasse)
	db	0	;bInterfaceProtocol
	db	0	;iInterface (4)
;Enden-Beschreiber C0I0A0:Bulk EP2OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	2	;bEndpointAddress	EP2OUT
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
;Enden-Beschreiber C0I0A0:Bulk EP6IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	86h	;bEndpointAddress	EP6IN
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
IF Interfaces = 1
;Enden-Beschreiber C0I0A0:Bulk EP4OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	4	;bEndpointAddress	EP4OUT
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
ENDIF
IF Interfaces > 1
 _as_ SET 0	;Drei Alternativen erzeugen lassen: Uni, BiDi, Dot4
 _ne_ SET 1	;Erste Alternative (Uni) mit einem Ende, BiDi und Dot4 mit zwei
 REPT 3
;Interface-Beschreiber 1, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	1	;bInterfaceNumber
	db	_as_	;bAlternateSetting
	db	_ne_	;bNumEndpoints
	db	7	;bInterfaceClass	Drucker
	db	1	;bInterfaceSubClass	Drucker
	db	_as_+1	;bInterfaceProtocol	Uni/BiDi/Dot4
	db	_as_+5	;iInterface		(5), (6), (7)
;Enden-Beschreiber C0I1A?:Bulk EP4OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	4	;bEndpointAddress	EP4OUT
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
  IF _ne_ = 2
;Enden-Beschreiber C0I1A?:Bulk EP8IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	88h	;bEndpointAddress	EP8IN
	db	2	;bmAttributes		2=BULK
	DWI	64	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
  ENDIF
  _as_ SET _as_+1
  _ne_ SET 2
 ENDM
ENDIF
IF Interfaces > 2
;Interface-Beschreiber 2, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	2	;bInterfaceNumber
	db	0	;bAlternateSetting
	db	2	;bNumEndpoints
	db	3	;bInterfaceClass	HID
	db	0	;bInterfaceSubClass
	db	0	;bInterfaceProtocol
	db	8	;iInterface
;HID-Beschreiber
HidDesc:
	db	9	;bLength
	db	21h	;HID
	DWI	101h	;BCD representation of HID version
	db	0	;target country code
	db	1	;number of HID Report Descriptor infos to follow
	db	22h	;descriptor type: report
	DWI	HRDE-HidReportDesc	;total length of report descriptor 
;Enden-Beschreiber C0I2A0:Interrupt EP1IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	81h	;bEndpointAddress	EP1IN
	db	3	;bmAttributes		3=Interrupt
	DWI	64	;wMaxPacketSize
	db	2	;bInterval		2 ms
;Enden-Beschreiber C0I2A0:Interrupt EP1OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	01h	;bEndpointAddress	EP1OUT
	db	3	;bmAttributes		3=Interrupt
	DWI	64	;wMaxPacketSize
	db	2	;bInterval		2 ms
ENDIF
CfgEFS:

EVEN
ConfigDescHS:	;480-Mbit/s-Version
;Konfigurations-Beschreiber 0
	db	9	;bLength
	db	2	;bDescriptorType	2=CONFIG
	DWI	CfgEHS-ConfigDescHS	;wTotalLength
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
IF Interfaces > 1
	db	2	;bNumEndpoints
ELSE
	db	3	;bNumEndpoints
ENDIF
	db	-1	;bInterfaceClass	hersteller-spezifisch
	db	0	;bInterfaceSubClass
	db	0	;bInterfaceProtocol
	db	4	;iInterface (4)
;Enden-Beschreiber C0I0A0:Bulk EP2OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	2	;bEndpointAddress	EP2OUT
	db	2	;bmAttributes		2=BULK
	DWI	512	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
;Enden-Beschreiber C0I0A0:Bulk EP6IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	86h	;bEndpointAddress	EP6IN
	db	2	;bmAttributes		2=BULK
	DWI	512	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
IF Interfaces = 1
;Enden-Beschreiber C0I0A0:Bulk EP4OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	4	;bEndpointAddress	EP4OUT
	db	2	;bmAttributes		2=BULK
	DWI	512	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
ENDIF
IF Interfaces > 1
 _as_ SET 0	;Drei Alternativen erzeugen lassen: Uni, BiDi, Dot4
 _ne_ SET 1	;Erste Alternative (Uni) mit einem Ende, BiDi und Dot4 mit zwei
 REPT 3
;Interface-Beschreiber 1, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	1	;bInterfaceNumber
	db	_as_	;bAlternateSetting
	db	_ne_	;bNumEndpoints
	db	7	;bInterfaceClass	Drucker
	db	1	;bInterfaceSubClass	Drucker
	db	_as_+1	;bInterfaceProtocol	Uni/BiDi/Dot4
	db	_as_+5	;iInterface		(5), (6), (7)
;Enden-Beschreiber C0I1A?:Bulk EP4OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	4	;bEndpointAddress	EP4OUT
	db	2	;bmAttributes		2=BULK
	DWI	512	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
  IF _ne_ = 2
;Enden-Beschreiber C0I1A?:Bulk EP8IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	88h	;bEndpointAddress	EP8IN
	db	2	;bmAttributes		2=BULK
	DWI	512	;wMaxPacketSize
	db	0	;bInterval		keine Bedeutung bei BULK
  ENDIF
  _as_ SET _as_+1
  _ne_ SET 2
 ENDM
ENDIF
IF Interfaces > 2
;Interface-Beschreiber 2, Alternative 0:
	db	9	;bLength
	db	4	;bDescriptorType	4=INTERFACE
	db	2	;bInterfaceNumber
	db	0	;bAlternateSetting
	db	2	;bNumEndpoints
	db	3	;bInterfaceClass	HID
	db	0	;bInterfaceSubClass
	db	0	;bInterfaceProtocol
	db	8	;iInterface
;HID-Beschreiber
	db	9	;bLength
	db	21h	;HID
	DWI	101h	;BCD representation of HID version
	db	0	;target country code
	db	1	;number of HID Report Descriptor infos to follow
	db	22h	;descriptor type: report
	DWI	HRDE-HidReportDesc	;total length of report descriptor 
;Enden-Beschreiber C0I2A0:Interrupt EP1IN
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	81h	;bEndpointAddress	EP1IN
	db	3	;bmAttributes		3=Interrupt
	DWI	64	;wMaxPacketSize
	db	1	;bInterval		1 ms
;Enden-Beschreiber C0I2A0:Interrupt EP1OUT
	db	7	;bLength
	db	5	;bDescriptorType	5=ENDPOINT
	db	01h	;bEndpointAddress	EP1OUT
	db	3	;bmAttributes		3=Interrupt
	DWI	64	;wMaxPacketSize
	db	1	;bInterval		1 ms
ENDIF
CfgEHS:

EVEN
;********************************************
;** Antwort auf Get Descriptor: String (3) **
;********************************************
StringDesc:
Str0:	db	Str1-$,3
	DWI	0407h		;Deutsch (Deutschland)
Str1:	db	Str2-$,3
	DWI	'h','a','f','t','m','a','n','n','#'
	DWI	's','o','f','t','w','a','r','e'
Str2:	db	Str3-$,3
	DWI	'h','#','s',' ','U','S','B','2','L','P','T','2',' ','-',' '
	DWI	'd','e','r',' ','U','m','s','e','t','z','e','r',',',' '
	DWI	'd','e','r',' ','w','i','r','k','l','i','c','h',' '
	DWI	'f','u','n','k','t','i','o','n','i','e','r','t','!'
Str3:	db	Str4-$,3
	DWI	'B','u','s','v','e','r','s','o','r','g','t',',',' '
	DWI	'k','e','i','n',' ','A','u','f','w','e','c','k','e','n'
Str4:	db	Str5-$,3
	DWI	'D','i','r','e','k','t','z','u','g','r','i','f','f'
Str5:
IF Interfaces > 1
	db	Str6-$,3
	DWI	'U','n','i','d','i','r','e','k','t','i','o','n','a','l'
Str6:	db	Str7-$,3
	DWI	'B','i','d','i','r','e','k','t','i','o','n','a','l'
Str7:	db	Str8-$,3
	DWI	'I','E','E','E','1','2','8','4','.','4',' '
	DWI	'(','D','o','t','4',')'
ENDIF
Str8:
IF Interfaces > 2
	db	Str9-$,3
	DWI	'T','r','e','i','b','e','r','l','o','s','e','r',' '
	DWI	'U','s','e','r','-','M','o','d','e','-'
	DWI	'Z','u','g','a','n','g',' '
	DWI	'f',252,'r',' ','W','i','n','6','4',' '
	DWI	'(','u','m','g','e','h','t',' '
	DWI	'T','r','e','i','b','e','r'
	DWI	'z','e','r','t','i','f','i','z','i','e','r','u','n','g','s'
	DWI	'z','w','a','n','g',')'
ENDIF
Str9:	db	0	;Endekennung

AnswerBuf:		;Platz für EP0InBuf (In-Bytes)
	ds	8

IF Gpif

SetWaveform:
;PE: AutoPtr1 = Quelle, AutoPtr2 = Ziel (durch 32 teilbar)
	mov	r0,#LOW(XAutoDat1)
	mov	r1,#LOW(XAutoDat2)
swf:	movx	a,@r0
	movx	@r1,a
	movx	a,@r0
	movx	@r1,a
	movx	a,@r0
	movx	@r1,a
	movx	a,@r0
	movx	@r1,a
	mov	a,AutoPtrL2
	add	a,#4	;die anderen Bytes ungenutzt lassen
	mov	AutoPtrL2,a
	anl	a,#1Fh
	jnz	swf	;Vier Runden
	
memcpy:
	mov	r0,#LOW(XAutoDat1)
	mov	r1,#LOW(XAutoDat2)
cpy:	LDX
	STX1
	djnz	r4,cpy
	ret

;GPIF-Wellenformen (max. 4 Zustände erforderlich)

STB_HI	equ	1 shl STB
AF_HI	equ	1 shl AF
SEL_HI	equ	1 shl SEL
ACK_HI	equ	11o shl ACK
BSY_HI	equ	11o shl BSY

SPP_W:	;Normales Schreiben
	db	071o	;THEN Idle ELSE Next
	db	4*48
	db	4*48
	db	4*48
	db	070o	;THEN 0 ELSE 0
	db	0,0,0
	
	db	001b	;IF
	db	010b	;4 µs Daten anlegen
	db	010b	;4 µs Strobe lo
	db	010b	;4 µs Daten nachhalten
	db	101b	;NEXT, JUMP
	db	0,0,0
	
	db	AF_HI+STB_HI
	db	AF_HI+STB_HI
	db	AF_HI
	db	AF_HI+STB_HI
	db	AF_HI+STB_HI
	db	0,0,0
	
	db	060o+BSY;FIFO EMPTY or PRINTER BUSY
	db	0,0,0
	db	060o+BSY;FIFO EMPTY or PRINTER BUSY
	db	0,0,0

BM_R:	;Byte Mode (Rücklesen)
	db	001o
	db	021o
	db	1*30
	db	077o
	
	db	001b	;AF lo, warten bis ACK lo
	db	011b	;Daten, AF hi, warten bis ACK hi
	db	000b	;1 µs STB lo
	db	001b	;STB hi, JUMP
	
	db	SEL_HI+STB_HI
	db	SEL_HI+AF_HI+STB_HI
	db	SEL_HI+AF_HI
	db	SEL_HI+AF_HI+STB_HI
	
	db	ACK_HI
	db	ACK_HI
	db	0,0

EPP_WA:	;EPP-Modus: Schreiben: Adresse
	db	010o
	db	012o
	db	077o
	db	0FFh
	
	db	011b	;SEL lo, warten bis BSY hi
	db	011b	;SEL hi, warten bis BSY lo
	db	101b	;NEXT, JUMP
	db	0FFh
	
	db	AF_HI	
	db	SEL_HI+AF_HI
	db	SEL_HI+AF_HI
	db	0FFh
	
	db	BSY_HI
	db	BSY_HI
	db	0
	db	0FFh
	
EPP_WD:	;EPP-Modus: Schreiben: Daten
	db	010o
	db	012o
	db	077o
	db	0FFh

	db	011b	;AF lo, warten bis BSY hi
	db	011b	;AF hi, warten bis BSY lo
	db	101b	;NEXT, JUMP
	db	0FFh

	db	SEL_HI
	db	SEL_HI+AF_HI
	db	SEL_HI+AF_HI
	db	0FFh

	db	BSY_HI
	db	BSY_HI
	db	0
	db	0FFh

EPP_RA:	;EPP-Modus: Lesen: Adresse
	db	010o
	db	017o
	db	0FFh
	db	0FFh

	db	001b	;SEL lo, warten bis BSY hi
	db	011b	;SEL hi, warten bis BSY lo
	db	0FFh
	db	0FFh

	db	AF_HI+STB_HI
	db	SEL_HI+AF_HI+STB_HI
	db	0FFh
	db	0FFh

	db	BSY_HI
	db	BSY_HI
	db	0FFh
	db	0FFh

EPP_RD:	;EPP-Modus: Lesen: Daten
	db	010o
	db	017o
	db	0FFh
	db	0FFh

	db	001b	;AF lo, warten bis BSY hi
	db	011b	;AF hi, warten bis BSY lo
	db	0FFh
	db	0FFh

	db	SEL_HI+STB_HI
	db	SEL_HI+AF_HI+STB_HI
	db	0FFh
	db	0FFh

	db	BSY_HI
	db	BSY_HI
	db	0FFh
	db	0FFh

ECP_WA:	;ECP-Modus: Schreiben: Adresse ("Kommando")
	db	010o
	db	012o
	db	077o
	db	0FFh

	db	011b	;STB lo, warten bis BSY hi
	db	011b	;STB hi, warten bis BSY lo
	db	101b	;NEXT, JUMP
	db	0FFh

	db	SEL_HI
	db	SEL_HI+STB_HI
	db	SEL_HI+STB_HI
	db	0FFh

	db	BSY_HI
	db	BSY_HI
	db	0
	db	0FFh

ECP_WD:	;ECP-Modus: Schreiben: Daten
	db	010o
	db	012o
	db	077o
	db	0FFh

	db	011b	;STB lo, warten bis BSY hi
	db	011b	;STB hi, warten bis BSY lo
	db	101b	;NEXT, JUMP
	db	0FFh

	db	SEL_HI+AF_HI
	db	SEL_HI+AF_HI+STB_HI
	db	SEL_HI+AF_HI+STB_HI
	db	0FFh

	db	BSY_HI
	db	BSY_HI
	db	0
	db	0FFh

ECP_R:	;ECP-Modus: Lesen: Adresse oder Daten (je nach BSY)
	db	001o
	db	071o
	db	0FFh
	db	0FFh

	db	001b	;AF hi, warten bis ACK lo
	db	011b	;AF lo, warten bis ACK hi
	db	0FFh
	db	0FFh

	db	SEL_HI+AF_HI+STB_HI
	db	SEL_HI+STB_HI
	db	0FFh
	db	0FFh

	db	ACK_HI
	db	ACK_HI
	db	0FFh
	db	0FFh
ENDIF
END

;"Richtige" Interrupt-Simulation (ACK) erfordert Schaltungsänderung!
;Und ist schwierig am PC zu simulieren (will sagen, ich weiß nicht, wie)
;Ganz zu schweigen von ECP-DMA!
;USB-mäßig wäre es natürlich ein simpler 1-Byte-USB-INT-Transfer...

