#include "seed.h"

#include <stdint.h>

#include "machine.h"

#define RECORZ_MVP_SEED_MAGIC_0 'R'
#define RECORZ_MVP_SEED_MAGIC_1 'C'
#define RECORZ_MVP_SEED_MAGIC_2 'Z'
#define RECORZ_MVP_SEED_MAGIC_3 'S'
#define RECORZ_MVP_SEED_VERSION 1U
#define RECORZ_MVP_SEED_HEADER_SIZE 32U
#define RECORZ_MVP_SEED_OBJECT_SIZE 20U

static struct recorz_mvp_seed_object loaded_objects[RECORZ_MVP_HEAP_LIMIT];
static uint8_t loaded_glyph_offsets[RECORZ_MVP_GLYPH_CODE_LIMIT];
static struct recorz_mvp_seed loaded_seed;

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static int16_t read_i16_le(const uint8_t *bytes) {
    return (int16_t)read_u16_le(bytes);
}

const struct recorz_mvp_seed *recorz_mvp_seed_load(const uint8_t *blob, uint32_t size) {
    uint16_t object_count;
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

    loaded_seed.object_count = object_count;
    for (global_index = 0U; global_index <= RECORZ_MVP_GLOBAL_BITMAP; ++global_index) {
        loaded_seed.global_object_indices[global_index] = read_u16_le(blob + 8U + (global_index * 2U));
    }
    loaded_seed.default_form_index = read_u16_le(blob + 22U);
    loaded_seed.framebuffer_bitmap_index = read_u16_le(blob + 24U);
    loaded_seed.glyph_bitmap_start_index = read_u16_le(blob + 26U);
    glyph_code_count = read_u16_le(blob + 28U);
    loaded_seed.glyph_code_count = glyph_code_count;
    loaded_seed.glyph_fallback_offset = blob[30U];

    if (glyph_code_count > RECORZ_MVP_GLYPH_CODE_LIMIT) {
        machine_panic("seed manifest glyph code count exceeds table capacity");
    }
    if (loaded_seed.default_form_index >= object_count || loaded_seed.framebuffer_bitmap_index >= object_count) {
        machine_panic("seed manifest root index out of range");
    }
    if (loaded_seed.glyph_bitmap_start_index >= object_count) {
        machine_panic("seed manifest glyph bitmap start index out of range");
    }
    if ((uint32_t)loaded_seed.glyph_bitmap_start_index + (uint32_t)loaded_seed.glyph_fallback_offset >= object_count) {
        machine_panic("seed manifest glyph fallback index out of range");
    }
    for (global_index = RECORZ_MVP_GLOBAL_TRANSCRIPT; global_index <= RECORZ_MVP_GLOBAL_BITMAP; ++global_index) {
        if (loaded_seed.global_object_indices[global_index] >= object_count) {
            machine_panic("seed manifest global object index out of range");
        }
    }

    expected_size = RECORZ_MVP_SEED_HEADER_SIZE + ((uint32_t)object_count * RECORZ_MVP_SEED_OBJECT_SIZE) + glyph_code_count;
    if (size != expected_size) {
        machine_panic("seed manifest size mismatch");
    }

    offset = RECORZ_MVP_SEED_HEADER_SIZE;
    for (object_index = 0U; object_index < object_count; ++object_index) {
        uint8_t field_index;
        struct recorz_mvp_seed_object *object = &loaded_objects[object_index];

        object->object_kind = blob[offset++];
        object->field_count = blob[offset++];
        offset += 2U;
        if (object->field_count > 4U) {
            machine_panic("seed manifest field count exceeds object field capacity");
        }
        for (field_index = 0U; field_index < 4U; ++field_index) {
            struct recorz_mvp_seed_field *field = &object->fields[field_index];
            field->kind = blob[offset++];
            field->value = read_i16_le(blob + offset);
            offset += 2U;
            offset += 1U;
            if (field->kind > RECORZ_MVP_SEED_FIELD_OBJECT_INDEX) {
                machine_panic("seed manifest field kind is unknown");
            }
        }
    }

    for (object_index = 0U; object_index < RECORZ_MVP_GLYPH_CODE_LIMIT; ++object_index) {
        loaded_glyph_offsets[object_index] = loaded_seed.glyph_fallback_offset;
    }
    for (object_index = 0U; object_index < glyph_code_count; ++object_index) {
        uint8_t glyph_offset = blob[offset++];
        if ((uint32_t)loaded_seed.glyph_bitmap_start_index + glyph_offset >= object_count) {
            machine_panic("seed manifest glyph object offset out of range");
        }
        loaded_glyph_offsets[object_index] = glyph_offset;
    }

    loaded_seed.objects = loaded_objects;
    loaded_seed.glyph_object_offsets_by_code = loaded_glyph_offsets;
    return &loaded_seed;
}
