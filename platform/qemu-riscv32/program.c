#include "program.h"

#include <stdint.h>

#include "machine.h"

#define RECORZ_MVP_PROGRAM_INSTRUCTION_LIMIT 512U
#define RECORZ_MVP_PROGRAM_LITERAL_LIMIT 128U
#define RECORZ_MVP_PROGRAM_OBJECT_FIELD_LIMIT 4U
#define RECORZ_MVP_PROGRAM_MAX_SELECTOR_ID RECORZ_MVP_SELECTOR_BE_CURSOR
#define RECORZ_MVP_PROGRAM_MAX_GLOBAL_ID RECORZ_MVP_GLOBAL_WORKSPACE_SELECTION

static struct recorz_mvp_instruction loaded_instructions[RECORZ_MVP_PROGRAM_INSTRUCTION_LIMIT];
static struct recorz_mvp_literal loaded_literals[RECORZ_MVP_PROGRAM_LITERAL_LIMIT];
static char loaded_lexical_names[RECORZ_MVP_PROGRAM_LEXICAL_LIMIT][RECORZ_MVP_PROGRAM_LEXICAL_NAME_LIMIT];
static struct recorz_mvp_program loaded_program;

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static int32_t read_i32_le(const uint8_t *bytes) {
    return (int32_t)((uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) | ((uint32_t)bytes[2] << 16U) |
                     ((uint32_t)bytes[3] << 24U));
}

static void validate_instruction(
    const struct recorz_mvp_instruction *instruction,
    uint16_t literal_count,
    uint16_t lexical_count
) {
    switch (instruction->opcode) {
        case RECORZ_MVP_OP_PUSH_GLOBAL:
            if (instruction->operand_a < RECORZ_MVP_GLOBAL_TRANSCRIPT ||
                instruction->operand_a > RECORZ_MVP_PROGRAM_MAX_GLOBAL_ID) {
                machine_panic("program manifest global operand is out of range");
            }
            return;
        case RECORZ_MVP_OP_PUSH_LITERAL:
        case RECORZ_MVP_OP_PUSH_BLOCK_LITERAL:
            if (instruction->operand_a != 0U || instruction->operand_b >= literal_count) {
                machine_panic("program manifest literal operand is invalid");
            }
            return;
        case RECORZ_MVP_OP_PUSH_LEXICAL:
        case RECORZ_MVP_OP_STORE_LEXICAL:
            if (instruction->operand_a != 0U || instruction->operand_b >= lexical_count) {
                machine_panic("program manifest lexical operand is invalid");
            }
            return;
        case RECORZ_MVP_OP_STORE_FIELD:
            if (instruction->operand_a >= RECORZ_MVP_PROGRAM_OBJECT_FIELD_LIMIT || instruction->operand_b != 0U) {
                machine_panic("program manifest field write operand is invalid");
            }
            return;
        case RECORZ_MVP_OP_SEND:
            if (instruction->operand_a < RECORZ_MVP_SELECTOR_SHOW ||
                instruction->operand_a > RECORZ_MVP_PROGRAM_MAX_SELECTOR_ID) {
                machine_panic("program manifest selector operand is out of range");
            }
            return;
        case RECORZ_MVP_OP_DUP:
        case RECORZ_MVP_OP_POP:
        case RECORZ_MVP_OP_RETURN:
        case RECORZ_MVP_OP_PUSH_NIL:
        case RECORZ_MVP_OP_PUSH_SELF:
        case RECORZ_MVP_OP_PUSH_THIS_CONTEXT:
            if (instruction->operand_a != 0U || instruction->operand_b != 0U) {
                machine_panic("program manifest stack opcode carries unexpected operands");
            }
            return;
        default:
            machine_panic("program manifest opcode is unknown");
    }
}

