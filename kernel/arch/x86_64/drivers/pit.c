#include <kernel/io.h>
#include <kernel/pit.h>

#include "../cpu/idt.h"
#include "../cpu/pic.h"

#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43
#define PIT_BASE_HZ     1193182
#define PIT_SQUARE_WAVE 0x36    /* Channel 0, low then high byte, mode 3 */

#define PIT_IRQ 0

static volatile uint64_t ticks;

static void pit_tick(registers_t *regs) {
    (void)regs;
    ticks++;
}

void pit_init(void) {
    uint16_t divisor = PIT_BASE_HZ / PIT_HZ;

    outb(PIT_COMMAND, PIT_SQUARE_WAVE);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, divisor >> 8);

    irq_register_handler(PIT_IRQ, pit_tick);
    pic_unmask(PIT_IRQ);
}

uint64_t pit_ticks(void) {
    return ticks;
}
