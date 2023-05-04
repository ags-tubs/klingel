/**********************************************************/
/* Optiboot bootloader for Arduino                        */
/*                                                        */
/* http://optiboot.googlecode.com                         */
/*                                                        */
/* Arduino-maintained version : See README.TXT            */
/* http://code.google.com/p/arduino/                      */
/*  It is the intent that changes not relevant to the     */
/*  Arduino production environment get moved from the     */
/*  optiboot project to the arduino project in "lumps."   */
/*                                                        */
/* Heavily optimised bootloader that is faster and        */
/* smaller than the Arduino standard bootloader           */
/*                                                        */
/* Enhancements:                                          */
/*   Fits in 512 bytes, saving 1.5K of code space         */
/*   Higher baud rate speeds up programming               */
/*   Written almost entirely in C                         */
/*   Customisable timeout with accurate timeconstant      */
/*                                                        */
/* What you lose:                                         */
/*   Implements a skeleton STK500 protocol which is       */
/*     missing several features including EEPROM          */
/*     programming and non-page-aligned writes            */
/*   High baud rate breaks compatibility with standard    */
/*     Arduino flash settings                             */
/*                                                        */
/* Copyright 2013-2019 by Bill Westfield.                 */
/* Copyright 2010 by Peter Knight.                        */
/*                                                        */
/* This program is free software; you can redistribute it */
/* and/or modify it under the terms of the GNU General    */
/* Public License as published by the Free Software       */
/* Foundation; either version 2 of the License, or        */
/* (at your option) any later version.                    */
/*                                                        */
/* This program is distributed in the hope that it will   */
/* be useful, but WITHOUT ANY WARRANTY; without even the  */
/* implied warranty of MERCHANTABILITY or FITNESS FOR A   */
/* PARTICULAR PURPOSE.  See the GNU General Public        */
/* License for more details.                              */
/*                                                        */
/* You should have received a copy of the GNU General     */
/* Public License along with this program; if not, write  */
/* to the Free Software Foundation, Inc.,                 */
/* 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */
/*                                                        */
/* Licence can be viewed at                               */
/* http://www.fsf.org/licenses/gpl.txt                    */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* Optional defines:                                      */
/*                                                        */
/**********************************************************/
/*                                                        */
/* BIGBOOT:                                               */
/* Build a 1k bootloader, not 512 bytes. This turns on    */
/* extra functionality.                                   */
/*                                                        */
/* BAUD_RATE:                                             */
/* Set bootloader baud rate.                              */
/*                                                        */
/* LED_START_FLASHES:                                     */
/* Number of LED flashes on bootup.                       */
/*                                                        */
/* LED_DATA_FLASH:                                        */
/* Flash LED when transferring data. For boards without   */
/* TX or RX LEDs, or for people who like blinky lights.   */
/*                                                        */
/* TIMEOUT_MS:                                            */
/* Bootloader timeout period, in milliseconds.            */
/* 500,1000,2000,4000,8000 supported.                     */
/*                                                        */
/* UART:                                                  */
/* UART number (0..n) for devices with more than          */
/* one hardware uart (644P, 1284P, etc)                   */
/*                                                        */
/**********************************************************/

/**********************************************************/
/* Version Numbers!                                       */
/*                                                        */
/* Arduino Optiboot now includes this Version number in   */
/* the source and object code.                            */
/*                                                        */
/* Version 3 was released as zip from the optiboot        */
/*  repository and was distributed with Arduino 0022.     */
/* Version 4 starts with the arduino repository commit    */
/*  that brought the arduino repository up-to-date with   */
/*  the optiboot source tree changes since v3.            */
/*    :                                                   */
/* Version 9 splits off the Mega0/Xtiny support.          */
/*  This is very different from normal AVR because of     */
/*  changed peripherals and unified address space.        */
/*                                                        */
/* It would be good if versions implemented outside the   */
/*  official repository used an out-of-seqeunce version   */
/*  number (like 104.6 if based on based on 4.5) to       */
/*  prevent collisions.  The CUSTOM_VERSION=n option      */
/*  adds n to the high version to facilitate this.        */
/*                                                        */
/**********************************************************/

/**********************************************************/
/* Edit History:                                          */
/*                                                        */
/* Sep 2020                                               */
/* 9.1 fix do_nvmctrl                                     */
/* Aug 2019                                               */
/* 9.0 Refactored for Mega0/Xtiny from optiboot.c         */
/*   :                                                    */
/* 4.1 WestfW: put version number in binary.              */
/**********************************************************/
/* *INDENT-OFF* - astyle hates optiboot                   */
/**********************************************************/

#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

/*
   optiboot uses several "address" variables that are sometimes byte pointers,
   sometimes word pointers. sometimes 16bit quantities, and sometimes built
   up from 8bit input characters.  avr-gcc is not great at optimizing the
   assembly of larger words from bytes, but we can use the usual union to
   do this manually.  Expanding it a little, we can also get rid of casts.
*/
typedef union {
    uint8_t* bptr;
    uint16_t* wptr;
    uint16_t word;
    uint8_t bytes[2];
} addr16_t;

