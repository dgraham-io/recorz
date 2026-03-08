.section .text.init
.globl _start

_start:
    la sp, _stack_top
    mv a0, a1
    call main

1:
    wfi
    j 1b

.section .bss.stack,"aw",@nobits
.align 16
_stack:
    .space 8192
_stack_top:
