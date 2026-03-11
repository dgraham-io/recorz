ROOT := $(abspath $(CURDIR)/../..)
RV32_PROFILE ?= dev
BUILD_DIR ?= $(ROOT)/misc/qemu-riscv32-$(RV32_PROFILE)-mvp
TOOLCHAIN_PREFIX ?= riscv64-unknown-elf-
CC := $(TOOLCHAIN_PREFIX)gcc
QEMU ?= qemu-system-riscv32
QEMU_MEMORY ?= 32M
PYTHON ?= python3
UPDATE_PAYLOAD ?=
UPDATE_FW_CFG_NAME ?= opt/recorz-method-update
FILE_IN_PAYLOAD ?=
FILE_IN_PAYLOADS ?=
DEFAULT_FILE_IN_PAYLOADS := $(ROOT)/kernel/textui/TextRendererBootstrap.rz $(ROOT)/kernel/textui/ViewBootstrap.rz $(ROOT)/kernel/textui/WidgetBootstrap.rz
KERNEL_SOURCE_BUNDLE ?=
FILE_IN_FW_CFG_NAME ?= opt/recorz-file-in
SNAPSHOT_PAYLOAD ?=
SNAPSHOT_FW_CFG_NAME ?= opt/recorz-snapshot
SNAPSHOT_OUTPUT ?= $(BUILD_DIR)/recorz-live-snapshot.bin
SNAPSHOT_TEMP_OUTPUT ?= $(SNAPSHOT_OUTPUT).tmp
SNAPSHOT_EXTRACT_TIMEOUT ?= 60
REGENERATED_BOOT_SOURCE_OUTPUT ?= $(BUILD_DIR)/regenerated_boot_source.rz
REGENERATED_BOOT_SOURCE_TEMP_OUTPUT ?= $(REGENERATED_BOOT_SOURCE_OUTPUT).tmp
REGENERATED_KERNEL_SOURCE_OUTPUT ?= $(BUILD_DIR)/regenerated_kernel_source.rz
REGENERATED_KERNEL_SOURCE_TEMP_OUTPUT ?= $(REGENERATED_KERNEL_SOURCE_OUTPUT).tmp
REGENERATED_BOOT_SOURCE_EXTRACT_TIMEOUT ?= 60
REGENERATED_RUNTIME_BINDINGS_OUTPUT ?= $(BUILD_DIR)/recorz_mvp_regenerated_runtime_bindings.h
REGENERATED_IMAGE_OUTPUT ?= $(BUILD_DIR)/regenerated_image.bin
QEMU_EXTRA_ARGS ?=
QEMU_WINDOW_INPUT_ARGS ?= -device virtio-keyboard-device
comma := ,
PYTHON_KERNEL_SOURCE_ENV := $(if $(KERNEL_SOURCE_BUNDLE),RECORZ_MVP_KERNEL_SOURCE_BUNDLE=$(KERNEL_SOURCE_BUNDLE))
QEMU_UPDATE_ARGS := $(if $(UPDATE_PAYLOAD),-fw_cfg name=$(UPDATE_FW_CFG_NAME)$(comma)file=$(UPDATE_PAYLOAD))
GENERATED_FILE_IN_PAYLOAD := $(BUILD_DIR)/external_file_in_payload.rz
ALL_FILE_IN_PAYLOADS := $(strip $(DEFAULT_FILE_IN_PAYLOADS) $(FILE_IN_PAYLOAD) $(FILE_IN_PAYLOADS))
FILE_IN_EFFECTIVE_PAYLOAD := $(if $(ALL_FILE_IN_PAYLOADS),$(GENERATED_FILE_IN_PAYLOAD))
QEMU_FILE_IN_ARGS := $(if $(FILE_IN_EFFECTIVE_PAYLOAD),-fw_cfg name=$(FILE_IN_FW_CFG_NAME)$(comma)file=$(FILE_IN_EFFECTIVE_PAYLOAD))
QEMU_SNAPSHOT_ARGS := $(if $(SNAPSHOT_PAYLOAD),-fw_cfg name=$(SNAPSHOT_FW_CFG_NAME)$(comma)file=$(SNAPSHOT_PAYLOAD))
QEMU_RUN_ARGS := $(QEMU_UPDATE_ARGS) $(QEMU_FILE_IN_ARGS) $(QEMU_SNAPSHOT_ARGS) $(QEMU_EXTRA_ARGS)
CONTINUE_SNAPSHOT_OUTPUT := $(SNAPSHOT_PAYLOAD)
CONTINUE_SNAPSHOT_TEMP_OUTPUT := $(CONTINUE_SNAPSHOT_OUTPUT).tmp
CONTINUE_SNAPSHOT_BACKUP_OUTPUT := $(CONTINUE_SNAPSHOT_OUTPUT).bak
CONTINUE_SNAPSHOT_SIGNAL ?=
QEMU_FILE_IN_DEP := $(if $(ALL_FILE_IN_PAYLOADS),$(GENERATED_FILE_IN_PAYLOAD))
DEV_SNAPSHOT ?= $(ROOT)/misc/qemu-riscv32-dev/live.bin
DEV_SNAPSHOT_BACKUP ?= $(DEV_SNAPSHOT).bak
DEV_LOOP_SIGNAL ?= $(BUILD_DIR)/dev-loop.snapshot
DEV_INIT_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_image_development_home_save.rz
DEV_BOOT_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_image_first_boot.rz
DEV_INTERACTIVE_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_image_development_home_boot.rz
DEV_SAVE_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_image_first_save.rz
DEV_REGENERATE_EXAMPLE ?= $(ROOT)/examples/qemu_riscv_emit_regenerated_boot_source_file_in.rz

