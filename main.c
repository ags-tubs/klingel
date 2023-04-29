#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


typedef struct __attribute__((packed)) {
	uint8_t green;
	uint8_t red;
	uint8_t blue;
} color;

extern void display(color *pixels);


static color pixelSpinner[] = {
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x05},
	{0x00, 0x00, 0x25},
	{0x00, 0x00, 0x45},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x05},
	{0x00, 0x00, 0x25},
};

static color off[] = {
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
};

void ring() {
	PORTA.OUTSET = PIN7_bm;
    	TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;

    uint8_t loc = 7;
    for(int i = 0; i < 3*8; i++) {
	    display(pixelSpinner + loc);
	    loc = (loc + 1) % 8;
	    _delay_ms(100);
    }
    display(off);

	PORTA.OUTCLR = PIN7_bm;
    	TCA0.SINGLE.CTRLA = 0;
}

static color dyn[] = {
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
};
static uint8_t cur = 0xa;
static uint8_t dir = 1;
void breathe() {
	cur = cur + dir;
	for(int i = 0; i < 8; i++) {
		uint8_t downscale = cur / 2;
		dyn[i].red = cur;
		dyn[i].green = downscale < 5 ? 5 : downscale;
		dyn[i].blue = 0; // downscale < 5 ? 5 : downscale;
	}
	display(dyn);
	if(cur == 0x30) {
		dir = -1;
	} else if(cur == 0xa) {
		dir = 1;
	}
	for(int i = 0; i < 130-cur; i++) {
		_delay_ms(1);
	}
}

uint8_t ring_timeout = 0;
int main(void){
    PORTA.DIRSET = PIN4_bm | PIN7_bm;
    PORTB.DIRSET = PIN0_bm;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_FRQ_gc;
    TCA0.SINGLE.CMP0L = 22;
    TCA0.SINGLE.CMP0H = 1;

    while(1) {
	    	if(ring_timeout) {
			ring_timeout--;
		} else {
			if(VPORTC.IN & PIN3_bm) {
				ring();
				ring_timeout = 0x60;
			}
		}
		breathe();
	}
}
