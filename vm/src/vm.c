#include "vm.h"
#include "object.h"
#include "uart.h"

void vm_init(VM *vm, Chunk *chunk) {
    vm->chunk = chunk;
    vm->ip    = 0;
    vm->sp    = 0;
    for (uint32_t i = 0; i < VM_LOCALS_MAX; i++)
        vm->locals[i] = VAL_NIL;
}

static inline uint8_t read_u8(VM *vm) {
    return vm->chunk->code[vm->ip++];
}

static inline uint16_t read_u16(VM *vm) {
    uint16_t lo = vm->chunk->code[vm->ip++];
    uint16_t hi = vm->chunk->code[vm->ip++];
    return lo | (uint16_t)(hi << 8);
}

static inline int32_t read_i32(VM *vm) {
    uint32_t b0 = vm->chunk->code[vm->ip++];
    uint32_t b1 = vm->chunk->code[vm->ip++];
    uint32_t b2 = vm->chunk->code[vm->ip++];
    uint32_t b3 = vm->chunk->code[vm->ip++];
    return (int32_t)(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

static inline void push(VM *vm, Value v) {
    if (vm->sp >= VM_STACK_MAX) {
        uart_puts("FATAL: stack overflow\n");
        for (;;) __asm__ volatile("wfi");
    }
    vm->stack[vm->sp++] = v;
}

static inline Value pop(VM *vm) {
    if (vm->sp == 0) {
        uart_puts("FATAL: stack underflow\n");
        for (;;) __asm__ volatile("wfi");
    }
    return vm->stack[--vm->sp];
}

void vm_run(VM *vm) {
    for (;;) {
        uint8_t op = read_u8(vm);
        switch (op) {

        case OP_PUSH_INT: {
            int32_t n = read_i32(vm);
            push(vm, INT_TO_VAL(n));
            break;
        }

        case OP_PUSH_STR: {
            uint16_t len = read_u16(vm);
            const char *data = (const char *)&vm->chunk->code[vm->ip];
            vm->ip += len;
            push(vm, val_new_string(data, len));
            break;
        }

        case OP_PUSH_NIL:
            push(vm, VAL_NIL);
            break;

        case OP_POP:
            pop(vm);
            break;

        case OP_LOAD: {
            uint8_t slot = read_u8(vm);
            push(vm, vm->locals[slot]);
            break;
        }

        case OP_STORE: {
            uint8_t slot = read_u8(vm);
            vm->locals[slot] = pop(vm);
            break;
        }

        case OP_SEND_UNARY: {
            uint8_t sel = read_u8(vm);
            Value receiver = pop(vm);
            switch (sel) {
            case SEL_PRINT:
                val_print(receiver);
                push(vm, receiver);
                break;
            default:
                uart_puts("FATAL: unknown unary selector\n");
                return;
            }
            break;
        }

        case OP_SEND_BINARY: {
            uint8_t sel = read_u8(vm);
            Value arg = pop(vm);
            Value receiver = pop(vm);
            switch (sel) {
            case SEL_PLUS:
                push(vm, val_add(receiver, arg));
                break;
            default:
                uart_puts("FATAL: unknown binary selector\n");
                return;
            }
            break;
        }

        case OP_HALT:
            return;

        default:
            uart_puts("FATAL: unknown opcode\n");
            return;
        }
    }
}
