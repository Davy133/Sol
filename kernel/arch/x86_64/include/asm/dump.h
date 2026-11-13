#ifndef DUMP_H
#define DUMP_H

#include <asm/idt.h>
#include <kernel/panic.h>

__attribute__((__noreturn__))
void panic_exception(const registers_t *regs, const char *name);

#endif
