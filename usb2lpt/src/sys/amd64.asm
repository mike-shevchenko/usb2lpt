; Hilfs-Quelltext für den fehlenden Inline-Assembler bei AMD64-Plattform des C-Compilers.
; Nur Debugregister-Anzapfung - die HAL von AMD64 hat keine Portzugriffsfunktionen.
; Funktioniert natürlich noch nicht! Die ISR muss erst disassembliert werden.
; Und, nicht zu vergessen, der PatchGuard ist auch noch abzuschießen!!
; Bis zu Windows 7 scheint es da brauchbare Lösungen zu geben (2015)
; Da mir jedoch kein einziger Fall eines lohnenswerten
; closed-source-64-Bit-Zugriffstreiber bekannt ist, bleibt's ungelöst.

.data?
Usage_Len	equ	32
extern Usage:word		;Array[32]
; Der Wert 0 bedeutet: freies Debugregister
; Der Wert 1 weist beim Treiber-Start besetzte Debugregister aus
; (diese werden jedoch bei UCB_ForceDebReg trotzdem verwendet)
; Ungerade Zahlen (Bit 0 gesetzt) für temporär ausgeschaltete Portadressen
; Die Indizes 0..3 gelten für DR0..DR3, alle anderen sind Reservepositionen
; Dieses Array kann mit "rep scasw" schnell nach dem passenden Index durchsucht werden.

OldInt1		dq	?
OldInt2E	dq	?	;Wird der noch verwendet?
OldSysCall	dq	?

SaveGS		dw	?	; Zeiger KPCR
		dw	?	; Ausrichtung
extern UsageBits:dword		; 0-Bits für temporär ausgeschaltete Portadressen
extern DebRegStolen:dword	;3 globale Zähler für "geklaute" Debugregister
				; Index 0: Timer
				; Index 1: Int2E
				; Index 2: SysCall
.code
extern DispatchHook:proc	;Parameter: CL=Index (0..3), DX=Portadresse
	;Weiterhin: R8B = Typ, R9 = Zeiger zu Quell/Zieldaten, Stack = Iterationen

;*****************************
;** Debug-Register und Int1 **
;*****************************/
; Auf Mehrprozessormaschinen müssen in _allen_ Prozessoren
; die Debugregister gesetzt/gelesen/geprüft werden.

; IRQL >= DISPATCH_LEVEL
; Der KDPC (1. Argument) wird nicht mehr benötigt.
;void EachProcessorDpc(KDPC*Dpc, PVOID Context, PVOID Arg1, PVOID Arg2)
EachProcessorDpc proc
	push	r9			;Arg2 = Zeiger auf Affinitätsmaske
	 mov	rcx,r8			;Arg1 = Argument
	 call	rdx			;Context = Prozedurzeiger
	pop	rcx
	movzx	eax,byte ptr gs:[184h]	;KeGetCurrentProcessorNumber() für AMD64
	lock btr dword ptr[rcx],eax	;Für diesen Prozessor als erledigt markieren
	ret
EachProcessorDpc endp


; IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
; PE: CL = Interrupt-Nummer
; PA: RAX = Adresse aus IDT (des aktuellen Prozessors)
; VR: EAX,ECX
;void* GetIdtAddr(UCHAR nr)
GetIdtAddr proc private
	movzx	rax,cl
	shl	eax,4
	mov	rcx,gs:[56]		;KPCR.IdtBase
	add	rcx,rax
	mov	eax,[rcx+8]		;High-Teil
	shl	rax,16
	mov	ax,[rcx+6]		;vorletztes WORD
	shl	rax,16
	mov	ax,[rcx+0]		;letztes WORD
	ret
GetIdtAddr endp

; IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
;void SetIdtAddr(UCHAR nr)
; PE: RAX=neue Adresse für INT
;     CL: Interrupt-Nummer
; PA: -
; VR: RCX,RDX,RAX,R8
SetIdtAddr proc private
	movzx	rdx,cl
	mov	rcx,cr0
	shl	edx,4
	mov	r8,rcx
	btr	rcx,16			; Supervisor Mode Write Protect
	mov	cr0,rcx
	mov	rcx,gs:[56]		;KPCR.IdtBase
	add	rcx,rdx
	mov	[rcx+0],ax		;unterstes WORD
	shr	rax,16
	mov	[rcx+6],ax		;nächstes WORD
	shr	rax,16
	mov	[rcx+8],eax		;High-Teil
	mov	cr0,r8
	ret
