#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/heap.h>
#include <kernel/initrd.h>
#include <kernel/log.h>
#include <kernel/panic.h>
#include <kernel/sched.h>
#include <kernel/tty.h>


void kernel_main(const struct boot_info* info) {

    klog(KLOG_INFO, "Kernel initialized");

    uint64_t usable = 0;

    for (size_t i = 0; i < info->memory_region_count; i++) {
        if (info->memory_regions[i].type == BOOT_MEMORY_USABLE)
            usable += info->memory_regions[i].length;
    }

    klog(KLOG_DEBUG,
         "Usable memory: %lu MiB, physical memory mapped at 0x%lx",
         usable / (1024 * 1024), info->physmap_offset);

    if (info->initrd.address == NULL)
        panic("the bootloader loaded no initrd");
    initrd_init(info->initrd.address, info->initrd.size);
    klog(KLOG_DEBUG, "initrd: %lu KiB at 0x%lx", info->initrd.size / 1024, (uint64_t)info->initrd.address);

    heap_selftest();
    klog(KLOG_INFO, "Memory self-test passed");

    sched_selftest();
    klog(KLOG_INFO, "Scheduler self-test passed");

    /* The boot log stays on the serial port and in the klog history. */
    klog(KLOG_INFO, "Starting init (bin/sh)");
    terminal_clear();

    if (!process_spawn("bin/sh", (const char *const[]){ "sh", NULL }))
        panic("could not start init (bin/sh)");

    sched_wait();
    panic("Attempted to kill init!");
}
