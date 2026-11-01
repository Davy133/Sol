#include <stdint.h>
#include <kernel/log.h>

static uint8_t log_lock;

uint64_t klog_lock_irqsave(void) {
    uint64_t flags;

    asm volatile(
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );

    while (__atomic_exchange_n(&log_lock, 1, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&log_lock, __ATOMIC_RELAXED))
            asm volatile("pause");
    }

    return flags;
}

void klog_unlock_irqrestore(uint64_t flags) {
    __atomic_store_n(&log_lock, 0, __ATOMIC_RELEASE);

    if (flags & (1ULL << 9))
        asm volatile("sti" ::: "memory");
}