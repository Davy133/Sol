#include <kernel/serial.h>

static int serial_available;

int serial_init(void) {
    serial_available = 0;

    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    // Loopback test: no echo means no UART.
    outb(SERIAL_PORT + 4, 0x1E);
    outb(SERIAL_PORT, 0xAE);

    int available = inb(SERIAL_PORT) == 0xAE;

    outb(SERIAL_PORT + 4, 0x0B);

    serial_available = available;
    return serial_available;
}

int serial_transmit_empty(void) {
    return inb(SERIAL_PORT + 5) & 0x20;
}

// shortcut: klog writes with interrupts off, so long lines can lose a PIT tick.
void serial_write(char a) {
    if (!serial_available)
        return;

    if (a == '\n')
        serial_write('\r');

    while (serial_transmit_empty() == 0);
    outb(SERIAL_PORT, a);
}

void serial_write_string(const char* str) {
    while (*str)
        serial_write(*str++);
}
