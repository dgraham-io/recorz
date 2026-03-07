#include "vm.h"

#include <stdint.h>

#include "display.h"
#include "machine.h"

#define STACK_LIMIT 64U
#define LEXICAL_LIMIT 32U
#define MAX_SEND_ARGS 10U
#define PRINT_BUFFER_SIZE 32U
#define HEAP_LIMIT RECORZ_MVP_HEAP_LIMIT
#define OBJECT_FIELD_LIMIT 4U
#define MONO_BITMAP_LIMIT 16U
#define MONO_BITMAP_MAX_WIDTH 32U
#define MONO_BITMAP_MAX_HEIGHT 64U
#define METHOD_SOURCE_LINE_LIMIT 128U
#define METHOD_SOURCE_NAME_LIMIT 64U
#define METHOD_SOURCE_CHUNK_LIMIT 512U
#define DYNAMIC_CLASS_LIMIT 16U

#define FORM_FIELD_BITS RECORZ_MVP_FORM_FIELD_BITS
#define BITMAP_FIELD_WIDTH RECORZ_MVP_BITMAP_FIELD_WIDTH
#define BITMAP_FIELD_HEIGHT RECORZ_MVP_BITMAP_FIELD_HEIGHT
#define BITMAP_FIELD_STORAGE_KIND RECORZ_MVP_BITMAP_FIELD_STORAGE_KIND
#define BITMAP_FIELD_STORAGE_ID RECORZ_MVP_BITMAP_FIELD_STORAGE_ID
#define TEXT_LAYOUT_FIELD_LEFT_MARGIN RECORZ_MVP_TEXT_LAYOUT_FIELD_LEFT
#define TEXT_LAYOUT_FIELD_TOP_MARGIN RECORZ_MVP_TEXT_LAYOUT_FIELD_TOP
#define TEXT_LAYOUT_FIELD_LINE_SPACING RECORZ_MVP_TEXT_LAYOUT_FIELD_LINE_SPACING
#define TEXT_LAYOUT_FIELD_PIXEL_SCALE RECORZ_MVP_TEXT_LAYOUT_FIELD_SCALE
#define TEXT_STYLE_FIELD_BACKGROUND_COLOR RECORZ_MVP_TEXT_STYLE_FIELD_BACKGROUND_COLOR
#define TEXT_STYLE_FIELD_FOREGROUND_COLOR RECORZ_MVP_TEXT_STYLE_FIELD_FOREGROUND_COLOR
#define TEXT_METRICS_FIELD_CELL_WIDTH RECORZ_MVP_TEXT_METRICS_FIELD_CELL_WIDTH
#define TEXT_METRICS_FIELD_CELL_HEIGHT RECORZ_MVP_TEXT_METRICS_FIELD_CELL_HEIGHT
#define TEXT_BEHAVIOR_FIELD_FALLBACK_BITMAP RECORZ_MVP_TEXT_BEHAVIOR_FIELD_FALLBACK_GLYPH
#define TEXT_BEHAVIOR_FIELD_CLEAR_ON_OVERFLOW RECORZ_MVP_TEXT_BEHAVIOR_FIELD_CLEAR_ON_OVERFLOW
#define CLASS_FIELD_SUPERCLASS RECORZ_MVP_CLASS_FIELD_SUPERCLASS
#define CLASS_FIELD_INSTANCE_KIND RECORZ_MVP_CLASS_FIELD_INSTANCE_KIND
#define CLASS_FIELD_METHOD_START RECORZ_MVP_CLASS_FIELD_METHOD_START
#define CLASS_FIELD_METHOD_COUNT RECORZ_MVP_CLASS_FIELD_METHOD_COUNT
#define METHOD_FIELD_SELECTOR RECORZ_MVP_METHOD_DESCRIPTOR_FIELD_SELECTOR
#define METHOD_FIELD_ARGUMENT_COUNT RECORZ_MVP_METHOD_DESCRIPTOR_FIELD_ARGUMENT_COUNT
#define METHOD_FIELD_PRIMITIVE_KIND RECORZ_MVP_METHOD_DESCRIPTOR_FIELD_PRIMITIVE_KIND
#define METHOD_FIELD_ENTRY RECORZ_MVP_METHOD_DESCRIPTOR_FIELD_ENTRY
#define METHOD_ENTRY_FIELD_EXECUTION_ID RECORZ_MVP_METHOD_ENTRY_FIELD_EXECUTION_ID
#define METHOD_ENTRY_FIELD_IMPLEMENTATION RECORZ_MVP_METHOD_ENTRY_FIELD_IMPLEMENTATION
#define SELECTOR_FIELD_SELECTOR_ID RECORZ_MVP_SELECTOR_FIELD_VALUE
#define COMPILED_METHOD_MAX_INSTRUCTIONS RECORZ_MVP_COMPILED_METHOD_MAX_INSTRUCTIONS

#define COMPILED_METHOD_OP_PUSH_GLOBAL RECORZ_MVP_COMPILED_METHOD_OP_PUSH_GLOBAL
#define COMPILED_METHOD_OP_PUSH_ROOT RECORZ_MVP_COMPILED_METHOD_OP_PUSH_ROOT
#define COMPILED_METHOD_OP_PUSH_ARGUMENT RECORZ_MVP_COMPILED_METHOD_OP_PUSH_ARGUMENT
#define COMPILED_METHOD_OP_PUSH_FIELD RECORZ_MVP_COMPILED_METHOD_OP_PUSH_FIELD
#define COMPILED_METHOD_OP_SEND RECORZ_MVP_COMPILED_METHOD_OP_SEND
#define COMPILED_METHOD_OP_RETURN_TOP RECORZ_MVP_COMPILED_METHOD_OP_RETURN_TOP
#define COMPILED_METHOD_OP_RETURN_RECEIVER RECORZ_MVP_COMPILED_METHOD_OP_RETURN_RECEIVER

#define BITMAP_STORAGE_FRAMEBUFFER RECORZ_MVP_BITMAP_STORAGE_FRAMEBUFFER
#define BITMAP_STORAGE_GLYPH_MONO RECORZ_MVP_BITMAP_STORAGE_GLYPH_MONO
#define BITMAP_STORAGE_HEAP_MONO 3U
#define MAX_OBJECT_KIND RECORZ_MVP_OBJECT_OBJECT

enum recorz_mvp_value_kind {
    RECORZ_MVP_VALUE_NIL = 0,
    RECORZ_MVP_VALUE_OBJECT = 1,
    RECORZ_MVP_VALUE_STRING = 2,
    RECORZ_MVP_VALUE_SMALL_INTEGER = 3,
};

struct recorz_mvp_value {
    uint8_t kind;
    int32_t integer;
    const char *string;
};

struct recorz_mvp_heap_object {
    uint8_t kind;
    uint8_t field_count;
    uint16_t class_handle;
    struct recorz_mvp_value fields[OBJECT_FIELD_LIMIT];
};

enum recorz_mvp_method_return_mode {
    RECORZ_MVP_METHOD_RETURN_RESULT = 1,
    RECORZ_MVP_METHOD_RETURN_RECEIVER = 2,
};

static const uint32_t font5x7[128][7] = {
    [' '] = {0, 0, 0, 0, 0, 0, 0},
    ['-'] = {0, 0, 0, 31, 0, 0, 0},
    ['.'] = {0, 0, 0, 0, 0, 6, 6},
    ['A'] = {14, 17, 17, 31, 17, 17, 17},
    ['B'] = {30, 17, 17, 30, 17, 17, 30},
    ['C'] = {14, 17, 16, 16, 16, 17, 14},
    ['D'] = {28, 18, 17, 17, 17, 18, 28},
    ['E'] = {31, 16, 16, 30, 16, 16, 31},
    ['F'] = {31, 16, 16, 30, 16, 16, 16},
    ['G'] = {14, 17, 16, 23, 17, 17, 14},
    ['H'] = {17, 17, 17, 31, 17, 17, 17},
    ['I'] = {31, 4, 4, 4, 4, 4, 31},
    ['J'] = {1, 1, 1, 1, 17, 17, 14},
    ['K'] = {17, 18, 20, 24, 20, 18, 17},
    ['L'] = {16, 16, 16, 16, 16, 16, 31},
    ['M'] = {17, 27, 21, 21, 17, 17, 17},
    ['N'] = {17, 25, 21, 19, 17, 17, 17},
    ['O'] = {14, 17, 17, 17, 17, 17, 14},
    ['P'] = {30, 17, 17, 30, 16, 16, 16},
    ['Q'] = {14, 17, 17, 17, 21, 18, 13},
    ['R'] = {30, 17, 17, 30, 20, 18, 17},
    ['S'] = {15, 16, 16, 14, 1, 1, 30},
    ['T'] = {31, 4, 4, 4, 4, 4, 4},
    ['U'] = {17, 17, 17, 17, 17, 17, 14},
    ['V'] = {17, 17, 17, 17, 17, 10, 4},
    ['W'] = {17, 17, 17, 21, 21, 21, 10},
    ['X'] = {17, 17, 10, 4, 10, 17, 17},
    ['Y'] = {17, 17, 10, 4, 4, 4, 4},
    ['Z'] = {31, 1, 2, 4, 8, 16, 31},
};

static struct recorz_mvp_value stack[STACK_LIMIT];
static struct recorz_mvp_heap_object heap[HEAP_LIMIT];
static uint32_t stack_size = 0U;
static uint16_t heap_size = 0U;
static uint16_t class_handles_by_kind[MAX_OBJECT_KIND + 1U];
static uint16_t class_descriptor_handles_by_kind[MAX_OBJECT_KIND + 1U];
static uint16_t selector_handles_by_id[RECORZ_MVP_SELECTOR_NEW + 1U];
static uint16_t global_handles[RECORZ_MVP_GLOBAL_KERNEL_INSTALLER + 1U];
static uint16_t dynamic_class_handles[DYNAMIC_CLASS_LIMIT];
static char dynamic_class_names[DYNAMIC_CLASS_LIMIT][METHOD_SOURCE_NAME_LIMIT];
static uint16_t default_form_handle = 0U;
static uint16_t framebuffer_bitmap_handle = 0U;
static uint16_t next_dynamic_method_entry_execution_id = RECORZ_MVP_METHOD_ENTRY_COUNT;
static uint16_t dynamic_class_count = 0U;
static uint16_t glyph_bitmap_handles[128];
static uint16_t glyph_fallback_handle = 0U;
static uint16_t transcript_layout_handle = 0U;
static uint16_t transcript_style_handle = 0U;
static uint16_t transcript_metrics_handle = 0U;
static uint16_t transcript_behavior_handle = 0U;
static uint32_t mono_bitmap_pool[MONO_BITMAP_LIMIT][MONO_BITMAP_MAX_HEIGHT];
static uint16_t mono_bitmap_count = 0U;
static uint32_t cursor_x = 0U;
static uint32_t cursor_y = 0U;
static char print_buffer[PRINT_BUFFER_SIZE];

static uint16_t seeded_handles[HEAP_LIMIT];
static const char *panic_phase = "idle";
static uint32_t panic_pc = 0U;
static struct recorz_mvp_instruction panic_instruction;
static uint8_t panic_have_instruction = 0U;
static uint8_t panic_have_send = 0U;
static uint8_t panic_send_selector = 0U;
static uint16_t panic_send_argument_count = 0U;
static struct recorz_mvp_value panic_send_receiver;
static struct recorz_mvp_value panic_send_arguments[MAX_SEND_ARGS];

static struct recorz_mvp_value nil_value(void);
static uint32_t small_integer_u32(struct recorz_mvp_value value, const char *message);
static void heap_set_class(uint16_t handle, uint16_t class_handle);
static uint32_t compiled_method_instruction_word(const struct recorz_mvp_heap_object *compiled_method, uint8_t index);
static uint8_t compiled_method_instruction_opcode(uint32_t instruction);
static uint8_t compiled_method_instruction_operand_a(uint32_t instruction);
static uint16_t compiled_method_instruction_operand_b(uint32_t instruction);
static void install_compiled_method_update(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector,
    uint16_t argument_count,
    uint16_t compiled_method_handle
);
static uint16_t allocate_compiled_method_from_words(
    const uint32_t instruction_words[],
    uint8_t instruction_count
);

