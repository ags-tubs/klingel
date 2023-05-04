#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define BAUD 38400
#include <util/setbaud.h>

typedef struct __attribute__((packed)) {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} color;

extern void display(color* pixels);

static color pixelSpinner[] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x05 },
    { 0x00, 0x00, 0x25 },
    { 0x00, 0x00, 0x45 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x05 },
    { 0x00, 0x00, 0x25 },
};

static color off[] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
};

static color on[] = {
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
    { 0xFF, 0xff, 0xff },
};

void ring()
{
    PORTA.OUTSET = PIN7_bm;
    // see bottom of function
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE_FRQ_gc;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;

    uint8_t loc = 7;
    for (int i = 0; i < 3 * 8; i++) {
        display(pixelSpinner + loc);
        loc = (loc + 1) % 8;
        _delay_ms(100);
    }
    display(off);

    PORTA.OUTCLR = PIN7_bm;
    TCA0.SINGLE.CTRLB = 0; // turn PWM override off, so we don't kill the piezo if it stays at 1
    TCA0.SINGLE.CTRLA = 0; // powersaving
}

static color dyn[] = {
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00 },
};
static uint8_t cur = 0xa;
static uint8_t dir = 1;
void breathe()
{
    cur = cur + dir;
    for (int i = 0; i < 8; i++) {
        uint8_t downscale = cur / 2;
        dyn[i].red = cur;
        dyn[i].green = downscale < 5 ? 5 : downscale;
        dyn[i].blue = 0; // downscale < 5 ? 5 : downscale;
    }
    display(dyn);
    if (cur == 0x30) {
        dir = -1;
    } else if (cur == 0xa) {
        dir = 1;
    }
    for (int i = 0; i < 130 - cur; i++) {
        _delay_ms(1);
    }
}

inline void uart_setup()
{
    USART0.CTRLA = USART_DREIE_bm;
    USART0.CTRLB = USART_TXEN_bm;
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_CHSIZE_8BIT_gc;

    USART0.BAUDH = 0;
    USART0.BAUDL = 116;
}

ISR(USART0_DRE_vect)
{
    USART0.TXDATAL = 0x31;
}

uint8_t ring_timeout = 0;
int main(void)
{
    PORTA.DIRSET = PIN4_bm | PIN7_bm;
    PORTB.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm;
    TCA0.SINGLE.CMP0L = 22;
    TCA0.SINGLE.CMP0H = 1;

    uart_setup();
    sei();
    PORTB.OUTSET = PIN1_bm;
    USART0.TXDATAL = 0x31;

    while (1) {
        if (ring_timeout) {
            ring_timeout--;
        } else {
            if (VPORTC.IN & PIN3_bm) {
                ring();
                ring_timeout = 0x60;
            }
        }
        breathe();
    }
}
