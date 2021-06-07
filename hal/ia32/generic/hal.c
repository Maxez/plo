/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer (ia32-generic)
 *
 * Copyright 2012, 2016, 2021 Phoenix Systems
 * Copyright 2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


struct {
	addr_t kernel_entry;
} hal_common;


/* Console */
extern void hal_consoleInit(void);
extern void hal_consoleDone(void);

/* Interrutps */
extern void hal_interruptsInit(void);
extern void hal_interruptsDone(void);

/* Timer */
extern void hal_timerInit(void);
extern void hal_timerDone(void);


void hal_setKernelEntry(addr_t addr)
{
	hal_common.kernel_entry = addr;
}


addr_t hal_vm2phym(addr_t addr)
{
	return addr;
}


const char *hal_cpuInfo(void)
{
	return "IA-32 Generic";
}


void hal_cli(void)
{
	__asm__ volatile("cli":);
}


void hal_sti(void)
{
	__asm__ volatile("sti":);
}


/* Flushes 8042 keyboard controller buffers */
static inline void hal_empty8042(void)
{
	unsigned char status;

	do {
		/* Discard input data */
		if ((status = hal_inb((void *)0x64)) & 0x01) {
			hal_inb((void *)0x60);
			continue;
		}
	} while (status & 0x02);
}


/* Enables A20 line mask */
static void hal_enableA20(void)
{
	hal_empty8042();
	hal_outb((void *)0x64, 0xd1);
	hal_empty8042();
	hal_outb((void *)0x60, 0xdf);
	hal_empty8042();
}


void hal_done(void)
{
	hal_timerDone();
	//hal_interruptsDone();
	//hal_consoleDone();
}


void hal_init(void)
{
	hal_enableA20();
	hal_consoleInit();
	hal_interruptsInit();
	hal_timerInit();
}
