#include "machine.h"

#include <stddef.h>
#include <stdint.h>

#define UART0_DEFAULT_BASE 0x10000000UL
#define FW_CFG_DEFAULT_BASE 0x10100000UL

#define FW_CFG_FILE_DIR 0x0019U
#define FW_CFG_DMA_CTL_ERROR 0x01U
#define FW_CFG_DMA_CTL_READ 0x02U
#define FW_CFG_DMA_CTL_SELECT 0x08U
#define FW_CFG_DMA_CTL_WRITE 0x10U

#define FDT_MAGIC 0xd00dfeedU
#define FDT_BEGIN_NODE 0x00000001U
#define FDT_END_NODE 0x00000002U
#define FDT_PROP 0x00000003U
#define FDT_NOP 0x00000004U
#define FDT_END 0x00000009U

#define FDT_DEFAULT_ADDRESS_CELLS 2U
#define FDT_DEFAULT_SIZE_CELLS 1U
#define MAX_FDT_DEPTH 32U
#define MAX_FDT_PATH 160U
#define MAX_FDT_ALIAS_NAME 32U
#define MAX_FDT_ALIASES 16U
#define MAX_UART_CANDIDATES 8U

#define DRM_FORMAT_XRGB8888 0x34325258U
#define MAX_FW_CFG_FILES 128U

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} __attribute__((packed));

struct fw_cfg_dma_access {
    uint32_t control;
    uint32_t length;
    uint64_t address;
} __attribute__((packed, aligned(8)));

struct fw_cfg_file {
    uint32_t size;
    uint16_t select;
    uint16_t reserved;
    char name[56];
} __attribute__((packed));

struct fw_cfg_files {
    uint32_t count;
    struct fw_cfg_file files[MAX_FW_CFG_FILES];
} __attribute__((packed));

struct ramfb_cfg {
    uint64_t addr;
    uint32_t fourcc;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} __attribute__((packed));

struct fdt_node_state {
    uint32_t address_cells;
    uint32_t size_cells;
    uint64_t reg_base;
    uint8_t has_reg;
    uint8_t uart_candidate;
    uint8_t fw_cfg_candidate;
};

struct discovered_devices {
    uint64_t uart_base;
    uint64_t fw_cfg_base;
    uint8_t have_uart;
    uint8_t have_fw_cfg;
};

struct alias_entry {
    char name[MAX_FDT_ALIAS_NAME];
    char path[MAX_FDT_PATH];
};

struct uart_candidate {
    uint64_t base;
    char path[MAX_FDT_PATH];
};

static uintptr_t uart_base = UART0_DEFAULT_BASE;
static uintptr_t fw_cfg_dma_register = FW_CFG_DEFAULT_BASE + 0x10U;
static machine_panic_hook current_panic_hook = 0;

static struct fw_cfg_dma_access fw_cfg_dma_request;
static struct fw_cfg_files fw_cfg_files;
static struct ramfb_cfg ramfb;

static uint16_t bswap16(uint16_t value) {
    return (uint16_t)((value >> 8) | (value << 8));
}

static uint32_t bswap32(uint32_t value) {
    return ((value & 0x000000ffU) << 24) |
           ((value & 0x0000ff00U) << 8) |
           ((value & 0x00ff0000U) >> 8) |
           ((value & 0xff000000U) >> 24);
}

static uint64_t bswap64(uint64_t value) {
    return ((uint64_t)bswap32((uint32_t)(value >> 32))) |
           ((uint64_t)bswap32((uint32_t)(value & 0xffffffffULL)) << 32);
}

static uint32_t read_be32(const void *buffer) {
    const uint8_t *bytes = (const uint8_t *)buffer;
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) |
           (uint32_t)bytes[3];
}

static uintptr_t align4(uintptr_t value) {
    return (value + 3U) & ~(uintptr_t)3U;
}

