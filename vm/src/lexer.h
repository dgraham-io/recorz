#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

typedef enum {
    TOK_INTEGER,
    TOK_STRING,
    TOK_IDENTIFIER,
    TOK_ASSIGN,       /* := */
    TOK_DOT,          /* . */
    TOK_LPAREN,       /* ( */
    TOK_RPAREN,       /* ) */
    TOK_BINARY_OP,    /* + - * / < > = etc. */
    TOK_EOF,
    TOK_ERROR,
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   /* pointer into source */
    uint32_t    length;
    int32_t     int_val; /* pre-parsed for TOK_INTEGER */
} Token;

typedef struct {
    const char *source;
    const char *current;
    Token       token;
} Lexer;

void lexer_init(Lexer *lex, const char *source);
void lexer_next(Lexer *lex);

#endif
