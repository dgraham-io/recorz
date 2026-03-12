#include "machine.h"

#include <stddef.h>
#include <stdint.h>

#define UART0_DEFAULT_BASE 0x10000000UL
#define FW_CFG_DEFAULT_BASE 0x10100000UL
#define UART_REGISTER_DATA 0x0U
#define UART_REGISTER_LINE_STATUS 0x5U
#define UART_LINE_STATUS_DATA_READY 0x01U
#define UART_LINE_STATUS_TRANSMITTER_EMPTY 0x20U

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
#define MAX_VIRTIO_CANDIDATES 8U

#define DRM_FORMAT_XRGB8888 0x34325258U
#define MAX_FW_CFG_FILES 128U

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976U
#define VIRTIO_MMIO_VERSION_VALUE 2U
#define VIRTIO_MMIO_DEVICE_ID_INPUT 18U
#define VIRTIO_MMIO_REGISTER_MAGIC 0x000U
#define VIRTIO_MMIO_REGISTER_VERSION 0x004U
#define VIRTIO_MMIO_REGISTER_DEVICE_ID 0x008U
#define VIRTIO_MMIO_REGISTER_DEVICE_FEATURES 0x010U
#define VIRTIO_MMIO_REGISTER_DEVICE_FEATURES_SEL 0x014U
#define VIRTIO_MMIO_REGISTER_DRIVER_FEATURES 0x020U
#define VIRTIO_MMIO_REGISTER_DRIVER_FEATURES_SEL 0x024U
#define VIRTIO_MMIO_REGISTER_GUEST_PAGE_SIZE 0x028U
#define VIRTIO_MMIO_REGISTER_QUEUE_SEL 0x030U
#define VIRTIO_MMIO_REGISTER_QUEUE_NUM_MAX 0x034U
#define VIRTIO_MMIO_REGISTER_QUEUE_NUM 0x038U
#define VIRTIO_MMIO_REGISTER_QUEUE_ALIGN 0x03cU
#define VIRTIO_MMIO_REGISTER_QUEUE_PFN 0x040U
#define VIRTIO_MMIO_REGISTER_QUEUE_READY 0x044U
#define VIRTIO_MMIO_REGISTER_QUEUE_NOTIFY 0x050U
#define VIRTIO_MMIO_REGISTER_INTERRUPT_STATUS 0x060U
#define VIRTIO_MMIO_REGISTER_INTERRUPT_ACK 0x064U
#define VIRTIO_MMIO_REGISTER_STATUS 0x070U
#define VIRTIO_MMIO_REGISTER_QUEUE_DESC_LOW 0x080U
#define VIRTIO_MMIO_REGISTER_QUEUE_DESC_HIGH 0x084U
#define VIRTIO_MMIO_REGISTER_QUEUE_AVAIL_LOW 0x090U
#define VIRTIO_MMIO_REGISTER_QUEUE_AVAIL_HIGH 0x094U
#define VIRTIO_MMIO_REGISTER_QUEUE_USED_LOW 0x0a0U
#define VIRTIO_MMIO_REGISTER_QUEUE_USED_HIGH 0x0a4U

#define VIRTIO_STATUS_ACKNOWLEDGE 0x01U
#define VIRTIO_STATUS_DRIVER 0x02U
#define VIRTIO_STATUS_DRIVER_OK 0x04U
#define VIRTIO_STATUS_FEATURES_OK 0x08U
#define VIRTIO_STATUS_FAILED 0x80U
#define VIRTIO_FEATURE_VERSION_1 32U

#define VIRTQ_DESC_F_WRITE 0x02U
#define VIRTIO_INPUT_EVENT_QUEUE_INDEX 0U
#define VIRTIO_INPUT_QUEUE_SIZE 32U
#define KEYBOARD_QUEUE_ALIGNMENT 4096U
#define KEYBOARD_QUEUE_REGION_SIZE (KEYBOARD_QUEUE_ALIGNMENT * 2U)
#define KEYBOARD_CHAR_QUEUE_SIZE 128U

#define INPUT_EVENT_TYPE_SYN 0U
#define INPUT_EVENT_TYPE_KEY 1U
#define INPUT_EVENT_VALUE_RELEASE 0U
#define INPUT_EVENT_VALUE_PRESS 1U
#define INPUT_EVENT_VALUE_REPEAT 2U

