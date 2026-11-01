#ifndef _KERNEL_LOG_H
#define _KERNEL_LOG_H

#include <stdio.h>

enum klog_level {
    KLOG_DEBUG = 0,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR
};


#define KLOG_MIN_LEVEL KLOG_DEBUG

static inline void klog_prefix(enum klog_level level) {
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

#define klog(level, ...) do {                  \
    enum klog_level _level = (level);          \
    if (_level >= KLOG_MIN_LEVEL) {            \
        klog_prefix(_level);                   \
        printf(__VA_ARGS__);                   \
        putchar('\n');                         \
    }                                          \
} while (0)

#endif