static void panic_put_u32(uint32_t value) {
    char digits[10];
    uint32_t index = 0U;

    if (value == 0U) {
        machine_putc('0');
        return;
    }
    while (value > 0U && index < 10U) {
        digits[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (index > 0U) {
        machine_putc(digits[--index]);
    }
}

static void panic_put_i32(int32_t value) {
    if (value < 0) {
        machine_putc('-');
        panic_put_u32((uint32_t)(-value));
        return;
    }
    panic_put_u32((uint32_t)value);
}

static void panic_put_string_sample(const char *text) {
    uint32_t index = 0U;

    machine_putc('"');
    if (text == 0) {
        machine_puts("<null>");
        machine_putc('"');
        return;
    }
    while (text[index] != '\0' && index < 24U) {
        char ch = text[index++];
        if (ch == '\n') {
            machine_puts("\\n");
            continue;
        }
        if (ch == '\r') {
            machine_puts("\\r");
            continue;
        }
        if (ch == '"' || ch == '\\') {
            machine_putc('\\');
            machine_putc(ch);
            continue;
        }
        if (ch < 32 || ch > 126) {
            machine_putc('?');
            continue;
        }
        machine_putc(ch);
    }
    if (text[index] != '\0') {
        machine_puts("...");
    }
    machine_putc('"');
}

static const char *opcode_name(uint8_t opcode) {
    switch (opcode) {
        case RECORZ_MVP_OP_PUSH_GLOBAL:
            return "pushGlobal";
        case RECORZ_MVP_OP_PUSH_LITERAL:
            return "pushLiteral";
        case RECORZ_MVP_OP_SEND:
            return "send";
        case RECORZ_MVP_OP_DUP:
            return "dup";
        case RECORZ_MVP_OP_POP:
            return "pop";
        case RECORZ_MVP_OP_RETURN:
            return "returnTop";
        case RECORZ_MVP_OP_PUSH_NIL:
            return "pushNil";
        case RECORZ_MVP_OP_PUSH_LEXICAL:
            return "pushLexical";
        case RECORZ_MVP_OP_STORE_LEXICAL:
            return "storeLexical";
        case RECORZ_MVP_OP_PUSH_ROOT:
            return "pushRoot";
        case RECORZ_MVP_OP_PUSH_ARGUMENT:
            return "pushArgument";
        case RECORZ_MVP_OP_PUSH_FIELD:
            return "pushField";
        case RECORZ_MVP_OP_RETURN_RECEIVER:
            return "returnReceiver";
    }
    return "unknown";
}

static const char *selector_name(uint8_t selector) {
    switch (selector) {
        case RECORZ_MVP_SELECTOR_SHOW:
            return "show:";
        case RECORZ_MVP_SELECTOR_CR:
            return "cr";
        case RECORZ_MVP_SELECTOR_WRITE_STRING:
            return "writeString:";
        case RECORZ_MVP_SELECTOR_NEWLINE:
            return "newline";
        case RECORZ_MVP_SELECTOR_DEFAULT_FORM:
            return "defaultForm";
        case RECORZ_MVP_SELECTOR_CLEAR:
            return "clear";
        case RECORZ_MVP_SELECTOR_WIDTH:
            return "width";
        case RECORZ_MVP_SELECTOR_HEIGHT:
            return "height";
        case RECORZ_MVP_SELECTOR_ADD:
            return "+";
        case RECORZ_MVP_SELECTOR_SUBTRACT:
            return "-";
        case RECORZ_MVP_SELECTOR_MULTIPLY:
            return "*";
        case RECORZ_MVP_SELECTOR_PRINT_STRING:
            return "printString";
        case RECORZ_MVP_SELECTOR_BITS:
            return "bits";
        case RECORZ_MVP_SELECTOR_FILL_FORM_COLOR:
            return "fillForm:color:";
        case RECORZ_MVP_SELECTOR_AT:
            return "at:";
        case RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE:
            return "copyBitmap:toForm:x:y:scale:";
        case RECORZ_MVP_SELECTOR_COPY_BITMAP_TO_FORM_X_Y_SCALE_COLOR:
            return "copyBitmap:toForm:x:y:scale:color:";
        case RECORZ_MVP_SELECTOR_MONO_WIDTH_HEIGHT:
            return "monoWidth:height:";
        case RECORZ_MVP_SELECTOR_FROM_BITS:
            return "fromBits:";
        case RECORZ_MVP_SELECTOR_COPY_BITMAP_SOURCE_X_SOURCE_Y_WIDTH_HEIGHT_TO_FORM_X_Y_SCALE_COLOR:
            return "copyBitmap:sourceX:sourceY:width:height:toForm:x:y:scale:color:";
        case RECORZ_MVP_SELECTOR_CLASS:
            return "class";
        case RECORZ_MVP_SELECTOR_INSTANCE_KIND:
            return "instanceKind";
        case RECORZ_MVP_SELECTOR_COMPILED_METHOD_WORD0_WORD1_WORD2_WORD3_INSTRUCTION_COUNT:
            return "compiledMethodWord0:word1:word2:word3:instructionCount:";
        case RECORZ_MVP_SELECTOR_INSTALL_COMPILED_METHOD_ON_CLASS_SELECTOR_ID_ARGUMENT_COUNT:
            return "installCompiledMethod:onClass:selectorId:argumentCount:";
        case RECORZ_MVP_SELECTOR_INSTALL_METHOD_SOURCE_ON_CLASS:
            return "installMethodSource:onClass:";
        case RECORZ_MVP_SELECTOR_FILE_IN_METHOD_CHUNKS_ON_CLASS:
            return "fileInMethodChunks:onClass:";
        case RECORZ_MVP_SELECTOR_FILE_IN_CLASS_CHUNKS:
            return "fileInClassChunks:";
        case RECORZ_MVP_SELECTOR_NEW:
            return "new";
    }
    return "unknown";
}

static const char *object_kind_name(uint8_t kind) {
    switch (kind) {
        case RECORZ_MVP_OBJECT_TRANSCRIPT:
            return "Transcript";
        case RECORZ_MVP_OBJECT_DISPLAY:
            return "Display";
        case RECORZ_MVP_OBJECT_FORM:
            return "Form";
        case RECORZ_MVP_OBJECT_BITMAP:
            return "Bitmap";
        case RECORZ_MVP_OBJECT_BITBLT:
            return "BitBlt";
        case RECORZ_MVP_OBJECT_GLYPHS:
            return "Glyphs";
        case RECORZ_MVP_OBJECT_FORM_FACTORY:
            return "FormFactory";
        case RECORZ_MVP_OBJECT_BITMAP_FACTORY:
            return "BitmapFactory";
        case RECORZ_MVP_OBJECT_TEXT_LAYOUT:
            return "TextLayout";
        case RECORZ_MVP_OBJECT_TEXT_STYLE:
            return "TextStyle";
        case RECORZ_MVP_OBJECT_TEXT_METRICS:
            return "TextMetrics";
        case RECORZ_MVP_OBJECT_TEXT_BEHAVIOR:
            return "TextBehavior";
        case RECORZ_MVP_OBJECT_CLASS:
            return "Class";
        case RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR:
            return "MethodDescriptor";
        case RECORZ_MVP_OBJECT_METHOD_ENTRY:
            return "MethodEntry";
        case RECORZ_MVP_OBJECT_SELECTOR:
            return "Selector";
        case RECORZ_MVP_OBJECT_COMPILED_METHOD:
            return "CompiledMethod";
        case RECORZ_MVP_OBJECT_KERNEL_INSTALLER:
            return "KernelInstaller";
        case RECORZ_MVP_OBJECT_OBJECT:
            return "Object";
    }
    return "UnknownObject";
}

static void panic_put_value(struct recorz_mvp_value value) {
    if (value.kind == RECORZ_MVP_VALUE_NIL) {
        machine_puts("nil");
        return;
    }
    if (value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_puts("smallInteger(");
        panic_put_i32(value.integer);
        machine_putc(')');
        return;
    }
    if (value.kind == RECORZ_MVP_VALUE_STRING) {
        machine_puts("string(");
        panic_put_string_sample(value.string);
        machine_putc(')');
        return;
    }
    if (value.kind == RECORZ_MVP_VALUE_OBJECT) {
        uint16_t handle = (uint16_t)value.integer;

        machine_puts("object#");
        panic_put_u32((uint32_t)handle);
        if (handle == 0U || handle > heap_size) {
            machine_puts("(invalid)");
            return;
        }
        machine_puts("(");
        machine_puts(object_kind_name(heap[handle - 1U].kind));
        machine_putc(')');
        return;
    }
    machine_puts("value(kind=");
    panic_put_u32((uint32_t)value.kind);
    machine_putc(')');
}

static void remember_send_context(
    uint8_t selector,
    uint16_t argument_count,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[]
) {
    uint32_t index;

    panic_have_send = 1U;
    panic_send_selector = selector;
    panic_send_argument_count = argument_count;
    panic_send_receiver = receiver;
    for (index = 0U; index < MAX_SEND_ARGS; ++index) {
        panic_send_arguments[index] = nil_value();
    }
    for (index = 0U; index < argument_count; ++index) {
        panic_send_arguments[index] = arguments[index];
    }
}

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8U);
}

static uint32_t read_u32_le(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) |
           ((uint32_t)bytes[3] << 24U);
}

static uint32_t encode_compiled_method_word(uint8_t opcode, uint8_t operand_a, uint16_t operand_b) {
    return (uint32_t)opcode |
           ((uint32_t)operand_a << 8U) |
           ((uint32_t)operand_b << 16U);
}

static uint8_t source_char_is_identifier_start(char ch) {
    return (uint8_t)(
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z') ||
        ch == '_'
    );
}

static uint8_t source_char_is_identifier_char(char ch) {
    return (uint8_t)(
        source_char_is_identifier_start(ch) ||
        (ch >= '0' && ch <= '9')
    );
}

static uint8_t source_names_equal(const char *left, const char *right) {
    uint32_t index = 0U;

    if (left == 0 || right == 0) {
        return 0U;
    }
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) {
            return 0U;
        }
        ++index;
    }
    return (uint8_t)(left[index] == right[index]);
}

static const char *source_skip_horizontal_space(const char *cursor) {
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r') {
        ++cursor;
    }
    return cursor;
}

static const char *source_skip_blank_lines(const char *cursor) {
    cursor = source_skip_horizontal_space(cursor);
    while (*cursor == '\n') {
        ++cursor;
        cursor = source_skip_horizontal_space(cursor);
    }
    return cursor;
}

static uint8_t source_starts_with(const char *text, const char *prefix) {
    uint32_t index = 0U;

    if (text == 0 || prefix == 0) {
        return 0U;
    }
    while (prefix[index] != '\0') {
        if (text[index] != prefix[index]) {
            return 0U;
        }
        ++index;
    }
    return 1U;
}

static uint32_t source_copy_trimmed_line(const char **cursor_ref, char buffer[], uint32_t buffer_size) {
    const char *cursor = *cursor_ref;
    const char *trimmed_start;
    const char *end;
    uint32_t length = 0U;

    cursor = source_skip_blank_lines(cursor);
    if (*cursor == '\0') {
        *cursor_ref = cursor;
        buffer[0] = '\0';
        return 0U;
    }
    trimmed_start = source_skip_horizontal_space(cursor);
    end = trimmed_start;
    while (*end != '\0' && *end != '\n') {
        ++end;
    }
    while (end > trimmed_start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
        --end;
    }
    while (trimmed_start + length < end) {
        if (length + 1U >= buffer_size) {
            machine_panic("KernelInstaller source line exceeds buffer capacity");
        }
        buffer[length] = trimmed_start[length];
        ++length;
    }
    buffer[length] = '\0';
    cursor = (*end == '\n') ? end + 1 : end;
    *cursor_ref = cursor;
    return length;
}

static uint32_t source_copy_next_chunk(const char **cursor_ref, char buffer[], uint32_t buffer_size) {
    const char *cursor = *cursor_ref;
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint32_t chunk_length = 0U;

    buffer[0] = '\0';
    while (source_copy_trimmed_line(&cursor, line, sizeof(line)) != 0U) {
        uint32_t line_length = 0U;

        if (source_names_equal(line, "!")) {
            if (chunk_length == 0U) {
                continue;
            }
            break;
        }
        while (line[line_length] != '\0') {
            ++line_length;
        }
        if (chunk_length != 0U) {
            if (chunk_length + 1U >= buffer_size) {
                machine_panic("KernelInstaller source chunk exceeds buffer capacity");
            }
            buffer[chunk_length++] = '\n';
        }
        if (chunk_length + line_length + 1U > buffer_size) {
            machine_panic("KernelInstaller source chunk exceeds buffer capacity");
        }
        for (line_length = 0U; line[line_length] != '\0'; ++line_length) {
            buffer[chunk_length++] = line[line_length];
        }
        buffer[chunk_length] = '\0';
    }
    *cursor_ref = cursor;
    return chunk_length;
}

static const char *source_parse_identifier(const char *cursor, char buffer[], uint32_t buffer_size);

static void source_parse_class_name_from_chunk(
    const char *chunk,
    char class_name[],
    uint32_t class_name_size
) {
    const char *cursor = chunk;

    if (!source_starts_with(cursor, "RecorzKernelClass:")) {
        machine_panic("KernelInstaller class chunk is missing a RecorzKernelClass header");
    }
    cursor += 18U;
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '#') {
        machine_panic("KernelInstaller class chunk header is missing a class name");
    }
    ++cursor;
    if (source_parse_identifier(cursor, class_name, class_name_size) == 0) {
        machine_panic("KernelInstaller class chunk header has an invalid class name");
    }
}

static const char *source_parse_identifier(const char *cursor, char buffer[], uint32_t buffer_size) {
    uint32_t length = 0U;

    if (!source_char_is_identifier_start(*cursor)) {
        return 0;
    }
    while (source_char_is_identifier_char(*cursor)) {
        if (length + 1U >= buffer_size) {
            machine_panic("KernelInstaller identifier exceeds buffer capacity");
        }
        buffer[length++] = *cursor++;
    }
    buffer[length] = '\0';
    return cursor;
}

static uint8_t source_global_id_for_name(const char *name) {
    if (source_names_equal(name, "Transcript")) {
        return RECORZ_MVP_GLOBAL_TRANSCRIPT;
    }
    if (source_names_equal(name, "Display")) {
        return RECORZ_MVP_GLOBAL_DISPLAY;
    }
    if (source_names_equal(name, "BitBlt")) {
        return RECORZ_MVP_GLOBAL_BITBLT;
    }
    if (source_names_equal(name, "Glyphs")) {
        return RECORZ_MVP_GLOBAL_GLYPHS;
    }
    if (source_names_equal(name, "Form")) {
        return RECORZ_MVP_GLOBAL_FORM;
    }
    if (source_names_equal(name, "Bitmap")) {
        return RECORZ_MVP_GLOBAL_BITMAP;
    }
    if (source_names_equal(name, "KernelInstaller")) {
        return RECORZ_MVP_GLOBAL_KERNEL_INSTALLER;
    }
    return 0U;
}

static uint8_t source_selector_id_for_name(const char *name) {
    uint8_t selector;

    for (selector = RECORZ_MVP_SELECTOR_SHOW;
         selector <= RECORZ_MVP_SELECTOR_NEW;
         ++selector) {
        if (source_names_equal(name, selector_name(selector))) {
            return selector;
        }
    }
    return 0U;
}

static void vm_panic_hook(const char *message) {
    uint32_t dump_count;
    uint32_t slot;

    (void)message;
    machine_puts("vm: phase=");
    machine_puts(panic_phase);
    machine_puts("\n");
    if (panic_have_instruction) {
        machine_puts("vm: pc=");
        panic_put_u32(panic_pc);
        machine_puts(" opcode=");
        machine_puts(opcode_name(panic_instruction.opcode));
        machine_puts(" operand_a=");
        panic_put_u32((uint32_t)panic_instruction.operand_a);
        machine_puts(" operand_b=");
        panic_put_u32((uint32_t)panic_instruction.operand_b);
        machine_puts("\n");
    }
    if (panic_have_send) {
        machine_puts("vm: send selector=");
        machine_puts(selector_name(panic_send_selector));
        machine_puts(" args=");
        panic_put_u32((uint32_t)panic_send_argument_count);
        machine_puts("\n");
        machine_puts("vm: receiver=");
        panic_put_value(panic_send_receiver);
        machine_puts("\n");
        for (slot = 0U; slot < panic_send_argument_count; ++slot) {
            machine_puts("vm: arg[");
            panic_put_u32(slot);
            machine_puts("]=");
            panic_put_value(panic_send_arguments[slot]);
            machine_puts("\n");
        }
    }
    machine_puts("vm: stack_size=");
    panic_put_u32(stack_size);
    machine_puts("\n");
    dump_count = stack_size < 4U ? stack_size : 4U;
    for (slot = 0U; slot < dump_count; ++slot) {
        uint32_t index = stack_size - 1U - slot;

        machine_puts("vm: stack_top[");
        panic_put_u32(slot);
        machine_puts("]=");
        panic_put_value(stack[index]);
        machine_puts("\n");
    }
}

static struct recorz_mvp_value nil_value(void) {
    return (struct recorz_mvp_value){RECORZ_MVP_VALUE_NIL, 0, 0};
}

