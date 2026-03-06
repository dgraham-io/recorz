#ifndef RECORZ_QEMU_RISCV64_SEED_H
#define RECORZ_QEMU_RISCV64_SEED_H

#include <stdint.h>

#include "vm.h"

const struct recorz_mvp_seed *recorz_mvp_seed_load(const uint8_t *blob, uint32_t size);

#endif
