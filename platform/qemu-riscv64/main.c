#include "display.h"
#include "image.h"
#include "machine.h"

extern const uint8_t recorz_demo_image_blob_start[];
extern const uint8_t recorz_demo_image_blob_end[];

#define RECORZ_MVP_SNAPSHOT_FW_CFG_NAME "opt/recorz-snapshot"
#define RECORZ_MVP_SNAPSHOT_BUFFER_SIZE 65536U
#define RECORZ_MVP_FILE_IN_FW_CFG_NAME "opt/recorz-file-in"
#define RECORZ_MVP_FILE_IN_BUFFER_SIZE 16384U

void main(const void *fdt) {
    const struct recorz_mvp_boot_image *image;
    uint32_t image_size = (uint32_t)((uintptr_t)recorz_demo_image_blob_end - (uintptr_t)recorz_demo_image_blob_start);
    uint8_t method_update_blob[RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE + (RECORZ_MVP_COMPILED_METHOD_MAX_INSTRUCTIONS * 4U)];
    uint32_t method_update_size;
    static uint8_t file_in_blob[RECORZ_MVP_FILE_IN_BUFFER_SIZE];
    uint32_t file_in_size;
    static uint8_t snapshot_blob[RECORZ_MVP_SNAPSHOT_BUFFER_SIZE];
    uint32_t snapshot_size;

    machine_init(fdt);
    machine_puts("recorz qemu-riscv64 mvp: booting\n");
    display_init();
    image = recorz_mvp_image_load(recorz_demo_image_blob_start, image_size);
    method_update_size = machine_fw_cfg_try_read_file(
        RECORZ_MVP_METHOD_UPDATE_FW_CFG_NAME,
        method_update_blob,
        sizeof(method_update_blob)
    );
    file_in_size = machine_fw_cfg_try_read_file(
        RECORZ_MVP_FILE_IN_FW_CFG_NAME,
        file_in_blob,
        sizeof(file_in_blob)
    );
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
    machine_puts("recorz qemu-riscv64 mvp: rendered\n");
    machine_wait_forever();
}
