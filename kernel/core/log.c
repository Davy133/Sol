#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <asm/irq.h>
#include <kernel/log.h>
#include <kernel/pit.h>
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

static bool klog_trylock(void) {
    return !__atomic_exchange_n(&log_lock, 1, __ATOMIC_ACQUIRE);
}

static uint64_t klog_lock_irqsave(void) {
    uint64_t flags = irq_save();

    while (!klog_trylock()) {
        while (__atomic_load_n(&log_lock, __ATOMIC_RELAXED))
            asm volatile("pause");
    }

    return flags;
}

static void klog_unlock_irqrestore(uint64_t flags) {
    __atomic_store_n(&log_lock, 0, __ATOMIC_RELEASE);
    irq_restore(flags);
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

static void klog_begin(enum klog_level level) {
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
static void klog_end(void) {
    if (active_entry != NULL) {
        active_entry->message[active_length] = '\0';
        active_entry = NULL;
        active_length = 0;
    }
}

/*
 * Prints the timestamp and severity prefix of a log entry.
 */
static void klog_prefix(enum klog_level level, uint64_t ticks) {
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
    }
}

/*
 * Logs a message that arrived while another klog() call holds the lock.
 *
 * Interrupts are disabled while the lock is held, so on a single CPU this
 * only happens when a fault interrupts klog() and its handler logs again.
 * Waiting would deadlock, so the message goes straight to the output
 * devices instead of the history. With SMP this must check the lock owner.
 */
static void klog_nested(enum klog_level level, const char* format, va_list arguments) {
    struct klog_entry* interrupted = active_entry;

    active_entry = NULL;
    klog_prefix(level, pit_ticks());
    printf("[NESTED] ");
    vprintf(format, arguments);
    putchar('\n');
    active_entry = interrupted;
}

void klog(enum klog_level level, const char* format, ...) {
    if ((unsigned int)level > KLOG_ERROR)
        return;

    va_list arguments;
    va_start(arguments, format);

    uint64_t flags = irq_save();

    if (!klog_trylock()) {
        klog_nested(level, format, arguments);
        irq_restore(flags);
        va_end(arguments);
        return;
    }

    klog_begin(level);
    klog_prefix(level, active_entry->timestamp);
    vprintf(format, arguments);
    putchar('\n');
    klog_end();

    klog_unlock_irqrestore(flags);
    va_end(arguments);
}

/* Interrupts stay off so an interrupt handler that prints cannot re-enter the terminal. */
void klog_write_char(char c) {
    uint64_t flags = irq_save();
    bool to_terminal = true;
    bool to_serial = true;

    if (active_entry != NULL) {
        if (active_length < KLOG_MESSAGE_MAX - 1) {
            active_entry->message[active_length++] = c;
            active_entry->message[active_length] = '\0';
        } else {
            active_entry->truncated = true;
        }

        to_terminal = active_entry->level >= terminal_min_level;
        to_serial = active_entry->level >= serial_min_level;
    }

    if (to_terminal)
        terminal_putchar(c);

    if (to_serial) {
        if (c == '\n')
            serial_write('\r');

        serial_write(c);
    }

    irq_restore(flags);
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
