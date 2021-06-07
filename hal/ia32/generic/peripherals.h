/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions (ia32-generic)
 *
 * Copyright 2021 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_


/* UARTs configuration */
#define UART_BAUDRATE 115200

/* Standard PC COMs */
#define UART_COM1 0x3f8
#define UART_COM2 0x2f8
#define UART_COM3 0x3e8
#define UART_COM4 0x2e8

#define UART_COM1_IRQ 4
#define UART_COM2_IRQ 3
#define UART_COM3_IRQ 4
#define UART_COM4_IRQ 3

/* Galileo UARTs */
#define UART_GALILEO1 0x9000f000
#define UART_GALILEO2 0x9000b000


/* PC VGA */
#define VGA_MONO 0xb0000
#define VGA_COLOR 0xb8000


/* TTY BIOS */
#define TTYBIOS_MAX_CNT 1


#endif
