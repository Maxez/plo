#
# Makefile for plo/ia32-generic
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

CFLAGS += -DVADDR_KERNEL_INIT=$(VADDR_KERNEL_INIT)
LDFLAGS += --defsym _plo_bss=__bss_start

INIT_FLASH := 0x7c00
PLO_COMMANDS := kernel

include devices/tty-bios/Makefile

OBJS += $(addprefix $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/, _startup.o _interrupts.o interrupts.o hal.o timer.o)

ifeq ($(CONSOLE), vga)
	OBJS += $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/console.o
else
	OBJS += $(PREFIX_O)hal/$(TARGET_SUFF)/$(TARGET_SUBFAMILY)/console-serial.o
endif
