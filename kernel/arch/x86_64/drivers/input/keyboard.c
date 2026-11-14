#include <stdbool.h>
#include <stdint.h>

#include <asm/idt.h>
#include <asm/irq.h>
#include <asm/pic.h>
#include <kernel/io.h>
#include <kernel/keyboard.h>
#include <kernel/log.h>
#include <kernel/pit.h>

#define KEYBOARD_IRQ        1

#define PS2_DATA            0x60
#define PS2_STATUS          0x64

#define PS2_STATUS_OUTPUT   0x01
#define PS2_STATUS_INPUT    0x02

#define PS2_WAIT_LOOPS      100000

#define KBD_CMD_SET_LEDS    0xED
#define KBD_CMD_TYPEMATIC   0xF3
#define KBD_CMD_ENABLE      0xF4

#define KBD_REPLY_ACK       0xFA
#define KBD_REPLY_RESEND    0xFE

#define KBD_MAX_RETRIES     3
#define KBD_REPLY_TICKS     (PIT_HZ / 10)

#define KBD_TYPEMATIC       0x20

#define LED_SCROLL_LOCK     0x01
#define LED_NUM_LOCK        0x02
#define LED_CAPS_LOCK       0x04

#define KBD_QUEUE_SIZE      16

struct kbd_cmd {
    uint8_t cmd;
    int16_t data;
    uint8_t retries;
    bool    cmd_acked;
};

static struct kbd_cmd queue[KBD_QUEUE_SIZE];
static unsigned int queue_head;
static unsigned int queue_tail;
static uint64_t sent_at;

static bool queue_empty(void) {
    return queue_head == queue_tail;
}

static bool kbd_send_byte(uint8_t byte) {
    for (int i = 0; i < PS2_WAIT_LOOPS; i++) {
        if (!(inb(PS2_STATUS) & PS2_STATUS_INPUT)) {
            outb(PS2_DATA, byte);
            return true;
        }
    }

    klog(KLOG_WARN, "keyboard: timed out sending 0x%x", byte);
    return false;
}

/* A command that cannot be sent gets no reply, so it is dropped here. */
static void kbd_send_head(void) {
    while (!queue_empty()) {
        struct kbd_cmd *head = &queue[queue_head];

        if (kbd_send_byte(head->cmd_acked ? (uint8_t)head->data : head->cmd)) {
            sent_at = pit_ticks();
            return;
        }

        queue_head = (queue_head + 1) % KBD_QUEUE_SIZE;
    }
}

static void kbd_next_cmd(void) {
    queue_head = (queue_head + 1) % KBD_QUEUE_SIZE;
    kbd_send_head();
}

static void kbd_queue_cmd(uint8_t cmd, int16_t data) {
    uint64_t flags = irq_save();

    if (!queue_empty() && pit_ticks() - sent_at > KBD_REPLY_TICKS) {
        klog(KLOG_WARN, "keyboard: no reply to 0x%x", queue[queue_head].cmd);
        kbd_next_cmd();
    }

    unsigned int next = (queue_tail + 1) % KBD_QUEUE_SIZE;

    if (next == queue_head) {
        irq_restore(flags);
        klog(KLOG_WARN, "keyboard: command queue full, dropping 0x%x", cmd);
        return;
    }

    bool was_empty = queue_empty();

    queue[queue_tail] = (struct kbd_cmd){ .cmd = cmd, .data = data };
    queue_tail = next;

    if (was_empty)
        kbd_send_head();

    irq_restore(flags);
}

static void handle_reply(uint8_t reply) {
    struct kbd_cmd *head = &queue[queue_head];

    if (reply == KBD_REPLY_RESEND) {
        if (++head->retries < KBD_MAX_RETRIES) {
            kbd_send_head();
            return;
        }

        klog(KLOG_WARN, "keyboard: command 0x%x failed", head->cmd);
        kbd_next_cmd();
        return;
    }

    if (head->data >= 0 && !head->cmd_acked) {
        head->cmd_acked = true;
        head->retries = 0;
        kbd_send_head();
        return;
    }

    kbd_next_cmd();
}

static bool is_reply(uint8_t byte) {
    if (queue_empty())
        return false;

    return byte == KBD_REPLY_ACK || byte == KBD_REPLY_RESEND;
}

#define KBD_BUFFER_SIZE     128

static char char_buffer[KBD_BUFFER_SIZE];
static unsigned int char_head;
static unsigned int char_tail;

static void push_char(char c) {
    unsigned int next = (char_tail + 1) % KBD_BUFFER_SIZE;

    if (next == char_head)
        return;

    char_buffer[char_tail] = c;
    char_tail = next;
}

int keyboard_getchar(void) {
    uint64_t flags = irq_save();
    int c = -1;

    if (char_head != char_tail) {
        c = (unsigned char)char_buffer[char_head];
        char_head = (char_head + 1) % KBD_BUFFER_SIZE;
    }

    irq_restore(flags);
    return c;
}

