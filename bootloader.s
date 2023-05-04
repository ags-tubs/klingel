.section .text
.global setup
.global putch
.global getch
.global wait
.global boot

setup:
	; PA4: LED Output, PA5 Universal input (inverted), PA7 COIL output
	ldi r18, 0b10010000
	out 0x00, r18
	; invert PA5 logic
	ldi r18, 0b10000000
	sts 0x415, r18
	; PB0: Piezo Output (disabled for security...), PB1: RS485send, PB2 RS485 TxD, PB3 RS485 RxD
	ldi r18, 0b00000110
	out 0x04, r18
	; PC3: Touch input

	; disable all unnecessary inputs
	ldi r18, 0x4
	ldi r27, 0x4
	; PORTA
	ldi r26, 0x10
	st X+, r18
	st X+, r18
	st X+, r18
	st X+, r18
	st X+, r18
	inc r26 ; PA5
	st X+, r18
	st X+, r18
	; PORTB
	ldi r26, 0x30
	st X+, r18
	st X+, r18
	st X+, r18
	inc r26 ; PB3
	st X+, r18
	st X+, r18
	st X+, r18
	st X+, r18
	; PORTC
	ldi r26, 0x50
	st X+, r18
	st X+, r18
	st X+, r18
	inc r26 ; PC3
	st X+, r18
	st X+, r18
	st X+, r18
	st X+, r18

	; enable 115200 UART, yay
	ldi r18, 0b11000000
	sts 0x806, r18
	ldi r18, 116
	sts 0x808, r18
	ret
	
getch:
	cbi 0x5, 1 ; enable input
	; wait for data
	lds r18, 0x804
	bst r18, 7
	brtc getch
	; recv data
	lds r24, 0x800
	lds r25, 0x801
	ret

putch:
	sbi 0x5, 1 ; enable output
	; wait for free data
	lds r18, 0x804
	bst r18, 5
	brtc putch
	; send data
	sts 0x802, r24
	ret

wait:
	clr r18
waitloop:
	inc r18
	brne waitloop
	ret

boot:
	jmp 0x8400
