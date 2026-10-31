#include <stdbool.h>
#include <kernel/panic.h>
#include <kernel/pit.h>
#include <kernel/serial.h>

#include "../drivers/font8x8_basic.h"

#define GLYPH_SIZE      16      /* 8x8 font drawn at twice its size */
#define GLYPH_SCALE     2
#define MARGIN          2       /* In characters */

static struct boot_framebuffer screen;
static volatile uint32_t *pixels;
static size_t pitch;
static size_t columns, rows;
static size_t column, row;
static uint32_t foreground, background;
static bool panicking;

static uint32_t channel(uint8_t value, uint8_t size, uint8_t shift)
{
    uint64_t max = ((uint64_t)1 << size) - 1;
    return (uint32_t)((value * max / 255) << shift);
}

static uint32_t color(uint8_t red, uint8_t green, uint8_t blue)
{
    return channel(red, screen.red_size, screen.red_shift)
         | channel(green, screen.green_size, screen.green_shift)
         | channel(blue, screen.blue_size, screen.blue_shift);
}

void panic_init(const struct boot_framebuffer *fb)
{
    if (fb->address == NULL || fb->bpp != 32)
        return;

    screen = *fb;
    pixels = fb->address;
    pitch = fb->pitch / sizeof(uint32_t);
    columns = fb->width / GLYPH_SIZE;
    rows = fb->height / GLYPH_SIZE;
    foreground = color(0xFF, 0xFF, 0xFF);
    background = color(0x00, 0x00, 0xAA);
}

static void clear_screen(void)
{
    if (pixels == NULL)
        return;

    for (size_t y = 0; y < screen.height; y++)
        for (size_t x = 0; x < screen.width; x++)
            pixels[y * pitch + x] = background;
}

static void draw_char(unsigned char c)
{
    const unsigned char *glyph = font8x8_basic[c < 128 ? c : '?'];
    size_t left = column * GLYPH_SIZE;
    size_t top = row * GLYPH_SIZE;

    for (size_t y = 0; y < GLYPH_SIZE; y++)
    {
        unsigned char bits = glyph[y / GLYPH_SCALE];
        for (size_t x = 0; x < GLYPH_SIZE; x++)
        {
            bool set = bits & (1 << (x / GLYPH_SCALE));
            pixels[(top + y) * pitch + left + x] = set ? foreground : background;
        }
    }
}

void panic_put_char(char c)
{
    if (c == '\n')
        serial_write('\r');
    serial_write(c);

    if (pixels == NULL)
        return;

    if (c != '\n' && row < rows)
        draw_char(c);

    if (c == '\n' || ++column >= columns - MARGIN)
    {
        column = MARGIN;
        row++;
    }
}

void panic_put_string(const char *str)
{
    while (*str)
        panic_put_char(*str++);
}

void panic_put_hex(uint64_t value)
{
    panic_put_string("0x");
    for (int shift = 60; shift >= 0; shift -= 4)
        panic_put_char("0123456789ABCDEF"[(value >> shift) & 0xF]);
}

void panic_put_decimal(uint64_t value)
{
    char digits[20];
    int count = 0;

    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value > 0);

    while (count > 0)
        panic_put_char(digits[--count]);
}

void panic_put_register(const char *name, uint64_t value)
{
    panic_put_string(name);
    panic_put_string(": ");
    panic_put_hex(value);
    panic_put_string("  ");
}

void panic_begin(void)
{
    asm volatile("cli");

    if (panicking)
        arch_halt();
    panicking = true;

    clear_screen();
    column = MARGIN;
    row = 1;

    uint32_t text = foreground;
    foreground = background;
    background = text;
    panic_put_string(" *** KERNEL PANIC *** ");
    background = foreground;
    foreground = text;
    panic_put_string("\n\nSolanum had an unexpected error and had to halt. Debug information follows.\n\n");
}

void panic_end(uint64_t frame)
{
    panic_put_string("\nUptime: ");
    panic_put_decimal(pit_ticks() / PIT_HZ);
    panic_put_string(" s\n");

    arch_stack_trace(frame);
    arch_halt();
}

void panic_at(const char *file, int line, const char *message)
{
    panic_begin();

    panic_put_string("Reason: ");
    panic_put_string(message);
    panic_put_string("\nLocation: ");
    panic_put_string(file);
    panic_put_char(':');
    panic_put_decimal(line);
    panic_put_string("\n\n");

    arch_dump_registers((uint64_t)__builtin_return_address(0));
    panic_end((uint64_t)__builtin_frame_address(0));
}
