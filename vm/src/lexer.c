#include "lexer.h"

static int is_digit(char c)  { return c >= '0' && c <= '9'; }
static int is_letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int is_alnum(char c)  { return is_digit(c) || is_letter(c); }

static int is_binary_char(char c) {
    switch (c) {
    case '~': case '!': case '@': case '%': case '&': case '*':
    case '-': case '+': case '=': case '|': case '\\': case '<':
    case '>': case ',': case '?': case '/':
        return 1;
    default:
        return 0;
    }
}

static void skip_whitespace_and_comments(Lexer *lex) {
    for (;;) {
        char c = *lex->current;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            lex->current++;
        } else if (c == '"') {
            /* Smalltalk comment: skip to closing " */
            lex->current++;
            while (*lex->current && *lex->current != '"')
                lex->current++;
            if (*lex->current == '"')
                lex->current++;
        } else {
            return;
        }
    }
}

static Token make_token(Lexer *lex, TokenType type, const char *start, uint32_t len) {
    Token t;
    t.type    = type;
    t.start   = start;
    t.length  = len;
    t.int_val = 0;
    (void)lex;
    return t;
}

void lexer_init(Lexer *lex, const char *source) {
    lex->source  = source;
    lex->current = source;
    lexer_next(lex); /* prime the first token */
}

void lexer_next(Lexer *lex) {
    skip_whitespace_and_comments(lex);

    const char *start = lex->current;
    char c = *lex->current;

    if (c == '\0') {
        lex->token = make_token(lex, TOK_EOF, start, 0);
        return;
    }

    /* Integer literal */
    if (is_digit(c)) {
        int32_t val = 0;
        while (is_digit(*lex->current)) {
            val = val * 10 + (*lex->current - '0');
            lex->current++;
        }
        lex->token = make_token(lex, TOK_INTEGER, start, (uint32_t)(lex->current - start));
        lex->token.int_val = val;
        return;
    }

    /* String literal: 'text' with '' for embedded quote */
    if (c == '\'') {
        lex->current++; /* skip opening quote */
        const char *str_start = lex->current;
        while (*lex->current) {
            if (*lex->current == '\'') {
                if (*(lex->current + 1) == '\'') {
                    lex->current += 2; /* escaped quote */
                } else {
                    break; /* closing quote */
                }
            } else {
                lex->current++;
            }
        }
        uint32_t len = (uint32_t)(lex->current - str_start);
        if (*lex->current == '\'')
            lex->current++; /* skip closing quote */
        lex->token = make_token(lex, TOK_STRING, str_start, len);
        return;
    }

    /* Identifier or keyword */
    if (is_letter(c) || c == '_') {
        while (is_alnum(*lex->current) || *lex->current == '_')
            lex->current++;
        lex->token = make_token(lex, TOK_IDENTIFIER, start, (uint32_t)(lex->current - start));
        return;
    }

    /* Assignment := */
    if (c == ':' && *(lex->current + 1) == '=') {
        lex->current += 2;
        lex->token = make_token(lex, TOK_ASSIGN, start, 2);
        return;
    }

    /* Period */
    if (c == '.') {
        lex->current++;
        lex->token = make_token(lex, TOK_DOT, start, 1);
        return;
    }

    /* Parentheses */
    if (c == '(') {
        lex->current++;
        lex->token = make_token(lex, TOK_LPAREN, start, 1);
        return;
    }
    if (c == ')') {
        lex->current++;
        lex->token = make_token(lex, TOK_RPAREN, start, 1);
        return;
    }

    /* Binary selector: 1 or 2 binary chars */
    if (is_binary_char(c)) {
        lex->current++;
        if (is_binary_char(*lex->current))
            lex->current++;
        lex->token = make_token(lex, TOK_BINARY_OP, start, (uint32_t)(lex->current - start));
        return;
    }

    /* Unknown character */
    lex->current++;
    lex->token = make_token(lex, TOK_ERROR, start, 1);
}