/*
   stk500.h contains the constant definitions for the stk500v1 comm protocol
*/
#include "stk500.h"

/*
   We can never load flash with more than 1 page at a time, so we can save
   some code space on parts with smaller pagesize by using a smaller int.
*/
#if MAPPED_PROGMEM_PAGE_SIZE > 255
typedef uint16_t pagelen_t;
#define GETLENGTH(len)  \
    len = getch() << 8; \
    len |= getch()
#else
typedef uint8_t pagelen_t;
#define GETLENGTH(len)                  \
    (void)getch() /* skip high byte */; \
    len = getch()
#endif

extern void putch(uint8_t ch);
extern uint8_t getch();
extern void setup();
extern void wait();
extern void boot();

void verifySpace()
{
    if (getch() != CRC_EOP) {
        // TODO ERROR Just bootup...
    }
    // wait();
    putch(STK_INSYNC);
}
static void getNch(uint8_t count)
{
    do {
        getch();
    } while (--count);
    verifySpace();
}

/* Function Prototypes
   The main() function is in init9, which removes the interrupt vector table
   we don't need. It is also 'OS_main', which means the compiler does not
   generate any entry or exit code itself (but unlike 'naked', it doesn't
   suppress some compile-time options we want.)
*/

// int main(void) __attribute__((OS_main)) __attribute__((section(".init9"))) __attribute__((used));

int main(void)
{
    uint8_t ch;

    register addr16_t address;
    register pagelen_t length;

    setup();
    uint8_t bootnow = 0;
    /* Forever loop: exits by causing WDT reset */
    while (!bootnow) {
        /* get character from UART */
        ch = getch();

        if (ch == STK_GET_PARAMETER) {
            unsigned char which = getch();
            verifySpace();
            /*
         Send optiboot version as "SW version"
         Note that the references to memory are optimized away.
      */
            if (which == STK_SW_MINOR) {
                putch(1);
            } else if (which == STK_SW_MAJOR) {
                putch(1);
            } else {
                /*
           GET PARAMETER returns a generic 0x03 reply for
           other parameters - enough to keep Avrdude happy
        */
                putch(0x03);
            }
        } else if (ch == STK_SET_DEVICE) {
            // SET DEVICE is ignored
            getNch(20);
        } else if (ch == STK_SET_DEVICE_EXT) {
            // SET DEVICE EXT is ignored
            getNch(4);
        } else if (ch == STK_LOAD_ADDRESS) {
            // LOAD ADDRESS
            address.bytes[0] = getch();
            address.bytes[1] = getch();
            verifySpace();
            // ToDo: will there be mega-0 chips with >128k of RAM?
            /*          UPDI chips apparently have byte-addressable FLASH ?
                  address.word *= 2; // Convert from word address to byte address
      */
        } else if (ch == STK_UNIVERSAL) {
#ifndef RAMPZ
            // UNIVERSAL command is ignored
            getNch(4);
            putch(0x00);
#endif
        }
        /* Write memory, length is big endian and is in bytes */
        else if (ch == STK_PROG_PAGE) {
            // PROGRAM PAGE - any kind of page!
            uint8_t desttype;

            GETLENGTH(length);
            desttype = getch();

            if (desttype == 'F') {
                address.word += MAPPED_PROGMEM_START;
            } else {
                address.word += MAPPED_EEPROM_START;
            }
            // TODO: user row?

            do {
                *(address.bptr++) = getch();
            } while (--length);
            verifySpace();

            /*
         Actually Write the buffer to flash (and wait for it to finish.)
      */
            _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
            while (NVMCTRL.STATUS & (NVMCTRL_FBUSY_bm | NVMCTRL_EEBUSY_bm))
                ; // wait for flash and EEPROM not busy, just in case.
        }
        /* Read memory block mode, length is big endian.  */
        else if (ch == STK_READ_PAGE) {
            uint8_t desttype;
            GETLENGTH(length);

            desttype = getch();
            verifySpace();

            if (desttype == 'F') {
                address.word += MAPPED_PROGMEM_START;
            } else {
                address.word += MAPPED_EEPROM_START;
            }
            // TODO: user row?

            do {
                putch(*(address.bptr++));
            } while (--length);
        }

        /* Get device signature bytes  */
        else if (ch == STK_READ_SIGN) {
            verifySpace();
            // READ SIGN - return what Avrdude wants to hear
            putch(SIGROW_DEVICEID0);
            putch(SIGROW_DEVICEID1);
            putch(SIGROW_DEVICEID2);
        } else if (ch == STK_LEAVE_PROGMODE) {
            verifySpace();
            bootnow = 1;
        } else {
            verifySpace();
        }
        USART0.STATUS |= 1 << 6;
        putch(STK_OK);
        while (!(USART0.STATUS & (1 << 6))) { }
    }
    boot();
}
