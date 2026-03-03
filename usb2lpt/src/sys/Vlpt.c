#include "usb2lpt.h"
#ifndef NTDDK
# define WORD USHORT
# define DWORD ULONG
# include <vmm.h>	// Install_IO_Handler
#endif

/***********************
 ** Globale Variablen **
 ***********************/

#define Usage_Len 32		// >=4, die ersten vier (auch) via DR0..DR3
USHORT Usage[Usage_Len];	// Überwachte Portadressen in Viererblöcken (Bits 1:0 = 0)
// Der Wert 0 bedeutet: freies Debugregister
// Der Wert 1 weist beim Treiber-Start besetzte Debugregister aus
// (diese werden jedoch bei UCB_ForceDebReg trotzdem verwendet)
// Ungerade Zahlen (Bit 0 gesetzt) für temporär ausgeschaltete Portadressen(?)
// Die Indizes 0..3 gelten für DR0..DR3, alle anderen: ausschließlich [RW]_PORT_U[CSL]
// Dieses Array kann mit "rep scasw" schnell nach dem passenden Index durchsucht werden.

ULONG UsageBits;		// 0-Bits für temporär ausgeschaltete Portadressen
#ifdef _X86_
static BOOLEAN HaveDebugExt;	// Wenn der Prozessor I/O-Breakpoints unterstützt (1 oder 0)
static BOOLEAN HaveSysEnter;	// Wenn der Prozessor den SysEnter-Befehl unterstützt (1 oder 0)
#else
# define HaveDebugExt 1
# define HaveSysEnter 1
#endif

static PDEVICE_EXTENSION X4DR[Usage_Len]; // zugehörige USB2LPT-Geräte

static struct {
 void* Int1;		/* bleibt Null ohne HaveDebugExt */
#ifdef NTDDK
 void* Int2E;		/* bleibt NULL ohne Pentium */
 void* SysEnter;
#endif
}OldInt;

static USHORT SaveDS;		// Für ISR zum Restaurieren
static USHORT SaveFS;		// FS ist numerisch gleich auf verschiedenen Prozessoren
#ifndef NTDDK
/*static*/ ULONG CurThreadPtr;	// Get_Cur_Thread_Handle-Ersatz
#endif
// Strukturzeiger für 2. Kernel-Stack (Win9x) C0013E78
// Unter W2K/WXP ist der aktuelle Thread durch FS: adressiert
// ...dann ist diese Variable = NULL.
void NewInt1(void);
#if defined(NTDDK) && defined(_X86_)
void NewInt2E(void);
void NewSysEnter(void);
#endif

static MYTIMER debset;	// Debugregister-Setz-Zeitgeber (periodisch)
// Der Timer dient zur Feststellung "geklauter" Debugregister.
ULONG DebRegStolen[3];	// globaler Zähler für "geklaute" Debugregister (nicht in X->ac)
// Index 0: Timer
// Index 1: Int2E
// Index 2: SysEnter / SysCall

/*****************************
 ** Debug-Register und Int1 **
 *****************************/
/* Zu tun: Auf Mehrprozessormaschinen müssen in _allen_ Prozessoren
 * die Debugregister gesetzt/gelesen/geprüft werden.
 * Ein NTDDK-Treiber kann dazu KeSetTargetProcessorDpc benutzen?
 * Auf Dual-Core-Maschinen (zumindest beim FSC Pentium4HT
 * "Mauersberger.mb.tu-chemnitz.de") haben die beiden Prozessoren
 * _verschiedene_ IDT-Einträge! Da stürzt der Debugregister-Trap zz. ab!
 * Problem: Für WDM-Treiber stehen die prozessorspezifischen Funktionen
 *          nicht zur Verfügung. Daher Aufspaltung Oktober 2009
 */

#ifdef NTDDK

#ifdef _X86_
// IRQL >= DISPATCH_LEVEL
// Der KDPC (1. Argument) wird nicht mehr benötigt.
static __declspec(naked) void EachProcessorDpc(KDPC*Dpc, PVOID Context, PVOID Arg1, PVOID Arg2) { _asm{
	mov	ecx,[esp+12]		// ECX = Arg1
	call	[esp+8]			// Context = Prozedurzeiger
	movzx	eax,_PCR KPCR.Number	// = KeGetCurrentProcessorNumber()
	mov	ecx,[esp+16]		// ECX = Zeiger auf Affinitätsmaske (Arg2)
	lock btr dword ptr[ecx],eax	// Für diesen Prozessor als erledigt markieren
	ret	16
}}
#else
void EachProcessorDpc(KDPC*,PVOID,PVOID,PVOID);
#endif

// IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
// Ruft die angegebene, in Assembler geschriebene Callback-Routine
// für alle Prozessoren auf mit ECX = Argument.
// Blockiert (per Eigenbau-Spinlock) den aktuellen Prozessor (Thread)
// bis alle Prozessoren den Kode ausgeführt haben.
static void __fastcall EachProcessor(void(__fastcall*Callback)(int),int Arg) {
 ULONG i;
 volatile KAFFINITY a;
 a=KeQueryActiveProcessors();
 if (a==1) Callback(Arg);			// Einzelprozessorsystem (Abk.)
 else{
  KAFFINITY m;
  KDPC *Dpc,*pDpc;
  Dpc=ExAllocatePoolWithTag(NonPagedPool,sizeof(KDPC)*MAXIMUM_PROCESSORS,'tplE');
  if (!Dpc) return;
  for (i=0,m=1,pDpc=Dpc; a>=m && i<MAXIMUM_PROCESSORS; i++,m<<=1,pDpc++) if (a&m) {
// Für den aktiven Prozessor direkt aufrufen, sonst unnötiges Kuddelmuddel mit IRQL
   if (i==KeGetCurrentProcessorNumber()) EachProcessorDpc(NULL,Callback,(PVOID)Arg,(PVOID)&a);
   else{
    KeInitializeDpc(pDpc,EachProcessorDpc,Callback);
    KeSetTargetProcessorDpc(pDpc,(char)i);
    KeInsertQueueDpc(pDpc,(PVOID)Arg,(PVOID)&a);
   }
  }
  while (a);	// warten bis alle fertig sind! (Aua! Aber besser geht's wohl nicht.)
  ExFreePoolWithTag(Dpc,'tplE');
 }
}
#else
# define EachProcessor(Callback,Arg) Callback(Arg)
#endif

#ifndef NTDDK
// Basisadresse der Interruptdeskriptortabelle (IDT) beschaffen
// PA: ECX=IDT-Basisadresse
static __declspec(naked) void GetIdtBase(void) { _asm{
	push	ecx
	push	ecx		/* Platz auf dem Stack */
	 sidt	[esp+2]
	pop	ecx
	pop	ecx		/* ECX = Basisadresse der IDT */
	ret
}}
#endif

#ifdef _X86_
// IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
// PE: CL = Interrupt-Nummer
// PA: EAX = Adresse aus IDT (des aktuellen Prozessors)
// VR: EAX,ECX
static __declspec(naked) void* __fastcall GetIdtAddr(UCHAR nr) { _asm{
	movzx	eax,cl
#ifdef NTDDK
	mov	ecx,_PCR KPCR.IDT
#else
	call	GetIdtBase
#endif
	lea	ecx,[ecx+eax*8]
	mov	ax,[ecx+6]		/* High-Teil Offset */
	shl	eax,16
	mov	ax,[ecx+0]		/* Low-Teil Offset */
	ret
}}

// IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
static __declspec(naked) void __fastcall SetIdtAddr(UCHAR nr) { _asm{
/* PE: EAX=neue Adresse für INT
 *     CL: Interrupt-Nummer
 * PA: -
 * VR: ECX,EDX,EAX */
	movzx	edx,cl
	mov	ecx,cr0
	push	ecx
	 btr	ecx,16		/* Supervisor Mode Write Protect */
	 mov	cr0,ecx
#ifdef NTDDK
	 mov	ecx,_PCR KPCR.IDT
#else
	 call	GetIdtBase
#endif
	 lea	ecx,[ecx+edx*8]
	 mov	[ecx+0],ax	/* Low-Teil Offset */
	 rol	eax,16
	 mov	[ecx+6],ax	/* High-Teil Offset */
	pop	ecx
	mov	cr0,ecx
	ret
}}

// IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
static __declspec(naked) void GetInts(void) { _asm{
#ifdef NTDDK
	push	edi
	 mov	edx,offset GetIdtAddr
	 mov	cl,1
	 call	edx
	 cmp	eax,offset NewInt1	// schon gehookt?
	 je	nosave			// nichts tun! (Schleifen vermeiden)
	 mov	edi,offset OldInt
	 stosd
	 mov	cl,0x2E
	 call	edx
	 stosd
	 cmp	[HaveSysEnter],0
	 jz	nosave
	 mov	ecx,0x176		// IA32_SYSENTER_EIP
	 rdmsr
	 stosd
nosave:	pop	edi
#else
	mov	cl,1
	call	GetIdtAddr
	cmp	eax,offset NewInt1	// schon gehookt?
	je	nosave			// nichts tun! (Schleifen vermeiden)
	mov	[OldInt.Int1],eax
nosave:
#endif
	ret
}}


static __declspec(naked) void _fastcall SetSysEnter(void) { _asm{
//PE: EAX = zu setzende Adresse für SysEnter-Befehl
	cmp	[HaveSysEnter],0	// SysEnter vorhanden?
	jz	e			// sonst GPF!
	mov	ecx,0x176		// IA32_SYSENTER_EIP
	xor	edx,edx			// sonst GPF!
	wrmsr
e:	ret
}}


// IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
static __declspec(naked) void _fastcall HookInts(int unused) { _asm{
// Faule Annahme: bei INT1 befindet sich bereits ein gültiger Gate-Deskriptor
// VR: EAX,EDX,ECX; EFlags unverändert
// Bei Hyperthreading hat jeder Prozessor eine eigene IDT.
	pushfd
	 cli
	 cmp	OldInt.Int1,0
	 jz	no_586
	 mov	eax,offset NewInt1	// ISR-Anfangsadresse + Prozessornummer*12
	 mov	cl,1
	 call	SetIdtAddr
#ifdef NTDDK
//	 cmp	[CurThreadPtr],0
//	 jnz	no_586		// kein NT
	 mov	eax,offset NewInt2E
	 mov	cl,0x2E
	 call	SetIdtAddr
	 mov	eax,offset NewSysEnter
	 call	SetSysEnter
#endif
no_586:
	popfd
	ret
}}

// IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
static __declspec(naked) void _fastcall UnhookInts(int unused) { _asm{
// VR: EAX,EDX,ECX; EFlags unverändert
	pushfd
	 cli
#ifdef NTDDK
	 push	esi
	  mov	esi,offset OldInt
	  lodsd
	  or	eax,eax
	  jz	no_586
	  mov	cl,1
	  call	SetIdtAddr
//	 cmp	[CurThreadPtr],0
//	 jnz	no_586		// kein NT
	  lodsd
	  mov	cl,0x2E
	  call	SetIdtAddr
	  lodsd
	  call	SetSysEnter
no_586:
	 pop	esi
#else
	 mov	eax,[OldInt.Int1]
	 or	eax,eax
	 jz	no_586
	 mov	cl,1
	 call	SetIdtAddr
no_586:
#endif
	popfd
	ret
}}

// Nur sinnvoll mit IRQL >= DISPATCH_LEVEL!
static __declspec(naked) void cyLoadDR(void) { _asm{
/* Debugregister des aktuellen Prozessors laden/wiederherstellen, VR: EAX,EBX,ECX,EDX,ESI,Flags */
// Liefert ECX!=0 wenn sich die Debugregister dabei verändern
	xor	ecx,ecx		// Bit-Sammler
	mov	esi,offset Usage// im Ring 0 immer CLD
	_emit	0x0F		// mov eax,cr4
	_emit	0x20
	_emit	0xE0
	bts	eax,3		/* PENTIUM Debug Extension (DE) aktivieren */
	_emit	0x0F		// mov cr4,eax
	_emit	0x22
	_emit	0xE0
	setnc	cl		// CL=1 wenn Debug Extension ausgeschaltet war
	mov	ebx,DR7
	xor	eax,eax
	lods	word ptr cs:[esi]
	or	ah,ah		// mit (gültiger) Adresse gefüllt?
	jz	no0
	mov	edx,DR0
	mov	DR0,eax
	xor	edx,eax
	or	ecx,edx		// ECX!=0 wenn DR0 verändert wurde
	or	ebx,0x000E0202
	btr	ebx,16		/* DR7=xxxxxxxx xxxx1110 xxxxxx1x xxxxxx1x */
no0:	lods	word ptr cs:[esi]
	or	ah,ah
	jz	no1
	mov	edx,DR1
	mov	DR1,eax
	xor	edx,eax
	or	ecx,edx		// ECX!=0 wenn DR1 verändert wurde
	or	ebx,0x00E00208
	btr	ebx,20		/* DR7=xxxxxxxx 1110xxxx xxxxxx1x xxxx1xxx */
no1:	lods	word ptr cs:[esi]
	or	ah,ah
	jz	no2
	mov	edx,DR2
	mov	DR2,eax
	xor	edx,eax
	or	ecx,edx		// ECX!=0 wenn DR2 verändert wurde
	or	ebx,0x0E000220
	btr	ebx,24		/* DR7=xxxx1110 xxxxxxxx xxxxxx1x xx1xxxxx */
no2:	lods	word ptr cs:[esi]
	or	ah,ah
	jz	no3
	mov	edx,DR3
	mov	DR3,eax
	xor	edx,eax
	or	ecx,edx		// ECX!=0 wenn DR3 verändert wurde
	or	ebx,0xE0000280
	btr	ebx,28		/* DR7=1110xxxx xxxxxxxx xxxxxx1x 1xxxxxxx */
no3:	mov	edx,DR7
	mov	DR7,ebx
	xor	edx,ebx
	or	ecx,edx		// ECX!=0 wenn DR7 verändert wurde
	ret
}}

// Nur sinnvoll mit IRQL >= DISPATCH_LEVEL!
static __declspec(naked) BOOLEAN _fastcall LoadDR(int unused) { _asm{
	push	esi
	push	ebx
	 call	cyLoadDR
	pop	ebx
	pop	esi
	add	ecx,-1		// Returnwert nach CY (gesetzt wenn ECX!=0)
	setc	al		// für sog. Hochsprachen...
	ret
}}
#else
void HookInts(int);	//amd64.asm
void UnhookInts(int);
BOOLEAN LoadDR(int);
#endif

// Alle DPCs laufen mit IRQL == DISPATCH_LEVEL
static VOID SetDebDpc(IN PKDPC dpc,PVOID x,PVOID a,PVOID b) {
 if (LoadDR(0)) {
  DebRegStolen[0]++;
  Vlpt_KdPrint2(("Debugregister gemaust!\n"));
 }
}

/*********************************
 * Prozessorfeatures detektieren *
 *********************************/
#ifdef _X86_
static __declspec(naked) void SwapID(void) { _asm{	/* Pentium-Test */
	pushfd
	pop	eax
	btc	eax,21
	push	eax
	popfd
	ret
}}

