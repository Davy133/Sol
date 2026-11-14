#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <asm/idt.h>

#define PROCESS_MAX_ARGS    32
#define PROCESS_MAX_FILES   16

/* data is NULL when the slot is free. */
struct file {
    const char* data;
    size_t size;
    size_t offset;
};

bool task_create(void (*entry)(void));
/* argv is NULL-ended, or NULL. Returns the process id, or 0. */
uint64_t process_create(const void* elf, size_t size, const char* const* argv);
uint64_t process_spawn(const char* path, const char* const* argv);
__attribute__((__noreturn__))
void task_exit(void);
void sched_tick(registers_t* regs);
void sched_selftest(void);
void sched_wait(void);
bool sched_task_running(uint64_t id);
uint64_t sched_current_id(void);
struct file* sched_files(void);

void syscall_handler(registers_t* regs);

#endif
