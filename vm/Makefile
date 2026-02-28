# ==========================================================================
#   recorz VM — Makefile
#   Bare-metal RV32I build for QEMU virt machine
# ==========================================================================

PREFIX    = riscv64-elf-
CC        = $(PREFIX)gcc
AS        = $(PREFIX)as
LD        = $(PREFIX)ld
OBJDUMP   = $(PREFIX)objdump
OBJCOPY   = $(PREFIX)objcopy

CFLAGS    = -march=rv32i -mabi=ilp32 -ffreestanding -nostdlib \
            -O2 -Wall -Wextra -Werror \
            -fno-builtin -fno-stack-protector \
            -I$(SRCDIR)
ASFLAGS   = -march=rv32i -mabi=ilp32
LIBGCC    = $(shell $(CC) -march=rv32i -mabi=ilp32 -print-libgcc-file-name)
LDFLAGS   = -T link.ld -m elf32lriscv -nostdlib

BUILDDIR  = build
SRCDIR    = src

TARGET    = $(BUILDDIR)/recorz.elf

ASM_SRCS  = $(SRCDIR)/boot.S
C_SRCS    = $(wildcard $(SRCDIR)/*.c)

ASM_OBJS  = $(patsubst $(SRCDIR)/%.S,$(BUILDDIR)/%.o,$(ASM_SRCS))
C_OBJS    = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(C_SRCS))
OBJS      = $(ASM_OBJS) $(C_OBJS)

QEMU      = qemu-system-riscv32
QFLAGS    = -machine virt -nographic -bios none -kernel $(TARGET)

# --------------------------------------------------------------------------

.PHONY: all build run debug clean disasm _builddir

all: build

build: $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.S | _builddir
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | _builddir
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBGCC)

_builddir:
	mkdir -p $(BUILDDIR)

run: build
	$(QEMU) $(QFLAGS)

debug: build
	$(QEMU) $(QFLAGS) -s -S

disasm: build
	$(OBJDUMP) -d $(TARGET)

clean:
	rm -rf $(BUILDDIR)
