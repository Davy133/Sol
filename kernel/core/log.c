
#include <stdint.h>

#include <kernel/log.h>
#include <kernel/serial.h>
#include <kernel/tty.h>

static uint8_t log_lock;

 /*
  * Circular log history.
  *
  * log_start points to the oldest entry.
  * log_count tracks the number of valid entries.
  * When full, new entries overwrite the oldest ones.
  *
  * Access is protected by the logger lock.
  */
static struct klog_entry log_buffer[KLOG_BUFFER_CAPACITY];
static size_t log_start;
static size_t log_count;

static enum klog_level terminal_min_level = KLOG_DEBUG;
static enum klog_level serial_min_level = KLOG_DEBUG;

static struct klog_entry* active_entry;
static size_t active_length;

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

bool klog_set_terminal_level(enum klog_level level) {
    if ((unsigned int)level > KLOG_ERROR)
        return false;

    uint64_t flags = klog_lock_irqsave();
    terminal_min_level = level;
    klog_unlock_irqrestore(flags);

    return true;
}

bool klog_set_serial_level(enum klog_level level) {
    if ((unsigned int)level > KLOG_ERROR)
        return false;

    uint64_t flags = klog_lock_irqsave();
    serial_min_level = level;
    klog_unlock_irqrestore(flags);

    return true;
}

enum klog_level klog_get_terminal_level(void) {
    uint64_t flags = klog_lock_irqsave();
    enum klog_level level = terminal_min_level;
    klog_unlock_irqrestore(flags);

    return level;
}

enum klog_level klog_get_serial_level(void) {
    uint64_t flags = klog_lock_irqsave();
    enum klog_level level = serial_min_level;
    klog_unlock_irqrestore(flags);

    return level;
}

void klog_begin(enum klog_level level) {
    size_t index;

    if (log_count < KLOG_BUFFER_CAPACITY) {
        index = (log_start + log_count) % KLOG_BUFFER_CAPACITY;
        log_count++;
    } else {
        index = log_start;
        log_start = (log_start + 1) % KLOG_BUFFER_CAPACITY;
    }

    active_entry = &log_buffer[index];
    active_entry->timestamp = pit_ticks();
    active_entry->level = level;
    active_entry->truncated = false;
    active_entry->message[0] = '\0';

    active_length = 0;
}

/*
 * Finalizes the active entry.
 */
void klog_end(void) {
    if (active_entry != NULL) {
        active_entry->message[active_length] = '\0';
        active_entry = NULL;
        active_length = 0;
    }
}

/*
 * Captures the formatted output of the active log entry.
 * The complete output is still sent to the configured destinations,
 * even if the stored message reaches its maximum capacity.
 */
bool klog_write_char(char c) {
    if (active_entry == NULL)
        return false;

    if (active_length < KLOG_MESSAGE_MAX - 1) {
        active_entry->message[active_length++] = c;
        active_entry->message[active_length] = '\0';
    } else {
        active_entry->truncated = true;
    }

    if (active_entry->level >= terminal_min_level)
        terminal_putchar(c);

    if (active_entry->level >= serial_min_level) {
        if (c == '\n')
            serial_write('\r');

        serial_write(c);
    }

    return true;
}

/*
 * Returns the number of entries currently stored.
 */
size_t klog_count(void) {
    uint64_t flags = klog_lock_irqsave();
    size_t count = log_count;
    klog_unlock_irqrestore(flags);

    return count;
}

/*
 * Copies an entry into the caller-provided structure.
 * Index 0 represents the oldest stored entry.
 * Returns false if the index is out of range or entry is NULL.
 */
bool klog_read(size_t index, struct klog_entry* entry) {
    if (entry == NULL)
        return false;

    uint64_t flags = klog_lock_irqsave();

    if (index >= log_count) {
        klog_unlock_irqrestore(flags);
        return false;
    }

    size_t position = (log_start + index) % KLOG_BUFFER_CAPACITY;
    *entry = log_buffer[position];

    klog_unlock_irqrestore(flags);
    return true;
}
