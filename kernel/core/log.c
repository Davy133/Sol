#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <asm/irq.h>
#include <kernel/log.h>
#include <kernel/pit.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

void klog(enum klog_level level, const char* format, ...) {
    static const char* const names[] = { "[DEBUG] ", "[INFO]  ", "[WARN]  ", "[ERROR] " };

    if ((unsigned int)level > KLOG_ERROR)
        return;

    va_list arguments;
    va_start(arguments, format);

    uint64_t flags = irq_save();
    uint64_t ticks = pit_ticks();

    printf("[%lu.%03lu] %s", ticks / PIT_HZ, (ticks % PIT_HZ) * 1000 / PIT_HZ, names[level]);
    vprintf(format, arguments);
    putchar('\n');

    irq_restore(flags);
    va_end(arguments);
}

void klog_write_char(char c) {
    uint64_t flags = irq_save();

    terminal_putchar(c);
    serial_write(c);

    irq_restore(flags);
}
