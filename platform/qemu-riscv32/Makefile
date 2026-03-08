ROOT := $(abspath $(CURDIR)/../..)
BUILD_DIR ?= $(ROOT)/misc/qemu-riscv32-serial
TOOLCHAIN_PREFIX ?= riscv64-unknown-elf-
CC := $(TOOLCHAIN_PREFIX)gcc
QEMU ?= qemu-system-riscv32
PYTHON ?= python3

CFLAGS := -march=rv32im -mabi=ilp32 -mcmodel=medany -nostdlib -ffreestanding -O2 -Wall -Wextra -I$(CURDIR) -I$(BUILD_DIR)
LDFLAGS := -T $(CURDIR)/linker.ld

EXAMPLE ?= $(ROOT)/examples/qemu_riscv_fb_demo.rz
IMAGE_SCRIPT := $(ROOT)/tools/build_qemu_riscv_mvp_image.py
GENERATED_BINDINGS_SCRIPT := $(ROOT)/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py
KERNEL_MVP_SOURCES := $(wildcard $(ROOT)/kernel/mvp/*.rz)
RUNTIME_SPEC := $(CURDIR)/runtime_spec.json
GENERATED_IMAGE := $(BUILD_DIR)/demo_image.bin
GENERATED_BINDINGS_HEADER := $(BUILD_DIR)/recorz_mvp_generated_runtime_bindings.h
GENERATED_IMAGE_ASM := $(BUILD_DIR)/demo_image_blob.S
GENERATED_IMAGE_OBJECT := $(BUILD_DIR)/demo_image_blob.o
ELF := $(BUILD_DIR)/recorz-qemu-riscv32-serial.elf

SOURCES := start.S machine.c display.c vm.c program.c seed.c image.c main.c
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SOURCES))) \
	$(patsubst %.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SOURCES))) \
	$(GENERATED_IMAGE_OBJECT)

.PHONY: all run run-headless inspect-image clean FORCE

all: $(ELF)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GENERATED_IMAGE): FORCE $(IMAGE_SCRIPT) $(EXAMPLE) $(KERNEL_MVP_SOURCES) $(RUNTIME_SPEC) $(ROOT)/src/recorz/parser.py $(ROOT)/src/recorz/ast.py $(ROOT)/src/recorz/compiler.py $(ROOT)/src/recorz/model.py | $(BUILD_DIR)
	PYTHONPATH=$(ROOT)/src $(PYTHON) $(IMAGE_SCRIPT) $(EXAMPLE) $(GENERATED_IMAGE)

$(GENERATED_BINDINGS_HEADER): $(GENERATED_BINDINGS_SCRIPT) $(IMAGE_SCRIPT) $(KERNEL_MVP_SOURCES) $(RUNTIME_SPEC) $(ROOT)/src/recorz/parser.py $(ROOT)/src/recorz/ast.py $(ROOT)/src/recorz/compiler.py $(ROOT)/src/recorz/model.py | $(BUILD_DIR)
	PYTHONPATH=$(ROOT)/src $(PYTHON) $(GENERATED_BINDINGS_SCRIPT) $(GENERATED_BINDINGS_HEADER)

$(BUILD_DIR)/%.o: $(CURDIR)/%.c $(CURDIR)/*.h $(GENERATED_BINDINGS_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(CURDIR)/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GENERATED_IMAGE_ASM): $(GENERATED_IMAGE) | $(BUILD_DIR)
	printf '%s\n' \
		'.section .rodata' \
		'.balign 8' \
		'.global recorz_demo_image_blob_start' \
		'.global recorz_demo_image_blob_end' \
		'recorz_demo_image_blob_start:' \
		'.incbin "$(GENERATED_IMAGE)"' \
		'recorz_demo_image_blob_end:' > $@

$(GENERATED_IMAGE_OBJECT): $(GENERATED_IMAGE_ASM)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJECTS) $(CURDIR)/linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

run: $(ELF)
	$(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial mon:stdio -display none

run-headless: $(ELF)
	$(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial stdio -display none

inspect-image: $(GENERATED_IMAGE)
	$(PYTHON) $(ROOT)/tools/inspect_qemu_riscv_mvp_image.py $(GENERATED_IMAGE)

clean:
	rm -rf $(BUILD_DIR)
