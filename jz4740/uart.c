/*
 * uart.c
 *
 * Simple UART console implemented.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <bsp.h>
#include <jz4740.h>

#ifndef u8
#define u8 unsigned char
#endif

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned int
#endif

#define DEBUG_UART_BASE  UART0_BASE

#define DEV_CLK    EXTAL_CLK
#define BAUDRATE   57600

void serial_setbrg (void)
{
	volatile u8 *uart_lcr = (volatile u8 *)(DEBUG_UART_BASE + OFF_LCR);
	volatile u8 *uart_dlhr = (volatile u8 *)(DEBUG_UART_BASE + OFF_DLHR);
	volatile u8 *uart_dllr = (volatile u8 *)(DEBUG_UART_BASE + OFF_DLLR);
	u32 baud_div, tmp;

	baud_div = DEV_CLK / 16 / BAUDRATE;
	tmp = *uart_lcr;
	tmp |= UART_LCR_DLAB;
	*uart_lcr = tmp;

	*uart_dlhr = (baud_div >> 8) & 0xff;
	*uart_dllr = baud_div & 0xff;

	tmp &= ~UART_LCR_DLAB;
	*uart_lcr = tmp;
}

int serial_init (void)
{
	volatile u8 *uart_fcr = (volatile u8 *)(DEBUG_UART_BASE + OFF_FCR);
	volatile u8 *uart_lcr = (volatile u8 *)(DEBUG_UART_BASE + OFF_LCR);
	volatile u8 *uart_ier = (volatile u8 *)(DEBUG_UART_BASE + OFF_IER);
	volatile u8 *uart_sircr = (volatile u8 *)(DEBUG_UART_BASE + OFF_SIRCR);
	__cpm_start_uart0(  );
	/* Disable port interrupts while changing hardware */
	*uart_ier = 0;

	/* Disable UART unit function */
	
	*uart_fcr &= ~UART_FCR_UUE;
	/* Set both receiver and transmitter in UART mode (not SIR) */
	*uart_sircr = ~(SIRCR_RSIRE | SIRCR_TSIRE);

	/* Set databits, stopbits and parity. (8-bit data, 1 stopbit, no parity) */
	*uart_lcr = UART_LCR_WLEN_8 | UART_LCR_STOP_1;

	/* Set baud rate */
	serial_setbrg();

	/* Enable UART unit, enable and clear FIFO */
	*uart_fcr = UART_FCR_UUE | UART_FCR_FE | UART_FCR_TFLS | UART_FCR_RFLS;

	return 0;
}

void serial_putc (const char c)
{
	volatile u8 *uart_lsr = (volatile u8 *)(DEBUG_UART_BASE + OFF_LSR);
	volatile u8 *uart_tdr = (volatile u8 *)(DEBUG_UART_BASE + OFF_TDR);

	if (c == '\n') serial_putc ('\r');

	/* Wait for fifo to shift out some bytes */
	while ( !((*uart_lsr & (UART_LSR_TDRQ)) == UART_LSR_TDRQ) );

	*uart_tdr = (u8)c;
}
void serial_waitfinish()
{
	volatile u8 *uart_lsr = (volatile u8 *)(DEBUG_UART_BASE + OFF_LSR);
    while(!((*uart_lsr) & UART_LSR_TEMT));
	
}


void serial_puts (const char *s)
{
	while (*s)
		serial_putc (*s++);
}

int serial_tstc (void)
{
	volatile u8 *uart_lsr = (volatile u8 *)(DEBUG_UART_BASE + OFF_LSR);

	if (*uart_lsr & UART_LSR_DR)
		/* Data in rfifo */
		return (1);

	return 0;
}

int serial_getc (void)
{
	volatile u8 *uart_rdr = (volatile u8 *)(DEBUG_UART_BASE + OFF_RDR);

	while (!serial_tstc());

	return *uart_rdr;
}

int serial_getc_noblock (unsigned char *c)
{
    volatile u8 *uart_rdr = (volatile u8 *)(DEBUG_UART_BASE + OFF_RDR);

    if(!serial_tstc())
        return 0;

    *c= *uart_rdr;
    return 1;
}