SetIdtAddr endp

; IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
;void GetInts(void)
GetInts proc
	push	rdi
	 mov	cl,1
	 call	GetIdtAddr
	 lea	rdx,[NewInt1]
	 cmp	rax,rdx			; schon gehookt?
	 je	@f			; nichts tun! (Schleifen vermeiden)
	 lea	rdi,[OldInt1]
	 stosq
	 mov	cl,2Eh
	 call	GetIdtAddr
	 stosq
	 mov	ecx,0C0000082h		; IA32_LSTAR
	 rdmsr
	 stosq
@@:	pop	rdi
	ret
GetInts endp


;void SetSysEnter(void)
;PE: RAX = zu setzende Adresse für SysEnter-Befehl
SetSysEnter proc private
	mov	rcx,0C0000082h		; IA32_LSTAR
	xor	rdx,rdx			; sonst GPF!
	wrmsr
	ret
SetSysEnter endp


; IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
;void HookInts(int unused)
; Faule Annahme: bei INT1 befindet sich bereits ein gültiger Gate-Deskriptor
; VR: RAX,RDX,RCX,R8; EFlags unverändert
; Bei Hyperthreading hat jeder Prozessor eine eigene IDT.
HookInts proc
	pushfq
	 cli
	 cmp	OldInt1,0
	 je	@f
	 lea	rax,[NewInt1]		; ISR-Anfangsadresse
	 mov	cl,1
	 call	SetIdtAddr
;	 lea	rax,[NewInt2E]
;	 mov	cl,2Eh
;	 call	SetIdtAddr
;	 lea	rax,[NewSysCall]
;	 call	SetSysEnter
@@:	popfq
	ret
HookInts endp

; IRQL == DISPATCH_LEVEL! (Prozessor darf nicht wechseln.)
;void UnhookInts(int unused)
; VR: RAX,RDX,RCX,R8; EFlags unverändert
UnhookInts proc
	pushfq
	 cli
	 push	rsi
	  lea	rsi,[OldInt1]
	  lodsq
	  or	rax,rax
	  jz	@f
	  mov	cl,1
	  call	SetIdtAddr
;	  lodsq
;	  mov	cl,2Eh
;	  call	SetIdtAddr
;	  lodsq
;	  call	SetSysEnter
@@:	 pop	rsi
	popfq
	ret
UnhookInts endp

; Nur sinnvoll mit IRQL >= DISPATCH_LEVEL!
;void cyLoadDR(void)
; Debugregister des aktuellen Prozessors laden/wiederherstellen, VR: RAX,RBX,RCX,RDX,RSI,Flags
; Liefert ECX!=0 wenn sich die Debugregister dabei verändern
cyLoadDR proc private
	xor	ecx,ecx		; Bit-Sammler
	lea	rsi,[Usage]	; im Ring 0 immer CLD
	mov	rax,cr4
	bts	eax,3		; PENTIUM Debug Extension (DE) aktivieren
	mov	cr4,rax
	setnc	cl		; CL=1 wenn Debug Extension ausgeschaltet war
	mov	rbx,DR7
	xor	rax,rax		; obere 48 Bits löschen
	lodsw			; 16 Bits laden
	or	ah,ah		; mit (gültiger) Adresse gefüllt?
	jz	@f
	mov	rdx,DR0
	mov	DR0,rax
	xor	edx,eax
	or	ecx,edx		; ECX!=0 wenn DR0 verändert wurde
	or	ebx,000E0202h
	btr	ebx,16		; DR7=xxxxxxxx xxxx1110 xxxxxx1x xxxxxx1x
@@:	lodsw
	or	ah,ah
	jz	@f
	mov	rdx,DR1
	mov	DR1,rax
	xor	edx,eax
	or	ecx,edx		; ECX!=0 wenn DR1 verändert wurde
	or	ebx,00E00208h
	btr	ebx,20		; DR7=xxxxxxxx 1110xxxx xxxxxx1x xxxx1xxx
@@:	lodsw
	or	ah,ah
	jz	@f
	mov	rdx,DR2
	mov	DR2,rax
	xor	edx,eax
	or	ecx,edx		; ECX!=0 wenn DR2 verändert wurde
	or	ebx,0E000220h
	btr	ebx,24		; DR7=xxxx1110 xxxxxxxx xxxxxx1x xx1xxxxx
