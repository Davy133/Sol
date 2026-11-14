#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

#include <kernel/boot.h>

int terminal_initialize(const struct boot_framebuffer* fb);
void terminal_putchar(char c);
void terminal_clear(void);

#endif