static struct recorz_mvp_value object_value(uint16_t handle) {
    return (struct recorz_mvp_value){RECORZ_MVP_VALUE_OBJECT, (int32_t)handle, 0};
}

static struct recorz_mvp_value global_value(uint8_t global_id) {
    if (global_id == 0U || global_id > RECORZ_MVP_GLOBAL_KERNEL_INSTALLER || global_handles[global_id] == 0U) {
        machine_panic("unknown global in MVP VM");
    }
    return object_value(global_handles[global_id]);
}

static struct recorz_mvp_value seed_root_value(uint32_t root_id) {
    switch (root_id) {
        case RECORZ_MVP_SEED_ROOT_DEFAULT_FORM:
            return object_value(default_form_handle);
        case RECORZ_MVP_SEED_ROOT_FRAMEBUFFER_BITMAP:
            return object_value(framebuffer_bitmap_handle);
        case RECORZ_MVP_SEED_ROOT_TRANSCRIPT_BEHAVIOR:
            return object_value(transcript_behavior_handle);
        case RECORZ_MVP_SEED_ROOT_TRANSCRIPT_LAYOUT:
            return object_value(transcript_layout_handle);
        case RECORZ_MVP_SEED_ROOT_TRANSCRIPT_STYLE:
            return object_value(transcript_style_handle);
        case RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS:
            return object_value(transcript_metrics_handle);
    }
    machine_panic("unknown seed root in method body");
    return nil_value();
}

static struct recorz_mvp_value string_value(const char *text) {
    return (struct recorz_mvp_value){RECORZ_MVP_VALUE_STRING, 0, text};
}

static struct recorz_mvp_value small_integer_value(int32_t integer) {
    return (struct recorz_mvp_value){RECORZ_MVP_VALUE_SMALL_INTEGER, integer, 0};
}

static void render_small_integer(int32_t value) {
    char scratch[PRINT_BUFFER_SIZE];
    uint32_t index = 0U;
    uint32_t out_index = 0U;
    uint32_t cursor;
    uint32_t magnitude;
    int negative = 0;

    if (value == 0) {
        print_buffer[0] = '0';
        print_buffer[1] = '\0';
        return;
    }

    if (value < 0) {
        negative = 1;
        magnitude = (uint32_t)(-value);
    } else {
        magnitude = (uint32_t)value;
    }

    while (magnitude > 0U && index < PRINT_BUFFER_SIZE - 1U) {
        scratch[index++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    }

    if (negative) {
        print_buffer[out_index++] = '-';
    }
    for (cursor = 0U; cursor < index; ++cursor) {
        print_buffer[out_index++] = scratch[index - cursor - 1U];
    }
    print_buffer[out_index] = '\0';
}

static uint16_t heap_allocate(uint8_t kind) {
    uint16_t handle;
    uint32_t field_index;

    if (heap_size >= HEAP_LIMIT) {
        machine_panic("object heap overflow");
    }
    handle = (uint16_t)(heap_size + 1U);
    heap[heap_size].kind = kind;
    heap[heap_size].field_count = 0U;
    heap[heap_size].class_handle = 0U;
    for (field_index = 0U; field_index < OBJECT_FIELD_LIMIT; ++field_index) {
        heap[heap_size].fields[field_index] = nil_value();
    }
    ++heap_size;
    return handle;
}

static uint16_t heap_allocate_seeded_class(uint8_t kind) {
    uint16_t handle = heap_allocate(kind);

    if (kind <= MAX_OBJECT_KIND && class_handles_by_kind[kind] != 0U) {
        heap_set_class(handle, class_handles_by_kind[kind]);
    }
    return handle;
}

static struct recorz_mvp_heap_object *heap_object(uint16_t handle) {
    if (handle == 0U || handle > heap_size) {
        machine_panic("invalid heap object handle");
    }
    return &heap[handle - 1U];
}

static const struct recorz_mvp_heap_object *heap_object_for_value(struct recorz_mvp_value value) {
    if (value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("receiver is not a heap object");
    }
    return (const struct recorz_mvp_heap_object *)heap_object((uint16_t)value.integer);
}

static void heap_set_field(uint16_t handle, uint8_t index, struct recorz_mvp_value value) {
    struct recorz_mvp_heap_object *object = heap_object(handle);

    if (index >= OBJECT_FIELD_LIMIT) {
        machine_panic("field index out of range");
    }
    if (object->field_count <= index) {
        object->field_count = (uint8_t)(index + 1U);
    }
    object->fields[index] = value;
}

static struct recorz_mvp_value heap_get_field(const struct recorz_mvp_heap_object *object, uint8_t index) {
    if (index >= object->field_count || index >= OBJECT_FIELD_LIMIT) {
        machine_panic("missing object field");
    }
    return object->fields[index];
}

static void heap_set_class(uint16_t handle, uint16_t class_handle) {
    struct recorz_mvp_heap_object *object = heap_object(handle);

    object->class_handle = class_handle;
}

static const struct recorz_mvp_heap_object *class_object_for_heap_object(const struct recorz_mvp_heap_object *object) {
    const struct recorz_mvp_heap_object *class_object;

    if (object->class_handle == 0U) {
        machine_panic("seed object is missing a class link");
    }
    class_object = (const struct recorz_mvp_heap_object *)heap_object(object->class_handle);
    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("seed object class link does not point at a class");
    }
    return class_object;
}

static uint16_t seed_handle_at(const struct recorz_mvp_seed *seed, uint16_t index) {
    if (index >= seed->object_count) {
        machine_panic("seed object index out of range");
    }
    return seeded_handles[index];
}

static struct recorz_mvp_value seed_field_value(
    const struct recorz_mvp_seed *seed,
    const struct recorz_mvp_seed_field *field
) {
    if (field->kind == RECORZ_MVP_SEED_FIELD_NIL) {
        return nil_value();
    }
    if (field->kind == RECORZ_MVP_SEED_FIELD_SMALL_INTEGER) {
        return small_integer_value((int32_t)field->value);
    }
    if (field->kind == RECORZ_MVP_SEED_FIELD_OBJECT_INDEX) {
        if (field->value < 0) {
            machine_panic("seed object field index must be non-negative");
        }
        return object_value(seed_handle_at(seed, (uint16_t)field->value));
    }
    machine_panic("unknown seed field kind");
    return nil_value();
}

static uint16_t heap_handle_for_object(const struct recorz_mvp_heap_object *object) {
    uintptr_t offset;

    if (object < heap || object >= heap + heap_size) {
        machine_panic("heap object pointer is out of range");
    }
    offset = (uintptr_t)(object - heap);
    return (uint16_t)(offset + 1U);
}

static uint32_t class_instance_kind(const struct recorz_mvp_heap_object *class_object) {
    return small_integer_u32(
        heap_get_field(class_object, CLASS_FIELD_INSTANCE_KIND),
        "class instance kind is not a small integer"
    );
}

static uint32_t selector_object_selector_id(const struct recorz_mvp_heap_object *selector_object) {
    if (selector_object->kind != RECORZ_MVP_OBJECT_SELECTOR) {
        machine_panic("method descriptor selector does not point at a selector object");
    }
    return small_integer_u32(
        heap_get_field(selector_object, SELECTOR_FIELD_SELECTOR_ID),
        "selector object id is not a small integer"
    );
}

static const struct recorz_mvp_heap_object *method_descriptor_selector_object(const struct recorz_mvp_heap_object *method_object) {
    struct recorz_mvp_value selector_value = heap_get_field(method_object, METHOD_FIELD_SELECTOR);

    if (selector_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("method descriptor selector is not a selector object");
    }
    return heap_object_for_value(selector_value);
}

static uint32_t method_descriptor_selector(const struct recorz_mvp_heap_object *method_object) {
    return selector_object_selector_id(method_descriptor_selector_object(method_object));
}

static uint32_t method_descriptor_argument_count(const struct recorz_mvp_heap_object *method_object) {
    return small_integer_u32(
        heap_get_field(method_object, METHOD_FIELD_ARGUMENT_COUNT),
        "method descriptor argument count is not a small integer"
    );
}

static uint32_t method_descriptor_primitive_kind(const struct recorz_mvp_heap_object *method_object) {
    return small_integer_u32(
        heap_get_field(method_object, METHOD_FIELD_PRIMITIVE_KIND),
        "method descriptor primitive kind is not a small integer"
    );
}

static const struct recorz_mvp_heap_object *method_descriptor_entry_object(const struct recorz_mvp_heap_object *method_object) {
    struct recorz_mvp_value entry_value = heap_get_field(method_object, METHOD_FIELD_ENTRY);

    if (entry_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("method descriptor entry is not a method entry object");
    }
    return heap_object_for_value(entry_value);
}

static struct recorz_mvp_heap_object *mutable_method_descriptor_entry_object(const struct recorz_mvp_heap_object *method_object) {
    return heap_object(heap_handle_for_object(method_descriptor_entry_object(method_object)));
}

static uint16_t selector_object_handle(uint8_t selector) {
    if (selector == 0U || selector > RECORZ_MVP_SELECTOR_NEW ||
        selector_handles_by_id[selector] == 0U) {
        machine_panic("selector handle is not installed");
    }
    return selector_handles_by_id[selector];
}

static uint32_t method_entry_execution_id(const struct recorz_mvp_heap_object *entry_object) {
    if (entry_object->kind != RECORZ_MVP_OBJECT_METHOD_ENTRY) {
        machine_panic("method descriptor entry does not point at a method entry");
    }
    return small_integer_u32(
        heap_get_field(entry_object, METHOD_ENTRY_FIELD_EXECUTION_ID),
        "method entry execution id is not a small integer"
    );
}

static struct recorz_mvp_value method_entry_implementation_value(const struct recorz_mvp_heap_object *entry_object) {
    if (entry_object->kind != RECORZ_MVP_OBJECT_METHOD_ENTRY) {
        machine_panic("method descriptor entry does not point at a method entry");
    }
    return heap_get_field(entry_object, METHOD_ENTRY_FIELD_IMPLEMENTATION);
}

static uint32_t compiled_method_instruction_word(
    const struct recorz_mvp_heap_object *compiled_method,
    uint8_t index
) {
    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("method entry implementation is not a compiled method");
    }
    return small_integer_u32(
        heap_get_field(compiled_method, index),
        "compiled method instruction is not a small integer"
    );
}

static uint8_t compiled_method_instruction_opcode(uint32_t instruction) {
    return (uint8_t)(instruction & 0xFFU);
}

static uint8_t compiled_method_instruction_operand_a(uint32_t instruction) {
    return (uint8_t)((instruction >> 8) & 0xFFU);
}

static uint16_t compiled_method_instruction_operand_b(uint32_t instruction) {
    return (uint16_t)((instruction >> 16) & 0xFFFFU);
}

static uint32_t class_method_count(const struct recorz_mvp_heap_object *class_object) {
    return small_integer_u32(
        heap_get_field(class_object, CLASS_FIELD_METHOD_COUNT),
        "class method count is not a small integer"
    );
}

static const struct recorz_mvp_heap_object *class_method_start_object(const struct recorz_mvp_heap_object *class_object) {
    struct recorz_mvp_value method_start = heap_get_field(class_object, CLASS_FIELD_METHOD_START);

    if (method_start.kind == RECORZ_MVP_VALUE_NIL) {
        return 0;
    }
    if (method_start.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("class method start is not a method descriptor");
    }
    return heap_object_for_value(method_start);
}

static const struct recorz_mvp_heap_object *lookup_builtin_method_descriptor(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector,
    uint16_t argument_count
) {
    const struct recorz_mvp_heap_object *method_object = class_method_start_object(class_object);
    const struct recorz_mvp_heap_object *selector_object;
    uint32_t method_count = class_method_count(class_object);
    uint32_t method_index;

    if (method_count == 0U) {
        return 0;
    }
    if (method_object == 0) {
        machine_panic("class with methods is missing a method start");
    }
    if (selector == 0U || selector > RECORZ_MVP_SELECTOR_NEW) {
        machine_panic("selector id is out of range");
    }
    if (selector_handles_by_id[selector] == 0U) {
        return 0;
    }
    selector_object = (const struct recorz_mvp_heap_object *)heap_object(selector_handles_by_id[selector]);
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *candidate =
            (const struct recorz_mvp_heap_object *)heap_object((uint16_t)(method_object - heap + 1U + method_index));

        if (candidate->kind != RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR) {
            machine_panic("class method range contains a non-method descriptor");
        }
        if (method_descriptor_selector_object(candidate) == selector_object) {
            if (method_descriptor_argument_count(candidate) != argument_count) {
                machine_panic("selector argument count does not match method descriptor");
            }
            return candidate;
        }
    }
    return 0;
}

static const struct recorz_mvp_heap_object *class_object_for_kind(uint8_t kind) {
    if (kind == 0U || kind > MAX_OBJECT_KIND || class_descriptor_handles_by_kind[kind] == 0U) {
        machine_panic("method update class kind is out of range");
    }
    return (const struct recorz_mvp_heap_object *)heap_object(class_descriptor_handles_by_kind[kind]);
}

static uint32_t primitive_kind_for_heap_object(const struct recorz_mvp_heap_object *object) {
    return class_instance_kind(class_object_for_heap_object(object));
}