@@:	lodsw
	or	ah,ah
	jz	@f
	mov	rdx,DR3
	mov	DR3,rax
	xor	edx,eax
	or	ecx,edx		; ECX!=0 wenn DR3 verändert wurde
	or	ebx,0E0000280h
	btr	ebx,28		; DR7=1110xxxx xxxxxxxx xxxxxx1x 1xxxxxxx
@@:	mov	rdx,DR7
	mov	DR7,rbx
	xor	edx,ebx
	or	ecx,edx		; ECX!=0 wenn DR7 verändert wurde
	ret
cyLoadDR endp

; Nur sinnvoll mit IRQL >= DISPATCH_LEVEL!
;BOOLEAN LoadDR(int unused)
LoadDR proc
	push	rsi
	push	rbx
	 call	cyLoadDR
	pop	rbx
	pop	rsi
	add	ecx,-1		; Returnwert nach CY (gesetzt wenn ECX!=0)
	setc	al		; für sog. Hochsprachen...
	ret
LoadDR endp


;void PrepareDR(void)
; PA: [OldInt.Int1]=Adresse von INT1
; Weiterhin SaveGS
iPrepareDR proc
	call	GetInts
	mov	[SaveGS],gs
; markiert die bereits vor dem Treiber-Start verwendeten Debugregister
; als "nicht verwendungsfähig für uns".  Aufzurufen beim Treiber-Start
	lea	rdx,[Usage]
	mov	rax,DR7
	push	4
	pop	rcx
@@:	test	al,3		; Lx oder Gx gesetzt? (Also in Benutzung?)
	setnz	byte ptr[rdx]	; wenn ja, auf 1 setzen, sonst 0 lassen
	shr	eax,2		; nächstes Debugregister
	add	rdx,2		; nächstes Usage-Word
	loop	@b
; KeInitializeTimer(&debset.tmr);
; KeInitializeDpc(&debset.dpc,SetDebDpc,NULL);
iPrepareDR endp

;********************************
;* API (öffentliche Funktionen) *
;********************************

; Scans the USHORT array for given USHORT and returns a pointer to entry; returns NULL if not found
;USHORT* ScanMemW(SIZE_T l /*RCX*/, const USHORT*p /*RDX*/, USHORT a /*R8*/)
ScanMemW proc
	xchg	rdx,rdi
	mov	eax,r8d
	repne	scasw
	lea	rax,[rdi-2]
	je	@f
	xor	rax,rax
@@:	xchg	rdi,rdx
	ret
ScanMemW endp

; Diese Routine wird mit ECX = Debugregister-Nummer (0..3) aufgerufen
; Nur sinnvoll mit IRQL >= DISPATCH_LEVEL!
;void UnloadDR(UCHAR debregnumber)
UnloadDR proc
	mov	rdx,DR7
	mov	eax,0FFFCFFF0h	; Maske für DR7, HiWord/LoWord vertauscht
	shl	cl,1		; Nr. mal zwei
	shl	eax,cl		; HiWord (künftiges LoWord) richtig
	shl	ax,cl		; LoWord (künftiges HiWord) richtig
	ror	eax,16		; Vertauschen HiWord/LoWord
	and	edx,eax		; Bits weg, Debugregister frei
	mov	DR7,rdx
	xor	edx,edx		; High-Teil sollte Null sein
	shr	cl,1		; wieder zurück
	jnz	@f
	mov	DR0,rdx		; hübsch machen (nicht erforderlich, aber macht das Debuggen übersichtlicher)
@@:	loop	@f
	mov	DR1,rdx
@@:	loop	@f
	mov	DR2,rdx
@@:	loop	@f
	mov	DR3,rdx
@@:	ret
UnloadDR endp


;************************************************************
;** Abfangen von READ_PORT_UCHAR und WRITE_PORT_UCHAR (NT) **
;************************************************************
; Gibt's bei AMD64 gar nicht!!

;***************************
;** Portzugriffe und Trap **
;***************************

NewInt2E proc private
	pushfq
	push	rax
	push	rcx
	push	rdx
	push	rbx
	push	rsi
	 call	cyLoadDR
	 jnc	@f
	 lock inc [DebRegStolen+4]
@@:	pop	rsi
	pop	rbx
	pop	rdx
	pop	rcx
	pop	rax
	popfq
	jmp	OldInt2E
NewInt2E endp

NewSysCall proc private
	pushfq
	push	rax
	push	rcx
	push	rdx
	push	rbx
	push	rsi
	 call	cyLoadDR
	 jnc	@f
	 lock inc [DebRegStolen+8]
