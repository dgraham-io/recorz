#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stddef.h>

/* A Value is a tagged 32-bit word */
typedef uint32_t Value;

/* --- SmallInteger (tag bit 0 = 1) --- */
#define VAL_IS_INT(v)    ((v) & 1)
#define VAL_TO_INT(v)    ((int32_t)(v) >> 1)
#define INT_TO_VAL(n)    (((uint32_t)(n) << 1) | 1)

/* --- Nil --- */
#define VAL_NIL          ((Value)0)
#define VAL_IS_NIL(v)    ((v) == 0)

/* --- Heap objects (tag bit 0 = 0, non-null) --- */
#define VAL_IS_OBJ(v)    (((v) & 1) == 0 && (v) != 0)
#define VAL_TO_OBJ(v)    ((ObjHeader *)(uintptr_t)(v))
#define OBJ_TO_VAL(p)    ((Value)(uintptr_t)(p))

/* Object types */
typedef enum {
    OBJ_STRING,
} ObjType;

/* Common header for all heap objects */
typedef struct {
    ObjType type;
} ObjHeader;

/* String object: header + length + inline char data */
typedef struct {
    ObjHeader header;
    uint32_t  length;
    char      data[];   /* flexible array member */
} ObjString;

/* Constructors */
Value val_new_string(const char *src, uint32_t len);

/* Operations */
void  val_print(Value v);
Value val_add(Value a, Value b);

#endif
