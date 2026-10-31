#ifndef PANIC_H
#define PANIC_H

#include <kernel/panic.h>

#include "idt.h"

__attribute__((__noreturn__))
void panic_exception(const registers_t *regs, const char *name);

#endif
