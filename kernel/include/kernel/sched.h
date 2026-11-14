#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdbool.h>
#include <stddef.h>
#include <asm/idt.h>

bool task_create(void (*entry)(void));
/* Runs a flat binary at USER_CODE in its own address space. */
bool process_create(const void* code, size_t size);
__attribute__((__noreturn__))
void task_exit(void);
void syscall_handler(registers_t* regs);
void sched_tick(registers_t* regs);
void sched_selftest(void);

#endif