#define KEY_ESC 1U
#define KEY_1 2U
#define KEY_2 3U
#define KEY_3 4U
#define KEY_4 5U
#define KEY_5 6U
#define KEY_6 7U
#define KEY_7 8U
#define KEY_8 9U
#define KEY_9 10U
#define KEY_0 11U
#define KEY_MINUS 12U
#define KEY_EQUAL 13U
#define KEY_BACKSPACE 14U
#define KEY_TAB 15U
#define KEY_Q 16U
#define KEY_W 17U
#define KEY_E 18U
#define KEY_R 19U
#define KEY_T 20U
#define KEY_Y 21U
#define KEY_U 22U
#define KEY_I 23U
#define KEY_O 24U
#define KEY_P 25U
#define KEY_LEFTBRACE 26U
#define KEY_RIGHTBRACE 27U
#define KEY_ENTER 28U
#define KEY_LEFTCTRL 29U
#define KEY_A 30U
#define KEY_S 31U
#define KEY_D 32U
#define KEY_F 33U
#define KEY_G 34U
#define KEY_H 35U
#define KEY_J 36U
#define KEY_K 37U
#define KEY_L 38U
#define KEY_SEMICOLON 39U
#define KEY_APOSTROPHE 40U
#define KEY_GRAVE 41U
#define KEY_LEFTSHIFT 42U
#define KEY_BACKSLASH 43U
#define KEY_Z 44U
#define KEY_X 45U
#define KEY_C 46U
#define KEY_V 47U
#define KEY_B 48U
#define KEY_N 49U
#define KEY_M 50U
#define KEY_COMMA 51U
#define KEY_DOT 52U
#define KEY_SLASH 53U
#define KEY_RIGHTSHIFT 54U
#define KEY_SPACE 57U
#define KEY_RIGHTCTRL 97U
#define KEY_HOME 102U
#define KEY_UP 103U
#define KEY_LEFT 105U
#define KEY_RIGHT 106U
#define KEY_END 107U
#define KEY_DOWN 108U

#define SBI_EXTENSION_SYSTEM_RESET 0x53525354UL
#define SBI_FUNCTION_SYSTEM_RESET 0UL
#define SBI_RESET_TYPE_SHUTDOWN 0UL
#define SBI_RESET_REASON_NONE 0UL

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed, aligned(16)));

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_INPUT_QUEUE_SIZE];
    uint16_t used_event;
} __attribute__((packed, aligned(2)));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[VIRTIO_INPUT_QUEUE_SIZE];
    uint16_t avail_event;
} __attribute__((packed, aligned(4)));

struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed));

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
    uint8_t virtio_candidate;
};

