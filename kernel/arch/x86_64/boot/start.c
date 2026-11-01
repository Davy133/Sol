#include <kernel/boot.h>
#include <kernel/log.h>
#include <kernel/panic.h>
#include <kernel/pit.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

#include "../cpu/gdt.h"
#include "../cpu/idt.h"

void arch_start(const struct boot_info* info) {
    int serial_ok = serial_init();

    if (terminal_initialize(&info->framebuffer))
    klog(KLOG_INFO, "Framebuffer terminal initialized");
    else
    klog(KLOG_ERROR, "Framebuffer terminal initialization failed");

    if (serial_ok)
        klog(KLOG_INFO, "Serial port initialized");
    else
        klog(KLOG_ERROR, "Serial initialization failed; serial output disabled");

    panic_init(&info->framebuffer);
    klog(KLOG_INFO, "Panic handler initialized");

    gdt_init();
    klog(KLOG_INFO, "GDT initialized");

    idt_init();
    klog(KLOG_INFO, "IDT initialized");

    pit_init();

    asm volatile("sti");
    klog(KLOG_INFO, "Interrupts enabled");

    kernel_main(info);

    for (;;)
        asm volatile("hlt");
}

void arch_halt(void) {
    asm volatile("cli");
    for (;;)
        asm volatile("hlt");
}