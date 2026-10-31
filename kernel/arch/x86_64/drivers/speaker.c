#include <kernel/io.h>
#include <kernel/panic.h>

#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43
#define PIT_BASE_HZ     1193182
#define PIT_SPEAKER_WAVE 0xB6   /* Channel 2, low then high byte, mode 3 */

#define SPEAKER_PORT    0x61
#define SPEAKER_GATE    (1 << 0)    /* Runs channel 2 */
#define SPEAKER_DATA    (1 << 1)    /* Connects channel 2 to the speaker */
#define SPEAKER_OUTPUT  (1 << 5)    /* Channel 2's output, read only */

#define BEEP_HZ         880

/* Gives up on a channel that stopped toggling, so a panic can't hang here. */
#define WAIT_LIMIT      1000000

/* The speaker's wave runs even while muted. */
void arch_alarm_start(void)
{
    uint16_t divisor = PIT_BASE_HZ / BEEP_HZ;

    outb(PIT_COMMAND, PIT_SPEAKER_WAVE);
    outb(PIT_CHANNEL2, divisor & 0xFF);
    outb(PIT_CHANNEL2, divisor >> 8);
    outb(SPEAKER_PORT, (inb(SPEAKER_PORT) | SPEAKER_GATE) & ~SPEAKER_DATA);
}

void arch_alarm_sound(bool on)
{
    uint8_t port = inb(SPEAKER_PORT);

    outb(SPEAKER_PORT, on ? port | SPEAKER_DATA : port & ~SPEAKER_DATA);
}

/* Interrupts are off, so time comes from channel 2's own wave, which flips
   twice per cycle. */
void arch_alarm_wait(unsigned int ms)
{
    unsigned int flips = BEEP_HZ * 2 * ms / 1000;
    uint8_t last = inb(SPEAKER_PORT) & SPEAKER_OUTPUT;

    for (unsigned int i = 0; i < flips; i++) {
        int spins = 0;

        while ((inb(SPEAKER_PORT) & SPEAKER_OUTPUT) == last)
            if (++spins == WAIT_LIMIT)
                return;
        last ^= SPEAKER_OUTPUT;
    }
}
