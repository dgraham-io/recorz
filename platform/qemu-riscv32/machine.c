#include "machine.h"

#include <stdint.h>

#define UART0_DEFAULT_BASE 0x10000000UL
#define UART_TXDATA_OFFSET 0x00U
#define UART_TXFULL_MASK 0x80000000U

static uintptr_t uart_base = UART0_DEFAULT_BASE;
static machine_panic_hook current_panic_hook = 0;

void machine_init(const void *fdt) {
    (void)fdt;
}

void machine_putc(char c) {
    volatile uint32_t *txdata = (volatile uint32_t *)(uart_base + UART_TXDATA_OFFSET);

    while ((*txdata & UART_TXFULL_MASK) != 0U) {
    }
    *txdata = (uint32_t)(uint8_t)c;
}

void machine_puts(const char *text) {
    while (*text != '\0') {
        if (*text == '\n') {
            machine_putc('\r');
        }
        machine_putc(*text);
        ++text;
    }
}

void machine_wait_forever(void) {
    while (1) {
        __asm__ volatile("wfi");
    }
}

void machine_set_panic_hook(machine_panic_hook hook) {
    current_panic_hook = hook;
}

void machine_panic(const char *message) {
    if (current_panic_hook != 0) {
        current_panic_hook(message);
    }
    machine_puts("panic: ");
    machine_puts(message);
    machine_puts("\n");
    machine_wait_forever();
}

void machine_ramfb_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t stride) {
    (void)framebuffer;
    (void)width;
    (void)height;
    (void)stride;
}

uint32_t machine_fw_cfg_try_read_file(const char *target, void *buffer, uint32_t buffer_size) {
    (void)target;
    (void)buffer;
    (void)buffer_size;
    return 0U;
}
