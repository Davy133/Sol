#include <kernel/serial.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

int serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    return serial_transmit_empty();
}

int serial_received(void) {
    return inb(SERIAL_PORT + 5) & 1;
}

char serial_read(void) {
    while (serial_received() == 0);
    return inb(SERIAL_PORT);
}

int serial_transmit_empty(void) {
    return inb(SERIAL_PORT + 5) & 0x20;
}

void serial_write(char a) {
    while (serial_transmit_empty() == 0);
    outb(SERIAL_PORT, a);
}

void serial_write_string(const char* str) {
    while (*str) {
        if (*str == '\n')
            serial_write('\r');
        serial_write(*str++);
    }
}

static void serial_write_num(unsigned int val, int base, int is_signed,
                             const char *digits, int width) {
    char buf[32];
    int idx = 31;
    int neg = 0;

    buf[idx] = '\0';

    if (is_signed && (int)val < 0) {
        neg = 1;
        val = (unsigned int)(-(int)val);
    }

    if (val == 0)
        buf[--idx] = '0';
    while (val > 0) {
        buf[--idx] = digits[val % base];
        val /= base;
    }
    if (neg)
        buf[--idx] = '-';

    int numlen = 31 - idx;
    while (numlen < width) {
        buf[--idx] = '0';
        numlen++;
    }

    serial_write_string(&buf[idx]);
}

void serial_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (fmt[0] != '%' || fmt[1] == '%') {
            if (*fmt == '%') fmt++;
            serial_write(*fmt++);
            continue;
        }
        fmt++;

        int width = 0;
        if (*fmt == '0') {
            fmt++;
            while (*fmt >= '0' && *fmt <= '9')
                width = width * 10 + (*fmt++ - '0');
        }

        switch (*fmt++) {
            case 'c': {
                char c = (char)va_arg(args, int);
                serial_write(c);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                serial_write_string(s ? s : "(null)");
                break;
            }
            case 'd':
            case 'i': {
                int v = va_arg(args, int);
                serial_write_num((unsigned int)v, 10, 1,
                                 "0123456789", width);
                break;
            }
            case 'u': {
                unsigned int v = va_arg(args, unsigned int);
                serial_write_num(v, 10, 0, "0123456789", width);
                break;
            }
            case 'x': {
                unsigned int v = va_arg(args, unsigned int);
                serial_write_num(v, 16, 0, "0123456789abcdef", width);
                break;
            }
            case 'X': {
                unsigned int v = va_arg(args, unsigned int);
                serial_write_num(v, 16, 0, "0123456789ABCDEF", width);
                break;
            }
            default:
                serial_write('%');
                fmt--;
                serial_write(*fmt++);
                break;
        }
    }

    va_end(args);
}

