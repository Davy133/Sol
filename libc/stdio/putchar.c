#include <stdio.h>

#if defined(__is_libk)
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <kernel/log.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
    char c = (char)ic;

    if (!klog_write_char(c)) {
        terminal_putchar(c);

        if (c == '\n')
            serial_write('\r');

        serial_write(c);
    }
#else
    // TODO: Implement stdio and the write system call.
#endif

    return ic;
}