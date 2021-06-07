/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt stubs (ia32-generic)
 *
 * Copyright 2012-2013, 2016-2017, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Jan Sikorski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"


.code32
.section .text


/* Interrupt dispatch function */
.extern interrupts_dispatch
.type interrupts_dispatch, @function


/* Interrupt acknowledge function */
.extern interrupts_ack
.type interrupts_ack, @function


/* Interrupt stub macro */
.macro INTERRUPT name, intr, func
.globl \name
.type \name, @function
.align 4, 0x90
\name:
	call _interrupts_init
	pushl $\intr
	call \func
	call interrupts_ack
	addl $0x4, %esp
	jmp _interrupts_done
.endm


_interrupts_init:
	cli

	/* Save context */
	pusha
	pushw %ds
	pushw %es
	pushw %fs
	pushw %gs

	/* Reload segments */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	ret


_interrupts_done:
	/* Restore segments and context */
	popw %gs
	popw %fs
	popw %es
	popw %ds
	popa

	iret


INTERRUPT _interrupts_irq0, 0, interrupts_dispatch
INTERRUPT _interrupts_irq1, 1, interrupts_dispatch
INTERRUPT _interrupts_irq2, 2, interrupts_dispatch
INTERRUPT _interrupts_irq3, 3, interrupts_dispatch
INTERRUPT _interrupts_irq4, 4, interrupts_dispatch
INTERRUPT _interrupts_irq5, 5, interrupts_dispatch
INTERRUPT _interrupts_irq6, 6, interrupts_dispatch
INTERRUPT _interrupts_irq7, 7, interrupts_dispatch
INTERRUPT _interrupts_irq8, 8, interrupts_dispatch
INTERRUPT _interrupts_irq9, 9, interrupts_dispatch
INTERRUPT _interrupts_irq10, 10, interrupts_dispatch
INTERRUPT _interrupts_irq11, 11, interrupts_dispatch
INTERRUPT _interrupts_irq12, 12, interrupts_dispatch
INTERRUPT _interrupts_irq13, 13, interrupts_dispatch
INTERRUPT _interrupts_irq14, 14, interrupts_dispatch
INTERRUPT _interrupts_irq15, 15, interrupts_dispatch
INTERRUPT _interrupts_unexpected, 255, _interrupts_unexpected


/* Loads IDTR */
.globl _interrupts_loadIDT
.type _interrupts_loadIDT, @function
_interrupts_loadIDT:
	movl 4(%esp), %eax
	lidt (%eax)

	ret


/* Executes BIOS interrupt call */
.globl _interrupts_bios
.type _interrupts_bios, @function
_interrupts_bios:
	cli
	pushl %eax

	/* Store BIOS interrupt number */
	movl 8(%esp), %eax
	movb %al, _interrupts_biosirq

	/* Switch to 16-bit protected mode */
	ljmp $0x18, $_interrupts_prot16

.code16
_interrupts_prot16:
	/* Disable protected mode */
	movl %cr0, %eax
	andl $~0x1, %eax
	movl %eax, %cr0
	ljmp $0x0, $_interrupts_real

_interrupts_real:
	/* Restore real mode data segments */
	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	popl %eax

	/* Load real mode interrupt vector table */
	//lidt _interrupts_ivt

	/* int n */
	.byte 0xcd
_interrupts_biosirq:
	.byte 0

	/* Save flags and %eax */
	pushf
	pushl %eax

	/* Restore IDTR */
	//lidt (ADDR_SYSPAGE + 0x8)

	/* Enable protected mode */
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	ljmp $0x8, $_interrupts_prot32

.code32
_interrupts_prot32:
	/* Reload 32-bit data segments */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss

	/* Restore flags and %eax */
	popl %eax
	popf

	sti
	ret


/* Real mode interrupt vector table */
.globl _interrupts_ivt
_interrupts_ivt:
	.word 0xffff
	.long 0
