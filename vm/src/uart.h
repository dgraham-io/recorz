#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_BASE ((volatile uint8_t *)0x10000000)

void uart_putc(char c);
void uart_puts(const char *s);
void uart_put_int(int32_t n);

#endif
