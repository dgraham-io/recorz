#include "display.h"
#include "image.h"
#include "machine.h"
#include "vm.h"

extern const uint8_t recorz_demo_image_blob_start[];
extern const uint8_t recorz_demo_image_blob_end[];
extern const uint8_t recorz_default_file_in_blob_start[];
extern const uint8_t recorz_default_file_in_blob_end[];

#define RECORZ_MVP_SNAPSHOT_FW_CFG_NAME "opt/recorz-snapshot"
#define RECORZ_MVP_SNAPSHOT_BUFFER_SIZE RECORZ_MVP_SNAPSHOT_BUFFER_LIMIT
#define RECORZ_MVP_FILE_IN_FW_CFG_NAME "opt/recorz-file-in"
#define RECORZ_MVP_FILE_IN_BUFFER_SIZE 131072U
#define RECORZ_MVP_FILE_IN_SEPARATOR_SIZE 3U

static uint32_t append_file_in_blob(
    uint8_t destination[],
    uint32_t destination_size,
    uint32_t offset,
    const uint8_t *source,
    uint32_t source_size
) {
    uint32_t index;

    if (source == 0 || source_size == 0U) {
        return offset;
    }
    if (offset != 0U) {
        if (offset + RECORZ_MVP_FILE_IN_SEPARATOR_SIZE > destination_size) {
            machine_panic("built-in file-in payload exceeds buffer capacity");
        }
        destination[offset++] = '\n';
        destination[offset++] = '!';
        destination[offset++] = '\n';
    }
    if (offset + source_size > destination_size) {
        machine_panic("built-in file-in payload exceeds buffer capacity");
    }
    for (index = 0U; index < source_size; ++index) {
        if (source[index] == '\0') {
            machine_panic("file-in payload contains an unexpected NUL byte");
        }
        destination[offset + index] = source[index];
    }
    return offset + source_size;
}

void main(const void *fdt) {
    const struct recorz_mvp_boot_image *image;
    uint32_t image_size = (uint32_t)((uintptr_t)recorz_demo_image_blob_end - (uintptr_t)recorz_demo_image_blob_start);
    uint32_t built_in_file_in_size =
        (uint32_t)((uintptr_t)recorz_default_file_in_blob_end - (uintptr_t)recorz_default_file_in_blob_start);
    uint8_t method_update_blob[RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE + (RECORZ_MVP_COMPILED_METHOD_MAX_INSTRUCTIONS * 4U)];
    uint32_t method_update_size;
    static uint8_t file_in_blob[RECORZ_MVP_FILE_IN_BUFFER_SIZE];
    uint32_t file_in_size;
    uint32_t external_file_in_size;
    static uint8_t snapshot_blob[RECORZ_MVP_SNAPSHOT_BUFFER_SIZE];
    uint32_t snapshot_size;

    machine_init(fdt);
    machine_puts("recorz qemu-riscv32 mvp: booting\n");
    display_init();
    image = recorz_mvp_image_load(recorz_demo_image_blob_start, image_size);
    method_update_size = machine_fw_cfg_try_read_file(
        RECORZ_MVP_METHOD_UPDATE_FW_CFG_NAME,
        method_update_blob,
        sizeof(method_update_blob)
    );
    file_in_size = append_file_in_blob(
        file_in_blob,
        sizeof(file_in_blob),
        0U,
        recorz_default_file_in_blob_start,
        built_in_file_in_size
    );
    external_file_in_size = machine_fw_cfg_try_read_file(
        RECORZ_MVP_FILE_IN_FW_CFG_NAME,
        file_in_blob + file_in_size + (file_in_size == 0U ? 0U : RECORZ_MVP_FILE_IN_SEPARATOR_SIZE),
        sizeof(file_in_blob) - file_in_size - (file_in_size == 0U ? 0U : RECORZ_MVP_FILE_IN_SEPARATOR_SIZE)
    );
    if (external_file_in_size != 0U) {
        if (file_in_size != 0U) {
            file_in_blob[file_in_size++] = '\n';
            file_in_blob[file_in_size++] = '!';
            file_in_blob[file_in_size++] = '\n';
        }
        file_in_size += external_file_in_size;
    }
    snapshot_size = machine_fw_cfg_try_read_file(
        RECORZ_MVP_SNAPSHOT_FW_CFG_NAME,
        snapshot_blob,
        sizeof(snapshot_blob)
    );
    recorz_mvp_vm_run(
        image->program,
        image->seed,
        method_update_blob,
        method_update_size,
        file_in_blob,
        file_in_size,
        snapshot_blob,
        snapshot_size
    );
    machine_puts("recorz qemu-riscv32 mvp: rendered\n");
    machine_wait_forever();
}
