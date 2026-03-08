#include "display.h"

#include <stdint.h>

void display_init(void) {
}

void display_form_fill_color(uint32_t color) {
    (void)color;
}

void display_form_blit_mono_bitmap(
    uint32_t x,
    uint32_t y,
    const uint32_t *rows,
    uint32_t bitmap_width,
    uint32_t bitmap_height,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t copy_width,
    uint32_t copy_height,
    uint32_t scale,
    uint32_t one_color,
    uint32_t zero_color,
    uint8_t transparent_zero
) {
    (void)x;
    (void)y;
    (void)rows;
    (void)bitmap_width;
    (void)bitmap_height;
    (void)source_x;
    (void)source_y;
    (void)copy_width;
    (void)copy_height;
    (void)scale;
    (void)one_color;
    (void)zero_color;
    (void)transparent_zero;
}
