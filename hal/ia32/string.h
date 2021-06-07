/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * HAL basic routines (ia32)
 *
 * Copyright 2012, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_BASE_H_
#define _HAL_BASE_H_

#include <lib/types.h>


static inline void hal_memcpy(void *dst, const void *src, unsigned int n)
{
	__asm__ volatile(
		"cld; "
		"movl %0, %%ecx; "
		"movl %%ecx, %%edx; "
		"andl $3, %%edx; "
		"shrl $2, %%ecx; "
		"movl %1, %%edi; "
		"movl %2, %%esi; "
		"rep; movsl; "
		"movl %%edx, %%ecx; "
		"rep; movsb; "
	:
	: "g" (n), "g" (dst), "g" (src)
	: "ecx", "edx", "esi", "edi", "cc", "memory");
}


static inline void hal_memset(void *dst, int v, unsigned int n)
{
	__asm__ volatile(
		"cld; "
		"movl %0, %%ecx; "
		"movl %%ecx, %%edx; "
		"andl $3, %%edx; "
		"shrl $2, %%ecx; "
		"xorl %%eax, %%eax; "
		"movb %1, %%al; "
		"movl %%eax, %%ebx; "
		"shll $8, %%ebx; "
		"orl %%ebx, %%eax; "
		"movl %%eax, %%ebx; "
		"shll $16, %%ebx; "
		"orl %%ebx, %%eax; "
		"movl %2, %%edi; "
		"rep; stosl; "
		"movl %%edx, %%ecx; "
		"rep; stosb; "
	: "+d" (n)
	: "m" (v), "m" (dst)
	: "eax", "ebx", "cc", "ecx", "edi" ,"memory");
}


static inline unsigned int hal_strlen(const char *s)
{
	unsigned int i;

	for (i = 0; *s; s++, i++);

	return i;
}


static inline int hal_strcmp(const char *s1, const char *s2)
{
	unsigned int i;
	const char *p;

	for (p = s1, i = 0; *p; i++, p++) {
		if (*p < *(s2 + i))
			return -1;
		else if (*p > *(s2 + i))
			return 1;
	}

	if (*p != *(s2 + i))
		return -1;

	return 0;
}


static inline int hal_strncmp(const char *s1, const char *s2, unsigned int n)
{
	unsigned int i;

	for (i = 0; (i < n) && *s1 && *s2 && (*s1 == *s2); i++, s1++, s2++);

	if ((i == n) || (!*s1 && !*s2))
		return 0;

	return (*s1 < *s2) ? -(int)i - 1 : i + 1;
}


static inline char *hal_strcpy(char *dst, const char *src)
{
	unsigned int i = 0;

	do {
		dst[i] = src[i];
	} while(src[i++]);

	return dst;
}


static inline char *hal_strncpy(char *dst, const char *src, size_t n)
{
	unsigned int i = 0;

	do {
		dst[i] = src[i];
		i++;
	} while ((i < n) && src[i - 1]);

	return dst;
}


#endif
