#ifndef _KERNEL_LOG_H
#define _KERNEL_LOG_H

enum klog_level {
    KLOG_DEBUG = 0,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR
};

void klog(enum klog_level level, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

void klog_write_char(char c);

#endif