static void validate_compiled_method(
    const struct recorz_mvp_heap_object *compiled_method,
    uint32_t argument_count
) {
    uint8_t instruction_index;
    uint32_t stack_depth = 0U;

    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("compiled method entry implementation is not a compiled method");
    }
    if (compiled_method->field_count == 0U || compiled_method->field_count > COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("compiled method field count is invalid");
    }
    for (instruction_index = 0U; instruction_index < compiled_method->field_count; ++instruction_index) {
        uint32_t instruction = compiled_method_instruction_word(compiled_method, instruction_index);
        uint8_t opcode = compiled_method_instruction_opcode(instruction);
        uint8_t operand_a = compiled_method_instruction_operand_a(instruction);
        uint16_t operand_b = compiled_method_instruction_operand_b(instruction);
        uint32_t send_count;

        if (instruction_index + 1U < compiled_method->field_count &&
            (opcode == COMPILED_METHOD_OP_RETURN_TOP || opcode == COMPILED_METHOD_OP_RETURN_RECEIVER)) {
            machine_panic("compiled method returns before the final instruction");
        }
        switch (opcode) {
            case COMPILED_METHOD_OP_PUSH_GLOBAL:
                if (operand_a < RECORZ_MVP_GLOBAL_TRANSCRIPT || operand_a > RECORZ_MVP_GLOBAL_KERNEL_INSTALLER) {
                    machine_panic("compiled method pushGlobal global id is out of range");
                }
                ++stack_depth;
                break;
            case COMPILED_METHOD_OP_PUSH_ROOT:
                if (operand_a < RECORZ_MVP_SEED_ROOT_DEFAULT_FORM ||
                    operand_a > RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS) {
                    machine_panic("compiled method pushRoot root id is out of range");
                }
                ++stack_depth;
                break;
            case COMPILED_METHOD_OP_PUSH_ARGUMENT:
                if (operand_a >= argument_count) {
                    machine_panic("compiled method pushArgument index is out of range");
                }
                ++stack_depth;
                break;
            case COMPILED_METHOD_OP_PUSH_FIELD:
                if (operand_a >= OBJECT_FIELD_LIMIT) {
                    machine_panic("compiled method pushField index is out of range");
                }
                ++stack_depth;
                break;
            case COMPILED_METHOD_OP_SEND:
                if (operand_a < RECORZ_MVP_SELECTOR_SHOW ||
                    operand_a > RECORZ_MVP_SELECTOR_NEW) {
                    machine_panic("compiled method send selector is out of range");
                }
                send_count = operand_b;
                if (send_count > MAX_SEND_ARGS || send_count > argument_count) {
                    machine_panic("compiled method send argument count is out of range");
                }
                if (stack_depth < send_count + 1U) {
                    machine_panic("compiled method send stack underflow");
                }
                stack_depth -= send_count;
                break;
            case COMPILED_METHOD_OP_RETURN_TOP:
                if (stack_depth == 0U) {
                    machine_panic("compiled method returnTop stack underflow");
                }
                break;
            case COMPILED_METHOD_OP_RETURN_RECEIVER:
                break;
            default:
                machine_panic("compiled method opcode is unknown");
        }
    }
    if (compiled_method_instruction_opcode(
            compiled_method_instruction_word(compiled_method, (uint8_t)(compiled_method->field_count - 1U))
        ) != COMPILED_METHOD_OP_RETURN_TOP &&
        compiled_method_instruction_opcode(
            compiled_method_instruction_word(compiled_method, (uint8_t)(compiled_method->field_count - 1U))
        ) != COMPILED_METHOD_OP_RETURN_RECEIVER) {
        machine_panic("compiled method is missing a final return");
    }
}

static void validate_class_superclass(const struct recorz_mvp_heap_object *class_object) {
    struct recorz_mvp_value superclass_value = heap_get_field(class_object, CLASS_FIELD_SUPERCLASS);
    const struct recorz_mvp_heap_object *superclass_object;

    if (superclass_value.kind == RECORZ_MVP_VALUE_NIL) {
        return;
    }
    if (superclass_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("class superclass is not a class object");
    }
    superclass_object = heap_object_for_value(superclass_value);
    if (superclass_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("class superclass is not a class object");
    }
}

static void validate_class_method_table(
    const struct recorz_mvp_seed *seed,
    const struct recorz_mvp_heap_object *class_object
) {
    struct recorz_mvp_value method_start_value;
    uint32_t method_count;
    uint32_t method_offset;
    uint32_t class_kind;
    uint16_t start_handle;

    if (class_object->field_count <= CLASS_FIELD_METHOD_COUNT) {
        machine_panic("class descriptor is missing method table fields");
    }
    method_start_value = heap_get_field(class_object, CLASS_FIELD_METHOD_START);
    method_count = small_integer_u32(
        heap_get_field(class_object, CLASS_FIELD_METHOD_COUNT),
        "class method count is not a small integer"
    );
    if (method_count == 0U) {
        if (method_start_value.kind != RECORZ_MVP_VALUE_NIL) {
            machine_panic("class with zero methods has a method start");
        }
        return;
    }
    if (method_start_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("class method start is not a method descriptor");
    }
    start_handle = (uint16_t)method_start_value.integer;
    if (start_handle == 0U || start_handle > seed->object_count) {
        machine_panic("class method start is out of range");
    }
    if ((uint32_t)start_handle + method_count - 1U > seed->object_count) {
        machine_panic("class method range is out of range");
    }
    class_kind = class_instance_kind(class_object);
    for (method_offset = 0U; method_offset < method_count; ++method_offset) {
        const struct recorz_mvp_heap_object *method_object =
            (const struct recorz_mvp_heap_object *)heap_object((uint16_t)(start_handle + method_offset));
        const struct recorz_mvp_heap_object *entry_object;
        const struct recorz_mvp_heap_object *implementation_object;
        struct recorz_mvp_value implementation_value;
        uint32_t selector;
        uint32_t argument_count;
        uint32_t primitive_kind;
        uint32_t entry;
        uint32_t primitive_binding_id;

        if (method_object->kind != RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR) {
            machine_panic("class method range contains a non-method descriptor");
        }
        if (method_object->field_count <= METHOD_FIELD_ENTRY) {
            machine_panic("method descriptor is missing required fields");
        }
        selector = method_descriptor_selector(method_object);
        argument_count = method_descriptor_argument_count(method_object);
        primitive_kind = method_descriptor_primitive_kind(method_object);
        entry_object = method_descriptor_entry_object(method_object);
        entry = method_entry_execution_id(entry_object);
        implementation_value = method_entry_implementation_value(entry_object);
        if (selector < RECORZ_MVP_SELECTOR_SHOW ||
            selector > RECORZ_MVP_SELECTOR_NEW) {
            machine_panic("method descriptor selector is out of range");
        }
        if (argument_count > MAX_SEND_ARGS) {
            machine_panic("method descriptor argument count exceeds send capacity");
        }
        if (primitive_kind != class_kind) {
            machine_panic("method descriptor primitive kind does not match class instance kind");
        }
        if (entry == 0U || entry >= RECORZ_MVP_METHOD_ENTRY_COUNT) {
            machine_panic("method entry execution id is out of range");
        }
        if (implementation_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER) {
            primitive_binding_id = (uint32_t)implementation_value.integer;
            if (primitive_binding_id == 0U || primitive_binding_id >= RECORZ_MVP_PRIMITIVE_COUNT) {
                machine_panic("primitive method entry binding id is out of range");
            }
            continue;
        }
        if (implementation_value.kind != RECORZ_MVP_VALUE_OBJECT) {
            machine_panic("method entry implementation object is missing");
        }
        implementation_object = heap_object_for_value(implementation_value);
        validate_compiled_method(implementation_object, argument_count);
    }
}

static void validate_seed_class_graph(const struct recorz_mvp_seed *seed) {
    uint16_t seed_index;

    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(seeded_handles[seed_index]);
        const struct recorz_mvp_heap_object *class_object = class_object_for_heap_object(object);
        const struct recorz_mvp_heap_object *metaclass_object = class_object_for_heap_object(class_object);

        if (metaclass_object->kind != RECORZ_MVP_OBJECT_CLASS) {
            machine_panic("class metaclass link does not point at a class");
        }
        validate_class_superclass(class_object);
        if (class_instance_kind(class_object) != object->kind) {
            machine_panic("seed object class instance kind does not match object kind");
        }
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(seeded_handles[seed_index]);

        if (object->kind == RECORZ_MVP_OBJECT_CLASS) {
            validate_class_method_table(seed, object);
        }
    }
}

static void initialize_class_handle_cache(const struct recorz_mvp_seed *seed) {
    uint16_t seed_index;
    uint16_t kind;

    for (kind = 0U; kind <= MAX_OBJECT_KIND; ++kind) {
        class_handles_by_kind[kind] = 0U;
        class_descriptor_handles_by_kind[kind] = 0U;
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(seeded_handles[seed_index]);

        if (object->kind != RECORZ_MVP_OBJECT_CLASS) {
            continue;
        }
        kind = (uint16_t)class_instance_kind(object);
        if (kind == 0U || kind > MAX_OBJECT_KIND) {
            continue;
        }
        if (class_descriptor_handles_by_kind[kind] == 0U) {
            class_descriptor_handles_by_kind[kind] = seeded_handles[seed_index];
            class_handles_by_kind[kind] = seeded_handles[seed_index];
        }
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(seeded_handles[seed_index]);

        if (object->kind > MAX_OBJECT_KIND || object->class_handle == 0U) {
            continue;
        }
        if (class_handles_by_kind[object->kind] == 0U) {
            class_handles_by_kind[object->kind] = object->class_handle;
        }
    }
}

static void initialize_selector_handle_cache(const struct recorz_mvp_seed *seed) {
    uint16_t seed_index;
    uint16_t selector_id;

    for (selector_id = 0U; selector_id <= RECORZ_MVP_SELECTOR_NEW; ++selector_id) {
        selector_handles_by_id[selector_id] = 0U;
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(seeded_handles[seed_index]);

        if (object->kind != RECORZ_MVP_OBJECT_SELECTOR) {
            continue;
        }
        selector_id = (uint16_t)selector_object_selector_id(object);
        if (selector_handles_by_id[selector_id] != 0U) {
            machine_panic("seed contains duplicate selector objects");
        }
        selector_handles_by_id[selector_id] = seeded_handles[seed_index];
    }
}

static uint32_t bitmap_row_mask(uint32_t width) {
    if (width == 0U || width > MONO_BITMAP_MAX_WIDTH) {
        machine_panic("bitmap width out of supported range");
    }
    if (width == 32U) {
        return UINT32_MAX;
    }
    return (1U << width) - 1U;
}

static uint32_t small_integer_u32(struct recorz_mvp_value value, const char *message) {
    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        machine_panic(message);
    }
    return (uint32_t)value.integer;
}

static const struct recorz_mvp_heap_object *transcript_layout_object(void) {
    const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(transcript_layout_handle);

    if (object->kind != RECORZ_MVP_OBJECT_TEXT_LAYOUT) {
        machine_panic("transcript layout root is not a text layout");
    }
    return object;
}

static uint32_t transcript_layout_u32(uint8_t index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_layout_object(), index), message);
}

static uint32_t text_left_margin(void) {
    return transcript_layout_u32(TEXT_LAYOUT_FIELD_LEFT_MARGIN, "text layout left margin is not a small integer");
}

static uint32_t text_top_margin(void) {
    return transcript_layout_u32(TEXT_LAYOUT_FIELD_TOP_MARGIN, "text layout top margin is not a small integer");
}

static uint32_t text_line_spacing(void) {
    return transcript_layout_u32(TEXT_LAYOUT_FIELD_LINE_SPACING, "text layout line spacing is not a small integer");
}

static uint32_t text_pixel_scale(void) {
    uint32_t scale = transcript_layout_u32(TEXT_LAYOUT_FIELD_PIXEL_SCALE, "text layout pixel scale is not a small integer");

    if (scale == 0U) {
        machine_panic("text layout pixel scale must be non-zero");
    }
    return scale;
}

static const struct recorz_mvp_heap_object *transcript_style_object(void) {
    const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(transcript_style_handle);

    if (object->kind != RECORZ_MVP_OBJECT_TEXT_STYLE) {
        machine_panic("transcript style root is not a text style");
    }
    return object;
}

static uint32_t transcript_style_u32(uint8_t index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_style_object(), index), message);
}

static uint32_t text_background_color(void) {
    return transcript_style_u32(
        TEXT_STYLE_FIELD_BACKGROUND_COLOR,
        "text style background color is not a small integer"
    );
}

static uint32_t text_foreground_color(void) {
    return transcript_style_u32(
        TEXT_STYLE_FIELD_FOREGROUND_COLOR,
        "text style foreground color is not a small integer"
    );
}

static const struct recorz_mvp_heap_object *transcript_metrics_object(void) {
    const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(transcript_metrics_handle);

    if (object->kind != RECORZ_MVP_OBJECT_TEXT_METRICS) {
        machine_panic("transcript metrics root is not a text metrics object");
    }
    return object;
}

static uint32_t transcript_metrics_u32(uint8_t index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_metrics_object(), index), message);
}

static const struct recorz_mvp_heap_object *transcript_behavior_object(void) {
    const struct recorz_mvp_heap_object *object =
        (const struct recorz_mvp_heap_object *)heap_object(transcript_behavior_handle);

    if (object->kind != RECORZ_MVP_OBJECT_TEXT_BEHAVIOR) {
        machine_panic("transcript behavior root is not a text behavior object");
    }
    return object;
}

static uint32_t transcript_clear_on_overflow(void) {
    return small_integer_u32(
        heap_get_field(transcript_behavior_object(), TEXT_BEHAVIOR_FIELD_CLEAR_ON_OVERFLOW),
        "text behavior clear-on-overflow is not a small integer"
    );
}

