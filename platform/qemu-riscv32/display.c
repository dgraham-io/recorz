#include "display.h"

#include <stddef.h>
#include <stdint.h>

#include "machine.h"

static uint32_t framebuffer[RECORZ_DISPLAY_WIDTH * RECORZ_DISPLAY_HEIGHT] __attribute__((aligned(4096)));
/* Match the seeded transcript background so boot and cleared forms stay legible. */
static uint32_t background = 0x00F7F3E8U;

static uint32_t *framebuffer_row(uint32_t y) {
    return framebuffer + ((size_t)y * (size_t)RECORZ_DISPLAY_WIDTH);
}

static void copy_pixel_row(uint32_t *dest, const uint32_t *source, uint32_t count) {
    uint32_t index;

    for (index = 0U; index < count; ++index) {
        dest[index] = source[index];
    }
}

static void move_pixel_row(uint32_t *dest, const uint32_t *source, uint32_t count) {
    uint32_t index;

    if (dest < source) {
        copy_pixel_row(dest, source, count);
        return;
    }
    if (dest == source) {
        return;
    }
    for (index = count; index != 0U; --index) {
        dest[index - 1U] = source[index - 1U];
    }
}

static void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= RECORZ_DISPLAY_WIDTH || y >= RECORZ_DISPLAY_HEIGHT) {
        return;
    }
    framebuffer[(y * RECORZ_DISPLAY_WIDTH) + x] = color;
}

static void put_pixel_i32(int32_t x, int32_t y, uint32_t color) {
    if (x < 0 || y < 0) {
        return;
    }
    put_pixel((uint32_t)x, (uint32_t)y, color);
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
    uint32_t draw_width_pixels,
    uint32_t draw_height_pixels,
    uint32_t one_color,
    uint32_t zero_color,
    uint8_t transparent_zero
) {
    uint32_t row;

    (void)bitmap_height;
    if (scale == 0U || draw_width_pixels == 0U || draw_height_pixels == 0U) {
        return;
    }

    for (row = 0; row < copy_height; ++row) {
        uint32_t source_row = source_y + row;
        uint32_t dest_row_offset = row * scale;
        uint32_t row_pixels = scale;
        uint32_t bits = rows[source_row];
        uint32_t col;

        if (dest_row_offset >= draw_height_pixels) {
            break;
        }
        if (row_pixels > draw_height_pixels - dest_row_offset) {
            row_pixels = draw_height_pixels - dest_row_offset;
        }
        for (col = 0; col < copy_width; ++col) {
            uint32_t source_col = source_x + col;
            uint32_t dest_col_offset = col * scale;
            uint32_t col_pixels = scale;
            uint32_t bit = 1U << (bitmap_width - source_col - 1U);
            uint8_t bit_is_set = (uint8_t)((bits & bit) != 0U);
            uint32_t color;
            uint32_t dy;
            uint32_t dx;

            if (dest_col_offset >= draw_width_pixels) {
                break;
            }
            if (!bit_is_set && transparent_zero) {
                continue;
            }
            if (col_pixels > draw_width_pixels - dest_col_offset) {
                col_pixels = draw_width_pixels - dest_col_offset;
            }
            color = bit_is_set ? one_color : zero_color;
            for (dy = 0; dy < row_pixels; ++dy) {
                uint32_t dest_y = y + dest_row_offset + dy;
                uint32_t *dest_row;

                dest_row = framebuffer_row(dest_y);
                for (dx = 0; dx < col_pixels; ++dx) {
                    dest_row[x + dest_col_offset + dx] = color;
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
    uint32_t *first_row;
    uint32_t col;

    if (width == 0U || height == 0U) {
        return;
    }
    first_row = framebuffer_row(y) + x;
    for (col = 0U; col < width; ++col) {
        first_row[col] = color;
    }
    for (row = 1U; row < height; ++row) {
        copy_pixel_row(framebuffer_row(y + row) + x, first_row, width);
    }
}

void display_form_copy_rect(
    uint32_t source_x,
    uint32_t source_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
) {
    uint32_t row;

    if (width == 0U || height == 0U) {
        return;
    }
    if (source_y < dest_y && source_y + height > dest_y) {
        for (row = height; row != 0U; --row) {
            move_pixel_row(
                framebuffer_row(dest_y + row - 1U) + dest_x,
                framebuffer_row(source_y + row - 1U) + source_x,
                width
            );
        }
        return;
    }
    for (row = 0U; row < height; ++row) {
        move_pixel_row(
            framebuffer_row(dest_y + row) + dest_x,
            framebuffer_row(source_y + row) + source_x,
            width
        );
    }
}

void display_form_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) {
    int32_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y0 < y1) ? (y0 - y1) : (y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    for (;;) {
        put_pixel_i32(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            return;
        }
        if ((err * 2) >= dy) {
            err += dy;
            x0 += sx;
        }
        if ((err * 2) <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
