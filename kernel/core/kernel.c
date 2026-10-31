#include <stdio.h>
#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/tty.h>
#include <kernel/serial.h>


void kernel_main(const struct boot_info* info) {
    terminal_initialize(&info->framebuffer);
    printf("Hello, kernel World!\n");

    serial_init();
    serial_write_string("Hello from serial port!\n");

    uint64_t usable = 0;
    for (size_t i = 0; i < info->memory_region_count; i++) {
        if (info->memory_regions[i].type == BOOT_MEMORY_USABLE)
            usable += info->memory_regions[i].length;
    }
    serial_printf("Usable memory: %lu MiB, physical memory mapped at 0x%lx\n",
                  usable / (1024 * 1024), info->physmap_offset);
}
