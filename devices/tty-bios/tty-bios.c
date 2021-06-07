/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Terminal emulator (based on BIOS interrupt calls)
 *
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/errno.h>


typedef struct {
	unsigned char sc; /* BIOS scan code */
	unsigned char c;  /* ASCII character */
} ttybios_t;


struct {
	ttybios_t ttys[TTYBIOS_MAX_CNT];
} ttybios_common;


/* Executes BIOS interrupt calls */
extern void _interrupts_bios(unsigned int irq);


/* Returnes terminal instance */
static inline ttybios_t *ttybios_get(unsigned int minor)
{
	if (minor >= TTYBIOS_MAX_CNT)
		return NULL;

	return &ttybios_common.ttys[minor];
}


/* Reads keyboard keystrokes */
static ssize_t ttybios_read(unsigned int minor, addr_t offs, unsigned char *buff, unsigned int len, unsigned int timeout)
{
	ttybios_t *tty;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	// if (tty->c == '\e') {
	// 	*buff = tty->sc;
	// 	tty->c = '\0';
	// }
	// else {
		__asm__ volatile(
			"xorb %%ah, %%ah; "
			"pushl $0x16; "
			"call _interrupts_bios; "
			"movb %%ah, %0; "
			"movb %%al, %1; "
			"addl $0x4, %%esp; "
		: "=m" (tty->sc), "=m" (tty->c));

		*buff = tty->c;
	// }

	return 1;
}


/* Writes character to terminal output */
static ssize_t ttybios_write(unsigned int minor, addr_t offs, const unsigned char *buff, unsigned int len)
{
	ttybios_t *tty;

	if ((tty = ttybios_get(minor)) == NULL)
		return -EINVAL;

	__asm__ volatile(
		"xorb %%bh, %%bh; "
		"movb $0x7, %%bl; "
		"movb $0xe, %%ah; "
		"pushl $0x10; "
		"call _interrupts_bios; "
		"addl $0x4, %%esp; "
	:: "a" (*buff));

	return 1;
}


static int ttybios_sync(unsigned int minor)
{
	return EOK;
}


static int ttybios_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	return dev_isNotMappable;
}


static int ttybios_done(unsigned int minor)
{
	return EOK;
}


static int ttybios_init(unsigned int minor)
{

	// if (minor >= TTYBIOS_MAX_CNT)
	// 	return -EINVAL;
	return EOK;
}


__attribute__((constructor)) static void ttybios_reg(void)
{
	static const dev_handler_t h = {
		.init = ttybios_init,
		.done = ttybios_done,
		.read = ttybios_read,
		.write = ttybios_write,
		.sync = ttybios_sync,
		.map = ttybios_map,
	};

	devs_register(DEV_TTY, 1, &h);
}
