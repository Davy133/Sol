#ifndef ASM_IRQ_H
#define ASM_IRQ_H

#include <stdint.h>

static inline uint64_t irq_save(void) {
    uint64_t flags;

    asm volatile(
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );

    return flags;
}

static inline void irq_restore(uint64_t flags) {
    if (flags & (1ULL << 9))
        asm volatile("sti" ::: "memory");
}

#endif
