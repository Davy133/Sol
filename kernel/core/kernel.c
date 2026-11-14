#include <stdint.h>
#include <kernel/boot.h>
#include <kernel/heap.h>
#include <kernel/log.h>
#include <kernel/panic.h>
#include <kernel/sched.h>


extern const char user_shell_start[], user_shell_end[];

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

    heap_selftest();
    klog(KLOG_INFO, "Memory self-test passed");

    sched_selftest();
    klog(KLOG_INFO, "Scheduler self-test passed");

    if (!process_create(user_shell_start, (uintptr_t)user_shell_end - (uintptr_t)user_shell_start))
        panic("could not start init");
    klog(KLOG_INFO, "Started init (the shell)");

    sched_wait();
    panic("Attempted to kill init!");
}
