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
 * Creates a log entry with a timestamp and severity prefix, sends it to the
 * configured destinations and stores it in history. A newline is appended.
 */
void klog(enum klog_level level, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

/*
 * Output destination severity configuration.
 */
bool klog_set_terminal_level(enum klog_level level);
bool klog_set_serial_level(enum klog_level level);

enum klog_level klog_get_terminal_level(void);
enum klog_level klog_get_serial_level(void);

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

/* Used by putchar(): stores the character in the entry klog() is formatting,
   if any, and sends it to the terminal and the serial port. */
void klog_write_char(char c);

#endif