struct discovered_devices {
    uint64_t uart_base;
    uint64_t fw_cfg_base;
    uint64_t virtio_bases[MAX_VIRTIO_CANDIDATES];
    uint8_t have_uart;
    uint8_t have_fw_cfg;
    uint8_t virtio_count;
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
static uintptr_t keyboard_mmio_base = 0U;
static uint16_t keyboard_queue_size = 0U;
static uint16_t keyboard_used_index = 0U;
static uint8_t keyboard_initialized = 0U;
static uint8_t keyboard_shift_down = 0U;
static uint8_t keyboard_ctrl_down = 0U;
static uint16_t keyboard_char_head = 0U;
static uint16_t keyboard_char_tail = 0U;
static char keyboard_char_queue[KEYBOARD_CHAR_QUEUE_SIZE];
static uint8_t keyboard_queue_region[KEYBOARD_QUEUE_REGION_SIZE] __attribute__((aligned(KEYBOARD_QUEUE_ALIGNMENT)));
static struct virtq_desc *keyboard_desc = 0;
static struct virtq_avail *keyboard_avail = 0;
static volatile struct virtq_used *keyboard_used = 0;
static struct virtio_input_event keyboard_events[VIRTIO_INPUT_QUEUE_SIZE];

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

static uint32_t mmio_read32(uintptr_t base, uintptr_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

static void mmio_write32(uintptr_t base, uintptr_t offset, uint32_t value) {
    *(volatile uint32_t *)(base + offset) = value;
}

static uint16_t min_u16(uint16_t lhs, uint16_t rhs) {
    return lhs < rhs ? lhs : rhs;
}

static uint16_t floor_power_of_two_u16(uint16_t value) {
    uint16_t result = 1U;

    if (value == 0U) {
        return 0U;
    }
    while ((uint16_t)(result << 1U) != 0U && (uint16_t)(result << 1U) <= value) {
        result = (uint16_t)(result << 1U);
    }
    return result;
}

static uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1U) & ~(alignment - 1U);
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

static void remember_virtio_candidate(
    struct discovered_devices *devices,
    uint64_t base
) {
    if (devices->virtio_count >= MAX_VIRTIO_CANDIDATES) {
        return;
    }
    devices->virtio_bases[devices->virtio_count++] = base;
}

static void keyboard_enqueue_byte(char ch) {
    uint16_t next_tail = (uint16_t)((keyboard_char_tail + 1U) % KEYBOARD_CHAR_QUEUE_SIZE);

    if (next_tail == keyboard_char_head) {
        return;
    }
    keyboard_char_queue[keyboard_char_tail] = ch;
    keyboard_char_tail = next_tail;
}

static void keyboard_enqueue_escape_sequence(const char *sequence) {
    while (*sequence != '\0') {
        keyboard_enqueue_byte(*sequence++);
    }
}

static uint8_t keyboard_dequeue_byte(char *out) {
    if (keyboard_char_head == keyboard_char_tail) {
        return 0U;
    }
    if (out != 0) {
        *out = keyboard_char_queue[keyboard_char_head];
    }
    keyboard_char_head = (uint16_t)((keyboard_char_head + 1U) % KEYBOARD_CHAR_QUEUE_SIZE);
    return 1U;
}

static uint8_t keyboard_modifier_is_pressed(uint32_t value) {
    return value == INPUT_EVENT_VALUE_PRESS || value == INPUT_EVENT_VALUE_REPEAT;
}

static int keyboard_letter_index(uint16_t code) {
    switch (code) {
        case KEY_A: return 0;
        case KEY_B: return 1;
        case KEY_C: return 2;
        case KEY_D: return 3;
        case KEY_E: return 4;
        case KEY_F: return 5;
        case KEY_G: return 6;
        case KEY_H: return 7;
        case KEY_I: return 8;
        case KEY_J: return 9;
        case KEY_K: return 10;
        case KEY_L: return 11;
        case KEY_M: return 12;
        case KEY_N: return 13;
        case KEY_O: return 14;
        case KEY_P: return 15;
        case KEY_Q: return 16;
        case KEY_R: return 17;
        case KEY_S: return 18;
        case KEY_T: return 19;
        case KEY_U: return 20;
        case KEY_V: return 21;
        case KEY_W: return 22;
        case KEY_X: return 23;
        case KEY_Y: return 24;
        case KEY_Z: return 25;
        default:
            return -1;
    }
}

static char keyboard_ascii_for_letter(uint16_t code, uint8_t uppercase) {
    int index = keyboard_letter_index(code);

    if (index < 0) {
        return '\0';
    }
    return (char)((uppercase ? 'A' : 'a') + index);
}

static char keyboard_ascii_for_key(uint16_t code, uint8_t shifted) {
    switch (code) {
        case KEY_1: return shifted ? '!' : '1';
        case KEY_2: return shifted ? '@' : '2';
        case KEY_3: return shifted ? '#' : '3';
        case KEY_4: return shifted ? '$' : '4';
        case KEY_5: return shifted ? '%' : '5';
        case KEY_6: return shifted ? '^' : '6';
        case KEY_7: return shifted ? '&' : '7';
        case KEY_8: return shifted ? '*' : '8';
        case KEY_9: return shifted ? '(' : '9';
        case KEY_0: return shifted ? ')' : '0';
        case KEY_MINUS: return shifted ? '_' : '-';
        case KEY_EQUAL: return shifted ? '+' : '=';
        case KEY_TAB: return '\t';
        case KEY_LEFTBRACE: return shifted ? '{' : '[';
        case KEY_RIGHTBRACE: return shifted ? '}' : ']';
        case KEY_SEMICOLON: return shifted ? ':' : ';';
        case KEY_APOSTROPHE: return shifted ? '"' : '\'';
        case KEY_GRAVE: return shifted ? '~' : '`';
        case KEY_BACKSLASH: return shifted ? '|' : '\\';
        case KEY_COMMA: return shifted ? '<' : ',';
        case KEY_DOT: return shifted ? '>' : '.';
        case KEY_SLASH: return shifted ? '?' : '/';
        case KEY_SPACE: return ' ';
        default:
            return keyboard_ascii_for_letter(code, shifted);
    }
}

static void keyboard_emit_key(uint16_t code) {
    if (keyboard_ctrl_down) {
        int letter_index = keyboard_letter_index(code);

        if (letter_index >= 0) {
            keyboard_enqueue_byte((char)(letter_index + 1));
            return;
        }
    }
    switch (code) {
        case KEY_ENTER:
            keyboard_enqueue_byte('\r');
            return;
        case KEY_BACKSPACE:
            keyboard_enqueue_byte('\b');
            return;
        case KEY_ESC:
            keyboard_enqueue_byte(0x1b);
            return;
        case KEY_UP:
            keyboard_enqueue_escape_sequence("\x1b[A");
            return;
        case KEY_DOWN:
            keyboard_enqueue_escape_sequence("\x1b[B");
            return;
        case KEY_RIGHT:
            keyboard_enqueue_escape_sequence("\x1b[C");
            return;
        case KEY_LEFT:
            keyboard_enqueue_escape_sequence("\x1b[D");
            return;
        case KEY_HOME:
            keyboard_enqueue_escape_sequence("\x1b[H");
            return;
        case KEY_END:
            keyboard_enqueue_escape_sequence("\x1b[F");
            return;
        default: {
            char ascii = keyboard_ascii_for_key(code, keyboard_shift_down);
            if (ascii != '\0') {
                keyboard_enqueue_byte(ascii);
            }
            return;
        }
    }
}

static void keyboard_handle_event(struct virtio_input_event event) {
    if (event.type != INPUT_EVENT_TYPE_KEY) {
        return;
    }
    if (event.code == KEY_LEFTSHIFT || event.code == KEY_RIGHTSHIFT) {
        keyboard_shift_down = keyboard_modifier_is_pressed(event.value);
        return;
    }
    if (event.code == KEY_LEFTCTRL || event.code == KEY_RIGHTCTRL) {
        keyboard_ctrl_down = keyboard_modifier_is_pressed(event.value);
        return;
    }
    if (event.value == INPUT_EVENT_VALUE_PRESS || event.value == INPUT_EVENT_VALUE_REPEAT) {
        keyboard_emit_key(event.code);
    }
}

static void keyboard_submit_descriptor(uint16_t descriptor_index) {
    uint16_t ring_index;

    if (keyboard_queue_size == 0U || keyboard_avail == 0) {
        return;
    }
    ring_index = (uint16_t)(keyboard_avail->idx % keyboard_queue_size);
    keyboard_avail->ring[ring_index] = descriptor_index;
    __sync_synchronize();
    keyboard_avail->idx = (uint16_t)(keyboard_avail->idx + 1U);
    __sync_synchronize();
    mmio_write32(keyboard_mmio_base, VIRTIO_MMIO_REGISTER_QUEUE_NOTIFY, VIRTIO_INPUT_EVENT_QUEUE_INDEX);
}

static void keyboard_poll_events(void) {
    uint16_t used_idx;

    if (!keyboard_initialized || keyboard_used == 0) {
        return;
    }
    __sync_synchronize();
    used_idx = keyboard_used->idx;
    while (keyboard_used_index != used_idx) {
        uint16_t used_slot = (uint16_t)(keyboard_used_index % keyboard_queue_size);
        uint16_t descriptor_index = (uint16_t)keyboard_used->ring[used_slot].id;
        struct virtio_input_event event = keyboard_events[descriptor_index];

        if (descriptor_index < keyboard_queue_size) {
            keyboard_handle_event(event);
            keyboard_submit_descriptor(descriptor_index);
        }
        keyboard_used_index = (uint16_t)(keyboard_used_index + 1U);
        __sync_synchronize();
        used_idx = keyboard_used->idx;
    }
    {
        uint32_t interrupt_status = mmio_read32(keyboard_mmio_base, VIRTIO_MMIO_REGISTER_INTERRUPT_STATUS);
        if (interrupt_status != 0U) {
            mmio_write32(keyboard_mmio_base, VIRTIO_MMIO_REGISTER_INTERRUPT_ACK, interrupt_status);
        }
    }
}

static void keyboard_discard_pending_events(void) {
    uint16_t used_idx;

    if (!keyboard_initialized || keyboard_used == 0) {
        return;
    }
    used_idx = keyboard_used->idx;
    while (keyboard_used_index != used_idx) {
        uint16_t used_slot = (uint16_t)(keyboard_used_index % keyboard_queue_size);
        uint16_t descriptor_index = (uint16_t)keyboard_used->ring[used_slot].id;
        struct virtio_input_event event = keyboard_events[descriptor_index];

        if (descriptor_index < keyboard_queue_size) {
            if (event.type == INPUT_EVENT_TYPE_KEY) {
                if (event.code == KEY_LEFTSHIFT || event.code == KEY_RIGHTSHIFT) {
                    keyboard_shift_down = keyboard_modifier_is_pressed(event.value);
                } else if (event.code == KEY_LEFTCTRL || event.code == KEY_RIGHTCTRL) {
                    keyboard_ctrl_down = keyboard_modifier_is_pressed(event.value);
                }
            }
            keyboard_submit_descriptor(descriptor_index);
        }
        keyboard_used_index = (uint16_t)(keyboard_used_index + 1U);
        used_idx = keyboard_used->idx;
    }
    {
        uint32_t interrupt_status = mmio_read32(keyboard_mmio_base, VIRTIO_MMIO_REGISTER_INTERRUPT_STATUS);

        if (interrupt_status != 0U) {
            mmio_write32(keyboard_mmio_base, VIRTIO_MMIO_REGISTER_INTERRUPT_ACK, interrupt_status);
        }
    }
}

static uint8_t keyboard_init_transport(uintptr_t base) {
    uint32_t version = mmio_read32(base, VIRTIO_MMIO_REGISTER_VERSION);
    uint32_t device_features_high;
    uint16_t queue_num_max;
    uint16_t descriptor_index;
    uintptr_t queue_region_base;
    uintptr_t used_region_base;

    if (mmio_read32(base, VIRTIO_MMIO_REGISTER_MAGIC) != VIRTIO_MMIO_MAGIC_VALUE) {
        return 0U;
    }
    if (mmio_read32(base, VIRTIO_MMIO_REGISTER_DEVICE_ID) != VIRTIO_MMIO_DEVICE_ID_INPUT) {
        return 0U;
    }
    if (version != 1U && version != VIRTIO_MMIO_VERSION_VALUE) {
        return 0U;
    }

    mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, 0U);
    mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    if (version == VIRTIO_MMIO_VERSION_VALUE) {
        mmio_write32(base, VIRTIO_MMIO_REGISTER_DEVICE_FEATURES_SEL, 1U);
        device_features_high = mmio_read32(base, VIRTIO_MMIO_REGISTER_DEVICE_FEATURES);
        if ((device_features_high & 0x1U) == 0U) {
            mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, VIRTIO_STATUS_FAILED);
            return 0U;
        }
        mmio_write32(base, VIRTIO_MMIO_REGISTER_DRIVER_FEATURES_SEL, 0U);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_DRIVER_FEATURES, 0U);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_DRIVER_FEATURES_SEL, 1U);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_DRIVER_FEATURES, 0x1U);
        mmio_write32(
            base,
            VIRTIO_MMIO_REGISTER_STATUS,
            VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK
        );
        if ((mmio_read32(base, VIRTIO_MMIO_REGISTER_STATUS) & VIRTIO_STATUS_FEATURES_OK) == 0U) {
            mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, VIRTIO_STATUS_FAILED);
            return 0U;
        }
    } else {
        mmio_write32(base, VIRTIO_MMIO_REGISTER_DRIVER_FEATURES, 0U);
    }

    mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_SEL, VIRTIO_INPUT_EVENT_QUEUE_INDEX);
    queue_num_max = (uint16_t)mmio_read32(base, VIRTIO_MMIO_REGISTER_QUEUE_NUM_MAX);
    keyboard_queue_size = floor_power_of_two_u16(min_u16(queue_num_max, VIRTIO_INPUT_QUEUE_SIZE));
    if (keyboard_queue_size == 0U) {
        mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, VIRTIO_STATUS_FAILED);
        return 0U;
    }

    queue_region_base = (uintptr_t)keyboard_queue_region;
    keyboard_desc = (struct virtq_desc *)queue_region_base;
    keyboard_avail = (struct virtq_avail *)(queue_region_base + sizeof(struct virtq_desc) * VIRTIO_INPUT_QUEUE_SIZE);
    used_region_base = align_up((uintptr_t)keyboard_avail + sizeof(struct virtq_avail), KEYBOARD_QUEUE_ALIGNMENT);
    keyboard_used = (volatile struct virtq_used *)used_region_base;

    for (descriptor_index = 0U; descriptor_index < keyboard_queue_size; ++descriptor_index) {
        keyboard_desc[descriptor_index].addr = (uint64_t)(uintptr_t)&keyboard_events[descriptor_index];
        keyboard_desc[descriptor_index].len = sizeof(struct virtio_input_event);
        keyboard_desc[descriptor_index].flags = VIRTQ_DESC_F_WRITE;
        keyboard_desc[descriptor_index].next = 0U;
        keyboard_avail->ring[descriptor_index] = descriptor_index;
    }
    keyboard_avail->flags = 0U;
    keyboard_avail->idx = keyboard_queue_size;
    keyboard_avail->used_event = 0U;
    keyboard_used->flags = 0U;
    keyboard_used->idx = 0U;
    keyboard_used->avail_event = 0U;
    keyboard_used_index = 0U;
    keyboard_char_head = 0U;
    keyboard_char_tail = 0U;
    keyboard_shift_down = 0U;
    keyboard_ctrl_down = 0U;

    mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_NUM, keyboard_queue_size);
    if (version == VIRTIO_MMIO_VERSION_VALUE) {
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_DESC_LOW, (uint32_t)(uintptr_t)keyboard_desc);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_DESC_HIGH, 0U);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_AVAIL_LOW, (uint32_t)(uintptr_t)keyboard_avail);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_AVAIL_HIGH, 0U);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_USED_LOW, (uint32_t)(uintptr_t)keyboard_used);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_USED_HIGH, 0U);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_READY, 1U);
        mmio_write32(
            base,
            VIRTIO_MMIO_REGISTER_STATUS,
            VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK
        );
    } else {
        mmio_write32(base, VIRTIO_MMIO_REGISTER_GUEST_PAGE_SIZE, KEYBOARD_QUEUE_ALIGNMENT);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_ALIGN, KEYBOARD_QUEUE_ALIGNMENT);
        mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_PFN, (uint32_t)(queue_region_base / KEYBOARD_QUEUE_ALIGNMENT));
        mmio_write32(base, VIRTIO_MMIO_REGISTER_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK);
    }
    __sync_synchronize();
    mmio_write32(base, VIRTIO_MMIO_REGISTER_QUEUE_NOTIFY, VIRTIO_INPUT_EVENT_QUEUE_INDEX);

    keyboard_mmio_base = base;
    keyboard_initialized = 1U;
    return 1U;
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
    devices->virtio_count = 0U;

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
            stack[depth].virtio_candidate = 0U;
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
                if (stack[depth].virtio_candidate) {
                    remember_virtio_candidate(devices, stack[depth].reg_base);
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
                if (string_list_contains((const char *)value, property_length, "virtio,mmio")) {
                    stack[depth].virtio_candidate = 1U;
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
    uint32_t virtio_index;

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
    keyboard_initialized = 0U;
    keyboard_mmio_base = 0U;
    keyboard_queue_size = 0U;
    for (virtio_index = 0U; virtio_index < devices.virtio_count; ++virtio_index) {
        if (keyboard_init_transport((uintptr_t)devices.virtio_bases[virtio_index])) {
            break;
        }
    }
}

void machine_putc(char c) {
    while ((*(volatile uint8_t *)(uart_base + UART_REGISTER_LINE_STATUS) & UART_LINE_STATUS_TRANSMITTER_EMPTY) == 0U) {
    }
    *(volatile uint8_t *)(uart_base + UART_REGISTER_DATA) = (uint8_t)c;
}

uint8_t machine_try_getc(char *out) {
    if (keyboard_dequeue_byte(out)) {
        return 1U;
    }
    keyboard_poll_events();
    if (keyboard_dequeue_byte(out)) {
        return 1U;
    }
    if ((*(volatile uint8_t *)(uart_base + UART_REGISTER_LINE_STATUS) & UART_LINE_STATUS_DATA_READY) == 0U) {
        return 0U;
    }
    if (out != 0) {
        *out = (char)(*(volatile uint8_t *)(uart_base + UART_REGISTER_DATA));
    } else {
        (void)*(volatile uint8_t *)(uart_base + UART_REGISTER_DATA);
    }
    return 1U;
}

char machine_wait_getc(void) {
    char ch = '\0';

    while (!machine_try_getc(&ch)) {
    }
    return ch;
}

void machine_discard_pending_input(void) {
    keyboard_char_head = keyboard_char_tail;
    keyboard_discard_pending_events();
    keyboard_char_head = keyboard_char_tail;
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

void machine_shutdown(void) {
    register uintptr_t a0 asm("a0") = SBI_RESET_TYPE_SHUTDOWN;
    register uintptr_t a1 asm("a1") = SBI_RESET_REASON_NONE;
    register uintptr_t a6 asm("a6") = SBI_FUNCTION_SYSTEM_RESET;
    register uintptr_t a7 asm("a7") = SBI_EXTENSION_SYSTEM_RESET;

    __asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a6), "r"(a7) : "memory");
    machine_wait_forever();
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