// Liefert TRUE wenn der Prozessor das Umschalten des CPUID-Flagbits zulässt
__declspec(naked) BOOLEAN IsPentium(void) { _asm{
 	call	SwapID
	xchg	ecx,eax
	call	SwapID
	sub	eax,ecx
	je	no
	xor	eax,eax
	push	ebx
	inc	eax
	cpuid
	bt	edx,2			// DE = Debugging Extensions?
	pop	ebx
	setc	[HaveDebugExt]
	bt	edx,11			// SEP = SysEnter Present?
	jnc	yes			// kein SYSENTER!
// Sonderfall (Family==6 && Model<3 && Stepping<3) prüfen
	mov	edx,eax
	and	edx,0xFF0		// DH = Family / DL = Model
	and	eax,0x0F		// AL = Stepping
	cmp	dh,6
	jne	k1
	cmp	dl,0x30
	jae	k1
	cmp	al,3
	jb	yes			// kein SysEnter
k1:	mov	[HaveSysEnter],1
yes:	mov	al,1
no:	ret
}}

// Nur aufrufen wenn IsPentium() TRUE lieferte
void PrepareDR(void) {
/* PA: [OldInt.Int1]=Adresse von INT1, aber nur beim Pentium */
// Weiterhin SaveDS, SaveFS, CurThreadPtr
 _asm{
#ifndef NTDDK 
	push	edi
 	 mov	cl,0		// Int0 (bei Int1 stört SoftICE)
	 call	GetIdtAddr
	 xchg	edi,eax
// Sucht ab Int0 den Befehl "mov edi,[xxxxxxxx]"; der Wert ist der
// globale Zeiger für den aktuellen Thread (Stack-Tausch) (Win98)
	 mov	eax,0xC766006A	// So geht's bei W2K los (PUSH 0, ...)
	 scasd
	 stc
	 je	f		// keine Bytefolge suchen, NT-Betriebssystem
	 mov	ecx,100h
	 mov	al,0x8B		// Bytefolge 8B 3D suchen
b:	 repne	scasb
	 stc
	 jne	f
	 cmp	byte ptr [edi],0x3D
	 jne	b
	 mov	eax,[edi+1]
	 mov	[CurThreadPtr],eax	// gleichzeitig Win9x-Kennung
f:	pop	edi
	jc	exi
#endif
// holt den OldInt1-Zeiger, aber bitte nur, wenn's ein Pentium ist
	call	GetInts
	mov	[SaveDS],ds	// Win9x: 30h	WinNT: 23h
	mov	[SaveFS],fs	// Win9x: 78h	WinNT: 30h
// markiert die bereits vor dem Treiber-Start verwendeten Debugregister
// als "nicht verwendungsfähig für uns".  Aufzurufen beim Treiber-Start
 	mov	edx,offset Usage
	mov	eax,DR7
	push	4
	pop	ecx
l:	test	al,3		// Lx oder Gx gesetzt? (Also in Benutzung?)
	setnz	[edx]		// wenn ja, auf 1 setzen, sonst 0 lassen
	shr	eax,2		// nächstes Debugregister
	add	edx,2		// nächstes Usage-Word
	loop	l
 }
// Debugregister-Diebstahl zyklisch rückgängig machen (WinXP)
// Besser wäre es, die Ursache auszumachen...
 KeInitializeTimer(&debset.tmr);
 KeInitializeDpc(&debset.dpc,SetDebDpc,NULL);
#ifndef NTDDK 
exi:;
#endif
}
#else
void iPrepareDR(void);	//amd64.asm

BOOLEAN IsPentium(void) { return TRUE;}	// vorläufig, liefert künftig stets TRUE

void PrepareDR(void) {
 iPrepareDR();
 KeInitializeTimer(&debset.tmr);
 KeInitializeDpc(&debset.dpc,SetDebDpc,NULL);
}
#endif

static void __fastcall SetTimerX() {
 KeSetTimerEx(&debset.tmr,RtlConvertLongToLargeInteger(100*-10000),
   100,&debset.dpc);
}

#ifdef _X86_
/**********************************
 * IOPM-basierter Trap (Win98/Me) *
 **********************************/

static __declspec(naked) void FindAddr(void) { _asm{
//PE: DX=Portadresse
//PA: NZ wenn nicht gefunden
//    EDI=X4DR-Eintrag, ESI=Index, EDX unverändert
//VR: ECX,EAX,ESI,EDI
//Stille Voraussetzung:
// Adressen beim Word- und DWord-Zugriff kreuzen keine 4-Byte-Grenze!
	cmp	dh,1		// Niemals <100h trappen!
	jc	exi		// NZ!
	mov	eax,edx
	and	al,0xFC		// Vier aufeinanderfolgende Adressen
	push	Usage_Len
	pop	ecx
	mov	edi,offset Usage
	repne	scasw		// Adresse suchen
	jne	exi
	push	Usage_Len-1
	pop	esi		// Index ermitteln
	sub	esi,ecx
	mov	edi,X4DR[esi*4]	// pDevExt laden
	test	byte ptr[edi]DEVICE_EXTENSION.f,No_Function	// (temporär) gesperrt?
exi:	ret
}}

#ifndef NTDDK

#ifdef IOPMDEBUG
// Irgendwie geht mein SoftICE nicht mehr!
static char debugstr[]= "USB2LPT:IOPM:INB(###)=##\r\n";
static char debugstr1[]="USB2LPT:IOPM:OUTB(###,##)\r\n";
static char debugstr2[]="USB2LPT:IOPM:Simulate_IO\r\n";

static __declspec(naked) void __fastcall outhex() { _asm{
	movzx	ecx,cl
	std
l1:	push	eax
	 and	al,0x0F
	 add	al,0x90
	 daa
	 adc	al,0x40
	 daa
	 stosb
	pop	eax
	shr	eax,4
	loop	l1
	cld
	ret
}}
#endif

// Versuch für/wegen GhaiRacer 091101
static __declspec(naked) void __fastcall IOCallback() { _asm{
//PE:	EBX = VM-Handle
//	ECX = IoType
//	EDX = Portadresse
//	EBP = Zeiger auf Client_Reg_Struc
//	EAX = Datenbyte/wort/doppelwort (nur bei einfachem OUT-Befehl)
//PA:	EAX = Datenbyte/wort/doppelwort (nur bei einfachem IN-Befehl)
//VR:	alle
	test	cl,0x38		// komplexer I/O-Befehl (Word, DWord, [rep] ins/outs)
	jnz	l2		// zerlegen lassen
	test	cl,BYTE_OUTPUT	// OUT-Befehl?
	jnz	l1
	call	FindAddr
	inc	[edi]DEVICE_EXTENSION.ac.rpu
#ifdef IOPMDEBUG
	 push	edx
#endif
	push	-1		// Platz auf Stack mit 0xFFFFFFFF
	mov	eax,esp
	push	1		// stets 1 Byte
	push	eax		// Adresse der Rückgabedaten
	push	edx		// Portadresse
	push	edi		// DEVICE_EXTENSION
	call	HandleIn	// blockiert Thread
	pop	eax		// Rückgabedaten
#ifdef IOPMDEBUG
	 pop	edx
	 pushad
	  mov	esi,offset debugstr
	  lea	edi,[esi+23]
	  mov	cl,2
	  call	outhex
	  lea	edi,[esi+19]
	  mov	eax,edx
	  mov	cl,3
	  call	outhex
	  VMMCall(Out_Debug_String)
	 popad
#endif
	ret
l1:
	push	eax
	 call	FindAddr
	pop	eax
	inc	[edi]DEVICE_EXTENSION.ac.wpu
#ifdef IOPMDEBUG
	 pushad
	  mov	esi,offset debugstr1
	  lea	edi,[esi+23]
	  mov	cl,2
	  call	outhex
	  dec	edi
	  mov	eax,edx
	  mov	cl,3
	  call	outhex
	  VMMCall(Out_Debug_String)
	 popad
#endif
	push	1
	push	eax
	push	edx
	push	edi
	call	HandleOut
	ret
l2:
#ifdef IOPMDEBUG
	pushad
	 mov	esi,offset debugstr2
	 VMMCall(Out_Debug_String)
	popad
#endif
	VMMJmp(Simulate_IO)	// von Windows 98/Me aufsplitten lassen
}}

