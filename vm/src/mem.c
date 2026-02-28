#include "mem.h"
#include "uart.h"

/* Linker-defined symbols — we only use their addresses */
extern char __heap_start[];
extern char _stack_top[];

static char *heap_ptr;
static char *heap_limit;

void mem_init(void) {
    heap_ptr   = (char *)__heap_start;
    heap_limit = (char *)((uintptr_t)_stack_top - 4096); /* safety margin */
}

void *mem_alloc(size_t size) {
    /* Align to 4 bytes */
    size = (size + 3) & ~(size_t)3;
    if (heap_ptr + size > heap_limit) {
        uart_puts("FATAL: out of memory\n");
        for (;;) __asm__ volatile("wfi");
    }
    void *p = heap_ptr;
    heap_ptr += size;
    return p;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    while (n--)
        *p++ = (uint8_t)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--)
        *d++ = *s++;
    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dest;
}