const struct recorz_mvp_program *recorz_mvp_program_load(const uint8_t *blob, uint32_t size) {
    uint16_t instruction_count;
    uint16_t literal_count;
    uint16_t lexical_count;
    uint16_t instruction_index;
    uint16_t literal_index;
    uint32_t offset;

    if (size < RECORZ_MVP_PROGRAM_HEADER_SIZE) {
        machine_panic("program manifest is too small");
    }
    if (blob[0] != RECORZ_MVP_PROGRAM_MAGIC_0 || blob[1] != RECORZ_MVP_PROGRAM_MAGIC_1 ||
        blob[2] != RECORZ_MVP_PROGRAM_MAGIC_2 || blob[3] != RECORZ_MVP_PROGRAM_MAGIC_3) {
        machine_panic("program manifest magic mismatch");
    }
    if (read_u16_le(blob + 4U) != RECORZ_MVP_PROGRAM_VERSION) {
        machine_panic("program manifest version mismatch");
    }

    instruction_count = read_u16_le(blob + 6U);
    literal_count = read_u16_le(blob + 8U);
    lexical_count = read_u16_le(blob + 10U);
    if (instruction_count > RECORZ_MVP_PROGRAM_INSTRUCTION_LIMIT) {
        machine_panic("program manifest instruction count exceeds capacity");
    }
    if (literal_count > RECORZ_MVP_PROGRAM_LITERAL_LIMIT) {
        machine_panic("program manifest literal count exceeds capacity");
    }
    if (lexical_count > RECORZ_MVP_PROGRAM_LEXICAL_LIMIT) {
        machine_panic("program manifest lexical count exceeds capacity");
    }

    offset = RECORZ_MVP_PROGRAM_HEADER_SIZE;
    if (size < offset + ((uint32_t)instruction_count * RECORZ_MVP_PROGRAM_INSTRUCTION_SIZE)) {
        machine_panic("program manifest instruction section is truncated");
    }

    for (instruction_index = 0U; instruction_index < instruction_count; ++instruction_index) {
        struct recorz_mvp_instruction *instruction = &loaded_instructions[instruction_index];
        instruction->opcode = blob[offset++];
        instruction->operand_a = blob[offset++];
        instruction->operand_b = read_u16_le(blob + offset);
        offset += 2U;
        validate_instruction(instruction, literal_count, lexical_count);
    }

    for (literal_index = 0U; literal_index < literal_count; ++literal_index) {
        struct recorz_mvp_literal *literal = &loaded_literals[literal_index];
        int32_t payload;

        if (size < offset + RECORZ_MVP_PROGRAM_LITERAL_HEADER_SIZE) {
            machine_panic("program manifest literal section is truncated");
        }

        literal->kind = blob[offset];
        payload = read_i32_le(blob + offset + 4U);
        if (literal->kind == RECORZ_MVP_LITERAL_SMALL_INTEGER) {
            literal->integer = payload;
            literal->string = 0;
            offset += RECORZ_MVP_PROGRAM_LITERAL_HEADER_SIZE;
            continue;
        }
        if (literal->kind == RECORZ_MVP_LITERAL_STRING) {
            uint32_t string_length;
            uint32_t string_offset = offset + RECORZ_MVP_PROGRAM_LITERAL_HEADER_SIZE;

            if (payload < 0) {
                machine_panic("program manifest string literal length is negative");
            }
            string_length = (uint32_t)payload;
            if (size < string_offset + string_length + 1U) {
                machine_panic("program manifest string literal is truncated");
            }
            if (blob[string_offset + string_length] != 0U) {
                machine_panic("program manifest string literal is not terminated");
            }
            literal->integer = 0;
            literal->string = (const char *)(blob + string_offset);
            offset = string_offset + string_length + 1U;
            continue;
        }
        machine_panic("program manifest literal kind is unknown");
    }
    for (literal_index = 0U; literal_index < lexical_count; ++literal_index) {
        uint16_t lexical_name_length = 0U;

        while (offset + lexical_name_length < size &&
               blob[offset + lexical_name_length] != 0U) {
            ++lexical_name_length;
        }
        if (offset + lexical_name_length >= size) {
            machine_panic("program manifest lexical name is truncated");
        }
        if (lexical_name_length == 0U) {
            machine_panic("program manifest lexical name is empty");
        }
        if (lexical_name_length >= RECORZ_MVP_PROGRAM_LEXICAL_NAME_LIMIT) {
            machine_panic("program manifest lexical name exceeds capacity");
        }
        for (instruction_index = 0U; instruction_index < lexical_name_length; ++instruction_index) {
            loaded_lexical_names[literal_index][instruction_index] = (char)blob[offset + instruction_index];
        }
        loaded_lexical_names[literal_index][lexical_name_length] = '\0';
        offset += lexical_name_length + 1U;
    }

    if (offset != size) {
        machine_panic("program manifest size mismatch");
    }

    loaded_program.instructions = loaded_instructions;
    loaded_program.instruction_count = instruction_count;
    loaded_program.literals = loaded_literals;
    loaded_program.literal_count = literal_count;
    loaded_program.lexical_count = lexical_count;
    loaded_program.lexical_names = lexical_count == 0U ? 0 : loaded_lexical_names;
    return &loaded_program;
}