static size_t ascii_length(const char *text) {
    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

static int ascii_equals(const char *lhs, const char *rhs) {
    while (*lhs != '\0' && *rhs != '\0' && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }
    return *lhs == '\0' && *rhs == '\0';
}

static void copy_string(char *buffer, uint32_t buffer_size, const char *value) {
    uint32_t index = 0U;

    if (buffer_size == 0U) {
        return;
    }
    while (index + 1U < buffer_size && value[index] != '\0') {
        buffer[index] = value[index];
        ++index;
    }
    buffer[index] = '\0';
}

static int string_list_contains(const char *value, uint32_t length, const char *target) {
    uint32_t index = 0U;
    while (index < length) {
        const char *entry = value + index;
        uint32_t entry_length = 0U;
        while (index + entry_length < length && entry[entry_length] != '\0') {
            ++entry_length;
        }
        if (entry_length > 0U && ascii_equals(entry, target)) {
            return 1;
        }
        index += entry_length + 1U;
    }
    return 0;
}

static int parse_reg_base(
    const uint8_t *value,
    uint32_t length,
    uint32_t address_cells,
    uint32_t size_cells,
    uint64_t *out_base
) {
    uint32_t required_length = (address_cells + size_cells) * 4U;
    uint64_t base = 0U;
    uint32_t index;

    if (address_cells == 0U || address_cells > 2U || length < required_length) {
        return 0;
    }

    for (index = 0U; index < address_cells; ++index) {
        base = (base << 32) | (uint64_t)read_be32(value + (index * 4U));
    }
    *out_base = base;
    return 1;
}

static void remember_path_value(char *buffer, uint32_t buffer_size, const char *value, uint32_t length) {
    uint32_t index = 0U;

    if (buffer_size == 0U) {
        return;
    }
    if (length == 0U || value[0] != '/') {
        buffer[0] = '\0';
        return;
    }

    while (index + 1U < buffer_size && index < length) {
        char ch = value[index];
        if (ch == '\0' || ch == ':') {
            break;
        }
        buffer[index] = ch;
        ++index;
    }
    buffer[index] = '\0';
}

static void remember_stdout_selector(
    char *path_buffer,
    uint32_t path_buffer_size,
    char *alias_buffer,
    uint32_t alias_buffer_size,
    const char *value,
    uint32_t length
) {
    uint32_t index = 0U;

    if (path_buffer_size > 0U) {
        path_buffer[0] = '\0';
    }
    if (alias_buffer_size > 0U) {
        alias_buffer[0] = '\0';
    }
    if (length == 0U) {
        return;
    }
    if (value[0] == '/') {
        remember_path_value(path_buffer, path_buffer_size, value, length);
        return;
    }
    if (alias_buffer_size == 0U) {
        return;
    }
    while (index + 1U < alias_buffer_size && index < length) {
        char ch = value[index];
        if (ch == '\0' || ch == ':') {
            break;
        }
        alias_buffer[index] = ch;
        ++index;
    }
    alias_buffer[index] = '\0';
}

static int append_path_component(char *path, uint32_t *path_length, const char *name) {
    uint32_t name_length = (uint32_t)ascii_length(name);
    uint32_t required = *path_length + 1U + name_length;

    if (name_length == 0U) {
        return 1;
    }
    if (required >= MAX_FDT_PATH) {
        return 0;
    }
    path[*path_length] = '/';
    ++(*path_length);
    for (; *name != '\0'; ++name) {
        path[*path_length] = *name;
        ++(*path_length);
    }
    path[*path_length] = '\0';
    return 1;
}

static void remember_alias(
    struct alias_entry *aliases,
    uint32_t *alias_count,
    const char *name,
    const char *value,
    uint32_t length
) {
    struct alias_entry *entry;

    if (*alias_count >= MAX_FDT_ALIASES) {
        return;
    }
    if (length == 0U || value[0] != '/') {
        return;
    }
    entry = &aliases[*alias_count];
    copy_string(entry->name, MAX_FDT_ALIAS_NAME, name);
    remember_path_value(entry->path, MAX_FDT_PATH, value, length);
    ++(*alias_count);
}

static const char *lookup_alias_path(const struct alias_entry *aliases, uint32_t alias_count, const char *name) {
    uint32_t index;

    for (index = 0U; index < alias_count; ++index) {
        if (ascii_equals(aliases[index].name, name)) {
            return aliases[index].path;
        }
    }
    return 0;
}

static void remember_uart_candidate(
    struct uart_candidate *uart_candidates,
    uint32_t *uart_count,
    uint64_t base,
    const char *path
) {
    if (*uart_count >= MAX_UART_CANDIDATES) {
        return;
    }
    uart_candidates[*uart_count].base = base;
    copy_string(uart_candidates[*uart_count].path, MAX_FDT_PATH, path);
    ++(*uart_count);
}

static int discover_devices_from_dtb(const void *fdt, struct discovered_devices *devices) {
    const struct fdt_header *header = (const struct fdt_header *)fdt;
    const uint8_t *bytes = (const uint8_t *)fdt;
    const uint8_t *strings;
    const uint8_t *structure;
    const uint8_t *structure_end;
    struct fdt_node_state stack[MAX_FDT_DEPTH];
    uint16_t path_lengths[MAX_FDT_DEPTH];
    struct alias_entry aliases[MAX_FDT_ALIASES];
    struct uart_candidate uart_candidates[MAX_UART_CANDIDATES];
    char path[MAX_FDT_PATH];
    char stdout_path[MAX_FDT_PATH];
    char stdout_alias[MAX_FDT_ALIAS_NAME];
    int32_t depth = -1;
    uint32_t alias_count = 0U;
    uint32_t path_length = 0U;
    uint32_t uart_count = 0U;
    const char *preferred_uart_path = 0;
    uint32_t index;

    if (fdt == 0 || devices == 0) {
        return 0;
    }
    if (read_be32(&header->magic) != FDT_MAGIC) {
        return 0;
    }

    strings = bytes + read_be32(&header->off_dt_strings);
    structure = bytes + read_be32(&header->off_dt_struct);
    structure_end = structure + read_be32(&header->size_dt_struct);
    path[0] = '\0';
    stdout_path[0] = '\0';
    stdout_alias[0] = '\0';
    devices->uart_base = 0U;
    devices->fw_cfg_base = 0U;
    devices->have_uart = 0U;
    devices->have_fw_cfg = 0U;

    while (structure < structure_end) {
        uint32_t token = read_be32(structure);
        structure += 4U;

        if (token == FDT_BEGIN_NODE) {
            const char *name = (const char *)structure;
            uintptr_t cursor = (uintptr_t)structure;
            if ((uint32_t)(depth + 1) >= MAX_FDT_DEPTH) {
                return 0;
            }
            ++depth;
            if (depth == 0) {
                stack[depth].address_cells = FDT_DEFAULT_ADDRESS_CELLS;
                stack[depth].size_cells = FDT_DEFAULT_SIZE_CELLS;
            } else {
                stack[depth].address_cells = stack[depth - 1].address_cells;
                stack[depth].size_cells = stack[depth - 1].size_cells;
            }
            stack[depth].reg_base = 0U;
            stack[depth].has_reg = 0U;
            stack[depth].uart_candidate = 0U;
            stack[depth].fw_cfg_candidate = 0U;
            path_lengths[depth] = (uint16_t)path_length;
            if (depth == 0) {
                path[0] = '\0';
                path_length = 0U;
            } else if (!append_path_component(path, &path_length, name)) {
                return 0;
            }
            while ((const uint8_t *)cursor < structure_end && *(const char *)cursor != '\0') {
                ++cursor;
            }
            cursor = align4(cursor + 1U);
            structure = (const uint8_t *)cursor;
            continue;
        }

        if (token == FDT_END_NODE) {
            if (depth < 0) {
                return 0;
            }
            if (stack[depth].has_reg) {
                if (stack[depth].fw_cfg_candidate && !devices->have_fw_cfg) {
                    devices->fw_cfg_base = stack[depth].reg_base;
                    devices->have_fw_cfg = 1U;
                }
                if (stack[depth].uart_candidate) {
                    remember_uart_candidate(uart_candidates, &uart_count, stack[depth].reg_base, path);
                }
            }
            path_length = path_lengths[depth];
            path[path_length] = '\0';
            --depth;
            continue;
        }

        if (token == FDT_PROP) {
            uint32_t property_length = read_be32(structure);
            uint32_t name_offset = read_be32(structure + 4U);
            const char *property_name = (const char *)(strings + name_offset);
            const uint8_t *value = structure + 8U;
            if (depth < 0) {
                return 0;
            }
            if (ascii_equals(property_name, "#address-cells") && property_length >= 4U) {
                stack[depth].address_cells = read_be32(value);
            } else if (ascii_equals(property_name, "#size-cells") && property_length >= 4U) {
                stack[depth].size_cells = read_be32(value);
            } else if (ascii_equals(property_name, "compatible")) {
                if (string_list_contains((const char *)value, property_length, "ns16550a")) {
                    stack[depth].uart_candidate = 1U;
                }
                if (string_list_contains((const char *)value, property_length, "qemu,fw-cfg-mmio")) {
                    stack[depth].fw_cfg_candidate = 1U;
                }
            } else if (ascii_equals(property_name, "reg")) {
                uint32_t address_cells = depth == 0 ? FDT_DEFAULT_ADDRESS_CELLS : stack[depth - 1].address_cells;
                uint32_t size_cells = depth == 0 ? FDT_DEFAULT_SIZE_CELLS : stack[depth - 1].size_cells;
                if (parse_reg_base(value, property_length, address_cells, size_cells, &stack[depth].reg_base)) {
                    stack[depth].has_reg = 1U;
                }
            } else if (ascii_equals(path, "/aliases")) {
                remember_alias(aliases, &alias_count, property_name, (const char *)value, property_length);
            } else if (ascii_equals(path, "/chosen") && ascii_equals(property_name, "stdout-path")) {
                remember_stdout_selector(
                    stdout_path,
                    MAX_FDT_PATH,
                    stdout_alias,
                    MAX_FDT_ALIAS_NAME,
                    (const char *)value,
                    property_length
                );
            }
            structure = (const uint8_t *)align4((uintptr_t)(value + property_length));
            continue;
        }

        if (token == FDT_NOP) {
            continue;
        }

        if (token == FDT_END) {
            break;
        }

        return 0;
    }

    if (stdout_path[0] != '\0') {
        preferred_uart_path = stdout_path;
    } else if (stdout_alias[0] != '\0') {
        preferred_uart_path = lookup_alias_path(aliases, alias_count, stdout_alias);
    }
    for (index = 0U; index < uart_count; ++index) {
        if (preferred_uart_path != 0 && ascii_equals(uart_candidates[index].path, preferred_uart_path)) {
            devices->uart_base = uart_candidates[index].base;
            devices->have_uart = 1U;
            break;
        }
    }
    if (!devices->have_uart && uart_count > 0U) {
        devices->uart_base = uart_candidates[0].base;
        devices->have_uart = 1U;
    }

    return 1;
}

void machine_init(const void *fdt) {
    struct discovered_devices devices;

    if (!discover_devices_from_dtb(fdt, &devices)) {
        machine_puts("warning: DTB parse failed, using QEMU virt MMIO defaults\n");
        return;
    }
    if (devices.have_uart) {
        uart_base = (uintptr_t)devices.uart_base;
    } else {
        machine_puts("warning: DTB missing UART, using QEMU virt default\n");
    }
    if (devices.have_fw_cfg) {
        fw_cfg_dma_register = (uintptr_t)(devices.fw_cfg_base + 0x10U);
    } else {
        machine_puts("warning: DTB missing fw_cfg, using QEMU virt default\n");
    }
}

void machine_putc(char c) {
    *(volatile uint8_t *)uart_base = (uint8_t)c;
}

void machine_puts(const char *text) {
    while (*text != '\0') {
        if (*text == '\n') {
            machine_putc('\r');
        }
        machine_putc(*text++);
    }
}

void machine_wait_forever(void) {
    for (;;) {
        __asm__ volatile("wfi");
    }
}

void machine_set_panic_hook(machine_panic_hook hook) {
    current_panic_hook = hook;
}

void machine_panic(const char *message) {
    machine_panic_hook hook = current_panic_hook;

    current_panic_hook = 0;
    machine_puts("panic: ");
    machine_puts(message);
    machine_puts("\n");
    if (hook != 0) {
        hook(message);
    }
    machine_wait_forever();
}

static void fw_cfg_dma_transfer(uint16_t selector, uint32_t control, void *buffer, uint32_t length) {
    fw_cfg_dma_request.control = bswap32(((uint32_t)selector << 16) | FW_CFG_DMA_CTL_SELECT | control);
    fw_cfg_dma_request.length = bswap32(length);
    fw_cfg_dma_request.address = bswap64((uintptr_t)buffer);
    __sync_synchronize();
    *(volatile uint64_t *)fw_cfg_dma_register = bswap64((uintptr_t)&fw_cfg_dma_request);

    for (;;) {
        __sync_synchronize();
        {
            uint32_t status = bswap32(fw_cfg_dma_request.control);
            if (status == 0U) {
                return;
            }
            if ((status & FW_CFG_DMA_CTL_ERROR) != 0U) {
                machine_panic("fw_cfg dma error");
            }
        }
    }
}

static int fw_cfg_lookup_file(const char *target, uint16_t *selector_out, uint32_t *size_out) {
    uint32_t count;
    uint32_t index;

    fw_cfg_dma_transfer(FW_CFG_FILE_DIR, FW_CFG_DMA_CTL_READ, &fw_cfg_files, sizeof(fw_cfg_files));
    count = bswap32(fw_cfg_files.count);
    if (count > MAX_FW_CFG_FILES) {
        machine_panic("fw_cfg file table too large");
    }

    for (index = 0; index < count; ++index) {
        const struct fw_cfg_file *file = &fw_cfg_files.files[index];
        const char *lhs = file->name;
        const char *rhs = target;
        while (*lhs != '\0' && *rhs != '\0' && *lhs == *rhs) {
            ++lhs;
            ++rhs;
        }
        if (*lhs == '\0' && *rhs == '\0') {
            if (selector_out != 0) {
                *selector_out = bswap16(file->select);
            }
            if (size_out != 0) {
                *size_out = bswap32(file->size);
            }
            return 1;
        }
    }

    return 0;
}

static uint16_t fw_cfg_find_file(const char *target) {
    uint16_t selector = 0U;

    if (!fw_cfg_lookup_file(target, &selector, 0)) {
        machine_panic("missing etc/ramfb");
    }
    return selector;
}

void machine_ramfb_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t stride) {
    uint16_t selector = fw_cfg_find_file("etc/ramfb");

    ramfb.addr = bswap64((uintptr_t)framebuffer);
    ramfb.fourcc = bswap32(DRM_FORMAT_XRGB8888);
    ramfb.flags = 0;
    ramfb.width = bswap32(width);
    ramfb.height = bswap32(height);
    ramfb.stride = bswap32(stride);

    fw_cfg_dma_transfer(selector, FW_CFG_DMA_CTL_WRITE, &ramfb, sizeof(ramfb));
}

uint32_t machine_fw_cfg_try_read_file(const char *target, void *buffer, uint32_t buffer_size) {
    uint16_t selector = 0U;
    uint32_t size = 0U;

    if (!fw_cfg_lookup_file(target, &selector, &size)) {
        return 0U;
    }
    if (size > buffer_size) {
        machine_panic("fw_cfg file exceeds destination buffer");
    }
    fw_cfg_dma_transfer(selector, FW_CFG_DMA_CTL_READ, buffer, size);
    return size;
}
