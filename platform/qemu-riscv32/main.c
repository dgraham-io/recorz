#include "display.h"
#include "image.h"
#include "machine.h"

extern const uint8_t recorz_demo_image_blob_start[];
extern const uint8_t recorz_demo_image_blob_end[];

void main(const void *fdt) {
    const struct recorz_mvp_boot_image *image;
    uint32_t image_size = (uint32_t)((uintptr_t)recorz_demo_image_blob_end - (uintptr_t)recorz_demo_image_blob_start);

    machine_init(fdt);
    machine_puts("recorz qemu-riscv32 serial: booting\n");
    display_init();
    image = recorz_mvp_image_load(recorz_demo_image_blob_start, image_size);
    recorz_mvp_vm_run(
        image->program,
        image->seed,
        0,
        0U,
        0,
        0U,
        0,
        0U
    );
    machine_puts("recorz qemu-riscv32 serial: rendered\n");
    machine_wait_forever();
}
