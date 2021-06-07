/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Memory layout and configuration (ia32-generic)
 *
 * Copyright 2001, 2005, 2006 Pawel Pisarczyk
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__
#include "cpu.h"
#include "peripherals.h"
#include "../string.h"
#endif


/* Platform specific definitions */
#define PAGE_SIZE 0x1000


/* Kernel path */
#define KERNEL_PATH "phoenix-ia32-generic.elf"


/* Memory layout */
#define ADDR_GDT 0x1000
#define SIZE_GDT 0x800

#define ADDR_IDT 0x1800
#define SIZE_IDT 0x800

#define ADDR_SYSPAGE 0x2000
#define SIZE_SYSPAGE 0x1000

#define ADDR_PDIR   0x3000
#define ADDR_PTABLE 0x4000

#define ADDR_STACK 0x6000
#define SIZE_STACK 0x1000

#define ADDR_PLO       0x7c00
#define ADDR_PLO_STACK 0xfff0

#define ADDR_RCACHE 0x10000
#define SIZE_RCACHE 0x4000

#define ADDR_WCACHE 0x14000
#define SIZE_WCACHE 0x4000


#endif
