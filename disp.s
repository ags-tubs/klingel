.section .text
.global display

display:
	; prevent interrupts from creating strange colors
	cli
	; mov arg to x register for iteration...
	movw r26, r24
	.rept 3*8
	ld r0, X+
	.rept 8
	rol r0
	brcc 0f
	sbi 0x1, 4
	nop
	cbi 0x1, 4
	rjmp 1f
0:
	sbi 0x1, 4
	cbi 0x1, 4
1:
	.endr
	.endr
	sei
	ret
