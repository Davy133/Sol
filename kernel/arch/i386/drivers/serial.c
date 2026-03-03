#include <kernel/serial.h>

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

