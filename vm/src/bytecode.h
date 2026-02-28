#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

typedef enum {
    OP_PUSH_INT    = 0x01,  /* operand: i32 (4 bytes LE) */
    OP_PUSH_STR    = 0x02,  /* operand: u16 len + raw bytes */
    OP_PUSH_NIL    = 0x03,
    OP_POP         = 0x04,
    OP_LOAD        = 0x05,  /* operand: u8 slot */
    OP_STORE       = 0x06,  /* operand: u8 slot */
    OP_SEND_UNARY  = 0x10,  /* operand: u8 selector_id */
    OP_SEND_BINARY = 0x11,  /* operand: u8 selector_id */
    OP_HALT        = 0xFF,
} Opcode;

typedef enum {
    SEL_PRINT = 0,
    SEL_PLUS  = 1,
} SelectorId;

#define CHUNK_MAX 4096

typedef struct {
    uint8_t  code[CHUNK_MAX];
    uint32_t count;
} Chunk;

void chunk_init(Chunk *c);
void chunk_emit_byte(Chunk *c, uint8_t byte);
void chunk_emit_u16(Chunk *c, uint16_t val);
void chunk_emit_i32(Chunk *c, int32_t val);

#endif