@@:	pop	rsi
	pop	rbx
	pop	rdx
	pop	rcx
	pop	rax
	popfq
	jmp	OldSysCall
NewSysCall endp

; Stackaufbau 	Vista64
;EBP+	Register-NT
;Der genaue Stackaufbau ist anscheinend bei Win64 nicht wichtig;
;es gibt keinen Bezug zu einer gepushten Struktur im KPCS, TEB o.ä. Windows-Strukturen.

;EFlags-Aufbau
;21 20  19  18 17 16 15 14 13-12 11 10  9  8  7  6 5  4 3  2 1  0
;ID VIP VIF AC VM RF  0 NT IOPL  OF DF IF TF SF ZF 0 AF 0 PF 1 CF

NewInt1 proc private
	push	rax
	push	rbp
	 mov	rbp,DR6
	 mov	eax,[UsageBits]
	 and	eax,0Fh		;nur die unteren 4 Bits repräsentieren Debugregister
	 and	eax,ebp
	 jnz	@f		;wir sind dran
	pop	rbp
	pop	rax
	jmp	OldInt1
;BUGBUG: Diese ISR funktioniert nicht für User-Mode-Traps! (swapgs fehlt)
;Ergibt ein Sicherheitsloch, wenn Portzugriffe möglich sein sollten.
;Dies würde jedoch wiederum ein verändertes TSS mit IOPM erfordern, vielleicht.
@@:	not	eax
	and	rax,rbp		; "Unsere" Bits in DR6 löschen
	mov	DR6,rax
	bsf	eax,ebp		; Merken, welches Debugregister es war (0..3) -> EAX
	xchg	[rsp+8],rax	; RAX restaurieren
	sub	rsp,0F0h
	push	r11
	push	r10
	push	r9
	push	r8
	push	rdx
	push	rcx
	push	rax
	lea	rbp,[rsp+50h]	;mir sin ma a bissel Kode sparen
	sub	rsp,28h		;Der Stack muss eine gewisse Ausrichtung aufweisen!
	mov	byte ptr[rbp-55h],1
	cld
	stmxcsr [rbp-54h]		;(Wird das Gleitkomma-Geraffel benötigt??)
	ldmxcsr gs:[180h]
	movdqa	[rbp-10h],xmm0
	movdqa	[rbp],xmm1
	movdqa	[rbp+10h],xmm2
	movdqa	[rbp+20h],xmm3
	movdqa	[rbp+30h],xmm4
	movdqa	[rbp+40h],xmm5
	sti
	mov	cl,[rbp+0E0h]		; Debugregister-Nummer (0..3), EDX stimmt schon/noch!
	mov	rax,[rbp+0E8h]		; RIP
	mov	ax,[rax-2]	;Opcode mit 1 Präfix - UNSAUBER, erwischt Mehrfachpräfixe sowie REP nicht
	cmp	al,066h
	setne	al		;1 wenn ohne Präfix
	xchg	ah,al
	cmp	al,0EEh		;OUT dx,al
	jz	io8
	cmp	al,0EFh		;OUT dx,eax oder OUT dx,ax
	jz	io16
	or	ah,16
	cmp	al,0ECh		;IN al,dx
	jz	io8
	cmp	al,0EDh		;IN eax,dx oder IN ax,dx
	jz	io16
	mov	ah,32		;unsupported!
	jmp	a2
io16:	add	ah,1		;1 (9) oder 2 (10)
	jmp	a2
io8:	and	ah,not 1	;Präfixtestergebnis töten
a2:	xchg	ah,al
	mov	r8b,al
	lea	r9,[rbp-50h]	;4. Parameter zeigt auf Client_RAX
	push	1		;5. Parameter stets 1
	call	DispatchHook
	ldmxcsr	[rbp-54h]
	movdqa	xmm0,[rbp-10h]
	movdqa	xmm1,[rbp]
	movdqa	xmm2,[rbp+10h]
	movdqa	xmm3,[rbp+20h]
	movdqa	xmm4,[rbp+30h]
	movdqa	xmm5,[rbp+40h]
	add	rsp,30h		;zusätzlich Stack von DispatchHook() aufräumen
	pop	rax
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	pop	r10
	pop	r11
	add	rsp,0F0h
	pop	rbp
	add	rsp,8		; "Fehlercode" übergehen
	iretq

NewInt1 endp

end
