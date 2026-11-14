#ifndef DRIVERS_FB_H
#define DRIVERS_FB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/boot.h>

#include "font8x8_basic.h"

/* Drawing helpers for a 32-bit framebuffer. They keep no state, so the panic
   screen can use them whatever state the terminal is in. */

#define FB_GLYPH_SCALE  2
#define FB_GLYPH_SIZE   (8 * FB_GLYPH_SCALE)

static inline uint32_t fb_channel(uint8_t value, uint8_t size, uint8_t shift) {
    uint64_t max = ((uint64_t)1 << size) - 1;
    return (uint32_t)((value * max / 255) << shift);
}

static inline uint32_t fb_color(const struct boot_framebuffer* fb, uint8_t red, uint8_t green, uint8_t blue) {
    return fb_channel(red, fb->red_size, fb->red_shift)
         | fb_channel(green, fb->green_size, fb->green_shift)
         | fb_channel(blue, fb->blue_size, fb->blue_shift);
}

/* pitch is in pixels; (left, top) is the glyph's top-left pixel. */
static inline void fb_draw_glyph(volatile uint32_t* pixels, size_t pitch, size_t left, size_t top,
                                 unsigned char c, uint32_t foreground, uint32_t background) {
    const unsigned char* glyph = font8x8_basic[c < 128 ? c : '?'];

    for (size_t y = 0; y < FB_GLYPH_SIZE; y++) {
        unsigned char bits = glyph[y / FB_GLYPH_SCALE];

        for (size_t x = 0; x < FB_GLYPH_SIZE; x++) {
            bool set = bits & (1 << (x / FB_GLYPH_SCALE));
            pixels[(top + y) * pitch + left + x] = set ? foreground : background;
        }
    }
}

#endif
