OUTPUT_ARCH(riscv)
ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS
{
  . = 0x80200000;

  .text : ALIGN(0x1000) {
    *(.text.init)
    *(.text*)
  } :text

  .rodata : ALIGN(16) {
    *(.rodata*)
  } :text

  . = ALIGN(0x1000);

  .data : ALIGN(16) {
    *(.sdata*)
    *(.data*)
  } :data

  .bss (NOLOAD) : ALIGN(16) {
    *(.sbss*)
    *(.bss*)
    *(COMMON)
  } :data
}