CFLAGS := -march=rv32im -mabi=ilp32 -mcmodel=medany -nostdlib -ffreestanding -O2 -Wall -Wextra -I$(CURDIR) -I$(BUILD_DIR)
ifeq ($(RV32_PROFILE),dev)
CFLAGS += -DRECORZ_MVP_PROFILE_DEV=1
else ifeq ($(RV32_PROFILE),target)
CFLAGS += -DRECORZ_MVP_PROFILE_TARGET=1
else
$(error RV32_PROFILE must be dev or target)
endif
LDFLAGS := -T $(CURDIR)/linker.ld

EXAMPLE ?= $(ROOT)/examples/qemu_riscv_fb_demo.rz
IMAGE_SCRIPT := $(ROOT)/tools/build_qemu_riscv_mvp_image.py
GENERATED_BINDINGS_SCRIPT := $(ROOT)/tools/generate_qemu_riscv_mvp_runtime_bindings_header.py
KERNEL_MVP_SOURCES := $(wildcard $(ROOT)/kernel/mvp/*.rz)
GENERATED_IMAGE := $(BUILD_DIR)/demo_image.bin
GENERATED_BINDINGS_HEADER := $(BUILD_DIR)/recorz_mvp_generated_runtime_bindings.h
GENERATED_IMAGE_ASM := $(BUILD_DIR)/demo_image_blob.S
GENERATED_IMAGE_OBJECT := $(BUILD_DIR)/demo_image_blob.o
ELF := $(BUILD_DIR)/recorz-qemu-riscv32-mvp.elf
PPM := $(BUILD_DIR)/recorz-qemu-riscv32-mvp.ppm
MONITOR_SOCK := $(BUILD_DIR)/monitor.sock
QEMU_PID := $(BUILD_DIR)/qemu.pid
QEMU_LOG := $(BUILD_DIR)/qemu.log
QEMU_INTERACTIVE_CHARDEV := -chardev stdio,id=recorzio,signal=off,logfile=$(QEMU_LOG),logappend=off -serial chardev:recorzio

SOURCES := start.S machine.c display.c vm.c program.c seed.c image.c main.c
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SOURCES))) \
	$(patsubst %.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SOURCES))) \
	$(GENERATED_IMAGE_OBJECT)

.PHONY: all run run-interactive run-headless screenshot save-snapshot continue-snapshot continue-snapshot-interactive regenerate-boot-source regenerate-runtime-bindings regenerate-image dev-init dev-boot dev-interactive dev-loop dev-screenshot dev-file-in dev-regenerate-boot-source dev-regenerate-runtime-bindings dev-regenerate-image dev-reset dev-restore inspect-image clean FORCE

all: $(ELF)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GENERATED_IMAGE): FORCE $(IMAGE_SCRIPT) $(EXAMPLE) $(KERNEL_MVP_SOURCES) $(ROOT)/platform/qemu-riscv64/runtime_spec.json $(ROOT)/src/recorz/parser.py $(ROOT)/src/recorz/ast.py $(ROOT)/src/recorz/compiler.py $(ROOT)/src/recorz/model.py | $(BUILD_DIR)
	$(PYTHON_KERNEL_SOURCE_ENV) PYTHONPATH=$(ROOT)/src $(PYTHON) $(IMAGE_SCRIPT) $(EXAMPLE) $(GENERATED_IMAGE)

$(GENERATED_BINDINGS_HEADER): $(GENERATED_BINDINGS_SCRIPT) $(IMAGE_SCRIPT) $(KERNEL_MVP_SOURCES) $(ROOT)/platform/qemu-riscv64/runtime_spec.json $(ROOT)/src/recorz/parser.py $(ROOT)/src/recorz/ast.py $(ROOT)/src/recorz/compiler.py $(ROOT)/src/recorz/model.py | $(BUILD_DIR)
	$(PYTHON_KERNEL_SOURCE_ENV) PYTHONPATH=$(ROOT)/src $(PYTHON) $(GENERATED_BINDINGS_SCRIPT) $(GENERATED_BINDINGS_HEADER)

$(GENERATED_FILE_IN_PAYLOAD): $(DEFAULT_FILE_IN_PAYLOADS) $(FILE_IN_PAYLOAD) $(FILE_IN_PAYLOADS) | $(BUILD_DIR)
	@rm -f $@
	@first=1; \
	for path in $(ALL_FILE_IN_PAYLOADS); do \
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
	$(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial mon:stdio -device ramfb $(QEMU_WINDOW_INPUT_ARGS) $(QEMU_RUN_ARGS)

run-interactive: $(ELF) $(QEMU_FILE_IN_DEP)
	$(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial stdio -monitor none -device ramfb $(QEMU_WINDOW_INPUT_ARGS) $(QEMU_RUN_ARGS)

run-headless: $(ELF) $(QEMU_FILE_IN_DEP)
	$(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial stdio -display none -device ramfb $(QEMU_RUN_ARGS)

screenshot: $(ELF) $(QEMU_FILE_IN_DEP)
	rm -f $(MONITOR_SOCK) $(PPM) $(QEMU_PID) $(QEMU_LOG)
	( $(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial stdio -display none -device ramfb $(QEMU_RUN_ARGS) -monitor unix:$(MONITOR_SOCK),server,nowait >$(QEMU_LOG) 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/qemu_hmp_screendump.py $(MONITOR_SOCK) $(PPM) $(QEMU_LOG) "recorz qemu-riscv32 mvp: rendered"
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@echo "wrote $(PPM)"

save-snapshot: $(ELF) $(QEMU_FILE_IN_DEP)
	rm -f $(QEMU_LOG) $(QEMU_PID) $(SNAPSHOT_TEMP_OUTPUT)
	( $(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial file:$(QEMU_LOG) -display none -device ramfb $(QEMU_RUN_ARGS) >/dev/null 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_snapshot.py --timeout $(SNAPSHOT_EXTRACT_TIMEOUT) $(QEMU_LOG) $(SNAPSHOT_TEMP_OUTPUT)
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@mkdir -p $(dir $(SNAPSHOT_OUTPUT))
	@mv $(SNAPSHOT_TEMP_OUTPUT) $(SNAPSHOT_OUTPUT)
	@echo "wrote $(SNAPSHOT_OUTPUT)"

continue-snapshot: $(ELF) $(QEMU_FILE_IN_DEP)
	@test -n "$(SNAPSHOT_PAYLOAD)" || (echo "SNAPSHOT_PAYLOAD is required for continue-snapshot" >&2; exit 1)
	rm -f $(QEMU_LOG) $(QEMU_PID) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT)
	( $(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial file:$(QEMU_LOG) -display none -device ramfb $(QEMU_RUN_ARGS) >/dev/null 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_snapshot.py --timeout $(SNAPSHOT_EXTRACT_TIMEOUT) $(QEMU_LOG) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT)
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@mkdir -p $(dir $(CONTINUE_SNAPSHOT_OUTPUT))
	@if [ -f "$(CONTINUE_SNAPSHOT_OUTPUT)" ]; then cp "$(CONTINUE_SNAPSHOT_OUTPUT)" "$(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; echo "backed up $(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; fi
	@mv $(CONTINUE_SNAPSHOT_TEMP_OUTPUT) $(CONTINUE_SNAPSHOT_OUTPUT)
	@echo "wrote $(CONTINUE_SNAPSHOT_OUTPUT)"

continue-snapshot-interactive: $(ELF) $(QEMU_FILE_IN_DEP)
	@test -n "$(SNAPSHOT_PAYLOAD)" || (echo "SNAPSHOT_PAYLOAD is required for continue-snapshot-interactive" >&2; exit 1)
	rm -f $(QEMU_LOG) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT) $(CONTINUE_SNAPSHOT_SIGNAL)
	@$(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) $(QEMU_INTERACTIVE_CHARDEV) -monitor none -device ramfb $(QEMU_WINDOW_INPUT_ARGS) $(QEMU_RUN_ARGS); \
	STATUS=$$?; \
	if grep -q 'recorz-snapshot-end' $(QEMU_LOG); then \
		$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_snapshot.py --timeout 1 $(QEMU_LOG) $(CONTINUE_SNAPSHOT_TEMP_OUTPUT); \
		mkdir -p $(dir $(CONTINUE_SNAPSHOT_OUTPUT)); \
		if [ -f "$(CONTINUE_SNAPSHOT_OUTPUT)" ]; then cp "$(CONTINUE_SNAPSHOT_OUTPUT)" "$(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; echo "backed up $(CONTINUE_SNAPSHOT_BACKUP_OUTPUT)"; fi; \
		mv $(CONTINUE_SNAPSHOT_TEMP_OUTPUT) $(CONTINUE_SNAPSHOT_OUTPUT); \
		if [ -n "$(CONTINUE_SNAPSHOT_SIGNAL)" ]; then mkdir -p $(dir $(CONTINUE_SNAPSHOT_SIGNAL)); touch "$(CONTINUE_SNAPSHOT_SIGNAL)"; fi; \
		echo "wrote $(CONTINUE_SNAPSHOT_OUTPUT)"; \
	else \
		echo "no snapshot emitted from interactive session"; \
	fi; \
	exit $$STATUS

regenerate-boot-source: $(ELF) $(QEMU_FILE_IN_DEP)
	rm -f $(QEMU_LOG) $(QEMU_PID) $(REGENERATED_BOOT_SOURCE_TEMP_OUTPUT) $(REGENERATED_KERNEL_SOURCE_TEMP_OUTPUT)
	( $(QEMU) -machine virt -m $(QEMU_MEMORY) -smp 1 -kernel $(ELF) -serial file:$(QEMU_LOG) -display none -device ramfb $(QEMU_RUN_ARGS) >/dev/null 2>&1 & echo $$! > $(QEMU_PID) )
	$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_regenerated_kernel_source.py --timeout $(REGENERATED_BOOT_SOURCE_EXTRACT_TIMEOUT) $(QEMU_LOG) $(REGENERATED_KERNEL_SOURCE_TEMP_OUTPUT)
	$(PYTHON) $(ROOT)/tools/extract_qemu_riscv_regenerated_boot_source.py --timeout $(REGENERATED_BOOT_SOURCE_EXTRACT_TIMEOUT) $(QEMU_LOG) $(REGENERATED_BOOT_SOURCE_TEMP_OUTPUT)
	@PID=$$(cat $(QEMU_PID)); kill $$PID >/dev/null 2>&1 || true
	@mkdir -p $(dir $(REGENERATED_KERNEL_SOURCE_OUTPUT))
	@mv $(REGENERATED_KERNEL_SOURCE_TEMP_OUTPUT) $(REGENERATED_KERNEL_SOURCE_OUTPUT)
	@echo "wrote $(REGENERATED_KERNEL_SOURCE_OUTPUT)"
	@mkdir -p $(dir $(REGENERATED_BOOT_SOURCE_OUTPUT))
	@mv $(REGENERATED_BOOT_SOURCE_TEMP_OUTPUT) $(REGENERATED_BOOT_SOURCE_OUTPUT)
	@echo "wrote $(REGENERATED_BOOT_SOURCE_OUTPUT)"

regenerate-runtime-bindings: regenerate-boot-source
	RECORZ_MVP_KERNEL_SOURCE_BUNDLE=$(REGENERATED_KERNEL_SOURCE_OUTPUT) PYTHONPATH=$(ROOT)/src $(PYTHON) $(GENERATED_BINDINGS_SCRIPT) $(REGENERATED_RUNTIME_BINDINGS_OUTPUT)
	@echo "wrote $(REGENERATED_RUNTIME_BINDINGS_OUTPUT)"

regenerate-image: regenerate-boot-source
	RECORZ_MVP_KERNEL_SOURCE_BUNDLE=$(REGENERATED_KERNEL_SOURCE_OUTPUT) PYTHONPATH=$(ROOT)/src $(PYTHON) $(IMAGE_SCRIPT) $(REGENERATED_BOOT_SOURCE_OUTPUT) $(REGENERATED_IMAGE_OUTPUT)
	@echo "wrote $(REGENERATED_IMAGE_OUTPUT)"

dev-init:
	@mkdir -p $(dir $(DEV_SNAPSHOT))
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_INIT_EXAMPLE) SNAPSHOT_OUTPUT=$(DEV_SNAPSHOT) save-snapshot

dev-boot:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_BOOT_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) run

dev-interactive:
	@mkdir -p $(dir $(DEV_SNAPSHOT))
	@if [ ! -f "$(DEV_SNAPSHOT)" ]; then $(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) DEV_SNAPSHOT=$(DEV_SNAPSHOT) dev-init; fi
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_INTERACTIVE_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) continue-snapshot-interactive

dev-loop:
	@mkdir -p $(dir $(DEV_SNAPSHOT))
	@if [ ! -f "$(DEV_SNAPSHOT)" ]; then $(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) DEV_SNAPSHOT=$(DEV_SNAPSHOT) dev-init; fi
	@while true; do \
		rm -f "$(DEV_LOOP_SIGNAL)"; \
		$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_INTERACTIVE_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) CONTINUE_SNAPSHOT_SIGNAL="$(DEV_LOOP_SIGNAL)" continue-snapshot-interactive; \
		STATUS=$$?; \
		if [ $$STATUS -ne 0 ]; then exit $$STATUS; fi; \
		if [ ! -f "$(DEV_LOOP_SIGNAL)" ]; then break; fi; \
		echo "resuming $(DEV_SNAPSHOT)"; \
	done

dev-screenshot:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_BOOT_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) screenshot

dev-file-in:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@test -n "$(strip $(FILE_IN_PAYLOAD) $(FILE_IN_PAYLOADS))" || (echo "FILE_IN_PAYLOAD or FILE_IN_PAYLOADS is required for dev-file-in" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_SAVE_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) $(if $(FILE_IN_PAYLOAD),FILE_IN_PAYLOAD=$(FILE_IN_PAYLOAD)) $(if $(strip $(FILE_IN_PAYLOADS)),FILE_IN_PAYLOADS="$(FILE_IN_PAYLOADS)") continue-snapshot

dev-regenerate-boot-source:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) EXAMPLE=$(DEV_BOOT_EXAMPLE) SNAPSHOT_PAYLOAD=$(DEV_SNAPSHOT) FILE_IN_PAYLOAD=$(DEV_REGENERATE_EXAMPLE) REGENERATED_BOOT_SOURCE_OUTPUT=$(REGENERATED_BOOT_SOURCE_OUTPUT) REGENERATED_KERNEL_SOURCE_OUTPUT=$(REGENERATED_KERNEL_SOURCE_OUTPUT) regenerate-boot-source

dev-regenerate-runtime-bindings:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) DEV_SNAPSHOT=$(DEV_SNAPSHOT) DEV_BOOT_EXAMPLE=$(DEV_BOOT_EXAMPLE) DEV_REGENERATE_EXAMPLE=$(DEV_REGENERATE_EXAMPLE) REGENERATED_BOOT_SOURCE_OUTPUT=$(REGENERATED_BOOT_SOURCE_OUTPUT) REGENERATED_KERNEL_SOURCE_OUTPUT=$(REGENERATED_KERNEL_SOURCE_OUTPUT) REGENERATED_RUNTIME_BINDINGS_OUTPUT=$(REGENERATED_RUNTIME_BINDINGS_OUTPUT) dev-regenerate-boot-source
	@RECORZ_MVP_KERNEL_SOURCE_BUNDLE=$(REGENERATED_KERNEL_SOURCE_OUTPUT) PYTHONPATH=$(ROOT)/src $(PYTHON) $(GENERATED_BINDINGS_SCRIPT) $(REGENERATED_RUNTIME_BINDINGS_OUTPUT)
	@echo "wrote $(REGENERATED_RUNTIME_BINDINGS_OUTPUT)"

dev-regenerate-image:
	@test -f "$(DEV_SNAPSHOT)" || (echo "DEV_SNAPSHOT does not exist; run dev-init first" >&2; exit 1)
	@$(MAKE) -C $(CURDIR) BUILD_DIR=$(BUILD_DIR) DEV_SNAPSHOT=$(DEV_SNAPSHOT) DEV_BOOT_EXAMPLE=$(DEV_BOOT_EXAMPLE) DEV_REGENERATE_EXAMPLE=$(DEV_REGENERATE_EXAMPLE) REGENERATED_BOOT_SOURCE_OUTPUT=$(REGENERATED_BOOT_SOURCE_OUTPUT) REGENERATED_KERNEL_SOURCE_OUTPUT=$(REGENERATED_KERNEL_SOURCE_OUTPUT) dev-regenerate-boot-source
	@RECORZ_MVP_KERNEL_SOURCE_BUNDLE=$(REGENERATED_KERNEL_SOURCE_OUTPUT) PYTHONPATH=$(ROOT)/src $(PYTHON) $(IMAGE_SCRIPT) $(REGENERATED_BOOT_SOURCE_OUTPUT) $(REGENERATED_IMAGE_OUTPUT)
	@echo "wrote $(REGENERATED_IMAGE_OUTPUT)"

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
