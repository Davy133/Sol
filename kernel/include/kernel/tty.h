#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

#include <kernel/boot.h>

int terminal_initialize(const struct boot_framebuffer* fb);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

#endif
