#ifndef RECORZ_QEMU_RISCV64_PROGRAM_H
#define RECORZ_QEMU_RISCV64_PROGRAM_H

#include <stdint.h>

#include "vm.h"

const struct recorz_mvp_program *recorz_mvp_program_load(const uint8_t *blob, uint32_t size);

#endif
