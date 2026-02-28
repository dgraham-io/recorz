#ifndef VM_H
#define VM_H

#include "bytecode.h"
#include "object.h"

#define VM_STACK_MAX  256
#define VM_LOCALS_MAX 256

typedef struct {
    Chunk   *chunk;
    uint32_t ip;
    Value    stack[VM_STACK_MAX];
    uint32_t sp;
    Value    locals[VM_LOCALS_MAX];
} VM;

void vm_init(VM *vm, Chunk *chunk);
void vm_run(VM *vm);

#endif
