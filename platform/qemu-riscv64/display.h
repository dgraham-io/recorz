#ifndef RECORZ_QEMU_RISCV64_DISPLAY_H
#define RECORZ_QEMU_RISCV64_DISPLAY_H

#include <stdint.h>

#define RECORZ_DISPLAY_WIDTH 1024U
#define RECORZ_DISPLAY_HEIGHT 768U

void display_init(void);
void display_form_fill_color(uint32_t color);
void display_form_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void display_form_copy_rect(
    uint32_t source_x,
    uint32_t source_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
);
void display_form_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
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
);

#endif
