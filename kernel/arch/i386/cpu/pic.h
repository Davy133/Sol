#ifndef PIC_H
#define PIC_H

#include <kernel/io.h>

#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

#define PIC_EOI         0x20    /* End-of-interrupt command */

#define PIC1_OFFSET     0x20    /* IRQs 0-7  -> IDT 32-39  */
#define PIC2_OFFSET     0x28    /* IRQs 8-15 -> IDT 40-47  */

void pic_remap(int offset1, int offset2);
void pic_send_eoi(uint8_t irq);
void pic_init(void);

#endif
