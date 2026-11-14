#include <asm/pic.h>

#define IA32_APIC_BASE          0x1B
#define IA32_APIC_BASE_ENABLE   (1 << 11)
#define IA32_APIC_BASE_X2APIC   (1 << 10)

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

// Spurious IRQs get no EOI, except the master's for IRQ 15.
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
    // ICW1: start initialization
    outb(PIC1_COMMAND, 0x11); io_wait();
    outb(PIC2_COMMAND, 0x11); io_wait();

    // ICW2: vector offsets
    outb(PIC1_DATA, offset1); io_wait();
    outb(PIC2_DATA, offset2); io_wait();

    // ICW3: slave on IRQ2, then the slave's cascade identity
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    // ICW4: 8086 mode
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    // Mask all but the cascade.
    outb(PIC1_DATA, 0xFB);
    outb(PIC2_DATA, 0xFF);
}

// Limine may leave the local APIC on, which blocks the PIC.
static void lapic_disable(void) {
    uint32_t low, high;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_APIC_BASE));
    /* Clearing only the enable bit in x2APIC mode faults. */
    low &= ~(IA32_APIC_BASE_ENABLE | IA32_APIC_BASE_X2APIC);
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(IA32_APIC_BASE));
}

void pic_init(void) {
    lapic_disable();
    pic_remap(PIC1_OFFSET, PIC2_OFFSET);
}
