/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception and interrupt handling (ia32-generic)
 *
 * Copyright 2012-2013, 2016-2017, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>


/* Number of hardware interrupts */
#define SIZE_INTERRUPTS 16


typedef struct {
	unsigned short low;
	unsigned short sel;
	unsigned char res;
	unsigned char type;
	unsigned short high;
} __attribute__((packed)) idt_entry_t;


typedef struct {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed)) idt_t;


typedef struct {
	int (*f)(unsigned int, void *);
	void *data;
} intr_handler_t;


struct {
	intr_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


/* Real mode interrupt vector table */
extern idt_t _interrupts_ivt;


/* Loads IDTR */
extern void _interrupts_loadIDT(idt_t *);


/* Hardware interrupt stubs */
extern void _interrupts_irq0(void);
extern void _interrupts_irq1(void);
extern void _interrupts_irq2(void);
extern void _interrupts_irq3(void);
extern void _interrupts_irq4(void);
extern void _interrupts_irq5(void);
extern void _interrupts_irq6(void);
extern void _interrupts_irq7(void);
extern void _interrupts_irq8(void);
extern void _interrupts_irq9(void);
extern void _interrupts_irq10(void);
extern void _interrupts_irq11(void);
extern void _interrupts_irq12(void);
extern void _interrupts_irq13(void);
extern void _interrupts_irq14(void);
extern void _interrupts_irq15(void);


/* Unhandled interrupts stub */
extern void _interrupts_unexpected(void);


int interrupts_dispatch(unsigned int n)
{
	if ((n >= SIZE_INTERRUPTS) || (interrupts_common.handlers[n].f == NULL))
		return -EINVAL;

	return interrupts_common.handlers[n].f(n, interrupts_common.handlers[n].data);
}


int interrupts_ack(unsigned int n)
{
	if (n >= SIZE_INTERRUPTS)
		return -EINVAL;

	if (n < 8) {
		hal_outb((void *)0x20, 0x60 | n);
	}
	else {
		hal_outb((void *)0x20, 0x62);
		hal_outb((void *)0xa0, 0x60 | (n - 8));
	}

	return EOK;
}


int hal_irqinst(unsigned int n, int (*f)(unsigned int, void *), void *data)
{
	if ((n >= SIZE_INTERRUPTS) || (f == NULL))
		return -EINVAL;

	hal_cli();

	interrupts_common.handlers[n].f = f;
	interrupts_common.handlers[n].data = data;

	hal_sti();

	return EOK;
}


int hal_irquninst(unsigned int n)
{
	if ((n >= SIZE_INTERRUPTS) || (interrupts_common.handlers[n].f == NULL))
		return -EINVAL;

	hal_cli();

	interrupts_common.handlers[n].f = NULL;
	interrupts_common.handlers[n].data = NULL;

	hal_sti();

	return EOK;
}


static int interrupts_setIDTEntry(unsigned int n, void (*base)(void), unsigned short sel, unsigned char flags)
{
	idt_entry_t *entry;

	if (n > 0xff)
		return -EINVAL;

	entry = (idt_entry_t *)ADDR_IDT + n;
	entry->low = (unsigned int)base & 0xffff;
	entry->high = (unsigned int)base >> 16;
	entry->sel = sel;
	entry->type = flags | 0xe0;
	entry->res = 0;

	return EOK;
}


void hal_interruptsDone(void)
{
	return;
}


void hal_interruptsInit(void)
{
	idt_t *idt = (idt_t *)(ADDR_SYSPAGE + 0x8);
	unsigned int i;

	/* Initialize IDT */
	idt->limit = SIZE_IDT - 1;
	idt->base = ADDR_IDT;
	_interrupts_loadIDT(idt);

	/* Initialize 8259A PIC */
	hal_outb((void *)0x20, 0x11);
	hal_outb((void *)0x21, 0x20);
	hal_outb((void *)0x21, 0x04);
	hal_outb((void *)0x21, 0x01);
	hal_outb((void *)0x21, 0xff);

	hal_outb((void *)0xa0, 0x11);
	hal_outb((void *)0xa1, 0x28);
	hal_outb((void *)0xa1, 0x02);
	hal_outb((void *)0xa1, 0x01);
	hal_outb((void *)0xa1, 0xff);

	/* TODO: Set exception stubs */
	for (i = 0; i < 0x20; i++)
		interrupts_setIDTEntry(i, _interrupts_unexpected, 0x8, 0xe);

	/* Set hardware interrupt stubs */
	interrupts_setIDTEntry(0x20 + 0, _interrupts_irq0, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 1, _interrupts_irq1, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 2, _interrupts_irq2, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 3, _interrupts_irq3, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 4, _interrupts_irq4, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 5, _interrupts_irq5, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 6, _interrupts_irq6, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 7, _interrupts_irq7, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 8, _interrupts_irq8, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 9, _interrupts_irq9, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 10, _interrupts_irq10, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 11, _interrupts_irq11, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 12, _interrupts_irq12, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 13, _interrupts_irq13, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 14, _interrupts_irq14, 0x8, 0xe);
	interrupts_setIDTEntry(0x20 + 15, _interrupts_irq15, 0x8, 0xe);

	/* Set unhandled interrupt stubs */
	for (i = SIZE_INTERRUPTS; i < 0xff - 0x20; i++)
		interrupts_setIDTEntry(0x20 + i, _interrupts_unexpected, 0x8, 0xe);
}
