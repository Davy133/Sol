#include "panic.h"
#include <stdbool.h>
#include <kernel/pit.h>
#include <kernel/serial.h>

#include "../drivers/font8x8_basic.h"

#define GLYPH_SIZE      16      /* 8x8 font drawn at twice its size */
#define GLYPH_SCALE     2
#define MARGIN          2       /* In characters */

#define VECTOR_PAGE_FAULT   14
#define PF_PRESENT          (1 << 0)
#define PF_WRITE            (1 << 1)
#define PF_USER             (1 << 2)
#define PF_FETCH            (1 << 4)

#define TRACE_DEPTH         10
#define HIGHER_HALF_START   0xFFFF800000000000

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

static void put_char(char c)
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

static void put_string(const char *str)
{
    while (*str)
        put_char(*str++);
}

static void put_hex(uint64_t value)
{
    put_string("0x");
    for (int shift = 60; shift >= 0; shift -= 4)
        put_char("0123456789ABCDEF"[(value >> shift) & 0xF]);
}

static void put_decimal(uint64_t value)
{
    char digits[20];
    int count = 0;

    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value > 0);

    while (count > 0)
        put_char(digits[--count]);
}

static void put_register(const char *name, uint64_t value)
{
    put_string(name);
    put_string(": ");
    put_hex(value);
    put_string("  ");
}

static void panic_begin(void)
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
    put_string(" *** KERNEL PANIC *** ");
    background = foreground;
    foreground = text;
    put_string("\n\nSolanum had an unexpected error and had to halt. Debug information follows.\n\n");
}

static void put_control_registers(void)
{
    uint64_t cr0, cr2, cr3, cr4;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    put_register("CR0", cr0);
    put_register("CR2", cr2);
    put_char('\n');
    put_register("CR3", cr3);
    put_register("CR4", cr4);
    put_char('\n');
}


static void put_stack_trace(uint64_t rbp)
{
    put_string("\nStack trace:\n");

    for (int i = 0; i < TRACE_DEPTH; i++)
    {
        if (rbp < HIGHER_HALF_START || (rbp & 7))
            break;

        uint64_t *frame = (uint64_t *)rbp;
        if (frame[1] == 0)
            break;

        put_string("  ");
        put_hex(frame[1]);
        put_char('\n');

        if (frame[0] <= rbp)
            break;
        rbp = frame[0];
    }
}

__attribute__((__noreturn__))
static void panic_end(uint64_t rbp)
{
    put_string("\nUptime: ");
    put_decimal(pit_ticks() / PIT_HZ);
    put_string(" s\n");

    put_stack_trace(rbp);
    arch_halt();
}

void panic_at(const char *file, int line, const char *message)
{
    panic_begin();

    put_string("Reason: ");
    put_string(message);
    put_string("\nLocation: ");
    put_string(file);
    put_char(':');
    put_decimal(line);
    put_string("\n\n");

    uint64_t rflags, rsp;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    asm volatile("mov %%rsp, %0" : "=r"(rsp));

    put_register("RIP", (uint64_t)__builtin_return_address(0));
    put_register("RSP", rsp);
    put_char('\n');
    put_register("RFL", rflags);
    put_char('\n');
    put_control_registers();

    panic_end((uint64_t)__builtin_frame_address(0));
}

static void put_page_fault(uint64_t err_code)
{
    put_string("Access: ");
    put_string(err_code & PF_FETCH ? "instruction fetch" :
               err_code & PF_WRITE ? "write" : "read");
    put_string(err_code & PF_PRESENT ? ", protection violation" : ", page not present");
    put_string(err_code & PF_USER ? ", user mode\n" : ", kernel mode\n");
}

void panic_exception(const registers_t *regs, const char *name)
{
    panic_begin();

    put_string("Reason: ");
    put_string(name);
    put_string(" (vector ");
    put_decimal(regs->int_no);
    put_string(", error code ");
    put_hex(regs->err_code);
    put_string(")\n");

    if (regs->int_no == VECTOR_PAGE_FAULT)
        put_page_fault(regs->err_code);
    put_char('\n');

    put_register("RIP", regs->rip);
    put_register("CS ", regs->cs);
    put_register("RFL", regs->rflags);
    put_char('\n');
    put_register("RSP", regs->rsp);
    put_register("SS ", regs->ss);
    put_register("RBP", regs->rbp);
    put_char('\n');
    put_register("RAX", regs->rax);
    put_register("RBX", regs->rbx);
    put_register("RCX", regs->rcx);
    put_char('\n');
    put_register("RDX", regs->rdx);
    put_register("RSI", regs->rsi);
    put_register("RDI", regs->rdi);
    put_char('\n');
    put_register("R8 ", regs->r8);
    put_register("R9 ", regs->r9);
    put_register("R10", regs->r10);
    put_char('\n');
    put_register("R11", regs->r11);
    put_register("R12", regs->r12);
    put_register("R13", regs->r13);
    put_char('\n');
    put_register("R14", regs->r14);
    put_register("R15", regs->r15);
    put_char('\n');
    put_control_registers();

    panic_end(regs->rbp);
}
