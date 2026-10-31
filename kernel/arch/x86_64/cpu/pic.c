#include "pic.h"

#define IA32_APIC_BASE          0x1B
#define IA32_APIC_BASE_ENABLE   (1 << 11)

static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_unmask(uint8_t irq) {
    uint16_t port = PIC1_DATA;

    if (irq >= 8) {
        port = PIC2_DATA;
        irq -= 8;
    }
    outb(port, inb(port) & ~(1 << irq));
}

// A spurious IRQ 7 or 15 has no in-service bit and must not get an EOI from the
// PIC that raised it. For IRQ 15 the master still saw a real cascade interrupt.
int pic_is_spurious(uint8_t irq) {
    if (irq != 7 && irq != 15)
        return 0;

    uint16_t port = irq == 7 ? PIC1_COMMAND : PIC2_COMMAND;
    outb(port, PIC_READ_ISR);
    if (inb(port) & 0x80)
        return 0;

    if (irq == 15)
        outb(PIC1_COMMAND, PIC_EOI);
    return 1;
}

void pic_remap(int offset1, int offset2) {
    // Start initialization (ICW1)
    outb(PIC1_COMMAND, 0x11); io_wait();
    outb(PIC2_COMMAND, 0x11); io_wait();

    // ICW2: Set vector offsets
    outb(PIC1_DATA, offset1); io_wait();
    outb(PIC2_DATA, offset2); io_wait();

    // ICW3: Tell Master PIC about Slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 0x04); io_wait();
    // ICW3: Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 0x02); io_wait();

    // ICW4: 8086 mode
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // Mask every line except the cascade; drivers unmask what they handle.
    outb(PIC1_DATA, 0xFB);
    outb(PIC2_DATA, 0xFF);
}

// The bootloader can leave the local APIC on with LINT0 masked, which blocks the
// PIC. With the APIC off, the PIC drives the CPU's INTR pin directly.
static void lapic_disable(void) {
    uint32_t low, high;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_APIC_BASE));
    low &= ~IA32_APIC_BASE_ENABLE;
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(IA32_APIC_BASE));
}

void pic_init(void) {
    lapic_disable();
    pic_remap(PIC1_OFFSET, PIC2_OFFSET);
}
