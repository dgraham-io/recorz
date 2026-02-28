# ==========================================================================
#   recorz VM — Makefile
#   Bare-metal RV32I build for QEMU virt machine
# ==========================================================================

PREFIX    = riscv64-elf-
AS        = $(PREFIX)as
LD        = $(PREFIX)ld
OBJDUMP   = $(PREFIX)objdump
OBJCOPY   = $(PREFIX)objcopy

ASFLAGS   = -march=rv32i -mabi=ilp32
LDFLAGS   = -T link.ld -m elf32lriscv

BUILDDIR  = build
SRCDIR    = src

TARGET    = $(BUILDDIR)/recorz.elf

SRCS      = $(SRCDIR)/boot.S
OBJS      = $(BUILDDIR)/boot.o

QEMU      = qemu-system-riscv32
QFLAGS    = -machine virt -nographic -bios none -kernel $(TARGET)

# --------------------------------------------------------------------------

.PHONY: all build run debug clean disasm _builddir

all: build

build: $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.S | _builddir
	$(AS) $(ASFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

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
