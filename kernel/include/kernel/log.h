#ifndef _KERNEL_LOG_H
#define _KERNEL_LOG_H

#include <stdint.h>
#include <kernel/pit.h>
#include <stdio.h>

enum klog_level {
    KLOG_DEBUG = 0,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR
};


#define KLOG_MIN_LEVEL KLOG_DEBUG
uint64_t klog_lock_irqsave(void);
void klog_unlock_irqrestore(uint64_t flags);

static inline void klog_prefix(enum klog_level level) {
    uint64_t ticks = pit_ticks();

    unsigned long seconds = ticks / PIT_HZ;
    unsigned long milliseconds = (ticks % PIT_HZ) * 1000 / PIT_HZ;

    printf("[%lu.%03lu] ", seconds, milliseconds);

    switch (level) {
        case KLOG_DEBUG:
            printf("[DEBUG] ");
            break;
        case KLOG_INFO:
            printf("[INFO]  ");
            break;
        case KLOG_WARN:
            printf("[WARN]  ");
            break;
        case KLOG_ERROR:
            printf("[ERROR] ");
            break;
        default:
            printf("[UNKNOWN] ");
            break;
    }
}

#define klog(level, ...) do {                       \
    enum klog_level _level = (level);               \
    if (_level >= KLOG_MIN_LEVEL &&                 \
        _level <= KLOG_ERROR) {                     \
        uint64_t _flags = klog_lock_irqsave();      \
        klog_prefix(_level);                        \
        printf(__VA_ARGS__);                        \
        putchar('\n');                              \
        klog_unlock_irqrestore(_flags);             \
    }                                               \
} while (0)

#endif
