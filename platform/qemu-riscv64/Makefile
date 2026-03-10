ROOT := $(abspath $(CURDIR)/../..)
BUILD_DIR ?= $(ROOT)/misc/qemu-riscv64-mvp
TOOLCHAIN_PREFIX ?= riscv64-unknown-elf-
CC := $(TOOLCHAIN_PREFIX)gcc
QEMU ?= qemu-system-riscv64
PYTHON ?= python3
UPDATE_PAYLOAD ?=
UPDATE_FW_CFG_NAME ?= opt/recorz-method-update
FILE_IN_PAYLOAD ?=
FILE_IN_PAYLOADS ?=
KERNEL_SOURCE_BUNDLE ?=
FILE_IN_FW_CFG_NAME ?= opt/recorz-file-in
SNAPSHOT_PAYLOAD ?=
SNAPSHOT_FW_CFG_NAME ?= opt/recorz-snapshot
SNAPSHOT_OUTPUT ?= $(BUILD_DIR)/recorz-live-snapshot.bin
SNAPSHOT_TEMP_OUTPUT ?= $(SNAPSHOT_OUTPUT).tmp
QEMU_EXTRA_ARGS ?=
QEMU_WINDOW_INPUT_ARGS ?= -device virtio-keyboard-device
comma := ,
PYTHON_KERNEL_SOURCE_ENV := $(if $(KERNEL_SOURCE_BUNDLE),RECORZ_MVP_KERNEL_SOURCE_BUNDLE=$(KERNEL_SOURCE_BUNDLE))
QEMU_UPDATE_ARGS := $(if $(UPDATE_PAYLOAD),-fw_cfg name=$(UPDATE_FW_CFG_NAME)$(comma)file=$(UPDATE_PAYLOAD))
GENERATED_FILE_IN_PAYLOAD := $(BUILD_DIR)/external_file_in_payload.rz
FILE_IN_EFFECTIVE_PAYLOAD := $(if $(strip $(FILE_IN_PAYLOADS)),$(GENERATED_FILE_IN_PAYLOAD),$(FILE_IN_PAYLOAD))
QEMU_FILE_IN_ARGS := $(if $(FILE_IN_EFFECTIVE_PAYLOAD),-fw_cfg name=$(FILE_IN_FW_CFG_NAME)$(comma)file=$(FILE_IN_EFFECTIVE_PAYLOAD))
QEMU_SNAPSHOT_ARGS := $(if $(SNAPSHOT_PAYLOAD),-fw_cfg name=$(SNAPSHOT_FW_CFG_NAME)$(comma)file=$(SNAPSHOT_PAYLOAD))
QEMU_RUN_ARGS := $(QEMU_UPDATE_ARGS) $(QEMU_FILE_IN_ARGS) $(QEMU_SNAPSHOT_ARGS) $(QEMU_EXTRA_ARGS)
CONTINUE_SNAPSHOT_OUTPUT := $(SNAPSHOT_PAYLOAD)
CONTINUE_SNAPSHOT_TEMP_OUTPUT := $(CONTINUE_SNAPSHOT_OUTPUT).tmp
CONTINUE_SNAPSHOT_BACKUP_OUTPUT := $(CONTINUE_SNAPSHOT_OUTPUT).bak
QEMU_FILE_IN_DEP := $(if $(strip $(FILE_IN_PAYLOADS)),$(GENERATED_FILE_IN_PAYLOAD))
DEV_SNAPSHOT ?= $(ROOT)/misc/qemu-riscv64-dev/live.bin
DEV_SNAPSHOT_BACKUP ?= $(DEV_SNAPSHOT).bak
DEV_BOOT_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_image_first_boot.rz
DEV_SAVE_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_image_first_save.rz

CFLAGS := -march=rv64gc -mabi=lp64d -mcmodel=medany -nostdlib -ffreestanding -O2 -Wall -Wextra -I$(CURDIR) -I$(BUILD_DIR)
LDFLAGS := -T $(CURDIR)/linker.ld

