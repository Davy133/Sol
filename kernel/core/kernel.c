#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include <kernel/serial.h>


void kernel_main(void) {
    terminal_initialize();
    printf("Hello, kernel World!\n");

    serial_init();
    serial_write_string("Hello from serial port!\n");
}