static uint32_t char_width(void) {
    return transcript_metrics_u32(
        TEXT_METRICS_FIELD_CELL_WIDTH,
        "text metrics cell width is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t char_height(void) {
    return transcript_metrics_u32(
        TEXT_METRICS_FIELD_CELL_HEIGHT,
        "text metrics cell height is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t bitmap_width(const struct recorz_mvp_heap_object *bitmap) {
    return small_integer_u32(heap_get_field(bitmap, BITMAP_FIELD_WIDTH), "bitmap width is not a small integer");
}

static uint32_t bitmap_height(const struct recorz_mvp_heap_object *bitmap) {
    return small_integer_u32(heap_get_field(bitmap, BITMAP_FIELD_HEIGHT), "bitmap height is not a small integer");
}

static uint32_t bitmap_storage_kind(const struct recorz_mvp_heap_object *bitmap) {
    return small_integer_u32(heap_get_field(bitmap, BITMAP_FIELD_STORAGE_KIND), "bitmap storage kind is not a small integer");
}

static uint32_t bitmap_storage_id(const struct recorz_mvp_heap_object *bitmap) {
    return small_integer_u32(heap_get_field(bitmap, BITMAP_FIELD_STORAGE_ID), "bitmap storage id is not a small integer");
}

static uint16_t mono_bitmap_storage_allocate(uint32_t height) {
    uint16_t storage_id;
    uint32_t row;

    if (height == 0U || height > MONO_BITMAP_MAX_HEIGHT) {
        machine_panic("bitmap height out of supported range");
    }
    if (mono_bitmap_count >= MONO_BITMAP_LIMIT) {
        machine_panic("mono bitmap pool overflow");
    }
    storage_id = mono_bitmap_count++;
    for (row = 0U; row < MONO_BITMAP_MAX_HEIGHT; ++row) {
        mono_bitmap_pool[storage_id][row] = 0U;
    }
    return storage_id;
}

static const uint32_t *mono_bitmap_rows(const struct recorz_mvp_heap_object *bitmap) {
    uint32_t storage_kind = bitmap_storage_kind(bitmap);
    uint32_t storage_id = bitmap_storage_id(bitmap);

    if (storage_kind == BITMAP_STORAGE_GLYPH_MONO) {
        if (storage_id >= 128U) {
            machine_panic("glyph bitmap storage id out of range");
        }
        return font5x7[storage_id];
    }
    if (storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        if (storage_id >= MONO_BITMAP_LIMIT) {
            machine_panic("mono bitmap storage id out of range");
        }
        return mono_bitmap_pool[storage_id];
    }
    machine_panic("bitmap storage is not monochrome");
    return 0;
}

static uint32_t *mutable_mono_bitmap_rows(const struct recorz_mvp_heap_object *bitmap) {
    uint32_t storage_id;

    if (bitmap_storage_kind(bitmap) != BITMAP_STORAGE_HEAP_MONO) {
        machine_panic("bitmap storage is not writable monochrome data");
    }
    storage_id = bitmap_storage_id(bitmap);
    if (storage_id >= MONO_BITMAP_LIMIT) {
        machine_panic("mono bitmap storage id out of range");
    }
    return mono_bitmap_pool[storage_id];
}

static const struct recorz_mvp_heap_object *bitmap_for_form(const struct recorz_mvp_heap_object *form) {
    struct recorz_mvp_value bits_value = heap_get_field(form, FORM_FIELD_BITS);
    const struct recorz_mvp_heap_object *bitmap = heap_object_for_value(bits_value);

    if (bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
        machine_panic("form bits is not a bitmap");
    }
    return bitmap;
}

static struct recorz_mvp_heap_object *mutable_bitmap_for_form(const struct recorz_mvp_heap_object *form) {
    struct recorz_mvp_value bits_value = heap_get_field(form, FORM_FIELD_BITS);
    struct recorz_mvp_heap_object *bitmap;

    if (bits_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("form bits is not a heap object");
    }
    bitmap = heap_object((uint16_t)bits_value.integer);
    if (bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
        machine_panic("form bits is not a bitmap");
    }
    return bitmap;
}

static void reset_text_cursor(void) {
    cursor_x = text_left_margin();
    cursor_y = text_top_margin();
}

static const struct recorz_mvp_heap_object *glyph_bitmap_for_char(char ch) {
    uint32_t index = (uint8_t)ch;

    if (index >= 128U || glyph_bitmap_handles[index] == 0U) {
        return (const struct recorz_mvp_heap_object *)heap_object(glyph_fallback_handle);
    }
    return (const struct recorz_mvp_heap_object *)heap_object(glyph_bitmap_handles[index]);
}

static struct recorz_mvp_value glyph_bitmap_value_for_code(uint32_t code) {
    if (code >= 128U || glyph_bitmap_handles[code] == 0U) {
        return object_value(glyph_fallback_handle);
    }
    return object_value(glyph_bitmap_handles[code]);
}

static void fill_mono_bitmap(const struct recorz_mvp_heap_object *bitmap, uint8_t bit_value) {
    uint32_t *rows = mutable_mono_bitmap_rows(bitmap);
    uint32_t row;
    uint32_t height = bitmap_height(bitmap);
    uint32_t mask = bit_value ? bitmap_row_mask(bitmap_width(bitmap)) : 0U;

    for (row = 0U; row < height; ++row) {
        rows[row] = mask;
    }
}

static void bitblt_copy_mono_bitmap_to_mono_bitmap(
    const struct recorz_mvp_heap_object *source_bitmap,
    struct recorz_mvp_heap_object *dest_bitmap,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t copy_width,
    uint32_t copy_height,
    uint32_t x,
    uint32_t y,
    uint32_t scale,
    uint8_t transparent_zero
) {
    const uint32_t *source_rows = mono_bitmap_rows(source_bitmap);
    uint32_t *dest_rows = mutable_mono_bitmap_rows(dest_bitmap);
    uint32_t source_width = bitmap_width(source_bitmap);
    uint32_t dest_width = bitmap_width(dest_bitmap);
    uint32_t dest_height = bitmap_height(dest_bitmap);
    uint32_t source_row_offset;

    if (scale == 0U) {
        machine_panic("BitBlt copy scale must be non-zero");
    }

    for (source_row_offset = 0U; source_row_offset < copy_height; ++source_row_offset) {
        uint32_t source_row = source_y + source_row_offset;
        uint32_t source_bits = source_rows[source_row];
        uint32_t source_col_offset;
        for (source_col_offset = 0U; source_col_offset < copy_width; ++source_col_offset) {
            uint32_t source_col = source_x + source_col_offset;
            uint32_t bit = 1U << (source_width - source_col - 1U);
            uint8_t bit_is_set = (uint8_t)((source_bits & bit) != 0U);
            uint32_t dy;
            uint32_t dx;

            if (!bit_is_set && transparent_zero) {
                continue;
            }
            for (dy = 0U; dy < scale; ++dy) {
                uint32_t dest_y = y + (source_row_offset * scale) + dy;
                if (dest_y >= dest_height) {
                    continue;
                }
                for (dx = 0U; dx < scale; ++dx) {
                    uint32_t dest_x = x + (source_col_offset * scale) + dx;
                    uint32_t dest_mask;
                    if (dest_x >= dest_width) {
                        continue;
                    }
                    dest_mask = 1U << (dest_width - dest_x - 1U);
                    if (bit_is_set) {
                        dest_rows[dest_y] |= dest_mask;
                    } else {
                        dest_rows[dest_y] &= ~dest_mask;
                    }
                }
            }
        }
    }
}

static uint8_t normalize_bitblt_source_region(
    const struct recorz_mvp_heap_object *source_bitmap,
    uint32_t *source_x,
    uint32_t *source_y,
    uint32_t *copy_width,
    uint32_t *copy_height
) {
    uint32_t source_width = bitmap_width(source_bitmap);
    uint32_t source_height = bitmap_height(source_bitmap);

    if (*source_x >= source_width || *source_y >= source_height || *copy_width == 0U || *copy_height == 0U) {
        return 0U;
    }
    if (*copy_width > source_width - *source_x) {
        *copy_width = source_width - *source_x;
    }
    if (*copy_height > source_height - *source_y) {
        *copy_height = source_height - *source_y;
    }
    return 1U;
}

static void bitblt_copy_mono_bitmap_to_form(
    const struct recorz_mvp_heap_object *source_bitmap,
    const struct recorz_mvp_heap_object *dest_form,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t copy_width,
    uint32_t copy_height,
    uint32_t x,
    uint32_t y,
    uint32_t scale,
    uint32_t one_color,
    uint32_t zero_color,
    uint8_t transparent_zero
) {
    const struct recorz_mvp_heap_object *dest_bitmap = bitmap_for_form(dest_form);
    uint32_t storage_kind = bitmap_storage_kind(dest_bitmap);

    if (scale == 0U) {
        machine_panic("BitBlt copy scale must be non-zero");
    }
    if (!normalize_bitblt_source_region(source_bitmap, &source_x, &source_y, &copy_width, &copy_height)) {
        return;
    }

    if (storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        display_form_blit_mono_bitmap(
            x,
            y,
            mono_bitmap_rows(source_bitmap),
            bitmap_width(source_bitmap),
            bitmap_height(source_bitmap),
            source_x,
            source_y,
            copy_width,
            copy_height,
            scale,
            one_color,
            zero_color,
            transparent_zero
        );
        return;
    }
    if (storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        bitblt_copy_mono_bitmap_to_mono_bitmap(
            source_bitmap,
            mutable_bitmap_for_form(dest_form),
            source_x,
            source_y,
            copy_width,
            copy_height,
            x,
            y,
            scale,
            transparent_zero
        );
        return;
    }
    machine_panic("BitBlt destination bitmap storage is unsupported");
}

static void require_bitblt_copy_operands_at(
    const struct recorz_mvp_value arguments[],
    uint32_t source_index,
    uint32_t dest_index,
    const struct recorz_mvp_heap_object **source_bitmap,
    const struct recorz_mvp_heap_object **dest_form
) {
    if (arguments[source_index].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("BitBlt copy expects a bitmap source");
    }
    if (arguments[dest_index].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("BitBlt copy expects a form destination");
    }
    *source_bitmap = heap_object_for_value(arguments[source_index]);
    *dest_form = heap_object_for_value(arguments[dest_index]);
    if ((*source_bitmap)->kind != RECORZ_MVP_OBJECT_BITMAP) {
        machine_panic("BitBlt copy expects a bitmap source");
    }
    if ((*dest_form)->kind != RECORZ_MVP_OBJECT_FORM) {
        machine_panic("BitBlt copy expects a form destination");
    }
}

static void fill_form_color(const struct recorz_mvp_heap_object *form, uint32_t color) {
    const struct recorz_mvp_heap_object *bitmap = bitmap_for_form(form);
    uint32_t storage_kind = bitmap_storage_kind(bitmap);

    if (storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        display_form_fill_color(color);
        reset_text_cursor();
        return;
    }
    if (storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        fill_mono_bitmap(bitmap, (uint8_t)(color != 0U));
        return;
    }
    machine_panic("fill expects a framebuffer or heap monochrome form");
}

static void form_clear(const struct recorz_mvp_heap_object *form) {
    fill_form_color(form, text_background_color());
}

static void form_newline(const struct recorz_mvp_heap_object *form) {
    cursor_x = text_left_margin();
    cursor_y += char_height() + text_line_spacing();
    if (transcript_clear_on_overflow() && cursor_y + char_height() >= bitmap_height(bitmap_for_form(form))) {
        form_clear(form);
    }
}

static void form_write_string(const struct recorz_mvp_heap_object *form, const char *text) {
    uint32_t form_width = bitmap_width(bitmap_for_form(form));

    while (*text != '\0') {
        const struct recorz_mvp_heap_object *glyph_bitmap;

        if (*text == '\n') {
            form_newline(form);
            ++text;
            continue;
        }
        if (cursor_x + char_width() >= form_width) {
            form_newline(form);
        }
        glyph_bitmap = glyph_bitmap_for_char(*text);
        bitblt_copy_mono_bitmap_to_form(
            glyph_bitmap,
            form,
            0U,
            0U,
            bitmap_width(glyph_bitmap),
            bitmap_height(glyph_bitmap),
            cursor_x,
            cursor_y,
            text_pixel_scale(),
            text_foreground_color(),
            text_background_color(),
            0U
        );
        cursor_x += char_width();
        ++text;
    }
}

static struct recorz_mvp_value allocate_mono_bitmap_value(uint32_t width, uint32_t height) {
    uint16_t bitmap_handle;
    uint16_t storage_id;

    if (width == 0U || width > MONO_BITMAP_MAX_WIDTH) {
        machine_panic("bitmap width out of supported range");
    }
    if (height == 0U || height > MONO_BITMAP_MAX_HEIGHT) {
        machine_panic("bitmap height out of supported range");
    }
    storage_id = mono_bitmap_storage_allocate(height);
    bitmap_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_BITMAP);
    heap_set_field(bitmap_handle, BITMAP_FIELD_WIDTH, small_integer_value((int32_t)width));
    heap_set_field(bitmap_handle, BITMAP_FIELD_HEIGHT, small_integer_value((int32_t)height));
    heap_set_field(bitmap_handle, BITMAP_FIELD_STORAGE_KIND, small_integer_value((int32_t)BITMAP_STORAGE_HEAP_MONO));
    heap_set_field(bitmap_handle, BITMAP_FIELD_STORAGE_ID, small_integer_value((int32_t)storage_id));
    return object_value(bitmap_handle);
}

static struct recorz_mvp_value allocate_form_from_bits_value(struct recorz_mvp_value bits_value) {
    const struct recorz_mvp_heap_object *bitmap = heap_object_for_value(bits_value);
    uint16_t form_handle;

    if (bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
        machine_panic("Form fromBits: expects a bitmap");
    }
    form_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_FORM);
    heap_set_field(form_handle, FORM_FIELD_BITS, bits_value);
    return object_value(form_handle);
}

static void initialize_roots(const struct recorz_mvp_seed *seed) {
    uint32_t glyph_index;
    uint32_t code_index;
    uint16_t seed_index;
    uint16_t global_index;
    uint16_t dynamic_index;

    if (seed->object_count > HEAP_LIMIT) {
        machine_panic("seed object count exceeds heap capacity");
    }

    heap_size = 0U;
    mono_bitmap_count = 0U;
    next_dynamic_method_entry_execution_id = RECORZ_MVP_METHOD_ENTRY_COUNT;
    dynamic_class_count = 0U;
    for (dynamic_index = 0U; dynamic_index < DYNAMIC_CLASS_LIMIT; ++dynamic_index) {
        dynamic_class_handles[dynamic_index] = 0U;
        dynamic_class_names[dynamic_index][0] = '\0';
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        seeded_handles[seed_index] = heap_allocate(seed->objects[seed_index].object_kind);
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_seed_object *seed_object = &seed->objects[seed_index];

        if (seed_object->class_index == RECORZ_MVP_SEED_INVALID_OBJECT_INDEX) {
            continue;
        }
        heap_set_class(seeded_handles[seed_index], seed_handle_at(seed, seed_object->class_index));
    }
    for (seed_index = 0U; seed_index < seed->object_count; ++seed_index) {
        const struct recorz_mvp_seed_object *seed_object = &seed->objects[seed_index];
        uint8_t field_index;
        for (field_index = 0U; field_index < seed_object->field_count; ++field_index) {
            heap_set_field(
                seeded_handles[seed_index],
                field_index,
                seed_field_value(seed, &seed_object->fields[field_index])
            );
        }
    }
    validate_seed_class_graph(seed);
    initialize_class_handle_cache(seed);
    initialize_selector_handle_cache(seed);

    for (global_index = 0U; global_index <= RECORZ_MVP_GLOBAL_KERNEL_INSTALLER; ++global_index) {
        global_handles[global_index] = 0U;
    }
    for (global_index = RECORZ_MVP_GLOBAL_TRANSCRIPT; global_index <= RECORZ_MVP_GLOBAL_KERNEL_INSTALLER; ++global_index) {
        global_handles[global_index] = seed_handle_at(seed, seed->global_object_indices[global_index]);
    }

    default_form_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_DEFAULT_FORM]);
    framebuffer_bitmap_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_FRAMEBUFFER_BITMAP]);
    transcript_behavior_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_BEHAVIOR]);
    transcript_layout_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_LAYOUT]);
    transcript_style_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_STYLE]);
    transcript_metrics_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS]);
    if (heap_get_field(heap_object(default_form_handle), FORM_FIELD_BITS).kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)heap_get_field(heap_object(default_form_handle), FORM_FIELD_BITS).integer != framebuffer_bitmap_handle) {
        machine_panic("seed default form does not point at the framebuffer bitmap");
    }

    if (seed->glyph_code_count > 128U) {
        machine_panic("seed glyph code count exceeds MVP table capacity");
    }
    for (glyph_index = 0U; glyph_index < 128U; ++glyph_index) {
        glyph_bitmap_handles[glyph_index] = 0U;
    }
    {
        struct recorz_mvp_value fallback_value =
            heap_get_field(transcript_behavior_object(), TEXT_BEHAVIOR_FIELD_FALLBACK_BITMAP);
        const struct recorz_mvp_heap_object *fallback_bitmap = heap_object_for_value(fallback_value);

        if (fallback_bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
            machine_panic("text behavior fallback bitmap is not a bitmap");
        }
        glyph_fallback_handle = (uint16_t)fallback_value.integer;
    }
    for (code_index = 0U; code_index < seed->glyph_code_count; ++code_index) {
        uint16_t glyph_object_index = seed->glyph_object_indices_by_code[code_index];
        if (glyph_object_index == RECORZ_MVP_SEED_INVALID_OBJECT_INDEX) {
            continue;
        }
        glyph_bitmap_handles[code_index] = seed_handle_at(seed, glyph_object_index);
    }
    reset_text_cursor();
}

