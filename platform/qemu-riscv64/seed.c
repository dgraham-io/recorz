#include "seed.h"

#include <stdint.h>

#include "machine.h"

#define RECORZ_MVP_SEED_MAGIC_0 'R'
#define RECORZ_MVP_SEED_MAGIC_1 'C'
#define RECORZ_MVP_SEED_MAGIC_2 'Z'
#define RECORZ_MVP_SEED_MAGIC_3 'S'
#define RECORZ_MVP_SEED_VERSION 15U
#define RECORZ_MVP_SEED_HEADER_SIZE 16U
#define RECORZ_MVP_SEED_OBJECT_SIZE 24U
#define RECORZ_MVP_SEED_BINDING_SIZE 4U

static struct recorz_mvp_seed_object loaded_objects[RECORZ_MVP_HEAP_LIMIT];
static uint16_t loaded_glyph_object_indices[RECORZ_MVP_GLYPH_CODE_LIMIT];
static struct recorz_mvp_seed loaded_seed;

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static int32_t read_i32_le(const uint8_t *bytes) {
    return (int32_t)(
        (uint32_t)bytes[0] |
        ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) |
        ((uint32_t)bytes[3] << 24U)
    );
}

const struct recorz_mvp_seed *recorz_mvp_seed_load(const uint8_t *blob, uint32_t size) {
    uint16_t object_count;
    uint16_t global_binding_count;
    uint16_t root_binding_count;
    uint16_t glyph_code_count;
    uint16_t global_index;
    uint16_t object_index;
    uint32_t offset;
    uint32_t expected_size;

    if (size < RECORZ_MVP_SEED_HEADER_SIZE) {
        machine_panic("seed manifest is too small");
    }
    if (blob[0] != RECORZ_MVP_SEED_MAGIC_0 || blob[1] != RECORZ_MVP_SEED_MAGIC_1 ||
        blob[2] != RECORZ_MVP_SEED_MAGIC_2 || blob[3] != RECORZ_MVP_SEED_MAGIC_3) {
        machine_panic("seed manifest magic mismatch");
    }
    if (read_u16_le(blob + 4U) != RECORZ_MVP_SEED_VERSION) {
        machine_panic("seed manifest version mismatch");
    }

    object_count = read_u16_le(blob + 6U);
    if (object_count > RECORZ_MVP_HEAP_LIMIT) {
        machine_panic("seed manifest object count exceeds heap limit");
    }

    global_binding_count = read_u16_le(blob + 8U);
    root_binding_count = read_u16_le(blob + 10U);
    glyph_code_count = read_u16_le(blob + 12U);
    loaded_seed.object_count = object_count;
    loaded_seed.glyph_code_count = glyph_code_count;

    if (glyph_code_count > RECORZ_MVP_GLYPH_CODE_LIMIT) {
        machine_panic("seed manifest glyph code count exceeds table capacity");
    }

    expected_size = RECORZ_MVP_SEED_HEADER_SIZE + ((uint32_t)object_count * RECORZ_MVP_SEED_OBJECT_SIZE) +
                    ((uint32_t)global_binding_count * RECORZ_MVP_SEED_BINDING_SIZE) +
                    ((uint32_t)root_binding_count * RECORZ_MVP_SEED_BINDING_SIZE) +
                    ((uint32_t)glyph_code_count * 2U);
    if (size != expected_size) {
        machine_panic("seed manifest size mismatch");
    }

    for (global_index = 0U; global_index <= RECORZ_MVP_GLOBAL_BITMAP; ++global_index) {
        loaded_seed.global_object_indices[global_index] = RECORZ_MVP_SEED_INVALID_OBJECT_INDEX;
    }
    for (global_index = 0U; global_index <= RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS; ++global_index) {
        loaded_seed.root_object_indices[global_index] = RECORZ_MVP_SEED_INVALID_OBJECT_INDEX;
    }

    offset = RECORZ_MVP_SEED_HEADER_SIZE;
    for (object_index = 0U; object_index < object_count; ++object_index) {
        uint8_t field_index;
        struct recorz_mvp_seed_object *object = &loaded_objects[object_index];

        object->object_kind = blob[offset++];
        object->field_count = blob[offset++];
        object->class_index = read_u16_le(blob + offset);
        offset += 2U;
        if (object->field_count > 4U) {
            machine_panic("seed manifest field count exceeds object field capacity");
        }
        if (object->class_index != RECORZ_MVP_SEED_INVALID_OBJECT_INDEX && object->class_index >= object_count) {
            machine_panic("seed manifest class index is out of range");
        }
        for (field_index = 0U; field_index < 4U; ++field_index) {
            struct recorz_mvp_seed_field *field = &object->fields[field_index];
            field->kind = blob[offset++];
            field->value = read_i32_le(blob + offset);
            offset += 4U;
            if (field->kind > RECORZ_MVP_SEED_FIELD_OBJECT_INDEX) {
                machine_panic("seed manifest field kind is unknown");
            }
        }
    }

    for (object_index = 0U; object_index < global_binding_count; ++object_index) {
        uint16_t binding_id = read_u16_le(blob + offset);
        uint16_t binding_object_index = read_u16_le(blob + offset + 2U);

        offset += RECORZ_MVP_SEED_BINDING_SIZE;
        if (binding_id < RECORZ_MVP_GLOBAL_TRANSCRIPT || binding_id > RECORZ_MVP_GLOBAL_BITMAP) {
            machine_panic("seed manifest global binding id is out of range");
        }
        if (binding_object_index >= object_count) {
            machine_panic("seed manifest global binding object index is out of range");
        }
        if (loaded_seed.global_object_indices[binding_id] != RECORZ_MVP_SEED_INVALID_OBJECT_INDEX) {
            machine_panic("seed manifest has duplicate global bindings");
        }
        loaded_seed.global_object_indices[binding_id] = binding_object_index;
    }

    for (object_index = RECORZ_MVP_GLOBAL_TRANSCRIPT; object_index <= RECORZ_MVP_GLOBAL_BITMAP; ++object_index) {
        if (loaded_seed.global_object_indices[object_index] == RECORZ_MVP_SEED_INVALID_OBJECT_INDEX) {
            machine_panic("seed manifest is missing a required global binding");
        }
    }

    for (object_index = 0U; object_index < root_binding_count; ++object_index) {
        uint16_t binding_id = read_u16_le(blob + offset);
        uint16_t binding_object_index = read_u16_le(blob + offset + 2U);

        offset += RECORZ_MVP_SEED_BINDING_SIZE;
        if (binding_id < RECORZ_MVP_SEED_ROOT_DEFAULT_FORM ||
            binding_id > RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS) {
            machine_panic("seed manifest root binding id is out of range");
        }
        if (binding_object_index >= object_count) {
            machine_panic("seed manifest root binding object index is out of range");
        }
        if (loaded_seed.root_object_indices[binding_id] != RECORZ_MVP_SEED_INVALID_OBJECT_INDEX) {
            machine_panic("seed manifest has duplicate root bindings");
        }
        loaded_seed.root_object_indices[binding_id] = binding_object_index;
    }

    for (object_index = RECORZ_MVP_SEED_ROOT_DEFAULT_FORM;
         object_index <= RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS;
         ++object_index) {
        if (loaded_seed.root_object_indices[object_index] == RECORZ_MVP_SEED_INVALID_OBJECT_INDEX) {
            machine_panic("seed manifest is missing a required root binding");
        }
    }

    for (object_index = 0U; object_index < RECORZ_MVP_GLYPH_CODE_LIMIT; ++object_index) {
        loaded_glyph_object_indices[object_index] = RECORZ_MVP_SEED_INVALID_OBJECT_INDEX;
    }
    for (object_index = 0U; object_index < glyph_code_count; ++object_index) {
        uint16_t glyph_object_index = read_u16_le(blob + offset);

        offset += 2U;
        if (glyph_object_index >= object_count) {
            machine_panic("seed manifest glyph object index is out of range");
        }
        loaded_glyph_object_indices[object_index] = glyph_object_index;
    }

    loaded_seed.objects = loaded_objects;
    loaded_seed.glyph_object_indices_by_code = loaded_glyph_object_indices;
    return &loaded_seed;
}
