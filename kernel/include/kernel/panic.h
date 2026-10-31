#ifndef _KERNEL_PANIC_H
#define _KERNEL_PANIC_H

#include <kernel/boot.h>

#define panic(message) panic_at(__FILE__, __LINE__, (message))

void panic_init(const struct boot_framebuffer* fb);

__attribute__((__noreturn__))
void panic_at(const char* file, int line, const char* message);

#endif
