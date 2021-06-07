/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization (ia32-generic)
 *
 * Copyright 2001, 2005, 2006 Pawel Pisarczyk
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"


.code16
.org 0
.section .init, "ax"


/* Loader high level entry point */
.extern _startc
.type _startc, @function


/* Loader end */
.extern _plo_bss


/* Loader entry point */
.globl _start
_start:
	/* Reload code segment */
	ljmp $0x0, $_plo_start

_plo_start:
	cli

	/* Setup real mode data segments and stack */
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $ADDR_PLO_STACK, %sp

	/* Store boot drive number */
	movb %dl, _plo_bdn

	sti

	/* Extended read drive parameters */
	movw $_plo_geo, %si
	movb $0x48, %ah
	int $0x13
	jnc _plo_load

	/* Fallback to CHS read drive parameters */
	xorw %di, %di
	movb $0x8, %ah
	int $0x13
	jc _plo_end

	/* Store cylinders */
	xorl %eax, %eax
	movb %ch, %al
	movb %cl, %ah
	andb $0xc0, %ah
	rolb $0x2, %ah
	incl %eax
	movl %eax, _plo_geo_cyls

	/* Store heads */
	xorl %eax, %eax
	movb %dh, %al
	incl %eax
	movl %eax, _plo_geo_heads

	/* Store sectors */
	xorl %eax, %eax
	movb %cl, %al
	andb $0x3f, %al
	movl %eax, _plo_geo_secs

	/* Store sector size */
	movw $0x200, _plo_geo_secsz

_plo_load:
	/* Load high level part into memory */
	movw $_plo_bss, %cx
	subw $(ADDR_PLO + 1), %cx
	shrw $0x9, %cx
	movw $((ADDR_PLO + 0x200) >> 4), _plo_lba_seg
	movw $0x0, _plo_lba_offs
	movw $0x1, _plo_lba_sec
	movb $0x2, _plo_chs_sec

_plo_load1:
	/* Extended read sectors from drive */
	movw $_plo_lba, %si
	movb $0x42, %ah
	int $0x13
	jnc _plo_load2

	/* Fallback to CHS read sectors from drive */
	pushw %cx
	movb _plo_chs_cyl, %ch
	movb _plo_chs_head, %dh
	movb _plo_chs_sec, %cl
	movw _plo_lba_seg, %es
	movw _plo_lba_offs, %bx
	movb $0x1, %al
	movb $0x2, %ah
	int $0x13
	popw %cx
	jc _plo_end

_plo_load2:
	/* Increment sector */
	incl _plo_lba_sec
	incb _plo_chs_sec

	/* Check track limit */
	movb _plo_geo_secs, %al
	cmp %al, _plo_chs_sec
	jna _plo_load3

	/* Sector exceeded */
	movb $0x1, _plo_chs_sec
	incb _plo_chs_head
	movl _plo_geo_heads, %eax
	decl %eax
	cmpb %al, _plo_chs_head
	jna _plo_load3

	/* Head exceeded */
	movb $0x0, _plo_chs_head
	incw _plo_chs_cyl

_plo_load3:
	/* Move offset */
	addw $0x200, _plo_lba_offs
	jnc _plo_load4

	/* Increment segment */
	addw $0x1000, _plo_lba_seg
	movw $0x0, _plo_lba_offs

_plo_load4:
	loop _plo_load1

	/* Setup GDT and enter protected mode */
	cli

	/* 32-bit code segment descriptor (selector 0x8) */
	movw $0xffff, (ADDR_GDT + 0x8)
	movw $0x0000, (ADDR_GDT + 0xa)
	movw $0x9a00, (ADDR_GDT + 0xc)
	movw $0x00cf, (ADDR_GDT + 0xe)

	/* 32-bit data segment descriptor (selector 0x10) */
	movw $0xffff, (ADDR_GDT + 0x10)
	movw $0x0000, (ADDR_GDT + 0x12)
	movw $0x9200, (ADDR_GDT + 0x14)
	movw $0x00cf, (ADDR_GDT + 0x16)

	/* 16-bit code segment descriptor (selector 0x18) */
	movw $0xffff, (ADDR_GDT + 0x18)
	movw $0x0000, (ADDR_GDT + 0x1a)
	movw $0x9a00, (ADDR_GDT + 0x1c)
	movw $0x0000, (ADDR_GDT + 0x1e)

	/* GDTR pseudodescriptor */
	movw $(SIZE_GDT - 1), (ADDR_SYSPAGE + 0x0)
	movw $(ADDR_GDT >> 0), (ADDR_SYSPAGE + 0x2)
	movw $(ADDR_GDT >> 16), (ADDR_SYSPAGE + 0x4)

	/* Load GDTR */
	lgdt (ADDR_SYSPAGE + 0x0)

	/* Enable protected mode */
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	ljmp $0x8, $_plo_prot

.code32
_plo_prot:
	/* Reload 32-bit data segments and stack */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	movl $ADDR_PLO_STACK, %esp

	/* Call high level part */
	call _startc

	/* Reboot in protected mode */
	movb $0x6, %al
	movw $0xcf9, %dx
	outb %al, %dx

.code16
_plo_end:
	/* Reboot in real mode */
	ljmp $0xffff, $0x0


/* Boot drive number */
.globl _plo_bdn
_plo_bdn:
	.byte 0


/* Drive geometry packet */
_plo_geo:
/* Packet length */
_plo_geo_len:
	.word 0x1e
/* Flags */
_plo_geo_flags:
	.word 0
/* Cylinders */
_plo_geo_cyls:
	.long 0
/* Heads */
_plo_geo_heads:
	.long 0
/* Sectors per track */
_plo_geo_secs:
	.long 0
/* Total sectors */
_plo_geo_size:
	.quad 0
/* Sector size */
_plo_geo_secsz:
	.word 0
/* EDD pointer */
_plo_geo_edd:
	.long 0


/* Drive LBA packet */
_plo_lba:
/* Packet length */
_plo_lba_len:
	.byte 0x10
/* Unused byte */
	.byte 0
/* Sectors */
_plo_lba_secs:
	.word 0x1
/* Offset */
_plo_lba_offs:
	.word 0
/* Segment */
_plo_lba_seg:
	.word 0
/* Sector (LBA) */
_plo_lba_sec:
	.quad 0


/* Drive CHS address */
_plo_chs:
/* Cylinder */
_plo_chs_cyl:
	.word 0
/* Head */
_plo_chs_head:
	.byte 0
/* Sector */
_plo_chs_sec:
	.byte 0x1


/* Pad boot sector to 446 bytes (end of bootstrap code area) */
.fill 0x1be - (. - _start), 1, 0


/* Partition table */
.fill 64, 1, 0


/* MBR signature */
.word 0xaa55
