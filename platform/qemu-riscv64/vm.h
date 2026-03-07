#ifndef RECORZ_QEMU_RISCV64_VM_H
#define RECORZ_QEMU_RISCV64_VM_H

#include <stdint.h>

#define RECORZ_MVP_HEAP_LIMIT 256U
#define RECORZ_MVP_GLYPH_CODE_LIMIT 128U
#define RECORZ_MVP_SEED_INVALID_OBJECT_INDEX 0xFFFFU

enum recorz_mvp_opcode {
    RECORZ_MVP_OP_PUSH_GLOBAL = 1,
    RECORZ_MVP_OP_PUSH_LITERAL = 2,
    RECORZ_MVP_OP_SEND = 3,
    RECORZ_MVP_OP_DUP = 4,
    RECORZ_MVP_OP_POP = 5,
    RECORZ_MVP_OP_RETURN = 6,
    RECORZ_MVP_OP_PUSH_NIL = 7,
    RECORZ_MVP_OP_PUSH_LEXICAL = 8,
    RECORZ_MVP_OP_STORE_LEXICAL = 9,
};

enum recorz_mvp_global {
    RECORZ_MVP_GLOBAL_TRANSCRIPT = 1,
    RECORZ_MVP_GLOBAL_DISPLAY = 2,
    RECORZ_MVP_GLOBAL_BITBLT = 3,
    RECORZ_MVP_GLOBAL_GLYPHS = 4,
    RECORZ_MVP_GLOBAL_FORM = 5,
    RECORZ_MVP_GLOBAL_BITMAP = 6,
};

enum recorz_mvp_selector {
    RECORZ_MVP_SELECTOR_SHOW = 1,
    RECORZ_MVP_SELECTOR_CR = 2,
    RECORZ_MVP_SELECTOR_WRITE_STRING = 3,
    RECORZ_MVP_SELECTOR_NEWLINE = 4,
    RECORZ_MVP_SELECTOR_DEFAULT_FORM = 5,
    RECORZ_MVP_SELECTOR_CLEAR = 6,
    RECORZ_MVP_SELECTOR_WIDTH = 7,
    RECORZ_MVP_SELECTOR_HEIGHT = 8,
    RECORZ_MVP_SELECTOR_ADD = 9,
    RECORZ_MVP_SELECTOR_SUBTRACT = 10,
    RECORZ_MVP_SELECTOR_MULTIPLY = 11,
    RECORZ_MVP_SELECTOR_PRINT_STRING = 12,
    RECORZ_MVP_SELECTOR_BITS = 13,
    RECORZ_MVP_SELECTOR_FILL_FORM_COLOR = 14,
    RECORZ_MVP_SELECTOR_AT = 15,
    RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE = 16,
    RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR = 17,
    RECORZ_MVP_SELECTOR_MONO_WIDTH_HEIGHT = 18,
    RECORZ_MVP_SELECTOR_FROM_BITS = 19,
    RECORZ_MVP_SELECTOR_COPY_BITMAP_SOURCE_X_SOURCE_Y_WIDTH_HEIGHT_TO_FORM_X_Y_SCALE_COLOR = 20,
    RECORZ_MVP_SELECTOR_CLASS = 21,
    RECORZ_MVP_SELECTOR_INSTANCE_KIND = 22,
};

enum recorz_mvp_literal_kind {
    RECORZ_MVP_LITERAL_STRING = 1,
    RECORZ_MVP_LITERAL_SMALL_INTEGER = 2,
};

enum recorz_mvp_object_kind {
    RECORZ_MVP_OBJECT_TRANSCRIPT = 1,
    RECORZ_MVP_OBJECT_DISPLAY = 2,
    RECORZ_MVP_OBJECT_FORM = 3,
    RECORZ_MVP_OBJECT_BITMAP = 4,
    RECORZ_MVP_OBJECT_BITBLT = 5,
    RECORZ_MVP_OBJECT_GLYPHS = 6,
    RECORZ_MVP_OBJECT_FORM_FACTORY = 7,
    RECORZ_MVP_OBJECT_BITMAP_FACTORY = 8,
    RECORZ_MVP_OBJECT_TEXT_LAYOUT = 9,
    RECORZ_MVP_OBJECT_TEXT_STYLE = 10,
    RECORZ_MVP_OBJECT_TEXT_METRICS = 11,
    RECORZ_MVP_OBJECT_TEXT_BEHAVIOR = 12,
    RECORZ_MVP_OBJECT_CLASS = 13,
    RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR = 14,
    RECORZ_MVP_OBJECT_METHOD_ENTRY = 15,
    RECORZ_MVP_OBJECT_SELECTOR = 16,
    RECORZ_MVP_OBJECT_ACCESSOR_METHOD = 17,
    RECORZ_MVP_OBJECT_FIELD_SEND_METHOD = 18,
    RECORZ_MVP_OBJECT_ROOT_SEND_METHOD = 19,
    RECORZ_MVP_OBJECT_ROOT_VALUE_METHOD = 20,
    RECORZ_MVP_OBJECT_INTERPRETED_METHOD = 21,
    RECORZ_MVP_OBJECT_COMPILED_METHOD = 22,
};

enum recorz_mvp_seed_field_kind {
    RECORZ_MVP_SEED_FIELD_NIL = 0,
    RECORZ_MVP_SEED_FIELD_SMALL_INTEGER = 1,
    RECORZ_MVP_SEED_FIELD_OBJECT_INDEX = 2,
};

enum recorz_mvp_seed_root {
    RECORZ_MVP_SEED_ROOT_DEFAULT_FORM = 1,
    RECORZ_MVP_SEED_ROOT_FRAMEBUFFER_BITMAP = 2,
    RECORZ_MVP_SEED_ROOT_TRANSCRIPT_BEHAVIOR = 3,
    RECORZ_MVP_SEED_ROOT_TRANSCRIPT_LAYOUT = 4,
    RECORZ_MVP_SEED_ROOT_TRANSCRIPT_STYLE = 5,
    RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS = 6,
};

struct recorz_mvp_instruction {
    uint8_t opcode;
    uint8_t operand_a;
    uint16_t operand_b;
};

struct recorz_mvp_literal {
    uint8_t kind;
    int32_t integer;
    const char *string;
};

struct recorz_mvp_program {
    const struct recorz_mvp_instruction *instructions;
    uint32_t instruction_count;
    const struct recorz_mvp_literal *literals;
    uint32_t literal_count;
    uint16_t lexical_count;
};

struct recorz_mvp_seed_field {
    uint8_t kind;
    int32_t value;
};

struct recorz_mvp_seed_object {
    uint8_t object_kind;
    uint8_t field_count;
    uint16_t class_index;
    struct recorz_mvp_seed_field fields[4];
};

struct recorz_mvp_seed {
    const struct recorz_mvp_seed_object *objects;
    uint16_t object_count;
    uint16_t global_object_indices[RECORZ_MVP_GLOBAL_BITMAP + 1];
    uint16_t root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS + 1];
    const uint16_t *glyph_object_indices_by_code;
    uint16_t glyph_code_count;
};

void recorz_mvp_vm_run(const struct recorz_mvp_program *program, const struct recorz_mvp_seed *seed);

#endif
