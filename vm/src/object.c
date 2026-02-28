#include "object.h"
#include "uart.h"
#include "mem.h"

Value val_new_string(const char *src, uint32_t len) {
    ObjString *s = mem_alloc(sizeof(ObjString) + len + 1);
    s->header.type = OBJ_STRING;
    s->length = len;
    for (uint32_t i = 0; i < len; i++)
        s->data[i] = src[i];
    s->data[len] = '\0';
    return OBJ_TO_VAL(s);
}

void val_print(Value v) {
    if (VAL_IS_INT(v)) {
        uart_put_int(VAL_TO_INT(v));
    } else if (VAL_IS_NIL(v)) {
        uart_puts("nil");
    } else if (VAL_IS_OBJ(v)) {
        ObjHeader *obj = VAL_TO_OBJ(v);
        if (obj->type == OBJ_STRING) {
            ObjString *s = (ObjString *)obj;
            for (uint32_t i = 0; i < s->length; i++)
                uart_putc(s->data[i]);
        }
    }
    uart_putc('\n');
}

Value val_add(Value a, Value b) {
    if (VAL_IS_INT(a) && VAL_IS_INT(b))
        return INT_TO_VAL(VAL_TO_INT(a) + VAL_TO_INT(b));
    /* Type error — return nil */
    return VAL_NIL;
}
