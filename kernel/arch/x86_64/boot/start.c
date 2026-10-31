#include <asm/gdt.h>
#include <asm/idt.h>
#include <asm/paging.h>
#include <kernel/boot.h>
#include <kernel/panic.h>
#include <kernel/pit.h>

void arch_start(struct boot_info* info) {
    panic_init(&info->framebuffer);
    paging_init(info);
    gdt_init();
    idt_init();
    pit_init();
    asm volatile("sti");

    kernel_main(info);

    for (;;)
        asm volatile("hlt");
}

void arch_halt(void) {
    asm volatile("cli");
    for (;;)
        asm volatile("hlt");
}
