#ifndef RECORZ_QEMU_RISCV64_VM_H
#define RECORZ_QEMU_RISCV64_VM_H

#include <stdint.h>

#include "recorz_mvp_generated_runtime_bindings.h"

#define RECORZ_MVP_HEAP_LIMIT 16384U
#define RECORZ_MVP_GLYPH_CODE_LIMIT 128U
#define RECORZ_MVP_NAMED_OBJECT_LIMIT 48U
#define RECORZ_MVP_PROGRAM_LEXICAL_LIMIT 32U
#define RECORZ_MVP_PROGRAM_LEXICAL_NAME_LIMIT 96U

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
    const char (*lexical_names)[RECORZ_MVP_PROGRAM_LEXICAL_NAME_LIMIT];
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
    uint16_t global_object_indices[RECORZ_MVP_GLOBAL_WORKSPACE_SELECTION + 1];
    uint16_t root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT + 1];
    const uint16_t *glyph_object_indices_by_code;
    uint16_t glyph_code_count;
};

void recorz_mvp_vm_run(
    const struct recorz_mvp_program *program,
    const struct recorz_mvp_seed *seed,
    const uint8_t *method_update_blob,
    uint32_t method_update_size,
    const uint8_t *file_in_blob,
    uint32_t file_in_size,
    const uint8_t *snapshot_blob,
    uint32_t snapshot_size
);

#endif
