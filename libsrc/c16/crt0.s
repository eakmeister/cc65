;
; Startup code for cc65 (C16 version)
;
; Note: The C16 is actually the Plus/4 with just 16KB of memory. So many
; things are similar here, and we even use the plus4.inc include file.
;

	.export		_exit
	.export		__STARTUP__ : absolute = 1      ; Mark as startup
	.import		initlib, donelib
	.import		callmain, zerobss
	.import		MEMTOP, RESTOR, BSOUT, CLRCH
	.importzp	ST

	.include	"zeropage.inc"
	.include	"plus4.inc"

; ------------------------------------------------------------------------
; Startup code

.segment	"STARTUP"

Start:

; Save the zero page locations we need

	ldx	#zpspace-1
L1:	lda	sp,x
	sta	zpsave,x
	dex
	bpl	L1

; Switch to second charset

	lda	#14
	jsr	BSOUT

; Clear the BSS data

	jsr	zerobss

; Save system stuff and setup the stack

	tsx
	stx	spsave		; save system stk ptr

	sec
	jsr	MEMTOP		; Get top memory
	cpy	#$80		; We can only use the low 32K :-(
	bcc	MemOk
	ldy	#$80
	ldx	#$00
MemOk:	stx	sp
	sty	sp+1		; set argument stack ptr

; Call module constructors

	jsr	initlib

; Push arguments and call main()

	jsr	callmain

; Call module destructors. This is also the _exit entry.

_exit: 	pha			; Save the return code on stack
	jsr	donelib		; Run module destructors

; Copy back the zero page stuff

	ldx	#zpspace-1
L2:	lda	zpsave,x
	sta	sp,x
	dex
	bpl	L2

; Store the return code into ST

	pla
	sta	ST

; Restore the stack pointer

	ldx	spsave
	txs

; Back to BASIC

	rts

; ------------------------------------------------------------------------

.segment	"ZPSAVE"

zpsave:	.res	zpspace

; ------------------------------------------------------------------------

.bss

spsave:	.res	1
