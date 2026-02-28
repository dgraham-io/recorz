#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "bytecode.h"

#define MAX_LOCALS 256

typedef struct {
    const char *name;
    uint32_t    length;
} Local;

typedef struct {
    Lexer    lexer;
    Chunk   *chunk;
    Local    locals[MAX_LOCALS];
    uint32_t local_count;
    int      had_error;
} Parser;

void parser_init(Parser *p, const char *source, Chunk *chunk);
void parse_program(Parser *p);

#endif