static UCHAR HasTrap[Usage_Len];	// Bits 0..3 = Belegungsbits für Portadresse+0 bis +3

// 4 aufeinanderfolgende Adressen per IOPM belegen
static __declspec(naked) void __fastcall InstallIoHandler(int i/*ECX*/) { _asm{
	push	esi
	 xor	eax,eax
	 inc	eax		// AH = Belegung, AL = Bitmaske
	 movzx	edx,Usage[ecx*2]
	 mov	esi,offset IOCallback
l1:	 VMMCall(Install_IO_Handler)
	 jc	l2
	 or	ah,al		// Trap installiert - vermerken
l2:	 shl	al,1
	 inc	edx
	 cmp	al,16
	 jb	l1
	 mov	HasTrap[ecx],ah	// Belegung abspeichern
	pop	esi
	ret
}}

// 4 aufeinanderfolgende Adressen per IOPM freigeben, aber jeweils nur bei vorhergehender Belegung
static __declspec(naked) void __fastcall RemoveIoHandler(int i/*ECX*/) { _asm{
	xor	eax,eax
	xchg	HasTrap[ecx],ah
	inc	eax		// AH = Belegung, AL = Bitmaske
	movzx	edx,Usage[ecx*2]
l1:	test	ah,al
	jz	l2		// Nicht belegt worden
	VMMCall(Remove_IO_Handler)
l2:	shl	al,1
	inc	edx
	cmp	al,16
	jb	l1
	ret
}}
#endif


/********************************
 * API (öffentliche Funktionen) *
 ********************************/

// Scans the USHORT array for given USHORT and returns a pointer to entry; returns NULL if not found
static __declspec(naked) USHORT* __fastcall ScanMemW(SIZE_T l /*ECX*/, const USHORT*p /*EDX*/, USHORT a /*stack*/){ _asm{
	xchg	edx,edi
	mov	eax,[esp+4]
	repne	scasw
	lea	eax,[edi-2]
	je	found
	xor	eax,eax
found:	xchg	edi,edx
	ret	4
}}
#else
USHORT* ScanMemW(SIZE_T,const USHORT*,USHORT);	//amd64.asm
#endif

static USHORT* MakeSpace(BOOLEAN DebReg) {
// Macht Platz in Usage[], entsprechend X4DR[] und UsageBits, für einen neuen, erzwungenen Eintrag.
// Entweder auf den ersten 4 Plätzen (DebReg!=0) oder auf den hinteren 28 Plätzen (DebReg==0).
// Voraussetzung: Die gewünschten Plätze sind bereits besetzt!
// Zurzeit keine Implementierung eines MRU-Mechanismus'.
 int i;
 ULONG m=0;				// Bitmaske für stehen bleibende Bits; 0-Bitpositionen werden geschoben
 USHORT*p=Usage;
 PDEVICE_EXTENSION*x=X4DR;
 if (DebReg) {
  USHORT*q=ScanMemW(4,p,1);		// Suche Win98-vorbelegtes Debugregister (markiert mit 1) zum Überschreiben
  if (q) {
   p=q;					// diese Stelle nehmen, nicht schieben
   DebReg=3;				// Für Debugausgabe markieren ("3" ausgeben)
   goto w98;
  }
  q=ScanMemW(Usage_Len-4,p+4,0);	// Suche freie Stelle auf den hinteren 28 Plätzen zum Zusammenschieben
  if (q) {
   i=(int)(q-p);			// Anzahl zu verschiebender Plätze (4..31)
   if (i<31) m=0xFFFFFFFFUL<<(i+1);	// oben stehen bleibende Bits ermitteln
  }else{
   i=Usage_Len-1;			// Wenn nichts frei, purzelt letzte Stelle heraus
   DebReg=2;				// Für Debugausgabe markieren ("2" ausgeben)
  }
 }else{
  p+=4;
  x+=4;
  i=Usage_Len-5;			// hier purzelt immer etwas heraus (wegen Voraussetzung, s.o.)
  m=0x0F;
 }
 RtlMoveMemory(p+1,p,i<<1);		// Platz machen bei Adressliste Usage[]
 RtlMoveMemory(x+1,x,i<<2);		// Platz machen bei Device-Extension-Liste X4DR[]
 UsageBits=(UsageBits<<1)&~m | UsageBits&m;
w98:
 Vlpt_KdPrint(("MakeSpace:%u\n",DebReg));
 return p;
}

char _stdcall GetIndexDR(USHORT adr) {
// Liefert Momentanbelegung der E/A-Adresse
// PA: 0..3: Debugregister
//     >=4: nur Anzapfung {READ|WRITE}_PORT_U{CHAR|SHORT|LONG}
//     -4: nicht gefunden (bspw. inzwischen herausgeworfen)
// Nebenfunktion: Findet auch freie Indizes mit adr==0 und Win98-vorbelegte mit adr==1
// Muss für verwertbares Ergebnis mit DPC_LEVEL (und angehaltenen anderen Prozessoren?) gerufen werden!
 USHORT*p=Usage;
 USHORT*q=ScanMemW(Usage_Len,p,adr);
 if (!q) return -4;
 return (char)(q-p);
}

char _stdcall AllocDR(USHORT adr,PDEVICE_EXTENSION X,UCHAR flags) {
// Belegung eines E/A-Debugregisters und andere Arbeiten
// Start der Umleitung von 4 aufeinander folgenden Portadressen
// via Debugregister (wenn in flags UC_Debugreg gesetzt ist)
// und via {READ|WRITE}_PORT_U{CHAR|SHORT|LONG}
// sowie (Win98/Me) via IOPM
// Problem: Kein vernünftiger MRU-Mechanismus für Debugregister-Belegung!
// Ist aufwändig und schwer kalkulierbar, es müssten die Traps geloggt werden.
// PA: 0..3: Debugregister wurde belegt, Umleitung zusätzlich per {READ|WRITE}_PORT_U{CHAR|SHORT|LONG}
//     >=4:  kein Debugregister belegt, Umleitung nur per {READ|WRITE}_PORT_U{CHAR|SHORT|LONG}
//     -1: Parameterfehler
//     -2: Adresse wird bereits benutzt (eine Adresse kann nicht mehrere Traps haben)
//     -3: Kein Platz in Tabelle
// Hinweis: Der rückgegebene (nichtnegative) Index zeigt die Momentanbelegung an und kann sich ändern,
// wenn andere Allokationen eine Verschiebung erwirken!
 int i=Usage_Len;
 USHORT*p=Usage;
 BOOLEAN DebReg=flags&HaveDebugExt;	// UC_Debugreg ist Bit 0
 if (adr&0x3) return -1;	// Fehlerkode: falsche Adresse
 if (!(adr&0xFF00)) return -1;	// Board-Register (<100h) sind ebenfalls unzulässig: viel zu gefährlich!
 if (ScanMemW(i,p,adr)) return -2; // Schon benutzt
 if (!DebReg) p+=4, i-=4;	// nur hintere 28 Plätze
 else if (flags&UC_ForceAlloc) i=4;	// nur vordere 4 Plätze
 p=ScanMemW(i,p,0);
 if (!p && flags&UC_ForceAlloc) p=MakeSpace(DebReg);
 if (p) {
#ifdef NTDDK
  KIRQL irql=KeRaiseIrqlToDpcLevel();
#endif
  i=(int)(p-Usage);
  X4DR[i]=X;
  *p=adr;
  if (!(UsageBits&0xF) && i<4) {
//#if defined(NTDDK) && !defined(_X86_)
//   InstallExceptionHandler();
//#else
   EachProcessor(HookInts,0);
//#endif
   SetTimerX();
  }
  UsageBits|=1<<i;
  if (i<4) EachProcessor(LoadDR,i);	// Rückgabewert egal
#ifdef NTDDK
  KeLowerIrql(irql);
#else
  InstallIoHandler(i);
#endif
  return i;
 }
 return -3;			// Kein Register frei
}