static void push(struct recorz_mvp_value value) {
    if (stack_size >= STACK_LIMIT) {
        machine_panic("bytecode stack overflow");
    }
    stack[stack_size++] = value;
}

static struct recorz_mvp_value pop_value(void) {
    if (stack_size == 0U) {
        machine_panic("bytecode stack underflow");
    }
    return stack[--stack_size];
}

static struct recorz_mvp_value literal_value(const struct recorz_mvp_literal *literal) {
    if (literal->kind == RECORZ_MVP_LITERAL_STRING) {
        return string_value(literal->string);
    }
    if (literal->kind == RECORZ_MVP_LITERAL_SMALL_INTEGER) {
        return small_integer_value(literal->integer);
    }
    machine_panic("unknown literal kind");
    return nil_value();
}

typedef struct recorz_mvp_instruction (*recorz_mvp_instruction_reader)(
    const void *instruction_source,
    uint32_t instruction_index
);

struct recorz_mvp_executable {
    const void *instruction_source;
    recorz_mvp_instruction_reader read_instruction;
    uint32_t instruction_count;
    const struct recorz_mvp_literal *literals;
    uint32_t literal_count;
    uint16_t lexical_count;
};

static struct recorz_mvp_instruction read_program_instruction(
    const void *instruction_source,
    uint32_t instruction_index
) {
    return ((const struct recorz_mvp_instruction *)instruction_source)[instruction_index];
}

static struct recorz_mvp_instruction decode_instruction_word(uint32_t instruction) {
    struct recorz_mvp_instruction decoded;

    decoded.opcode = compiled_method_instruction_opcode(instruction);
    decoded.operand_a = compiled_method_instruction_operand_a(instruction);
    decoded.operand_b = compiled_method_instruction_operand_b(instruction);
    return decoded;
}

static struct recorz_mvp_instruction read_compiled_method_instruction(
    const void *instruction_source,
    uint32_t instruction_index
) {
    const struct recorz_mvp_heap_object *compiled_method =
        (const struct recorz_mvp_heap_object *)instruction_source;

    return decode_instruction_word(compiled_method_instruction_word(compiled_method, (uint8_t)instruction_index));
}

typedef void (*recorz_mvp_method_entry_handler)(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
);

static void execute_entry_bitblt_fill_form_color(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *form;

    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("BitBlt fillForm:color: expects a form receiver argument");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("BitBlt fillForm:color: expects a small integer color");
    }
    form = heap_object_for_value(arguments[0]);
    if (primitive_kind_for_heap_object(form) != RECORZ_MVP_OBJECT_FORM) {
        machine_panic("BitBlt fillForm:color: expects a form");
    }
    fill_form_color(form, (uint32_t)arguments[1].integer);
    push(arguments[0]);
}

static void execute_entry_bitblt_copy_bitmap_to_form_x_y_scale(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *source_bitmap;
    const struct recorz_mvp_heap_object *dest_form;

    (void)object;
    (void)receiver;
    (void)text;
    require_bitblt_copy_operands_at(arguments, 0U, 1U, &source_bitmap, &dest_form);
    bitblt_copy_mono_bitmap_to_form(
        source_bitmap,
        dest_form,
        0U,
        0U,
        bitmap_width(source_bitmap),
        bitmap_height(source_bitmap),
        small_integer_u32(arguments[2], "BitBlt copy x must be a non-negative small integer"),
        small_integer_u32(arguments[3], "BitBlt copy y must be a non-negative small integer"),
        small_integer_u32(arguments[4], "BitBlt copy scale must be a non-negative small integer"),
        text_foreground_color(),
        0U,
        1U
    );
    push(arguments[1]);
}

static void execute_entry_bitblt_copy_bitmap_to_form_x_y_scale_color(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *source_bitmap;
    const struct recorz_mvp_heap_object *dest_form;

    (void)object;
    (void)receiver;
    (void)text;
    require_bitblt_copy_operands_at(arguments, 0U, 1U, &source_bitmap, &dest_form);
    bitblt_copy_mono_bitmap_to_form(
        source_bitmap,
        dest_form,
        0U,
        0U,
        bitmap_width(source_bitmap),
        bitmap_height(source_bitmap),
        small_integer_u32(arguments[2], "BitBlt copy x must be a non-negative small integer"),
        small_integer_u32(arguments[3], "BitBlt copy y must be a non-negative small integer"),
        small_integer_u32(arguments[4], "BitBlt copy scale must be a non-negative small integer"),
        small_integer_u32(arguments[5], "BitBlt copy color must be a non-negative small integer"),
        0U,
        1U
    );
    push(arguments[1]);
}

static void execute_entry_bitblt_copy_bitmap_region_to_form_x_y_scale_color(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *source_bitmap;
    const struct recorz_mvp_heap_object *dest_form;

    (void)object;
    (void)receiver;
    (void)text;
    require_bitblt_copy_operands_at(arguments, 0U, 5U, &source_bitmap, &dest_form);
    bitblt_copy_mono_bitmap_to_form(
        source_bitmap,
        dest_form,
        small_integer_u32(arguments[1], "BitBlt copy sourceX must be a non-negative small integer"),
        small_integer_u32(arguments[2], "BitBlt copy sourceY must be a non-negative small integer"),
        small_integer_u32(arguments[3], "BitBlt copy width must be a non-negative small integer"),
        small_integer_u32(arguments[4], "BitBlt copy height must be a non-negative small integer"),
        small_integer_u32(arguments[6], "BitBlt copy x must be a non-negative small integer"),
        small_integer_u32(arguments[7], "BitBlt copy y must be a non-negative small integer"),
        small_integer_u32(arguments[8], "BitBlt copy scale must be a non-negative small integer"),
        small_integer_u32(arguments[9], "BitBlt copy color must be a non-negative small integer"),
        0U,
        1U
    );
    push(arguments[5]);
}

static void execute_entry_glyphs_at(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)text;
    push(glyph_bitmap_value_for_code(small_integer_u32(arguments[0], "Glyphs at: expects a non-negative small integer")));
}

static void execute_entry_form_factory_from_bits(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("Form fromBits: expects a bitmap");
    }
    push(allocate_form_from_bits_value(arguments[0]));
}

static void execute_entry_bitmap_factory_mono_width_height(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)text;
    push(
        allocate_mono_bitmap_value(
            small_integer_u32(arguments[0], "Bitmap monoWidth:height: width must be a non-negative small integer"),
            small_integer_u32(arguments[1], "Bitmap monoWidth:height: height must be a non-negative small integer")
        )
    );
}

static void execute_entry_form_clear(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    form_clear(object);
    push(receiver);
}

static void execute_entry_form_write_string(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    form_write_string(object, text);
    push(receiver);
}

static void execute_entry_form_newline(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    form_newline(object);
    push(receiver);
}

static void execute_entry_class_new(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t instance_handle;

    (void)receiver;
    (void)arguments;
    (void)text;
    if (object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("Class new expects a class receiver");
    }
    instance_handle = heap_allocate(RECORZ_MVP_OBJECT_OBJECT);
    heap_set_class(instance_handle, heap_handle_for_object(object));
    push(object_value(instance_handle));
}

static uint8_t compile_source_operand_push(
    const char *name,
    const char *argument_name,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    uint8_t global_id;

    if (*instruction_count >= COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("KernelInstaller source method exceeds compiled method capacity");
    }
    if (argument_name[0] != '\0' && source_names_equal(name, argument_name)) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_ARGUMENT, 0U, 0U);
        return 1U;
    }
    global_id = source_global_id_for_name(name);
    if (global_id != 0U) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_GLOBAL, global_id, 0U);
        return 1U;
    }
    return 0U;
}

static uint8_t compile_source_statement_line(
    const char *line,
    const char *argument_name,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    char receiver_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_buffer[METHOD_SOURCE_NAME_LIMIT];
    char argument_buffer[METHOD_SOURCE_NAME_LIMIT];
    const char *cursor = line;
    const char *keyword_cursor;
    uint8_t selector_id;

    cursor = source_skip_horizontal_space(cursor);
    if (source_names_equal(cursor, "")) {
        return 1U;
    }
    if (source_names_equal(cursor, "Display defaultForm clear.") ||
        source_names_equal(cursor, "Display defaultForm newline.")) {
        const char *selector_name = source_names_equal(cursor, "Display defaultForm clear.") ? "clear" : "newline";

        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_ROOT, RECORZ_MVP_SEED_ROOT_DEFAULT_FORM, 0U);
        selector_id = source_selector_id_for_name(selector_name);
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_SEND, selector_id, 0U);
        return 1U;
    }
    if (source_names_equal(cursor, "Display defaultForm")) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_ROOT, RECORZ_MVP_SEED_ROOT_DEFAULT_FORM, 0U);
        return 1U;
    }
    if (source_names_equal(cursor, "Display newline.")) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_GLOBAL, RECORZ_MVP_GLOBAL_DISPLAY, 0U);
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_SEND, RECORZ_MVP_SELECTOR_NEWLINE, 0U);
        return 1U;
    }
    if (source_names_equal(cursor, "Display clear.")) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_GLOBAL, RECORZ_MVP_GLOBAL_DISPLAY, 0U);
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_SEND, RECORZ_MVP_SELECTOR_CLEAR, 0U);
        return 1U;
    }

    if (source_names_equal(cursor, "Display defaultForm writeString: text.")) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_ROOT, RECORZ_MVP_SEED_ROOT_DEFAULT_FORM, 0U);
        if (!compile_source_operand_push("text", argument_name, instruction_words, instruction_count)) {
            machine_panic("KernelInstaller source method uses an unknown argument operand");
        }
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_SEND, RECORZ_MVP_SELECTOR_WRITE_STRING, 1U);
        return 1U;
    }

    cursor = source_parse_identifier(cursor, receiver_name, sizeof(receiver_name));
    if (cursor == 0) {
        return 0U;
    }
    cursor = source_skip_horizontal_space(cursor);
    if (
        source_names_equal(receiver_name, "Display") &&
        cursor[0] == 'd' &&
        cursor[1] == 'e' &&
        cursor[2] == 'f' &&
        cursor[3] == 'a' &&
        cursor[4] == 'u' &&
        cursor[5] == 'l' &&
        cursor[6] == 't' &&
        cursor[7] == 'F' &&
        cursor[8] == 'o' &&
        cursor[9] == 'r' &&
        cursor[10] == 'm' &&
        (cursor[11] == ' ' || cursor[11] == '\t')
    ) {
        cursor += 11;
        cursor = source_skip_horizontal_space(cursor);
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_ROOT, RECORZ_MVP_SEED_ROOT_DEFAULT_FORM, 0U);
    } else {
        if (!compile_source_operand_push(receiver_name, argument_name, instruction_words, instruction_count)) {
            machine_panic("KernelInstaller source method uses an unsupported receiver expression");
        }
    }

    keyword_cursor = source_parse_identifier(cursor, selector_name_buffer, sizeof(selector_name_buffer));
    if (keyword_cursor == 0) {
        machine_panic("KernelInstaller source method statement is missing a selector");
    }
    cursor = keyword_cursor;
    if (*cursor == ':') {
        ++cursor;
        cursor = source_skip_horizontal_space(cursor);
        if (cursor == 0 || !source_char_is_identifier_start(*cursor)) {
            machine_panic("KernelInstaller source method keyword send is missing an argument");
        }
        if (*instruction_count >= COMPILED_METHOD_MAX_INSTRUCTIONS) {
            machine_panic("KernelInstaller source method exceeds compiled method capacity");
        }
        if (source_names_equal(cursor, "text.") || source_names_equal(cursor, "text")) {
            if (!compile_source_operand_push("text", argument_name, instruction_words, instruction_count)) {
                machine_panic("KernelInstaller source method uses an unsupported keyword argument");
            }
        } else {
            cursor = source_parse_identifier(cursor, argument_buffer, sizeof(argument_buffer));
            if (cursor == 0) {
                machine_panic("KernelInstaller source method keyword argument is invalid");
            }
            cursor = source_skip_horizontal_space(cursor);
            if (*cursor != '.' || cursor[1] != '\0') {
                machine_panic("KernelInstaller source method statement has unexpected trailing text");
            }
            if (!compile_source_operand_push(argument_buffer, argument_name, instruction_words, instruction_count)) {
                machine_panic("KernelInstaller source method uses an unsupported keyword argument");
            }
        }
        {
            uint32_t selector_length = 0U;

            while (selector_name_buffer[selector_length] != '\0') {
                ++selector_length;
            }
            if (selector_length + 1U >= sizeof(selector_name_buffer)) {
                machine_panic("KernelInstaller keyword selector exceeds buffer capacity");
            }
            selector_name_buffer[selector_length++] = ':';
            selector_name_buffer[selector_length] = '\0';
        }
        selector_id = source_selector_id_for_name(selector_name_buffer);
        if (selector_id == 0U) {
            machine_panic("KernelInstaller source method uses an unknown keyword selector");
        }
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_SEND, selector_id, 1U);
        return 1U;
    }
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '.' || cursor[1] != '\0') {
        machine_panic("KernelInstaller source method statement has unexpected trailing text");
    }
    selector_id = source_selector_id_for_name(selector_name_buffer);
    if (selector_id == 0U) {
        machine_panic("KernelInstaller source method uses an unknown unary selector");
    }
    instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_SEND, selector_id, 0U);
    return 1U;
}

