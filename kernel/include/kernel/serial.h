#ifndef SERIAL_H
#define SERIAL_H
#define SERIAL_PORT 0x3F8 //COM 1
#include <kernel/io.h>

int serial_init(void);
int serial_received(void);
char serial_read(void);
int serial_transmit_empty(void);
void serial_write(char a);
void serial_write_string(const char* str);

#endif