# ==========================================================================
#   recorz VM — boot.S
#   Bare-metal entry point for RISC-V 32-bit (RV32I)
#   Target: QEMU virt machine
#
#   UART (ns16550a) at 0x10000000
#   RAM starts at     0x80000000
# ==========================================================================

    .section .text
    .globl _start

_start:
    # Set up stack pointer (2MB above RAM base)
    lui     sp, %hi(_stack_top)
    addi    sp, sp, %lo(_stack_top)

    # Load UART base address: 0x10000000
    lui     t0, 0x10000

    # Load address of the hello string
    la      t1, hello_msg

print_loop:
    lb      t2, 0(t1)              # Load next byte from string
    beqz    t2, done               # If null terminator, we're done
    sb      t2, 0(t0)              # Write byte to UART THR
    addi    t1, t1, 1              # Advance string pointer
    j       print_loop             # Next character

done:
    # Halt: loop with wait-for-interrupt
halt:
    wfi
    j       halt

    .section .rodata
hello_msg:
    .asciz  "Hello, recorz!\n"