static uint16_t compile_source_method_and_allocate(
    const char *source,
    uint8_t *selector_id_out,
    uint16_t *argument_count_out
) {
    char header_line[METHOD_SOURCE_LINE_LIMIT];
    char statement_line[METHOD_SOURCE_LINE_LIMIT];
    char return_line[METHOD_SOURCE_LINE_LIMIT];
    char trailing_line[METHOD_SOURCE_LINE_LIMIT];
    char selector_name_buffer[METHOD_SOURCE_NAME_LIMIT];
    char argument_name[METHOD_SOURCE_NAME_LIMIT];
    const char *cursor = source;
    const char *header_cursor;
    uint32_t instruction_words[COMPILED_METHOD_MAX_INSTRUCTIONS];
    uint8_t instruction_count = 0U;
    uint8_t selector_id;
    uint16_t argument_count = 0U;

    argument_name[0] = '\0';
    if (source == 0 || *source == '\0') {
        machine_panic("KernelInstaller source method string is empty");
    }
    if (source_copy_trimmed_line(&cursor, header_line, sizeof(header_line)) == 0U) {
        machine_panic("KernelInstaller source method is missing a header");
    }
    header_cursor = source_parse_identifier(header_line, selector_name_buffer, sizeof(selector_name_buffer));
    if (header_cursor == 0) {
        machine_panic("KernelInstaller source method header is invalid");
    }
    header_cursor = source_skip_horizontal_space(header_cursor);
    if (*header_cursor == ':') {
        ++header_cursor;
        header_cursor = source_skip_horizontal_space(header_cursor);
        header_cursor = source_parse_identifier(header_cursor, argument_name, sizeof(argument_name));
        if (header_cursor == 0) {
            machine_panic("KernelInstaller source method keyword header is missing an argument name");
        }
        header_cursor = source_skip_horizontal_space(header_cursor);
        if (*header_cursor != '\0') {
            machine_panic("KernelInstaller source method header has unexpected trailing text");
        }
        {
            uint32_t selector_length = 0U;

            while (selector_name_buffer[selector_length] != '\0') {
                ++selector_length;
            }
            if (selector_length + 1U >= sizeof(selector_name_buffer)) {
                machine_panic("KernelInstaller source method selector exceeds buffer capacity");
            }
            selector_name_buffer[selector_length++] = ':';
            selector_name_buffer[selector_length] = '\0';
        }
        argument_count = 1U;
    } else if (*header_cursor != '\0') {
        machine_panic("KernelInstaller source method header has unexpected trailing text");
    }
    selector_id = source_selector_id_for_name(selector_name_buffer);
    if (selector_id == 0U) {
        machine_panic("KernelInstaller source method uses an unknown selector");
    }

    if (source_copy_trimmed_line(&cursor, statement_line, sizeof(statement_line)) == 0U) {
        machine_panic("KernelInstaller source method is missing a return");
    }
    if (statement_line[0] == '^') {
        uint32_t return_index = 0U;

        while (statement_line[return_index] != '\0') {
            if (return_index + 1U >= sizeof(return_line)) {
                machine_panic("KernelInstaller source method return exceeds buffer capacity");
            }
            return_line[return_index] = statement_line[return_index];
            ++return_index;
        }
        return_line[return_index] = '\0';
    } else {
        if (!compile_source_statement_line(statement_line, argument_name, instruction_words, &instruction_count)) {
            machine_panic("KernelInstaller source method statement is unsupported");
        }
        if (source_copy_trimmed_line(&cursor, return_line, sizeof(return_line)) == 0U) {
            machine_panic("KernelInstaller source method is missing a return");
        }
    }
    if (!source_names_equal(return_line, "^self")) {
        machine_panic("KernelInstaller source method currently supports only ^self returns");
    }
    if (instruction_count >= COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("KernelInstaller source method exceeds compiled method capacity");
    }
    instruction_words[instruction_count++] = encode_compiled_method_word(COMPILED_METHOD_OP_RETURN_RECEIVER, 0U, 0U);
    if (source_copy_trimmed_line(&cursor, trailing_line, sizeof(trailing_line)) != 0U) {
        machine_panic("KernelInstaller source method has unexpected trailing lines");
    }
    *selector_id_out = selector_id;
    *argument_count_out = argument_count;
    return allocate_compiled_method_from_words(instruction_words, instruction_count);
}

static void file_in_method_chunks_on_class(
    const char *source,
    const struct recorz_mvp_heap_object *class_object
) {
    const char *cursor = source;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    uint8_t installed_method_count = 0U;

    if (source == 0 || *source == '\0') {
        machine_panic("KernelInstaller method chunk source is empty");
    }
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        uint16_t compiled_method_handle;
        uint8_t selector_id;
        uint16_t argument_count;

        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            continue;
        }
        compiled_method_handle = compile_source_method_and_allocate(chunk, &selector_id, &argument_count);
        validate_compiled_method(heap_object(compiled_method_handle), argument_count);
        install_compiled_method_update(class_object, selector_id, argument_count, compiled_method_handle);
        ++installed_method_count;
    }
    if (installed_method_count == 0U) {
        machine_panic("KernelInstaller fileInMethodChunks:onClass: found no installable method chunks");
    }
}

static const struct recorz_mvp_heap_object *lookup_class_by_name(const char *class_name) {
    uint8_t kind;
    uint16_t dynamic_index;

    if (class_name == 0 || *class_name == '\0') {
        machine_panic("KernelInstaller class name is empty");
    }
    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] == 0U) {
            continue;
        }
        if (source_names_equal(class_name, object_kind_name(kind))) {
            return (const struct recorz_mvp_heap_object *)heap_object(class_descriptor_handles_by_kind[kind]);
        }
    }
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        if (source_names_equal(class_name, dynamic_class_names[dynamic_index])) {
            return (const struct recorz_mvp_heap_object *)heap_object(dynamic_class_handles[dynamic_index]);
        }
    }
    return 0;
}

static const struct recorz_mvp_heap_object *ensure_class_named(const char *class_name) {
    const struct recorz_mvp_heap_object *class_object = lookup_class_by_name(class_name);
    uint16_t class_handle;
    uint32_t name_index = 0U;

    if (class_object != 0) {
        return class_object;
    }
    if (dynamic_class_count >= DYNAMIC_CLASS_LIMIT) {
        machine_panic("dynamic class registry overflow");
    }
    class_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_CLASS);
    heap_set_field(
        class_handle,
        CLASS_FIELD_SUPERCLASS,
        object_value(heap_handle_for_object(class_object_for_kind(RECORZ_MVP_OBJECT_OBJECT)))
    );
    heap_set_field(class_handle, CLASS_FIELD_INSTANCE_KIND, small_integer_value((int32_t)RECORZ_MVP_OBJECT_OBJECT));
    heap_set_field(class_handle, CLASS_FIELD_METHOD_START, nil_value());
    heap_set_field(class_handle, CLASS_FIELD_METHOD_COUNT, small_integer_value(0));
    while (class_name[name_index] != '\0') {
        if (name_index + 1U >= METHOD_SOURCE_NAME_LIMIT) {
            machine_panic("dynamic class name exceeds buffer capacity");
        }
        dynamic_class_names[dynamic_class_count][name_index] = class_name[name_index];
        ++name_index;
    }
    dynamic_class_names[dynamic_class_count][name_index] = '\0';
    dynamic_class_handles[dynamic_class_count] = class_handle;
    ++dynamic_class_count;
    machine_puts("recorz qemu-riscv64 mvp: created class ");
    machine_puts(class_name);
    machine_putc('\n');
    return (const struct recorz_mvp_heap_object *)heap_object(class_handle);
}

static void execute_entry_kernel_installer_compiled_method_word0_word1_word2_word3_instruction_count(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint32_t instruction_words[COMPILED_METHOD_MAX_INSTRUCTIONS];
    uint32_t instruction_count;
    uint32_t instruction_index;

    (void)object;
    (void)receiver;
    (void)text;
    instruction_count = small_integer_u32(
        arguments[4],
        "KernelInstaller instructionCount must be a non-negative small integer"
    );
    if (instruction_count == 0U || instruction_count > COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("KernelInstaller instructionCount is out of range");
    }
    for (instruction_index = 0U; instruction_index < instruction_count; ++instruction_index) {
        instruction_words[instruction_index] = small_integer_u32(
            arguments[instruction_index],
            "KernelInstaller compiled method words must be non-negative small integers"
        );
    }
    push(object_value(allocate_compiled_method_from_words(instruction_words, (uint8_t)instruction_count)));
}

static void execute_entry_kernel_installer_install_compiled_method_on_class_selector_id_argument_count(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *compiled_method;
    const struct recorz_mvp_heap_object *class_object;
    uint32_t selector_id;
    uint32_t argument_count;

    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller installCompiledMethod:onClass:selectorId:argumentCount: expects a compiled method");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller installCompiledMethod:onClass:selectorId:argumentCount: expects a class object");
    }
    compiled_method = heap_object_for_value(arguments[0]);
    class_object = heap_object_for_value(arguments[1]);
    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("KernelInstaller install expects a CompiledMethod");
    }
    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("KernelInstaller install expects a Class");
    }
    selector_id = small_integer_u32(
        arguments[2],
        "KernelInstaller selectorId must be a non-negative small integer"
    );
    argument_count = small_integer_u32(
        arguments[3],
        "KernelInstaller argumentCount must be a non-negative small integer"
    );
    if (selector_id == 0U || selector_id > RECORZ_MVP_SELECTOR_NEW) {
        machine_panic("KernelInstaller selectorId is out of range");
    }
    if (argument_count > MAX_SEND_ARGS) {
        machine_panic("KernelInstaller argumentCount exceeds send capacity");
    }
    validate_compiled_method(compiled_method, argument_count);
    install_compiled_method_update(
        class_object,
        (uint8_t)selector_id,
        (uint16_t)argument_count,
        heap_handle_for_object(compiled_method)
    );
    push(receiver);
}

static void execute_entry_kernel_installer_install_method_source_on_class(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    uint16_t compiled_method_handle;
    uint8_t selector_id;
    uint16_t argument_count;

    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller installMethodSource:onClass: expects a source string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller installMethodSource:onClass: expects a class object");
    }
    class_object = heap_object_for_value(arguments[1]);
    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("KernelInstaller installMethodSource:onClass: expects a Class");
    }
    compiled_method_handle = compile_source_method_and_allocate(arguments[0].string, &selector_id, &argument_count);
    validate_compiled_method(heap_object(compiled_method_handle), argument_count);
    install_compiled_method_update(class_object, selector_id, argument_count, compiled_method_handle);
    push(receiver);
}

static void execute_entry_kernel_installer_file_in_method_chunks_on_class(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller fileInMethodChunks:onClass: expects a source string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller fileInMethodChunks:onClass: expects a class object");
    }
    class_object = heap_object_for_value(arguments[1]);
    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("KernelInstaller fileInMethodChunks:onClass: expects a Class");
    }
    file_in_method_chunks_on_class(arguments[0].string, class_object);
    push(receiver);
}

static void execute_entry_kernel_installer_file_in_class_chunks(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    const char *cursor;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    char class_name[METHOD_SOURCE_NAME_LIMIT];

    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller fileInClassChunks: expects a source string");
    }
    cursor = arguments[0].string;
    if (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) == 0U) {
        machine_panic("KernelInstaller fileInClassChunks: source is empty");
    }
    source_parse_class_name_from_chunk(chunk, class_name, sizeof(class_name));
    class_object = ensure_class_named(class_name);
    file_in_method_chunks_on_class(arguments[0].string, class_object);
    push(object_value(heap_handle_for_object(class_object)));
}

static const recorz_mvp_method_entry_handler primitive_binding_handlers[RECORZ_MVP_PRIMITIVE_COUNT] = {
    RECORZ_MVP_GENERATED_PRIMITIVE_BINDING_HANDLERS
};

static void perform_send(
    struct recorz_mvp_value receiver,
    uint8_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    const char *text
);

static void activation_push(
    struct recorz_mvp_value activation_stack[],
    uint32_t *activation_stack_size,
    struct recorz_mvp_value value
) {
    if (*activation_stack_size >= STACK_LIMIT) {
        machine_panic("activation stack overflow");
    }
    activation_stack[(*activation_stack_size)++] = value;
}

static struct recorz_mvp_value activation_pop(
    struct recorz_mvp_value activation_stack[],
    uint32_t *activation_stack_size
) {
    if (*activation_stack_size == 0U) {
        machine_panic("activation stack underflow");
    }
    return activation_stack[--(*activation_stack_size)];
}

static struct recorz_mvp_value activation_peek(
    const struct recorz_mvp_value activation_stack[],
    uint32_t activation_stack_size
) {
    if (activation_stack_size == 0U) {
        machine_panic("activation stack is empty");
    }
    return activation_stack[activation_stack_size - 1U];
}

static void execute_executable(
    const struct recorz_mvp_executable *executable,
    const struct recorz_mvp_heap_object *receiver_object,
    struct recorz_mvp_value receiver,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[]
) {
    struct recorz_mvp_value activation_stack[STACK_LIMIT];
    struct recorz_mvp_value lexical[LEXICAL_LIMIT];
    uint32_t activation_stack_size = 0U;
    uint32_t lexical_index;
    uint32_t pc = 0U;

    if (executable->lexical_count > LEXICAL_LIMIT) {
        machine_panic("too many lexical slots for MVP VM");
    }
    for (lexical_index = 0U; lexical_index < executable->lexical_count; ++lexical_index) {
        lexical[lexical_index] = nil_value();
    }

    while (pc < executable->instruction_count) {
        struct recorz_mvp_instruction instruction =
            executable->read_instruction(executable->instruction_source, pc++);

        panic_phase = "execute";
        panic_pc = pc - 1U;
        panic_instruction = instruction;
        panic_have_instruction = 1U;
        panic_have_send = 0U;

        switch (instruction.opcode) {
            case RECORZ_MVP_OP_PUSH_GLOBAL:
                activation_push(activation_stack, &activation_stack_size, global_value(instruction.operand_a));
                break;
            case RECORZ_MVP_OP_PUSH_LITERAL:
                if ((uint32_t)instruction.operand_b >= executable->literal_count) {
                    machine_panic("literal out of range");
                }
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                    literal_value(&executable->literals[instruction.operand_b])
                );
                break;
            case RECORZ_MVP_OP_PUSH_NIL:
                activation_push(activation_stack, &activation_stack_size, nil_value());
                break;
            case RECORZ_MVP_OP_PUSH_LEXICAL:
                if ((uint32_t)instruction.operand_b >= executable->lexical_count) {
                    machine_panic("lexical read out of range");
                }
                activation_push(activation_stack, &activation_stack_size, lexical[instruction.operand_b]);
                break;
            case RECORZ_MVP_OP_STORE_LEXICAL:
                if ((uint32_t)instruction.operand_b >= executable->lexical_count) {
                    machine_panic("lexical write out of range");
                }
                lexical[instruction.operand_b] = activation_pop(activation_stack, &activation_stack_size);
                break;
            case RECORZ_MVP_OP_DUP:
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                    activation_peek(activation_stack, activation_stack_size)
                );
                break;
            case RECORZ_MVP_OP_POP:
                (void)activation_pop(activation_stack, &activation_stack_size);
                break;
            case RECORZ_MVP_OP_PUSH_ROOT:
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                    seed_root_value((uint32_t)instruction.operand_a)
                );
                break;
            case RECORZ_MVP_OP_PUSH_ARGUMENT:
                if (instruction.operand_a >= argument_count) {
                    machine_panic("argument read out of range");
                }
                activation_push(activation_stack, &activation_stack_size, arguments[instruction.operand_a]);
                break;
            case RECORZ_MVP_OP_PUSH_FIELD:
                if (receiver_object == 0) {
                    machine_panic("pushField requires a receiver object");
                }
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                    heap_get_field(receiver_object, instruction.operand_a)
                );
                break;
            case RECORZ_MVP_OP_SEND: {
                struct recorz_mvp_value send_arguments[MAX_SEND_ARGS];
                struct recorz_mvp_value send_receiver;
                uint16_t send_index;

                if (instruction.operand_b > MAX_SEND_ARGS) {
                    machine_panic("too many bytecode arguments");
                }
                if (activation_stack_size < (uint32_t)instruction.operand_b + 1U) {
                    machine_panic("send stack underflow");
                }
                for (send_index = instruction.operand_b; send_index > 0U; --send_index) {
                    send_arguments[send_index - 1U] = activation_pop(activation_stack, &activation_stack_size);
                }
                send_receiver = activation_pop(activation_stack, &activation_stack_size);
                panic_phase = "send";
                remember_send_context(instruction.operand_a, instruction.operand_b, send_receiver, send_arguments);
                perform_send(send_receiver, instruction.operand_a, instruction.operand_b, send_arguments, 0);
                activation_push(activation_stack, &activation_stack_size, pop_value());
                break;
            }
            case RECORZ_MVP_OP_RETURN:
                if (activation_stack_size == 0U) {
                    machine_panic("returnTop stack underflow");
                }
                push(activation_peek(activation_stack, activation_stack_size));
                return;
            case RECORZ_MVP_OP_RETURN_RECEIVER:
                push(receiver);
                return;
            default:
                machine_panic("unknown opcode in MVP VM");
        }
    }

    machine_panic("executable did not return");
}

