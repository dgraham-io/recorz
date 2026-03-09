#include "display.h"

#include <stddef.h>
#include <stdint.h>

#include "machine.h"

static uint32_t framebuffer[RECORZ_DISPLAY_WIDTH * RECORZ_DISPLAY_HEIGHT] __attribute__((aligned(4096)));
/* Match the seeded transcript background so boot and cleared forms stay legible. */
static uint32_t background = 0x00F7F3E8U;

static void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= RECORZ_DISPLAY_WIDTH || y >= RECORZ_DISPLAY_HEIGHT) {
        return;
    }
    framebuffer[(y * RECORZ_DISPLAY_WIDTH) + x] = color;
}

/* Generic 1bpp blit used as the first copy-based rendering path. */
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
    uint32_t row;

    if (scale == 0U) {
        return;
    }
    if (source_x >= bitmap_width || source_y >= bitmap_height || copy_width == 0U || copy_height == 0U) {
        return;
    }
    if (copy_width > bitmap_width - source_x) {
        copy_width = bitmap_width - source_x;
    }
    if (copy_height > bitmap_height - source_y) {
        copy_height = bitmap_height - source_y;
    }

    for (row = 0; row < copy_height; ++row) {
        uint32_t source_row = source_y + row;
        uint32_t bits = rows[source_row];
        uint32_t col;
        for (col = 0; col < copy_width; ++col) {
            uint32_t source_col = source_x + col;
            uint32_t bit = 1U << (bitmap_width - source_col - 1U);
            uint8_t bit_is_set = (uint8_t)((bits & bit) != 0U);
            uint32_t color;
            uint32_t dy;
            uint32_t dx;

            if (!bit_is_set && transparent_zero) {
                continue;
            }
            color = bit_is_set ? one_color : zero_color;
            for (dy = 0; dy < scale; ++dy) {
                for (dx = 0; dx < scale; ++dx) {
                    put_pixel(x + (col * scale) + dx, y + (row * scale) + dy, color);
                }
            }
        }
    }
}

static void clear_to_color(uint32_t color) {
    size_t index;
    background = color;
    for (index = 0; index < (size_t)RECORZ_DISPLAY_WIDTH * (size_t)RECORZ_DISPLAY_HEIGHT; ++index) {
        framebuffer[index] = color;
    }
}

void display_init(void) {
    machine_ramfb_init(framebuffer, RECORZ_DISPLAY_WIDTH, RECORZ_DISPLAY_HEIGHT, RECORZ_DISPLAY_WIDTH * 4U);
    clear_to_color(background);
}

void display_form_fill_color(uint32_t color) {
    clear_to_color(color);
}

void display_form_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    uint32_t row;

    for (row = 0U; row < height; ++row) {
        uint32_t col;

        for (col = 0U; col < width; ++col) {
            put_pixel(x + col, y + row, color);
        }
    }
}
