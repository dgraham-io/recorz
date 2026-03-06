#ifndef RECORZ_QEMU_RISCV64_IMAGE_H
#define RECORZ_QEMU_RISCV64_IMAGE_H

#include <stdint.h>

#include "vm.h"

struct recorz_mvp_boot_image {
    const struct recorz_mvp_program *program;
    const struct recorz_mvp_seed *seed;
};

const struct recorz_mvp_boot_image *recorz_mvp_image_load(const uint8_t *blob, uint32_t size);

#endif
