#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>

#include "font8x8_basic.h"

static const size_t GLYPH_SIZE = 8;
static const size_t GLYPH_SCALE = 2;

static size_t terminal_row;
static size_t terminal_column;
static size_t terminal_rows;
static size_t terminal_columns;
static uint32_t terminal_foreground;
static uint32_t terminal_background;
static volatile uint32_t* terminal_buffer;
static size_t terminal_pitch;

static uint32_t terminal_channel(uint8_t value, uint8_t size, uint8_t shift) {
	uint64_t max = ((uint64_t) 1 << size) - 1;
	return (uint32_t) ((value * max / 255) << shift);
}

static uint32_t terminal_pixel(const struct boot_framebuffer* fb, uint8_t gray) {
	return terminal_channel(gray, fb->red_size, fb->red_shift)
	     | terminal_channel(gray, fb->green_size, fb->green_shift)
	     | terminal_channel(gray, fb->blue_size, fb->blue_shift);
}

/* Without a 32-bit framebuffer the terminal stays off and output is dropped. */
void terminal_initialize(const struct boot_framebuffer* fb) {
	if (fb->address == NULL || fb->bpp != 32)
		return;

	terminal_row = 0;
	terminal_column = 0;
	terminal_columns = fb->width / (GLYPH_SIZE * GLYPH_SCALE);
	terminal_rows = fb->height / (GLYPH_SIZE * GLYPH_SCALE);
	terminal_foreground = terminal_pixel(fb, 0xAA);
	terminal_background = terminal_pixel(fb, 0x00);
	terminal_pitch = fb->pitch / sizeof(uint32_t);
	terminal_buffer = fb->address;
	for (size_t y = 0; y < fb->height; y++) {
		for (size_t x = 0; x < fb->width; x++) {
			terminal_buffer[y * terminal_pitch + x] = terminal_background;
		}
	}
}

static void terminal_putentryat(unsigned char c, size_t x, size_t y) {
	const unsigned char* glyph = font8x8_basic[c < 128 ? c : '?'];
	const size_t left = x * GLYPH_SIZE * GLYPH_SCALE;
	const size_t top = y * GLYPH_SIZE * GLYPH_SCALE;

	for (size_t row = 0; row < GLYPH_SIZE * GLYPH_SCALE; row++) {
		const unsigned char bits = glyph[row / GLYPH_SCALE];
		for (size_t column = 0; column < GLYPH_SIZE * GLYPH_SCALE; column++) {
			const bool set = bits & (1 << (column / GLYPH_SCALE));
			terminal_buffer[(top + row) * terminal_pitch + left + column] =
				set ? terminal_foreground : terminal_background;
		}
	}
}

void terminal_putchar(char c) {
	unsigned char uc = c;
	if (terminal_buffer == NULL)
		return;
	if (c == '\n') {
		terminal_column = 0;
		terminal_row++;
	}
	else if (c == '\r') {
		terminal_column = 0;
	}
	else if (c == '\t') {
		terminal_column = (terminal_column + 8) / 8 * 8;
	}
	else {
		terminal_putentryat(uc, terminal_column, terminal_row);
		terminal_column++;
	}
	if (terminal_column >= terminal_columns) {
		terminal_column = 0;
		terminal_row++;
	}
	if (terminal_row >= terminal_rows) {
		terminal_row = 0;
	}
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}
