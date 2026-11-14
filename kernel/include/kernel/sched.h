#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <asm/idt.h>

#define PROCESS_MAX_ARGS    32
#define PROCESS_MAX_FILES   16

/* An initrd file a process opened; data is NULL when the slot is free. */
struct file {
    const char* data;
    size_t size;
    size_t offset;
};

bool task_create(void (*entry)(void));
/* Runs a static ELF executable in its own address space with argv (NULL-ended,
   or NULL for none) on its stack. Returns the process id, or 0 on failure. */
uint64_t process_create(const void* elf, size_t size, const char* const* argv);
/* Runs the initrd file at path, as process_create() does. */
uint64_t process_spawn(const char* path, const char* const* argv);
__attribute__((__noreturn__))
void task_exit(void);
void sched_tick(registers_t* regs);
void sched_selftest(void);
/* Halts the boot task until every other task has exited. */
void sched_wait(void);
/* Whether the task with this id is still running. */
bool sched_task_running(uint64_t id);
uint64_t sched_current_id(void);
/* The current task's open files. */
struct file* sched_files(void);

void syscall_handler(registers_t* regs);

#endif
