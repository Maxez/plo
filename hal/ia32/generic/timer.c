/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * System timer driver
 *
 * Copyright 2012, 2016, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <hal/timer.h>
#include <lib/errno.h>


struct {
	volatile unsigned int jiffies;
} timer_common;


/* Executes BIOS interrupt calls */
extern void _interrupts_bios(unsigned int irq);


static int timer_isr(unsigned int n, void *arg)
{
	timer_common.jiffies += 62;

	return EOK;
}


static int timer_keystroke(void)
{
	int ret;

	__asm__ volatile(
		"movb $0x1, %%ah; "
		"pushl $0x16; "
		"call _interrupts_bios; "
		"jnz 0f; "
		"xorl %0, %0; "
		"jmp 1f; "
		"0: "
		"movl $0x1, %0; "
		"1: "
		"addl $0x4, %%esp; "
	: "=a" (ret));

	return ret;
}


static int timer_valchg(volatile unsigned char *val, unsigned char *src, unsigned int n)
{
	unsigned int i;

	for (i = 0; (i < n) && (*val == *src); i++, val++, src++);

	return !(i == n);
}


int timer_wait(unsigned int ms, unsigned char flags, volatile unsigned char *val, unsigned char *src, unsigned int n)
{
	unsigned int dt, start = timer_common.jiffies;

	for (;;) {
		if (timer_common.jiffies < start)
			dt = (unsigned int)-1 - start + timer_common.jiffies;
		else
			dt = timer_common.jiffies - start;

		if (ms && (dt >= ms))
			break;

		if (((flags & TIMER_KEYSTROKE) && timer_keystroke()) || ((flags & TIMER_VALCHG) && timer_valchg(val, src, n)))
			return -EINTR;
	}

	return EOK;
}


void hal_timerDone(void)
{
	hal_irquninst(0);
}


void hal_timerInit(void)
{
	hal_irqinst(0, timer_isr, &timer_common);
}
