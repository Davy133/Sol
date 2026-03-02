#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include "../arch/i386/cpu/idt.h"

static volatile uint32_t ticks = 0;

static void timer_handler(registers_t *regs) {
    (void)regs;
    ticks++;
    if (ticks % 18 == 0)          /* ~1 segundo */
        printf("tick: %d s\n", ticks / 18);
}

void kernel_main(void) {
    terminal_initialize();
    printf("Hello, kernel World!\n");

    irq_register_handler(0, timer_handler);
    asm volatile("sti");           /* habilita interrupções */

    for (;;)
        asm volatile("hlt");       /* espera a próxima interrupção */
}