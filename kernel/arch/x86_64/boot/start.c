#include <kernel/boot.h>

#include "../cpu/gdt.h"
#include "../cpu/idt.h"

void arch_start(const struct boot_info* info) {
    gdt_init();
    idt_init();

    // Transfer control to the main kernel.
    kernel_main(info);

    // Hang if kernel_main unexpectedly returns.
    arch_halt();
}

void arch_halt(void) {
    asm volatile("cli");
    for (;;)
        asm volatile("hlt");
}
