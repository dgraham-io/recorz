#include "uart.h"

void uart_putc(char c) {
    *UART_BASE = (uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s)
        uart_putc(*s++);
}

void uart_put_int(int32_t n) {
    if (n < 0) {
        uart_putc('-');
        if (n == (int32_t)0x80000000) {
            uart_puts("2147483648");
            return;
        }
        n = -n;
    }
    if (n >= 10)
        uart_put_int(n / 10);
    uart_putc('0' + (n % 10));
}
