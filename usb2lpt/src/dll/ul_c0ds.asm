	PAGE	250,200	; (fast) keine lästigen Seiten im Listing
;Startup-Datei für Eigenschaftsseiten-Lieferant 16bit
;untersdrückt das Einbinden der C-Standardbibliothek
;und liefert die Funktion _pascal ss_strtoul(char _ss*, char _ss* _ss*, int)
;Für MASM 5.10 (im „Windows Server 2003 DDK“ enthalten unter bin/bin16/)
;Für MASM 6.11d (im „Windows 98 DDK“ enthalten als bin/bin16/ml.exe)

EXTRN pascal LocalInit :far

	.MODEL	small,pascal
	
	.DATA
	db	16 dup (0)	; Die ersten 16 Bytes eines LocalHeap frei!
	
	.CODE

;Eintrittspunkt: hier keine DLL-Initialisierung
.386	;kein "push 0"-Umweg bitte!
LibEntry proc  far
	push	ds
	push	0
	push	cx
	call	LocalInit
	ret
LibEntry endp

;Windows-Exit-Prozedur (Einsprungpunkt anbieten)
WEP proc far
	ret	2
WEP endp

;Zeichenkette in Zahl umwandeln, alle Zeiger near SS-basiert
;Ohne Übergehung führender Leerzeichen und Tabs
.8086
ss_strtoul proc uses si, s:WORD, endptr:WORD, base:WORD 
	mov	si,s		; Erste Anweisung in 8086-Modus generiert
.386				; kürzeres "push bp; mov bp,sp..."
	movzx	ebx,base
        xor	edx,edx		; 32-bit-Akku
        cld
@@l:
	db	036h		;segss
	lodsb			; Ziffer->AL laden
	sub	al,'0'
	jc	@f		; keine Ziffer
	cmp	al,10
	jc	@f		; Ziffer OK: '0'..'9' -> 0..9
	sub	al,11h		; 'A'..'Z' -> 0..25
	and	al,not 20h	; zum Großbuchstaben
	add	al,10		; (Routine gemopst von CAOS NT)
@@:	cmp	al,bl		; Größer-gleich Zahlenbasis?
	jnc	@f		; ungültiges Zeichen
        movzx	ecx,al		; Ziffer retten
        xchg	eax,edx
	mul	ebx		; Akku*=Radix
	or	edx,edx
	jnz	@f		; Zahl zu groß
	add	eax,ecx
	jc	@f		; Zahl zu groß
	xchg	edx,eax		; zurück als EDX = Akku
	jmp	@@l
@@:	mov	bx,endptr
	or	bx,bx
	jz	@f
	dec	si
	mov	ss:[bx],si	; Adresse falsches Zeichen ablegen
@@:	push	edx
	pop	ax
	pop	dx
.8086
	ret			; generiert "pop si; pop bp; retn 6"
ss_strtoul endp

;Speicher füllen
RtlFillMemory proc uses di, buf:DWORD, len:WORD, fill:WORD
	les	di,[buf]
	cld
	mov	ax,[fill]
	mov	cx,[len]
	rep stosb
	ret
RtlFillMemory endp

;Speicher kopieren, Überlappung beachten
RtlMoveMemory proc uses ds si di, d:DWORD, s:DWORD, l:WORD
	lds	si,[s]
	les	di,[d]
	mov	cx,[l]
	cld
.386
	mov	eax,[s]
	cmp	eax,[d]
.8086
	jnc	@f		; Ziel < Quelle
	add	si,cx
	add	di,cx
	dec	si
	dec	di
	std
@@:	rep movsb
	cld
	ret
RtlMoveMemory endp

end LibEntry
