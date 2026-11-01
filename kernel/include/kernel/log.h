
#ifndef _KERNEL_LOG_H
#define _KERNEL_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/pit.h>
#include <stdio.h>

enum klog_level {
    KLOG_DEBUG = 0,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR
};

#define KLOG_BUFFER_CAPACITY 128
#define KLOG_MESSAGE_MAX 256

/*
 * A stored log entry.
 *
 * timestamp: PIT ticks when the entry was created.
 * level: original severity level.
 * message: formatted output, including timestamp and severity prefix.
 *          At most KLOG_MESSAGE_MAX - 1 characters are stored.
 * truncated: true if the output exceeded the available message capacity.
 */
struct klog_entry {
    uint64_t timestamp;
    enum klog_level level;
    bool truncated;
    char message[KLOG_MESSAGE_MAX];
};

/*
 * Logger synchronization.
 */
uint64_t klog_lock_irqsave(void);
void klog_unlock_irqrestore(uint64_t flags);

/*
 * Output destination severity configuration.
 */
bool klog_set_terminal_level(enum klog_level level);
bool klog_set_serial_level(enum klog_level level);

enum klog_level klog_get_terminal_level(void);
enum klog_level klog_get_serial_level(void);

/*
 * Active log entry handling.
 */
void klog_begin(enum klog_level level);
void klog_end(void);
bool klog_write_char(char c);

/*
 * Circular history access.
 *
 * Returns the number of entries currently stored.
 */
size_t klog_count(void);

/*
 * Copies an entry into the caller-provided structure.
 * Index 0 represents the oldest stored entry.
 * Returns false if the index is out of range or entry is NULL.
 */
bool klog_read(size_t index, struct klog_entry* entry);

/*
 * Prints the timestamp and severity prefix of a log entry.
 */
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

/*
 * Creates a log entry, formats its output and stores it in history.
 */
#define klog(level, ...) do {                       \
    enum klog_level _level = (level);               \
    if (_level >= KLOG_DEBUG &&                     \
        _level <= KLOG_ERROR) {                     \
        uint64_t _flags = klog_lock_irqsave();      \
        klog_begin(_level);                         \
        klog_prefix(_level);                        \
        printf(__VA_ARGS__);                        \
        putchar('\n');                              \
        klog_end();                                 \
        klog_unlock_irqrestore(_flags);             \
    }                                               \
} while (0)

#endif