static void execute_compiled_method(
    const struct recorz_mvp_heap_object *receiver_object,
    struct recorz_mvp_value receiver,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    const struct recorz_mvp_heap_object *compiled_method
) {
    const struct recorz_mvp_executable executable = {
        .instruction_source = compiled_method,
        .read_instruction = read_compiled_method_instruction,
        .instruction_count = compiled_method->field_count,
        .literals = 0,
        .literal_count = 0U,
        .lexical_count = 0U,
    };

    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("method entry implementation is not a compiled method");
    }
    execute_executable(&executable, receiver_object, receiver, argument_count, arguments);
}

static uint16_t allocate_compiled_method_from_words(
    const uint32_t instruction_words[],
    uint8_t instruction_count
) {
    uint16_t compiled_method_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_COMPILED_METHOD);
    uint8_t instruction_index;

    for (instruction_index = 0U; instruction_index < instruction_count; ++instruction_index) {
        heap_set_field(
            compiled_method_handle,
            instruction_index,
            small_integer_value((int32_t)instruction_words[instruction_index])
        );
    }
    return compiled_method_handle;
}

static uint16_t allocate_updated_compiled_method(
    const uint8_t *blob,
    uint16_t instruction_count
) {
    uint32_t instruction_words[COMPILED_METHOD_MAX_INSTRUCTIONS];
    uint32_t offset = RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE;
    uint16_t instruction_index;

    for (instruction_index = 0U; instruction_index < instruction_count; ++instruction_index) {
        instruction_words[instruction_index] = read_u32_le(blob + offset);
        offset += 4U;
    }
    return allocate_compiled_method_from_words(instruction_words, (uint8_t)instruction_count);
}

static uint16_t allocate_dynamic_method_entry(uint16_t compiled_method_handle) {
    uint16_t entry_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_METHOD_ENTRY);

    if (next_dynamic_method_entry_execution_id == 0U) {
        machine_panic("dynamic method entry execution id overflow");
    }
    heap_set_field(
        entry_handle,
        METHOD_ENTRY_FIELD_EXECUTION_ID,
        small_integer_value((int32_t)next_dynamic_method_entry_execution_id++)
    );
    heap_set_field(entry_handle, METHOD_ENTRY_FIELD_IMPLEMENTATION, object_value(compiled_method_handle));
    return entry_handle;
}

static uint16_t allocate_method_descriptor(
    uint8_t class_kind,
    uint8_t selector,
    uint16_t argument_count,
    uint16_t entry_handle
) {
    uint16_t descriptor_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR);

    heap_set_field(
        descriptor_handle,
        METHOD_FIELD_SELECTOR,
        object_value(selector_object_handle(selector))
    );
    heap_set_field(
        descriptor_handle,
        METHOD_FIELD_ARGUMENT_COUNT,
        small_integer_value((int32_t)argument_count)
    );
    heap_set_field(
        descriptor_handle,
        METHOD_FIELD_PRIMITIVE_KIND,
        small_integer_value((int32_t)class_kind)
    );
    heap_set_field(descriptor_handle, METHOD_FIELD_ENTRY, object_value(entry_handle));
    return descriptor_handle;
}

static uint16_t clone_method_descriptor(const struct recorz_mvp_heap_object *method_object) {
    uint16_t cloned_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR);
    uint8_t field_index;

    for (field_index = 0U; field_index < method_object->field_count; ++field_index) {
        heap_set_field(cloned_handle, field_index, heap_get_field(method_object, field_index));
    }
    return cloned_handle;
}

static void append_compiled_method_to_class(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector,
    uint16_t argument_count,
    uint16_t compiled_method_handle
) {
    uint16_t class_handle = heap_handle_for_object(class_object);
    uint32_t class_kind = class_instance_kind(class_object);
    uint32_t method_count = class_method_count(class_object);
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint16_t entry_handle = allocate_dynamic_method_entry(compiled_method_handle);
    uint16_t new_method_start_handle;
    uint32_t method_index;

    if (method_count == 0U) {
        uint16_t descriptor_handle = allocate_method_descriptor((uint8_t)class_kind, selector, argument_count, entry_handle);
        heap_set_field(class_handle, CLASS_FIELD_METHOD_START, object_value(descriptor_handle));
        heap_set_field(class_handle, CLASS_FIELD_METHOD_COUNT, small_integer_value(1));
        return;
    }
    if (method_start_object == 0) {
        machine_panic("class with methods is missing a method start");
    }
    new_method_start_handle = (uint16_t)(heap_size + 1U);
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *existing_method_object =
            (const struct recorz_mvp_heap_object *)heap_object((uint16_t)(heap_handle_for_object(method_start_object) + method_index));

        if (existing_method_object->kind != RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR) {
            machine_panic("class method range contains a non-method descriptor");
        }
        clone_method_descriptor(existing_method_object);
    }
    (void)allocate_method_descriptor((uint8_t)class_kind, selector, argument_count, entry_handle);
    heap_set_field(class_handle, CLASS_FIELD_METHOD_START, object_value(new_method_start_handle));
    heap_set_field(class_handle, CLASS_FIELD_METHOD_COUNT, small_integer_value((int32_t)(method_count + 1U)));
}

static void install_compiled_method_update(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector,
    uint16_t argument_count,
    uint16_t compiled_method_handle
) {
    const struct recorz_mvp_heap_object *method_object =
        lookup_builtin_method_descriptor(class_object, selector, argument_count);

    if (method_object != 0) {
        struct recorz_mvp_heap_object *entry_object = mutable_method_descriptor_entry_object(method_object);

        heap_set_field(
            heap_handle_for_object(entry_object),
            METHOD_ENTRY_FIELD_IMPLEMENTATION,
            object_value(compiled_method_handle)
        );
        return;
    }
    append_compiled_method_to_class(class_object, selector, argument_count, compiled_method_handle);
}

static void apply_method_update_payload(const uint8_t *blob, uint32_t size) {
    const struct recorz_mvp_heap_object *class_object;
    uint16_t version;
    uint16_t class_kind;
    uint16_t selector;
    uint16_t argument_count;
    uint16_t instruction_count;
    uint16_t reserved;
    uint32_t expected_size;
    uint16_t compiled_method_handle;

    if (size == 0U) {
        return;
    }
    if (size < RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE) {
        machine_panic("method update payload is too small");
    }
    if (blob[0] != RECORZ_MVP_METHOD_UPDATE_MAGIC_0 || blob[1] != RECORZ_MVP_METHOD_UPDATE_MAGIC_1 ||
        blob[2] != RECORZ_MVP_METHOD_UPDATE_MAGIC_2 || blob[3] != RECORZ_MVP_METHOD_UPDATE_MAGIC_3) {
        machine_panic("method update payload magic mismatch");
    }
    version = read_u16_le(blob + 4U);
    class_kind = read_u16_le(blob + 6U);
    selector = read_u16_le(blob + 8U);
    argument_count = read_u16_le(blob + 10U);
    instruction_count = read_u16_le(blob + 12U);
    reserved = read_u16_le(blob + 14U);
    if (version != RECORZ_MVP_METHOD_UPDATE_VERSION) {
        machine_panic("method update payload version mismatch");
    }
    if (reserved != 0U) {
        machine_panic("method update payload reserved field is nonzero");
    }
    if (selector < RECORZ_MVP_SELECTOR_SHOW ||
        selector > RECORZ_MVP_SELECTOR_NEW) {
        machine_panic("method update payload selector is out of range");
    }
    if (argument_count > MAX_SEND_ARGS) {
        machine_panic("method update payload argument count exceeds send capacity");
    }
    if (instruction_count == 0U || instruction_count > COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("method update payload instruction count is invalid");
    }
    expected_size = RECORZ_MVP_METHOD_UPDATE_HEADER_SIZE + ((uint32_t)instruction_count * 4U);
    if (size != expected_size) {
        machine_panic("method update payload size mismatch");
    }

    class_object = class_object_for_kind((uint8_t)class_kind);
    compiled_method_handle = allocate_updated_compiled_method(blob, instruction_count);
    validate_compiled_method(heap_object(compiled_method_handle), argument_count);
    install_compiled_method_update(class_object, (uint8_t)selector, argument_count, compiled_method_handle);
}

static void dispatch_heap_object_send(
    const struct recorz_mvp_heap_object *object,
    uint8_t selector,
    uint16_t argument_count,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    const struct recorz_mvp_heap_object *method_object;
    const struct recorz_mvp_heap_object *entry_object;
    const struct recorz_mvp_heap_object *implementation_object;
    struct recorz_mvp_value implementation_value;
    recorz_mvp_method_entry_handler handler;
    uint32_t entry;
    uint32_t primitive_binding_id;

    if (selector == RECORZ_MVP_SELECTOR_CLASS) {
        push(object_value(object->class_handle));
        return;
    }
    class_object = class_object_for_heap_object(object);
    method_object = lookup_builtin_method_descriptor(class_object, selector, argument_count);
    if (method_object == 0) {
        machine_panic("selector is not understood by receiver class");
    }
    entry_object = method_descriptor_entry_object(method_object);
    entry = method_entry_execution_id(entry_object);
    implementation_value = method_entry_implementation_value(entry_object);
    if (entry == 0U) {
        machine_panic("method entry execution id is out of range");
    }
    if (implementation_value.kind == RECORZ_MVP_VALUE_OBJECT) {
        implementation_object = heap_object_for_value(implementation_value);
        execute_compiled_method(object, receiver, argument_count, arguments, implementation_object);
        return;
    }
    if (implementation_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("method entry implementation kind is unknown");
    }
    primitive_binding_id = (uint32_t)implementation_value.integer;
    if (primitive_binding_id == 0U || primitive_binding_id >= RECORZ_MVP_PRIMITIVE_COUNT) {
        machine_panic("primitive method entry binding id is out of range");
    }
    handler = primitive_binding_handlers[primitive_binding_id];
    if (handler == 0) {
        machine_panic("primitive binding handler is not installed");
    }
    handler(object, receiver, arguments, text);
}

static void perform_send(
    struct recorz_mvp_value receiver,
    uint8_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *object = 0;

    if (selector == RECORZ_MVP_SELECTOR_SHOW || selector == RECORZ_MVP_SELECTOR_WRITE_STRING) {
        if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
            machine_panic("text send expects a string literal");
        }
        text = arguments[0].string;
    }

    if (receiver.kind == RECORZ_MVP_VALUE_OBJECT) {
        object = heap_object_for_value(receiver);
        dispatch_heap_object_send(object, selector, send_argument_count, receiver, arguments, text);
        return;
    }

    if (receiver.kind == RECORZ_MVP_VALUE_SMALL_INTEGER) {
        if (selector == RECORZ_MVP_SELECTOR_ADD) {
            if (arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
                machine_panic("+ expects a small integer argument");
            }
            push(small_integer_value(receiver.integer + arguments[0].integer));
            return;
        }
        if (selector == RECORZ_MVP_SELECTOR_SUBTRACT) {
            if (arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
                machine_panic("- expects a small integer argument");
            }
            push(small_integer_value(receiver.integer - arguments[0].integer));
            return;
        }
        if (selector == RECORZ_MVP_SELECTOR_MULTIPLY) {
            if (arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
                machine_panic("* expects a small integer argument");
            }
            push(small_integer_value(receiver.integer * arguments[0].integer));
            return;
        }
        if (selector == RECORZ_MVP_SELECTOR_PRINT_STRING) {
            render_small_integer(receiver.integer);
            push(string_value(print_buffer));
            return;
        }
        machine_panic("unsupported SmallInteger selector");
    }

    if (receiver.kind == RECORZ_MVP_VALUE_STRING) {
        if (selector == RECORZ_MVP_SELECTOR_PRINT_STRING) {
            push(receiver);
            return;
        }
        machine_panic("unsupported String selector");
    }

    machine_panic("unsupported receiver in MVP VM");
}

void recorz_mvp_vm_run(
    const struct recorz_mvp_program *program,
    const struct recorz_mvp_seed *seed,
    const uint8_t *method_update_blob,
    uint32_t method_update_size
) {
    const struct recorz_mvp_executable executable = {
        .instruction_source = program->instructions,
        .read_instruction = read_program_instruction,
        .instruction_count = program->instruction_count,
        .literals = program->literals,
        .literal_count = program->literal_count,
        .lexical_count = program->lexical_count,
    };

    stack_size = 0U;
    panic_phase = "bootstrap";
    panic_pc = 0U;
    panic_have_instruction = 0U;
    panic_have_send = 0U;
    machine_set_panic_hook(vm_panic_hook);
    initialize_roots(seed);
    if (method_update_blob != 0 && method_update_size != 0U) {
        panic_phase = "update";
        apply_method_update_payload(method_update_blob, method_update_size);
        machine_puts("recorz qemu-riscv64 mvp: applied method update\n");
    }
    execute_executable(&executable, 0, nil_value(), 0U, 0);
    panic_phase = "return";
    machine_set_panic_hook(0);
}
