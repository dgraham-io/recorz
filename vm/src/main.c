#include "uart.h"
#include "mem.h"
#include "bytecode.h"
#include "parser.h"
#include "vm.h"

static const char *source =
    "x := 3 + 4.\n"
    "x print.\n"
    "'Hello, recorz!' print.\n";

static Chunk  chunk;
static VM     vm;

void main(void) {
    mem_init();

    /* Compile */
    chunk_init(&chunk);
    Parser parser;
    parser_init(&parser, source, &chunk);
    parse_program(&parser);

    if (parser.had_error) {
        uart_puts("Compilation failed.\n");
        return;
    }

    /* Execute */
    vm_init(&vm, &chunk);
    vm_run(&vm);
}
