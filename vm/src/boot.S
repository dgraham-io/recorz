# ==========================================================================
#   recorz VM — boot.S
#   Bare-metal entry point for RISC-V 32-bit (RV32I)
#   Target: QEMU virt machine
#
#   Sets up stack, zeroes .bss, calls C main()
# ==========================================================================

    .section .text
    .globl _start

_start:
    # Set up stack pointer (top of 2MB RAM region)
    lui     sp, %hi(_stack_top)
    addi    sp, sp, %lo(_stack_top)

    # Zero the .bss section (required for C globals)
    la      t0, __bss_start
    la      t1, __bss_end
.Lbss_loop:
    bge     t0, t1, .Lbss_done
    sw      zero, 0(t0)
    addi    t0, t0, 4
    j       .Lbss_loop
.Lbss_done:

    # Call C entry point
    call    main

    # Halt after main returns
.Lhalt:
    wfi
    j       .Lhalt