#define SC_RELEASE          0x80
#define SC_EXTENDED         0xE0
#define SC_PAUSE            0xE1
#define SC_PAUSE_LENGTH     5

#define SC_LCTRL            0x1D
#define SC_LSHIFT           0x2A
#define SC_RSHIFT           0x36
#define SC_CAPS_LOCK        0x3A
#define SC_NUM_LOCK         0x45
#define SC_SCROLL_LOCK      0x46

#define SC_KEYPAD_FIRST     0x47
#define SC_KEYPAD_LAST      0x53

#define SC_E0_KEYPAD_ENTER  0x1C
#define SC_E0_KEYPAD_SLASH  0x35

static const char keymap[128] = {
    [0x01] = 27,
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';',
    [0x28] = '\'', [0x29] = '`', [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x37] = '*', [0x39] = ' ',
};

static const char keymap_shift[128] = {
    [0x01] = 27,
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L', [0x27] = ':',
    [0x28] = '"', [0x29] = '~', [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B',
    [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x37] = '*', [0x39] = ' ',
};

static const char keypad[] = "789-456+1230.";

enum kbd_state {
    KBD_STATE_DEFAULT,
    KBD_STATE_EXTENDED,
    KBD_STATE_PAUSE,
};

static enum kbd_state state = KBD_STATE_DEFAULT;
static unsigned int pause_remaining;

static bool lshift, rshift, lctrl, rctrl;
static bool caps_held, num_held, scroll_held;
static uint8_t leds;

static void toggle_led(uint8_t led) {
    leds ^= led;
    kbd_queue_cmd(KBD_CMD_SET_LEDS, leds);
}

static void handle_lock_key(bool *held, bool released, uint8_t led) {
    if (released) {
        *held = false;
        return;
    }

    if (!*held) {
        *held = true;
        toggle_led(led);
    }
}

static void handle_key(uint8_t code, bool released) {
    bool shift = lshift || rshift;

    switch (code) {
    case SC_LSHIFT:      lshift = !released; return;
    case SC_RSHIFT:      rshift = !released; return;
    case SC_LCTRL:       lctrl = !released;  return;
    case SC_CAPS_LOCK:   handle_lock_key(&caps_held, released, LED_CAPS_LOCK);     return;
    case SC_NUM_LOCK:    handle_lock_key(&num_held, released, LED_NUM_LOCK);       return;
    case SC_SCROLL_LOCK: handle_lock_key(&scroll_held, released, LED_SCROLL_LOCK); return;
    }

    if (released)
        return;

    if (code >= SC_KEYPAD_FIRST && code <= SC_KEYPAD_LAST) {
        char c = keypad[code - SC_KEYPAD_FIRST];

        if (c == '-' || c == '+' || (leds & LED_NUM_LOCK))
            push_char(c);
        return;
    }

    char c = shift ? keymap_shift[code] : keymap[code];

    if (c == 0)
        return;

    if ((leds & LED_CAPS_LOCK) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
        c ^= 0x20;

    if ((lctrl || rctrl) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
        c &= 0x1F;

    push_char(c);
}

static void handle_extended_key(uint8_t code, bool released) {
    if (code == SC_LCTRL) {
        rctrl = !released;
        return;
    }

    if (released)
        return;

    if (code == SC_E0_KEYPAD_ENTER)
        push_char('\n');
    else if (code == SC_E0_KEYPAD_SLASH)
        push_char('/');
}

static void handle_scancode(uint8_t byte) {
    switch (state) {
    case KBD_STATE_PAUSE:
        if (--pause_remaining == 0)
            state = KBD_STATE_DEFAULT;
        return;

    case KBD_STATE_EXTENDED:
        state = KBD_STATE_DEFAULT;
        handle_extended_key(byte & ~SC_RELEASE, byte & SC_RELEASE);
        return;

    case KBD_STATE_DEFAULT:
        if (byte == SC_EXTENDED) {
            state = KBD_STATE_EXTENDED;
            return;
        }

        if (byte == SC_PAUSE) {
            state = KBD_STATE_PAUSE;
            pause_remaining = SC_PAUSE_LENGTH;
            return;
        }

        handle_key(byte & ~SC_RELEASE, byte & SC_RELEASE);
        return;
    }
}

static void keyboard_irq(registers_t *regs) {
    (void)regs;

    uint8_t byte = inb(PS2_DATA);

    if (is_reply(byte))
        handle_reply(byte);
    else
        handle_scancode(byte);
}

void keyboard_init(void) {
    for (int i = 0; i < 16 && (inb(PS2_STATUS) & PS2_STATUS_OUTPUT); i++)
        inb(PS2_DATA);

    irq_register_handler(KEYBOARD_IRQ, keyboard_irq);
    pic_unmask(KEYBOARD_IRQ);

    kbd_queue_cmd(KBD_CMD_TYPEMATIC, KBD_TYPEMATIC);
    kbd_queue_cmd(KBD_CMD_SET_LEDS, leds);
    kbd_queue_cmd(KBD_CMD_ENABLE, -1);

    klog(KLOG_INFO, "PS/2 keyboard initialized");
}
