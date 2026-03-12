#ifndef RECORZ_QEMU_RISCV64_MACHINE_H
#define RECORZ_QEMU_RISCV64_MACHINE_H

#include <stdint.h>

typedef void (*machine_panic_hook)(const char *message);

void machine_init(const void *fdt);
void machine_putc(char c);
uint8_t machine_try_getc(char *out);
char machine_wait_getc(void);
void machine_discard_pending_input(void);
void machine_puts(const char *text);
void machine_wait_forever(void);
void machine_shutdown(void);
void machine_set_panic_hook(machine_panic_hook hook);
void machine_panic(const char *message);
void machine_ramfb_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t stride);
uint32_t machine_fw_cfg_try_read_file(const char *target, void *buffer, uint32_t buffer_size);

#endif
