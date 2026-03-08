#include "image.h"

#include <stdint.h>

#include "machine.h"
#include "program.h"
#include "seed.h"

#define RECORZ_MVP_IMAGE_SECTION_LIMIT 8U
#define RECORZ_MVP_IMAGE_KNOWN_FEATURES RECORZ_MVP_IMAGE_FEATURE_FNV1A32
#define RECORZ_MVP_FNV1A32_OFFSET_BASIS 2166136261U
#define RECORZ_MVP_FNV1A32_PRIME 16777619U

static struct recorz_mvp_boot_image loaded_image;

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static uint32_t read_u32_le(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) | ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static uint32_t fnv1a32(const uint8_t *bytes, uint32_t size) {
    uint32_t index;
    uint32_t value = RECORZ_MVP_FNV1A32_OFFSET_BASIS;

    for (index = 0U; index < size; ++index) {
        value ^= (uint32_t)bytes[index];
        value *= RECORZ_MVP_FNV1A32_PRIME;
    }
    return value;
}

static void validate_entry_section(const uint8_t *blob, uint32_t size) {
    if (size != RECORZ_MVP_IMAGE_ENTRY_SIZE) {
        machine_panic("boot image entry section size is invalid");
    }
    if (blob[0] != RECORZ_MVP_IMAGE_ENTRY_MAGIC_0 || blob[1] != RECORZ_MVP_IMAGE_ENTRY_MAGIC_1 ||
        blob[2] != RECORZ_MVP_IMAGE_ENTRY_MAGIC_2 || blob[3] != RECORZ_MVP_IMAGE_ENTRY_MAGIC_3) {
        machine_panic("boot image entry magic mismatch");
    }
    if (read_u16_le(blob + 4U) != RECORZ_MVP_IMAGE_ENTRY_VERSION) {
        machine_panic("boot image entry version mismatch");
    }
    if (read_u16_le(blob + 6U) != RECORZ_MVP_IMAGE_ENTRY_KIND_DOIT) {
        machine_panic("boot image entry kind is unsupported");
    }
    if (read_u16_le(blob + 8U) != 0U) {
        machine_panic("boot image entry flags are unsupported");
    }
    if (read_u16_le(blob + 10U) != RECORZ_MVP_IMAGE_SECTION_PROGRAM) {
        machine_panic("boot image entry does not target the program section");
    }
    if (read_u16_le(blob + 12U) != 0U) {
        machine_panic("boot image entry argument count is unsupported");
    }
    if (read_u16_le(blob + 14U) != 0U) {
        machine_panic("boot image entry reserved field is nonzero");
    }
}

const struct recorz_mvp_boot_image *recorz_mvp_image_load(const uint8_t *blob, uint32_t size) {
    uint16_t section_count;
    uint16_t section_index;
    uint32_t feature_flags;
    uint32_t expected_checksum;
    const uint8_t *entry_blob = 0;
    const uint8_t *program_blob = 0;
    const uint8_t *seed_blob = 0;
    uint32_t entry_size = 0U;
    uint32_t program_size = 0U;
    uint32_t seed_size = 0U;
    uint32_t offset = RECORZ_MVP_IMAGE_HEADER_SIZE;

    if (size < RECORZ_MVP_IMAGE_HEADER_SIZE) {
        machine_panic("boot image is too small");
    }
    if (blob[0] != RECORZ_MVP_IMAGE_MAGIC_0 || blob[1] != RECORZ_MVP_IMAGE_MAGIC_1 ||
        blob[2] != RECORZ_MVP_IMAGE_MAGIC_2 || blob[3] != RECORZ_MVP_IMAGE_MAGIC_3) {
        machine_panic("boot image magic mismatch");
    }
    if (read_u16_le(blob + 4U) != RECORZ_MVP_IMAGE_VERSION) {
        machine_panic("boot image version mismatch");
    }

    section_count = read_u16_le(blob + 6U);
    feature_flags = read_u32_le(blob + 8U);
    expected_checksum = read_u32_le(blob + 12U);
    if (section_count == 0U || section_count > RECORZ_MVP_IMAGE_SECTION_LIMIT) {
        machine_panic("boot image section count is invalid");
    }
    if ((feature_flags & ~RECORZ_MVP_IMAGE_KNOWN_FEATURES) != 0U) {
        machine_panic("boot image feature flags are unknown");
    }
    if ((feature_flags & RECORZ_MVP_IMAGE_FEATURE_FNV1A32) == 0U) {
        machine_panic("boot image checksum feature is required");
    }
    if (blob[16U] != RECORZ_MVP_IMAGE_PROFILE_0 || blob[17U] != RECORZ_MVP_IMAGE_PROFILE_1 ||
        blob[18U] != RECORZ_MVP_IMAGE_PROFILE_2 || blob[19U] != RECORZ_MVP_IMAGE_PROFILE_3 ||
        blob[20U] != RECORZ_MVP_IMAGE_PROFILE_4 || blob[21U] != RECORZ_MVP_IMAGE_PROFILE_5 ||
        blob[22U] != RECORZ_MVP_IMAGE_PROFILE_6 || blob[23U] != RECORZ_MVP_IMAGE_PROFILE_7) {
        machine_panic("boot image profile mismatch");
    }
    if (size < RECORZ_MVP_IMAGE_HEADER_SIZE + ((uint32_t)section_count * RECORZ_MVP_IMAGE_SECTION_SIZE)) {
        machine_panic("boot image section table is truncated");
    }
    if (fnv1a32(blob + RECORZ_MVP_IMAGE_HEADER_SIZE, size - RECORZ_MVP_IMAGE_HEADER_SIZE) != expected_checksum) {
        machine_panic("boot image checksum mismatch");
    }

    for (section_index = 0U; section_index < section_count; ++section_index) {
        uint16_t kind = read_u16_le(blob + offset);
        uint32_t section_offset = read_u32_le(blob + offset + 4U);
        uint32_t section_size = read_u32_le(blob + offset + 8U);

        offset += RECORZ_MVP_IMAGE_SECTION_SIZE;
        if (section_offset > size || section_size > size - section_offset) {
            machine_panic("boot image section bounds are invalid");
        }
        if (kind == RECORZ_MVP_IMAGE_SECTION_PROGRAM) {
            if (program_blob != 0) {
                machine_panic("boot image has duplicate program sections");
            }
            program_blob = blob + section_offset;
            program_size = section_size;
            continue;
        }
        if (kind == RECORZ_MVP_IMAGE_SECTION_SEED) {
            if (seed_blob != 0) {
                machine_panic("boot image has duplicate seed sections");
            }
            seed_blob = blob + section_offset;
            seed_size = section_size;
            continue;
        }
        if (kind == RECORZ_MVP_IMAGE_SECTION_ENTRY) {
            if (entry_blob != 0) {
                machine_panic("boot image has duplicate entry sections");
            }
            entry_blob = blob + section_offset;
            entry_size = section_size;
            continue;
        }
        machine_panic("boot image section kind is unknown");
    }

    if (entry_blob == 0 || program_blob == 0 || seed_blob == 0) {
        machine_panic("boot image is missing required sections");
    }

    validate_entry_section(entry_blob, entry_size);
    loaded_image.program = recorz_mvp_program_load(program_blob, program_size);
    loaded_image.seed = recorz_mvp_seed_load(seed_blob, seed_size);
    return &loaded_image;
}