EXAMPLE ?= $(ROOT)/examples/qemu_riscv_fb_demo.rz
IMAGE_SCRIPT := $(ROOT)/tools/build_qemu_riscv_mvp_image.py
GENERATED_BINDINGS_SCRIPT := $(ROOT)/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py
KERNEL_MVP_SOURCES := $(wildcard $(ROOT)/kernel/mvp/*.rz)
GENERATED_IMAGE := $(BUILD_DIR)/demo_image.bin
GENERATED_BINDINGS_HEADER := $(BUILD_DIR)/recorz_mvp_generated_runtime_bindings.h
GENERATED_IMAGE_ASM := $(BUILD_DIR)/demo_image_blob.S
GENERATED_IMAGE_OBJECT := $(BUILD_DIR)/demo_image_blob.o
ELF := $(BUILD_DIR)/recorz-qemu-riscv64-mvp.elf
PPM := $(BUILD_DIR)/recorz-qemu-riscv64-mvp.ppm
MONITOR_SOCK := $(BUILD_DIR)/monitor.sock
QEMU_LOG := $(BUILD_DIR)/qemu.log
QEMU_PID := $(BUILD_DIR)/qemu.pid
QEMU_INTERACTIVE_CHARDEV := -chardev stdio,id=recorzio,signal=off,logfile=$(QEMU_LOG),logappend=off -serial chardev:recorzio

SOURCES := start.S machine.c display.c vm.c program.c seed.c image.c main.c
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SOURCES))) \
	$(patsubst %.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SOURCES))) \
	$(GENERATED_IMAGE_OBJECT)

.PHONY: all run run-headless screenshot save-snapshot continue-snapshot continue-snapshot-interactive dev-init dev-boot dev-interactive dev-screenshot dev-file-in dev-reset dev-restore inspect-image clean FORCE

all: $(ELF)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GENERATED_IMAGE): FORCE $(IMAGE_SCRIPT) $(EXAMPLE) $(KERNEL_MVP_SOURCES) $(ROOT)/platform/qemu-riscv64/runtime_spec.json $(ROOT)/src/recorz/parser.py $(ROOT)/src/recorz/ast.py $(ROOT)/src/recorz/compiler.py $(ROOT)/src/recorz/model.py | $(BUILD_DIR)
	$(PYTHON_KERNEL_SOURCE_ENV) PYTHONPATH=$(ROOT)/src $(PYTHON) $(IMAGE_SCRIPT) $(EXAMPLE) $(GENERATED_IMAGE)

$(GENERATED_BINDINGS_HEADER): $(GENERATED_BINDINGS_SCRIPT) $(IMAGE_SCRIPT) $(KERNEL_MVP_SOURCES) $(ROOT)/platform/qemu-riscv64/runtime_spec.json $(ROOT)/src/recorz/parser.py $(ROOT)/src/recorz/ast.py $(ROOT)/src/recorz/compiler.py $(ROOT)/src/recorz/model.py | $(BUILD_DIR)
	$(PYTHON_KERNEL_SOURCE_ENV) PYTHONPATH=$(ROOT)/src $(PYTHON) $(GENERATED_BINDINGS_SCRIPT) $(GENERATED_BINDINGS_HEADER)

$(GENERATED_FILE_IN_PAYLOAD): $(FILE_IN_PAYLOADS) | $(BUILD_DIR)
	@rm -f $@
	@first=1; \
	for path in $(FILE_IN_PAYLOADS); do \
		if [ $$first -eq 0 ]; then printf '\n!\n' >> $@; fi; \
		cat $$path >> $@; \
		first=0; \
	done

$(BUILD_DIR)/%.o: $(CURDIR)/%.c $(CURDIR)/*.h $(ROOT)/platform/shared/*.h $(GENERATED_BINDINGS_HEADER) | $(BUILD_DIR)
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

run: $(ELF) $(QEMU_FILE_IN_DEP)
	$(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial mon:stdio -device ramfb $(QEMU_WINDOW_INPUT_ARGS) $(QEMU_RUN_ARGS)

run-headless: $(ELF) $(QEMU_FILE_IN_DEP)
	$(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial stdio -display none -device ramfb $(QEMU_RUN_ARGS)

screenshot: $(ELF) $(QEMU_FILE_IN_DEP)
	rm -f $(MONITOR_SOCK) $(PPM) $(QEMU_PID)
	( $(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial stdio -display none -device ramfb $(QEMU_RUN_ARGS) -monitor unix:$(MONITOR_SOCK),server,nowait >$(BUILD_DIR)/qemu.log 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/qemu_hmp_screendump.py $(MONITOR_SOCK) $(PPM) $(QEMU_LOG) "recorz qemu-riscv64 mvp: rendered"
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@echo "wrote $(PPM)"

save-snapshot: $(ELF) $(QEMU_FILE_IN_DEP)
	rm -f $(BUILD_DIR)/qemu.log $(QEMU_PID) $(SNAPSHOT_TEMP_OUTPUT)
	( $(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial stdio -display none -device ramfb $(QEMU_RUN_ARGS) >$(BUILD_DIR)/qemu.log 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_snapshot.py $(BUILD_DIR)/qemu.log $(SNAPSHOT_TEMP_OUTPUT)
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@mkdir -p $(dir $(SNAPSHOT_OUTPUT))
	@mv $(SNAPSHOT_TEMP_OUTPUT) $(SNAPSHOT_OUTPUT)
	@echo "wrote $(SNAPSHOT_OUTPUT)"

continue-snapshot: $(ELF) $(QEMU_FILE_IN_DEP)
	@test -n "$(SNAPSHOT_PAYLOAD)" || (echo "SNAPSHOT_PAYLOAD is required for continue-snapshot" >&2; exit 1)
	rm -f $(BUILD_DIR)/qemu.log $(QEMU_PID) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT)
	( $(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) -serial stdio -display none -device ramfb $(QEMU_RUN_ARGS) >$(BUILD_DIR)/qemu.log 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_snapshot.py $(BUILD_DIR)/qemu.log $(CONTINUE_SNAPSHOT_TEMP_OUTPUT)
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@mkdir -p $(dir $(CONTINUE_SNAPSHOT_OUTPUT))
	@if [ -f "$(CONTINUE_SNAPSHOT_OUTPUT)" ]; then cp "$(CONTINUE_SNAPSHOT_OUTPUT)" "$(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; echo "backed up $(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; fi
	@mv $(CONTINUE_SNAPSHOT_TEMP_OUTPUT) $(CONTINUE_SNAPSHOT_OUTPUT)
	@echo "wrote $(CONTINUE_SNAPSHOT_OUTPUT)"

continue-snapshot-interactive: $(ELF) $(QEMU_FILE_IN_DEP)
	@test -n "$(SNAPSHOT_PAYLOAD)" || (echo "SNAPSHOT_PAYLOAD is required for continue-snapshot-interactive" >&2; exit 1)
	rm -f $(QEMU_LOG) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT)
	@$(QEMU) -machine virt -m 128M -smp 1 -kernel $(ELF) $(QEMU_INTERACTIVE_CHARDEV) -monitor none -device ramfb $(QEMU_WINDOW_INPUT_ARGS) $(QEMU_RUN_ARGS); \
	STATUS=$$?; \
	if grep -q 'recorz-snapshot-end' $(QEMU_LOG); then \
		$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_snapshot.py --timeout 1 $(QEMU_LOG) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT); \
		mkdir -p $(dir $(CONTINUE_SNAPSHOT_OUTPUT)); \
		if [ -f "$(CONTINUE_SNAPSHOT_OUTPUT)" ]; then cp "$(CONTINUE_SNAPSHOT_OUTPUT)" "$(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; echo "backed up $(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; fi; \
		mv $(CONTINUE_SNAPSHOT_TEMP_OUTPUT) $(CONTINUE_SNAPSHOT_OUTPUT); \
		echo "wrote $(CONTINUE_SNAPSHOT_OUTPUT)"; \
	else \
		echo "no snapshot emitted from interactive session"; \
	fi; \
	exit $$STATUS

dev-init:
	@mkdir -p $(dir $(DEV_SNAPSHOT))
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_SAVE_EXAMPLE) SNAPSHOT_OUTPUT=$(DEV_SNAPSHOT) save-snapshot

dev-boot:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_BOOT_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) run

dev-interactive:
	@mkdir -p $(dir $(DEV_SNAPSHOT))
	@if [ ! -f "$(DEV_SNAPSHOT)" ]; then $(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) DEV_SNAPSHOT=$(DEV_SNAPSHOT) dev-init; fi
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_BOOT_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) continue-snapshot-interactive

dev-screenshot:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_BOOT_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) screenshot

dev-file-in:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@test -n "$(strip $(FILE_IN_PAYLOAD) $(FILE_IN_PAYLOADS))" || (echo "FILE_IN_PAYLOAD or FILE_IN_PAYLOADS is required for dev-file-in" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_SAVE_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) $(if $(FILE_IN_PAYLOAD),FILE_IN_PAYLOAD=$(FILE_IN_PAYLOAD)) $(if $(strip $(FILE_IN_PAYLOADS)),FILE_IN_PAYLOADS="$(FILE_IN_PAYLOADS)") continue-snapshot

dev-reset:
	@rm -f "$(DEV_SNAPSHOT)" "$(DEV_SNAPSHOT_BACKUP)"
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) DEV_SNAPSHOT=$(DEV_SNAPSHOT) dev-init

dev-restore:
	@test -f "$(DEV_SNAPSHOT_BACKUP)" || (echo "DEV_SNAPSHOT_BACKUP does not exist; save at least one continued snapshot first" >&2; exit 1)
	@mkdir -p $(dir $(DEV_SNAPSHOT))
	@cp "$(DEV_SNAPSHOT_BACKUP)" "$(DEV_SNAPSHOT)"
	@echo "restored $(DEV_SNAPSHOT) from $(DEV_SNAPSHOT_BACKUP)"

inspect-image: $(GENERATED_IMAGE)
	$(PYTHON) $(ROOT)/tools/inspect_qemu_riscv_mvp_image.py $(GENERATED_IMAGE)

clean:
	rm -rf $(BUILD_DIR)