#ifdef _X86_
// Diese Routine wird mit ECX = Debugregister-Nummer aufgerufen
// Nur sinnvoll mit IRQL >= DISPATCH_LEVEL!
static __declspec(naked) void _fastcall UnloadDR(int debregnumber) { _asm{
	mov	edx,DR7
	mov	eax,0xFFFCFFF0	// Maske für DR7, HiWord/LoWord vertauscht
	shl	cl,1		// Nr. mal zwei
	shl	eax,cl		// HiWord (künftiges LoWord) richtig
	shl	ax,cl		// LoWord (künftiges HiWord) richtig
	ror	eax,16		// Vertauschen HiWord/LoWord
	and	eax,edx		// Bits weg, Debugregister frei
	mov	DR7,eax
	xor	eax,eax
	shr	cl,1		// wieder zurück
	jnz	no0
	mov	DR0,eax		// hübsch machen (nicht erforderlich, aber macht das Debuggen übersichtlicher)
no0:	loop	no1
	mov	DR1,eax
no1:	loop	no2
	mov	DR2,eax
no2:	loop	no3
	mov	DR3,eax
no3:	ret
}}
#else
void UnloadDR(int);	//amd64.asm
#endif


char _stdcall FreeDR(USHORT adr) {	// Freigabe eines Debugregisters / einer Anzapfung
 int dr;
 if (adr&3 || !(adr&0xFF00)) return -1;	// Parameterfehler
 dr=GetIndexDR(adr);
 if (dr<0) return dr;		// Nicht gefunden (-4)
#ifdef NTDDK
 {KIRQL irql=KeRaiseIrqlToDpcLevel();
#else
 RemoveIoHandler(dr);
#endif
 Usage[dr]=0;			// Freigabe (unsauber, <dr> könnte ungültig sein! cmpxchg oder Mutex!)
 X4DR[dr]=NULL;
 UsageBits&=~(1<<dr);
 if (dr<4) {
  EachProcessor(UnloadDR,dr);
  if (!(UsageBits&0xF)) {	// Kein Debugregister beteiligt?
   KeCancelTimer(&debset.tmr);	// Ganz zurückziehen
//#if defined(NTDDK) && !defined(_X86_)
//   UninstallExceptionHandler();
//#else
   EachProcessor(UnhookInts,0);
//#endif
  }
 }
#ifdef NTDDK
 KeLowerIrql(irql);}
#endif
 return dr;			// okay (>=0)
}

/************************************************************
 ** Abfangen von READ_PORT_UCHAR und WRITE_PORT_UCHAR (NT) **
 ************************************************************/
// Für LabVIEW muss extra ...ULONG abgefangen werden!
#ifdef _X86_

static __declspec(naked) ULONG MyWritePort(void) { _asm{
// Gleiches "Ende" für MyWritePortUchar/Ushort/Ulong
	 inc	[edi]DEVICE_EXTENSION.ac.wpu
	 push	[esp+1Ch+4]	// EAX
	 push	edx
	 push	edi
	 call	HandleOut	// blockiert Thread gelegentlich
	popad
	popfd
	ret	8
}}

static __declspec(naked) ULONG MyReadPort(void) { _asm{
// Gleiches "Ende" für MyReadPortUchar/Ushort/Ulong
	 inc	[edi]DEVICE_EXTENSION.ac.rpu
	 lea	eax,[esp+1Ch+4]	// PUSHAD-Struktur modifizieren lassen
	 push	eax
	 push	edx
	 push	edi
	 call	HandleIn	// blockiert Thread immer für ca. 1 ms
	popad
	popfd
	ret	4
}}

static __declspec(naked) ULONG MyWritePortBuffer(void) { _asm{
// Gleiches "Ende" für MyWritePortBufferUchar/Ushort/Ulong
	 inc	[edi]DEVICE_EXTENSION.ac.wpu
	 inc	[edi]DEVICE_EXTENSION.ac.fail
	 pop	eax		// Längenangabe
	 dec	eax
	 jz	l1
	 inc	[edi]DEVICE_EXTENSION.ac.wdw
l1:	popad
	popfd
	ret	12
}}

static __declspec(naked) ULONG MyReadPortBuffer(void) { _asm{
// Gleiches "Ende" für MyReadPortBufferUchar/Ushort/Ulong
	 inc	[edi]DEVICE_EXTENSION.ac.rpu
	 inc	[edi]DEVICE_EXTENSION.ac.fail
	 pop	eax		// Längenangabe
	 dec	eax
	 jz	l1
	 inc	[edi]DEVICE_EXTENSION.ac.wdw
l1:	popad
	popfd
	ret	12
}}

static __declspec(naked) void MyWritePortUchar(PUCHAR a,UCHAR b) { _asm{
// Gleiche Signatur wie WRITE_PORT_UCHAR()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// b
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	1
	 jmp	MyWritePort
l1:
	popad
	popfd
	out	dx,al
	ret	8
}}

static __declspec(naked) UCHAR MyReadPortUchar(PUCHAR a) { _asm{
// Gleiche Signatur wie READ_PORT_UCHAR()
	mov	edx,[esp+4]	// a
	xor	eax,eax
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	1
	 jmp	MyReadPort
l1:
	popad
	popfd
	in	al,dx
	ret	4
}}

static __declspec(naked) void MyWritePortUshort(PUCHAR a,USHORT b) { _asm{
// Gleiche Signatur wie WRITE_PORT_USHORT()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// b
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	2
	 jmp	MyWritePort
l1:
	popad
	popfd
	out	dx,ax
	ret	8
}}

static __declspec(naked) USHORT MyReadPortUshort(PUCHAR a) { _asm{
// Gleiche Signatur wie READ_PORT_USHORT()
	mov	edx,[esp+4]	// a
	xor	eax,eax
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	2
	 jmp	MyReadPort
l1:
	popad
	popfd
	in	ax,dx
	ret	4
}}

static __declspec(naked) void MyWritePortUlong(PUCHAR a,ULONG b) { _asm{
// Gleiche Signatur wie WRITE_PORT_ULONG()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// b
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	4
	 jmp	MyWritePort
l1:
	popad
	popfd
	out	dx,eax
	ret	8
}}

static __declspec(naked) ULONG MyReadPortUlong(PUCHAR a) { _asm{
// Gleiche Signatur wie READ_PORT_ULONG()
	mov	edx,[esp+4]	// a
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	4
	 jmp	MyReadPort
l1:
	popad
	popfd
	in	eax,dx
	ret	4
}}

static __declspec(naked) void MyWritePortBufferUchar(PUCHAR a,PUCHAR m,ULONG l) { _asm{
// Gleiche Signatur wie WRITE_PORT_BUFFER_UCHAR()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// m
	mov	ecx,[esp+12]	// l
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	1
	 jmp	MyWritePortBuffer
l1:
	popad
	popfd
	xchg	esi,eax
	rep outsb
	xchg	esi,eax
	ret	12
}}

static __declspec(naked) void MyReadPortBufferUchar(PUCHAR a,PUCHAR m,ULONG l) { _asm{
// Gleiche Signatur wie READ_PORT_BUFFER_UCHAR()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// m
	mov	ecx,[esp+12]	// l
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	1
	 jmp	MyReadPortBuffer
l1:
	popad
	popfd
	xchg	edi,eax
	rep insb
	xchg	edi,eax
	ret	12
}}

static __declspec(naked) void MyWritePortBufferUshort(PUSHORT a,PUSHORT m,ULONG l) { _asm{
// Gleiche Signatur wie WRITE_PORT_BUFFER_USHORT()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// m
	mov	ecx,[esp+12]	// l
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	2
	 jmp	MyWritePortBuffer
l1:
	popad
	popfd
	xchg	esi,eax
	rep outsw
	xchg	esi,eax
	ret	12
}}

