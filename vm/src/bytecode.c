#include "bytecode.h"
#include "uart.h"

void chunk_init(Chunk *c) {
    c->count = 0;
}

void chunk_emit_byte(Chunk *c, uint8_t byte) {
    if (c->count >= CHUNK_MAX) {
        uart_puts("FATAL: bytecode overflow\n");
        for (;;) __asm__ volatile("wfi");
    }
    c->code[c->count++] = byte;
}

void chunk_emit_u16(Chunk *c, uint16_t val) {
    chunk_emit_byte(c, (uint8_t)(val & 0xFF));
    chunk_emit_byte(c, (uint8_t)(val >> 8));
}

void chunk_emit_i32(Chunk *c, int32_t val) {
    uint32_t u = (uint32_t)val;
    chunk_emit_byte(c, (uint8_t)(u & 0xFF));
    chunk_emit_byte(c, (uint8_t)((u >> 8) & 0xFF));
    chunk_emit_byte(c, (uint8_t)((u >> 16) & 0xFF));
    chunk_emit_byte(c, (uint8_t)((u >> 24) & 0xFF));
}
