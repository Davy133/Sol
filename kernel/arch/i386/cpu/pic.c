#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#include <stdint.h>


static inline void io_wait(void) {
    asm volatile("outb %0, $0x80" : : "a"((uint8_t)0));
}


void pic_remap(int offset1, int offset2) {
    // Save masks
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    // Start initialization
    outb(PIC1_COMMAND, 0x11); io_wait();
    outb(PIC2_COMMAND, 0x11); io_wait();

    // Set vector offsets
    outb(PIC1_DATA, offset1); io_wait();
    outb(PIC2_DATA, offset2); io_wait();

    // Tell Master PIC about Slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 0x04); io_wait();
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 0x02); io_wait();

    // Set PICs to operate in 8086 mode
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // Restore saved masks
    outb(PIC1_DATA, a1); io_wait();
    outb(PIC2_DATA, a2); io_wait();
}