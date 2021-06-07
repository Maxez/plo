/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer driver
 *
 * Copyright 2012, 2020-2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <lib/types.h>

/* Timer flags */
#define TIMER_EXPIRE    (0 << 0)
#define TIMER_KEYSTROKE (1 << 0)
#define TIMER_VALCHG    (1 << 1)


/* Function waits for specific period of time or event */
extern int timer_wait(unsigned int ms, unsigned char flags, volatile unsigned char *val, unsigned char *src, unsigned int n);


#endif
