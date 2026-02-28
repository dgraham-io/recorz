#include "parser.h"
#include "uart.h"

/* ---- helpers ---- */

static void error(Parser *p, const char *msg) {
    if (p->had_error) return; /* suppress cascading errors */
    p->had_error = 1;
    uart_puts("Error: ");
    uart_puts(msg);
    uart_putc('\n');
}

static Token current(Parser *p) {
    return p->lexer.token;
}

static void advance(Parser *p) {
    lexer_next(&p->lexer);
}

static int check(Parser *p, TokenType type) {
    return current(p).type == type;
}

static void expect(Parser *p, TokenType type, const char *msg) {
    if (!check(p, type)) {
        error(p, msg);
        return;
    }
    advance(p);
}

/* ---- name matching ---- */

static int names_equal(const char *a, uint32_t alen, const char *b, uint32_t blen) {
    if (alen != blen) return 0;
    for (uint32_t i = 0; i < alen; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

/* ---- local variable resolution ---- */

static int find_local(Parser *p, const char *name, uint32_t len) {
    for (uint32_t i = 0; i < p->local_count; i++) {
        if (names_equal(p->locals[i].name, p->locals[i].length, name, len))
            return (int)i;
    }
    return -1;
}

static uint8_t resolve_or_add_local(Parser *p, const char *name, uint32_t len) {
    int idx = find_local(p, name, len);
    if (idx >= 0) return (uint8_t)idx;
    if (p->local_count >= MAX_LOCALS) {
        error(p, "too many local variables");
        return 0;
    }
    uint8_t slot = (uint8_t)p->local_count;
    p->locals[p->local_count].name   = name;
    p->locals[p->local_count].length = len;
    p->local_count++;
    return slot;
}

static uint8_t resolve_local(Parser *p, const char *name, uint32_t len) {
    int idx = find_local(p, name, len);
    if (idx < 0) {
        error(p, "undefined variable");
        return 0;
    }
    return (uint8_t)idx;
}

/* ---- selector resolution (hard-coded for step 2) ---- */

static uint8_t resolve_selector(Parser *p, const char *name, uint32_t len) {
    if (names_equal(name, len, "print", 5)) return SEL_PRINT;
    if (names_equal(name, len, "+", 1))     return SEL_PLUS;
    error(p, "unknown selector");
    return 0;
}

/* ---- recursive descent ---- */

static void parse_expression(Parser *p);

static void parse_operand(Parser *p) {
    Token tok = current(p);

    switch (tok.type) {
    case TOK_INTEGER:
        chunk_emit_byte(p->chunk, OP_PUSH_INT);
        chunk_emit_i32(p->chunk, tok.int_val);
        advance(p);
        break;

    case TOK_STRING:
        chunk_emit_byte(p->chunk, OP_PUSH_STR);
        chunk_emit_u16(p->chunk, (uint16_t)tok.length);
        for (uint32_t i = 0; i < tok.length; i++)
            chunk_emit_byte(p->chunk, (uint8_t)tok.start[i]);
        advance(p);
        break;

    case TOK_IDENTIFIER: {
        uint8_t slot = resolve_local(p, tok.start, tok.length);
        chunk_emit_byte(p->chunk, OP_LOAD);
        chunk_emit_byte(p->chunk, slot);
        advance(p);
        break;
    }

    case TOK_LPAREN:
        advance(p); /* consume ( */
        parse_expression(p);
        expect(p, TOK_RPAREN, "expected ')'");
        break;

    default:
        error(p, "expected expression");
        break;
    }
}

static void parse_unary_messages(Parser *p) {
    while (check(p, TOK_IDENTIFIER)) {
        Token tok = current(p);
        uint8_t sel = resolve_selector(p, tok.start, tok.length);
        advance(p);
        chunk_emit_byte(p->chunk, OP_SEND_UNARY);
        chunk_emit_byte(p->chunk, sel);
    }
}

static void parse_binary_messages(Parser *p) {
    while (check(p, TOK_BINARY_OP)) {
        Token op = current(p);
        advance(p); /* consume operator */

        parse_operand(p);
        parse_unary_messages(p);

        uint8_t sel = resolve_selector(p, op.start, op.length);
        chunk_emit_byte(p->chunk, OP_SEND_BINARY);
        chunk_emit_byte(p->chunk, sel);
    }
}

static void parse_expression(Parser *p) {
    parse_operand(p);
    parse_unary_messages(p);
    parse_binary_messages(p);
}

static void parse_statement(Parser *p) {
    /* Check for assignment: identifier followed by := */
    if (check(p, TOK_IDENTIFIER)) {
        /* Save lexer state for potential backtrack */
        Token saved_tok = current(p);
        Lexer saved_lex = p->lexer;

        advance(p); /* consume identifier */
        if (check(p, TOK_ASSIGN)) {
            /* It's an assignment */
            uint8_t slot = resolve_or_add_local(p, saved_tok.start, saved_tok.length);
            advance(p); /* consume := */
            parse_expression(p);
            chunk_emit_byte(p->chunk, OP_STORE);
            chunk_emit_byte(p->chunk, slot);
            return;
        }
        /* Not an assignment — backtrack */
        p->lexer = saved_lex;
    }

    /* Bare expression — discard result */
    parse_expression(p);
    chunk_emit_byte(p->chunk, OP_POP);
}

/* ---- public API ---- */

void parser_init(Parser *p, const char *source, Chunk *chunk) {
    lexer_init(&p->lexer, source);
    p->chunk       = chunk;
    p->local_count = 0;
    p->had_error   = 0;
}

void parse_program(Parser *p) {
    while (!check(p, TOK_EOF) && !p->had_error) {
        parse_statement(p);
        if (!check(p, TOK_EOF))
            expect(p, TOK_DOT, "expected '.'");
    }
    chunk_emit_byte(p->chunk, OP_HALT);
}