static __declspec(naked) void MyReadPortBufferUshort(PUSHORT a,PUSHORT m,ULONG l) { _asm{
// Gleiche Signatur wie READ_PORT_BUFFER_USHORT()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// m
	mov	ecx,[esp+12]	// l
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	2
	 jmp	MyReadPortBuffer
l1:
	popad
	popfd
	xchg	edi,eax
	rep insw
	xchg	edi,eax
	ret	12
}}

static __declspec(naked) void MyWritePortBufferUlong(PULONG a,PULONG m,ULONG l) { _asm{
// Gleiche Signatur wie WRITE_PORT_BUFFER_ULONG()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// m
	mov	ecx,[esp+12]	// l
// künftig kürzer:
//	push	4|PT_WRITE|PT_BUFFER
//	call	Redirect	// kümmert sich um den Rest
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	4
	 jmp	MyWritePortBuffer
l1:
	popad
	popfd
	xchg	esi,eax
	rep outsd
	xchg	esi,eax
	ret	12
}}

static __declspec(naked) void MyReadPortBufferUlong(PULONG a,PULONG m,ULONG l) { _asm{
// Gleiche Signatur wie READ_PORT_BUFFER_ULONG()
	mov	edx,[esp+4]	// a
	mov	eax,[esp+8]	// m
	mov	ecx,[esp+12]	// l
	pushfd
	pushad
	 call	FindAddr
	 jnz	l1
	 push	4
	 jmp	MyReadPortBuffer
l1:
	popad
	popfd
	xchg	edi,eax
	rep insd
	xchg	edi,eax
	ret	12
}}
#endif

/***************************
 * Tabelle der Anzapfungen *
 ***************************/

#ifdef _X86_
// Funktionszeiger in Importtabelle (Keine Thunks generieren lassen!) - Nicht aufrufen!
extern void _imp__WRITE_PORT_UCHAR(int,int);
extern void _imp__READ_PORT_UCHAR(int);
extern void _imp__WRITE_PORT_USHORT(int,int);
extern void _imp__READ_PORT_USHORT(int);
extern void _imp__WRITE_PORT_ULONG(int,int);
extern void _imp__READ_PORT_ULONG(int);
extern void _imp__WRITE_PORT_BUFFER_UCHAR(int,int,int);
extern void _imp__READ_PORT_BUFFER_UCHAR(int,int,int);
extern void _imp__WRITE_PORT_BUFFER_USHORT(int,int,int);
extern void _imp__READ_PORT_BUFFER_USHORT(int,int,int);
extern void _imp__WRITE_PORT_BUFFER_ULONG(int,int,int);
extern void _imp__READ_PORT_BUFFER_ULONG(int,int,int);

// Alte und neue Funktionszeiger
void* func[24]={
 _imp__WRITE_PORT_UCHAR,	MyWritePortUchar,	_imp__READ_PORT_UCHAR,		MyReadPortUchar,
 _imp__WRITE_PORT_USHORT,	MyWritePortUshort,	_imp__READ_PORT_USHORT,		MyReadPortUshort,
 _imp__WRITE_PORT_ULONG,	MyWritePortUlong,	_imp__READ_PORT_ULONG,		MyReadPortUlong,
 _imp__WRITE_PORT_BUFFER_UCHAR,	MyWritePortBufferUchar,	_imp__READ_PORT_BUFFER_UCHAR,	MyReadPortBufferUchar,
 _imp__WRITE_PORT_BUFFER_USHORT,MyWritePortBufferUshort,_imp__READ_PORT_BUFFER_USHORT,	MyReadPortBufferUshort,
 _imp__WRITE_PORT_BUFFER_ULONG,	MyWritePortBufferUlong,	_imp__READ_PORT_BUFFER_ULONG,	MyReadPortBufferUlong};
#endif

/**********************
 * Utility-Funktionen *
 **********************/
#ifdef _X86_
#ifdef NTDDK
static __declspec(naked) void BlockProcessorDpc(KDPC *Dpc, PVOID Context, PVOID Arg1, PVOID Arg2) { _asm{
	movzx	eax,_PCR KPCR.Number
	mov	ecx,[esp+12]		// ECX = Zeiger auf Affinitätsmaske (Arg1)
	lock btr dword ptr[ecx],eax	// Für diesen Prozessor als aufgerufen markieren
	mov	ecx,[esp+8]		// ECX = Zeiger auf Blocking (BOOLEAN)
	pushfd
	 cli				// Nicht einmal Interrupts dürfen dazwischenfunken!
block:	 cmp	BOOLEAN[ecx],0
	 jnz	block
	popfd
	mov	ecx,[esp+16]		// ECX = Zeiger auf Affinitätsmaske (Arg2)
	lock btr dword ptr[ecx],eax	// Für diesen Prozessor als erledigt markieren
	ret	16
}}

static void BlockSiblingsAndCall(void(*Callback)(void)) {
 ULONG i;
 volatile BOOLEAN Blocking;
 volatile KAFFINITY a,b;
 KIRQL irql=KeRaiseIrqlToDpcLevel();
 a=KeQueryActiveProcessors();
 i=KeGetCurrentProcessorNumber();
 a&=~(1<<i);		// aktuellen Prozessor aus Bitmaske herausnehmen - ob der Compiler "btr" einsetzt?
 b=a;
 if (a) {	 	// Bei Einzelprozessorsystem: Keine weitere Arbeit
  KAFFINITY m;
  KDPC *Dpc,*pDpc;
  Dpc=ExAllocatePoolWithTag(NonPagedPool,sizeof(KDPC)*MAXIMUM_PROCESSORS,'tplB');
  if (Dpc) {
   Blocking=TRUE;
   for (i=0,m=1,pDpc=Dpc; a>=m && i<MAXIMUM_PROCESSORS; i++,m<<=1,pDpc++) if (a&m) {
    KeInitializeDpc(pDpc,BlockProcessorDpc,(PVOID)&Blocking);
    KeSetTargetProcessorDpc(pDpc,(char)i);
    KeInsertQueueDpc(pDpc,(PVOID)&a,(PVOID)&b);
   }
   while (a);		// warten bis alle blockieren
   Callback();
   Blocking=FALSE;	// Andere Prozessoren erlösen sich
   while (b);		// warten bis alle fertig sind (ist vermutlich unnötig)
   ExFreePoolWithTag(Dpc,'tplB');
  }
 }else Callback();
 KeLowerIrql(irql);
}
#endif//NTDDK

static UCHAR save[12][5];	// Platz für die an 12 Stellen überschriebenen 5 Code-Bytes für JMP

