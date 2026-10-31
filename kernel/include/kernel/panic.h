#ifndef _KERNEL_PANIC_H
#define _KERNEL_PANIC_H

#include <stdbool.h>
#include <kernel/boot.h>

#define panic(message) panic_at(__FILE__, __LINE__, (message))

void panic_init(const struct boot_framebuffer* fb);

__attribute__((__noreturn__))
void panic_at(const char* file, int line, const char* message);

/* Building blocks for the architecture's own reports. */
void panic_begin(void);
__attribute__((__noreturn__))
void panic_end(uint64_t frame);
void panic_put_char(char c);
void panic_put_string(const char* str);
void panic_put_hex(uint64_t value);
void panic_put_decimal(uint64_t value);
void panic_put_register(const char* name, uint64_t value);

/* Provided by the architecture. */
void arch_dump_registers(uint64_t pc);
void arch_stack_trace(uint64_t frame);

/* Timing and sound for the panic screen, which runs with interrupts off. */
void arch_alarm_start(void);
void arch_alarm_sound(bool on);
void arch_alarm_wait(unsigned int ms);

#endif
