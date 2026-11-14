#ifndef _KERNEL_LOG_H
#define _KERNEL_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum klog_level {
    KLOG_DEBUG = 0,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR
};

#define KLOG_BUFFER_CAPACITY 128
#define KLOG_MESSAGE_MAX 256

/* message holds the formatted line, prefix included; truncated is set when it
   did not fit. timestamp is in PIT ticks. */
struct klog_entry {
    uint64_t timestamp;
    enum klog_level level;
    bool truncated;
    char message[KLOG_MESSAGE_MAX];
};

/* Prefixes the message with a timestamp and level, appends a newline and keeps
   it in the history. */
void klog(enum klog_level level, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

size_t klog_count(void);

/* Index 0 is the oldest entry. False when index is out of range or entry is NULL. */
bool klog_read(size_t index, struct klog_entry* entry);

/* Used by putchar(). */
void klog_write_char(char c);

#endif