#ifdef NTDDK
static __declspec(naked) void iHookSyscalls(void) { _asm{
#else
extern __declspec(naked) void HookSyscalls(void) { _asm{
#endif
	mov	edx,cr0
	pushad
	pushfd
	 btr	edx,16			// Supervisor Mode Write Protect
	 cli				// höchstes IRQL, keine ISR darf funken!
	 mov	cr0,edx
	 mov	edi,offset save
	 mov	esi,offset func
	 push	12
	 pop	ecx
l:	 lodsd				// Zeiger in Importtabelle
	 mov	ebx,[eax]		// Originaladresse
 	 mov	al,0E9h			// JMP rel.
	 xchg	[ebx],al
	 stosb
	 lodsd				// Umleitungsadresse
	 sub	eax,5			// Opcode-Länge JMP rel.
	 sub	eax,ebx
	 xchg	[ebx+1],eax
	 stosd
	 loop	l
	popfd
	popad
	mov	cr0,edx
	ret
}}

#ifdef NTDDK
static __declspec(naked) void iUnhookSyscalls(void) { _asm{
#else
extern __declspec(naked) void UnhookSyscalls(void) { _asm{
#endif
	mov	edx,cr0
	pushad
	pushfd
	 btr	edx,16			// Supervisor Mode Write Protect
	 cli				// höchstes IRQL, keine ISR darf funken!
	 mov	cr0,edx
	 xor	eax,eax
	 mov	esi,offset save
l:	 mov	edi,func[eax*8]
	 mov	edi,[edi]
	 inc	eax
	 movsb
	 cmp	al,12
	 movsd
	 jb	l
	popfd
	popad
	mov	cr0,edx
	ret
}}

#ifdef NTDDK
extern void HookSyscalls(void) { BlockSiblingsAndCall(iHookSyscalls); }
extern void UnhookSyscalls(void) { BlockSiblingsAndCall(iUnhookSyscalls); }
#endif

/***************************
 ** Portzugriffe und Trap **
 ***************************/
//#define CurXX 0xC0013E78
//#define CurVM 0xC0013EEC
#ifdef NTDDK
static __declspec(naked) void NewInt2E(void) { _asm{
	pushfd
	pushad
	 call	cyLoadDR
	 lock adc ss:[DebRegStolen+4],0		// Datensegmentregister ist nicht gesetzt, deshalb SS nehmen
	popad
	popfd
	jmp	cs:OldInt.Int2E
}}

static __declspec(naked) void NewSysEnter(void) { _asm{
	pushfd
	pushad
	 call	cyLoadDR
	 lock adc ss:[DebRegStolen+8],0
	popad
	popfd
	jmp	cs:OldInt.SysEnter
}}
#endif
static _declspec(naked) void HandleOutIn(void) { _asm{
// Hilfsroutine für Debugregister-Anzapfung
//  insb/outsb-Unterstützung würde tiefgreifende Erweiterungen erfordern,
//  um die verschiedenen Adressierungsarten (VM, PM16, PM32; Segmentpräfix) zu verarbeiten
// PE:	ECX Bit 22 = USE32-Bit (aus LAR-Befehl)
//	AH=Opcode EC(inb), ED(inw), EE(outb), EF(outw)
//	   Opcode 6C(insb), 6D(insw), 6E(outsb), 6F(outsw) nicht unterstützt!
//	   Opcode E4, E5, E6, E7 nicht unterstützt! (Wäre in AL)
//	AL=Präfix (66)
//	DX=Portadresse
//	ESI=Client-EAX-Zeiger
//	EDI=DevExt-Zeiger
//	Interrupts gesperrt
	mov	cl,2		// USE16
	bt	ecx,22
	jnc	use16
	mov	cl,4		// USE32
//jetzt:CL=4 oder 2 (USE32 oder USE16 je nach Attribut des unterbrochenen Kodesegmentes)
use16: 	cmp	al,0x66		// Präfixbyte? (Schätzung!!)
	jne	no_swap
	xor	cl,6		// aus 4 mach 2 und aus 2 mach 4
no_swap:xchg	ah,al
	cmp	al,0xEF		//OUT dx,ax oder OUT dx,eax
	je	out_cl
	cmp	al,0xEE		//OUT dx,al
	jne	no_outb
	mov	cl,1		//Präfix gilt nicht!
out_cl:	inc	[edi]DEVICE_EXTENSION.ac.out
	sti
	push	ecx
	push	[esi]		//Client_EAX
	push	edx
	push	edi
	call	HandleOut
	jmp	supported	//mit AL/AX/EAX=geschriebenes Byte
no_outb:
	cmp	al,0xED		//IN ax,dx oder IN eax,dx
	je	in_cl
	cmp	al,0xEC		//IN al,dx
	jne	no_inb
	mov	cl,1
in_cl:	inc	[edi]DEVICE_EXTENSION.ac.in
	sti
	push	ecx
	push	esi
	push	edx
	push	edi
	call	HandleIn	//Thread blockieren...
	jmp	supported
no_inb:
	inc	[edi]DEVICE_EXTENSION.ac.fail
#if DBG
 	int 3
#endif
supported:
	cli
	ret
}}

/* Stackaufbau 80386+, Win9x	WinNT
EBP+	Register-9x	EBP+	Register-NT
44	 GS		88		(nur V86)
40	 FS		84		(nur V86)
3C	 DS		80		(nur V86)
38	 ES		7C		(nur V86)
34	 SS		78		(nur Ring3)
30	 ESP		74		(nur Ring3)
2C	 EFlags		70		(Bit17 = V86)
28	 CS		6C		(kein Selektor wenn V86!)
24	 EIP		68
20	 Fehlerkode	64
1C	EAX		60	EBP
18	ECX		5C	EBX
14	EDX		58	ESI
10	EBX		54	EDI
0C	(ESP)		50	FS
08	EBP		4C	Geheimnisvolle Speicherzelle
04	ESI		48	0
00	EDI		44	EAX
			40	ECX
			3C	EDX
			38	DS
			34	ES
			30	GS
EFlags-Aufbau
21 20  19  18 17 16 15 14 13-12 11 10  9  8  7  6 5  4 3  2 1  0
ID VIP VIF AC VM RF  0 NT IOPL  OF DF IF TF SF ZF 0 AF 0 PF 1 CF
*/

static __declspec(naked) void NewInt1(void) { _asm{

#ifdef NTDDK
// NT-Version mit angepassten Registern ohne Stack-Umstapeln:
	push	eax		// Fehlerkode (später)
	push	ebp		// Mit EBP beginnt der Stack bei NT32, nicht mit PUSHAD
	 mov	eax,cs:[UsageBits]
	 mov	ebp,DR6
	 and	eax,0Fh
	 test	eax,ebp
	 jnz	access03
	pop	ebp
	pop	eax
	jmp	cs:OldInt.Int1
access03:
	 not	eax
	 and	eax,ebp		// "Unsere" Bits in DR6 löschen
	 mov	DR6,eax
	 bsf	ebp,ebp		// Merken, welches Debugregister es war (0..3) -> EBP
	 xor	eax,eax
	 xchg	[esp+4],eax	// Fehlerkode Null setzen, EAX restaurieren
	 cld
	 push	ebx		// Restlicher NT-Stack
	 push	esi
	 push	edi
	 push	fs
	 mov	fs,cs:[SaveFS]
	 push	fs:dword ptr[0]
	//mov	fs:dword ptr[0],-1
	 push	0
	 push	eax
	 push	ecx
	 push	edx
	 push	ds		// Bei V86-Unterbrechung sind da nur Nullen drin
	 push	es
	 push	gs
	  mov	di,cs:[SaveDS]
	  sub	esp,0x30
	  mov	ds,di
	  mov	edx,ebp		// Debugregister-Nummer (0..3)
	  mov	ebp,esp
	  test	byte ptr[ebp+0x72],2	// V86?
	  jz	protmode
	  lea	esi,[ebp+0x7C]	// Segmentregister umkopieren
	  lodsd
	  mov	[ebp+0x34],eax
	  lodsd
	  mov	[ebp+0x38],eax
	  lodsd
	  mov	[ebp+0x50],eax
	  lodsd
	  mov	[ebp+0x30],eax
	  mov	es,di
	  mov	ebx,[ebp+0x6C]	// Client_CS (V86-Modus, <64K)
	  shl	ebx,4
	  add	ebx,[ebp+0x68]	// Client_EIP (V86-Modus, <64K)
	  xor	ecx,ecx		// niemals USE32
	  jmp	fromvm
protmode:	// Protected Mode, entweder Ring 0 oder Ring 3
	  les	ebx,[ebp+0x68]	// Client_CS_EIP
	  lar	ecx,[ebp+0x6C]	// Client_CS -> USE32-Bit
fromvm:
	  mov	esi,fs:[0x124]	//TEB
	  add	esi,0x128	// lange Opcodes vermeiden
	  push	dword ptr[esi]	// alten Rahmenzeiger retten
	  mov	[esi],ebp	// neuen Rahmenzeiger setzen
	  mov	al,[ebp+0x6C]	// Client_CS
	  and	al,1
	  mov	[esi+0x134-0x128],al	// Privileg setzen
	  push	esi
//-------------------------
	   mov	ax,es:[ebx-2]	//Opcode (ggf. mit Präfix) - UNSAUBER, erwischt Mehrfachpräfixe sowie REP nicht
	   mov	es,di
	   mov	gs,di
//DevExt-Zeiger beschaffen für Rückruf
	   mov	edi,X4DR[edx*4]
//Richtung des Portzugriffs (Lesen oder Schreiben) ermitteln
	   lea	esi,[ebp+0x44]	// Client_EAX
	   mov	edx,[ebp+0x3C]	// Client_EDX
	   call	HandleOutIn
//-------------------------
	  pop	esi
	  pop	dword ptr[esi]		// Rahmenzeiger wiederherstellen
	  add	esp,0x30
	 test	byte ptr[ebp+0x72],2	// V86?
	 jz	novm
	add	esp,12
	jmp	nopop
novm:	pop	gs
	pop	es
	pop	ds
nopop:	pop	edx
	pop	ecx
	pop	eax
	pop	ebx		// Dummy-Lesen
	pop	fs:dword ptr[0]
	pop	fs
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	add	esp,4		// "Fehlercode" übergehen
	iretd

#else	// Win98/Me

	push	0
 	pushad
	 mov	ecx,cs:[UsageBits]
	 mov	eax,DR6
	 and	ecx,0Fh
	 test	ecx,eax		// Zugriff auf eine "unserer" Adressen?
	 jnz	access03	// ja
	popad			// nein, andere Software ist dran
	lea	esp,[esp+4]	// Fehlerkode wegnehmen, verändert Flags nicht!
	jmp	cs:OldInt.Int1	// weiter (zu SoftICE o.ä.)
access03:			// Hier: Returnadresse wird zur Berechnung der Prozessornummer benötigt!
//bei jedem Portzugriff (LPT) steht in DX die Portadresse
//In CLIENT_EAX steht das Datenbyte (nur bei OUT)
	 not	ecx
	 and	ecx,eax		// "Unsere" Bits in DR6 löschen
	 mov	DR6,ecx
	 bsf	esi,eax		// Merken, welches Debugregister es war (0..3)
/* Kontext-Retten Start */
	 cld
	 mov	eax,cs:[CurThreadPtr]
	 mov	di,cs:[SaveDS]
	 mov	eax,cs:[eax]	// Cur_Thread_Handle
	 mov	ebp,esp		// CRS adressierbar machen (ab EIP -4!)
//Opcode beschaffen; Adreßrechnung je nach V86- oder Protmode
	 test	byte ptr [ebp+0x2E],2	// Client_EFlags:17: V86-Mode?
	 jz	protmode
// V86: Segmentregister sind schon auf dem Stack gerettet
	 mov	es,di		// Lineares Segment
	 mov	ebx,[ebp+0x28]	// Client_CS (V86-Mode, stets <64K)
	 shl	ebx,4
	 add	ebx,[ebp+0x24]	// Client_EIP (V86-Mode, stets <64K)
	 xor	ecx,ecx		// immer USE16
	 jmp	fromvm
protmode:
//Faule Annahme: Zugriff im Protected Mode mit lesbarem Code-Segment
//	mov	ebx,cs:[eax+0x14]	// Momentane Virtuelle Maschine CurVM
//	cmp	ebp,cs:[ebx+8]	// VM_ClientRegs
// Etwas sauberer ist der Test des Kodesegment-Registers auf dem Stack...
	 lar	ecx,[ebp+0x28]	// Client_CS: USE32 und DPL beschaffen
	 test	ch,0x60		// Privileg-Level
	 jz	pm0		// Intra-Ring-Aufruf wenn Null
// PM3: Bei Aufruf aus Ring3 heraus die V86-Segmentregister nachbilden
	 mov	[ebp+0x38],es
	 mov	[ebp+0x3C],ds
	 mov	[ebp+0x40],fs
	 mov	[ebp+0x44],gs	// V86-kompatibel ablegen
	 les	ebx,[ebp+0x24]	// Code-Zeiger (FAR48)
// Win98-typische Stack-Umschaltung bei Ring-Übergang...
fromvm:
	 mov	ds,di
	 add	eax,0x4C
	 xchg	esp,[eax]	// Stack (nicht aber EBP!) umschalten
	 push	eax
	 jmp	saved
pm0:
// PM0: Keine Ring- und Stackumschaltung! Aber trotzdem Register retten!
	 inc	dword ptr [ebp+0x20]	// Fehlerkode als Kennung missbrauchen
	 push	ds
	 push	es
	 push	fs
	 push	gs
	 les	ebx,[ebp+0x24]	// Code-Zeiger (FAR48)
	 mov	ds,di
saved:
	 mov	fs,[SaveFS]
	 mov	ax,es:[ebx-2]		//Opcode (AH, ggf. mit Präfix 66h) - UNSAUBER!!
	 mov	es,di
	 mov	gs,di
/* Kontext-Retten Ende */
//DevExt-Zeiger beschaffen für Rückruf
	 mov	edi,X4DR[esi*4]
//Richtung des Portzugriffs (Lesen oder Schreiben) ermitteln
	 lea	esi,[ebp+0x1C]	// Client_EAX
	 call	HandleOutIn
/* Kontext-Wiederherstellen (Win9x) Start */
	 bt	dword ptr [ebp+0x20],0	//Ohne Ringübergang?
	 jnc	trans
	 pop	gs
	 pop	fs
	 pop	es
	 pop	ds
	 jmp	nopop
trans:
	 pop	eax
	 xchg	esp,[eax]	// Win98-Stack rückschalten
	 test	byte ptr [ebp+0x2E],2	//V86-Mode?
	 jnz	nopop
	 mov	gs,[ebp+0x44]
	 mov	fs,[ebp+0x40]
	 mov	ds,[ebp+0x3C]
	 mov	es,[ebp+0x38]
nopop:
/* Kontext-Wiederherstellen (Win9x) Ende */
	popad
	add	esp,4		// "Fehlercode" übergehen
	iretd
#endif
}}

#else//_X86_

// Übergangsfunktion, wird vom 64-Bit-Assemblerkode (Int1-ISR) aufgerufen
// flags Bit 1:0 = Operandengröße, damit Typ von <data>: PBYTE,PWORD,PDWORD,PQWORD
// flags Bit 2 = Direction-Down-Bit (immer post-inkrement/dekrement)
// flags Bit 4 = IN (sonst OUT, damit <data> = const)
// flags Bit 5 = Nur Fehler zählen
void DispatchHook(char idx/*cl*/, USHORT adr/*dx*/, char flags/*r8*/, void*data/*r9*/, size_t reps/*stack*/) {
 PDEVICE_EXTENSION X=X4DR[idx];
 int opsize=1<<(flags&3);
 if (flags&3) X->ac.wdw++;	// kurioser Befehl für Parallelport
// int incval=opsize;
// if (flags&4) incval=-incval;
 if (flags<16) {	// OUTPUT BYTE/WORD/DWORD/QWORD
  X->ac.out++;
//  for(;reps;--reps} {
   HandleOut(X,adr,*(ULONG*)data,opsize);
//   (BYTE*)data+=incval;
//  }
 }else if (flags<32) {	// INPUT BYTE/WORD/DWORD/QWORD
  X->ac.in++;
//  for(;reps;--reps} {
   HandleIn(X,adr,(ULONG*)data,opsize);
//   (BYTE*)data+=incval;
//  }
 }else{
  X->ac.fail++;
 }
}

#define CASSERT(predicate, file) _impl_CASSERT_LINE(predicate,__LINE__,file)
#define _impl_PASTE(a,b) a##b
#define _impl_CASSERT_LINE(predicate, line, file) typedef char _impl_PASTE(assertion_failed_##file##_,line)[2*!!(predicate)-1];

CASSERT(FIELD_OFFSET(KPCR,IdtBase)==56,vlpt);

#endif//_X86_
