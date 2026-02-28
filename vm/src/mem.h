#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void  mem_init(void);
void *mem_alloc(size_t size);

/* Minimal libc replacements that GCC may emit calls to */
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);

#endif
