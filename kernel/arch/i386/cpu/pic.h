#ifndef PIC_H
#define PIC_H

#include <stdint.h>

#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

#define PIC_EOI         0x20    /* End-of-interrupt command */

#define PIC1_OFFSET     0x20    /* IRQs 0-7  -> IDT 32-39  */
#define PIC2_OFFSET     0x28    /* IRQs 8-15 -> IDT 40-47  */

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pic_remap(int offset1, int offset2);
void pic_send_eoi(uint8_t irq);
void pic_init(void);

#endif
