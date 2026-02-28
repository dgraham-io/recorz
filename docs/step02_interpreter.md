# Step 2: Bare-Metal Bytecode Interpreter

## What was built

A complete interpreter pipeline running bare-metal on RISC-V (RV32I) in QEMU:
**source text** → **lexer** → **parser** → **bytecodes** → **stack-based VM** → **UART output**

The test program:
```
x := 3 + 4.
x print.
'Hello, recorz!' print.
```

Produces:
```
7
Hello, recorz!
```

## Architecture

### Boot sequence
`boot.S` → set stack pointer → zero .bss → call C `main()` → halt

### Object representation (tagged 32-bit values)

| Bit pattern | Type | Encoding |
|-------------|------|----------|
| `xxxxxxx1` | SmallInteger | Value in bits [31:1], arithmetic shift to extract |
| `00000000` | nil | Zero |
| `xxxxxxx0` (non-zero) | Heap object | Pointer to ObjHeader |

### Bytecode set

| Opcode | Byte | Operand | Stack effect |
|--------|------|---------|-------------|
| PUSH_INT | 0x01 | i32 (4B LE) | +1 |
| PUSH_STR | 0x02 | u16 len + raw bytes | +1 |
| PUSH_NIL | 0x03 | — | +1 |
| POP | 0x04 | — | -1 |
| LOAD | 0x05 | u8 slot | +1 |
| STORE | 0x06 | u8 slot | -1 |
| SEND_UNARY | 0x10 | u8 selector_id | 0 |
| SEND_BINARY | 0x11 | u8 selector_id | -1 |
| HALT | 0xFF | — | — |

### Parser grammar (step 2 subset)
```
Program        = { Statement "." }
Statement      = [ Identifier ":=" ] Expression
Expression     = Operand UnaryMessages BinaryMessages
UnaryMessages  = { Identifier }
BinaryMessages = { BinaryOp Operand UnaryMessages }
Operand        = Integer | String | Identifier | "(" Expression ")"
```

### Memory layout
```
0x80000000  .text → .rodata → .data → .bss → heap↑ ... stack↓  0x80200000
```

## Files

| File | Purpose |
|------|---------|
| `vm/src/boot.S` | Entry point: stack setup, .bss zero, call main |
| `vm/src/main.c` | Embeds source, runs lex→parse→VM pipeline |
| `vm/src/uart.h/c` | Bare-metal UART output |
| `vm/src/mem.h/c` | Bump allocator + memset/memcpy/memmove stubs |
| `vm/src/object.h/c` | Tagged values, SmallInteger, String, print/add |
| `vm/src/bytecode.h/c` | Opcodes, Chunk, emit helpers |
| `vm/src/lexer.h/c` | Tokenizer for recorz subset |
| `vm/src/parser.h/c` | Recursive descent, one-pass bytecode emitter |
| `vm/src/vm.h/c` | Bytecode interpreter loop |
| `vm/link.ld` | Linker script with heap symbol |
| `vm/Makefile` | C cross-compilation for RV32I |

## Design decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Implementation language | C (cross-compiled for RV32I) | Practical for iteration; hot loop can move to assembly later |
| Compilation strategy | One-pass (parser emits bytecodes directly) | No AST needed; simpler, less memory |
| Memory allocator | Bump (no free) | Program runs once and halts; GC deferred |
| Selector dispatch | Hard-coded switch | Only `print` and `+` needed; dynamic dispatch deferred |
| Division support | Link libgcc (software division) | RV32I has no hardware divide; keeps ISA minimal for FPGA |

## Build and run

```bash
cd vm
make build    # Compile all C + assembly → build/recorz.elf
make run      # Run in QEMU (Ctrl-A X to quit)
make disasm   # Show full disassembly
make clean    # Remove build artifacts
```
