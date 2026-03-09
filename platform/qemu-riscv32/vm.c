#include "vm.h"

#include <stdint.h>

#include "display.h"
#include "machine.h"

#define STACK_LIMIT 64U
#define LEXICAL_LIMIT 32U
#define MAX_SEND_ARGS 10U
#define PRINT_BUFFER_SIZE 32U
#define MEMORY_REPORT_BUFFER_SIZE 256U
#define HEAP_LIMIT RECORZ_MVP_HEAP_LIMIT
#define OBJECT_FIELD_LIMIT 4U
#define MONO_BITMAP_LIMIT RECORZ_MVP_MONO_BITMAP_LIMIT
#define MONO_BITMAP_MAX_WIDTH 32U
#define MONO_BITMAP_MAX_HEIGHT 64U
#define METHOD_SOURCE_LINE_LIMIT 192U
#define METHOD_SOURCE_NAME_LIMIT 64U
#define CLASS_COMMENT_LIMIT 128U
#define PACKAGE_COMMENT_LIMIT CLASS_COMMENT_LIMIT
#define METHOD_SOURCE_CHUNK_LIMIT 512U
#define WORKSPACE_SOURCE_INSTRUCTION_LIMIT 64U
#define WORKSPACE_SOURCE_LITERAL_LIMIT 16U
#define DYNAMIC_CLASS_LIMIT RECORZ_MVP_DYNAMIC_CLASS_LIMIT
#define PACKAGE_LIMIT DYNAMIC_CLASS_LIMIT
#define DYNAMIC_CLASS_IVAR_LIMIT OBJECT_FIELD_LIMIT
#define NAMED_OBJECT_LIMIT RECORZ_MVP_NAMED_OBJECT_LIMIT
#define LIVE_METHOD_SOURCE_LIMIT RECORZ_MVP_LIVE_METHOD_SOURCE_LIMIT
#define LIVE_METHOD_SOURCE_POOL_LIMIT RECORZ_MVP_LIVE_METHOD_SOURCE_POOL_LIMIT
#define LIVE_STRING_LITERAL_LIMIT 64U
#define RUNTIME_STRING_POOL_LIMIT RECORZ_MVP_RUNTIME_STRING_POOL_LIMIT
#define SNAPSHOT_STRING_LIMIT RECORZ_MVP_SNAPSHOT_STRING_LIMIT
#define SNAPSHOT_BUFFER_LIMIT RECORZ_MVP_SNAPSHOT_BUFFER_LIMIT

#define SNAPSHOT_MAGIC_0 'R'
#define SNAPSHOT_MAGIC_1 'C'
#define SNAPSHOT_MAGIC_2 'Z'
#define SNAPSHOT_MAGIC_3 'T'
#define SNAPSHOT_VERSION 5U
#define SNAPSHOT_HEADER_SIZE 42U
#define SNAPSHOT_VALUE_SIZE 8U
#define SNAPSHOT_OBJECT_SIZE (4U + (OBJECT_FIELD_LIMIT * SNAPSHOT_VALUE_SIZE))
#define SNAPSHOT_DYNAMIC_CLASS_RECORD_SIZE \
    (8U + (2U * METHOD_SOURCE_NAME_LIMIT) + CLASS_COMMENT_LIMIT + (DYNAMIC_CLASS_IVAR_LIMIT * METHOD_SOURCE_NAME_LIMIT))
#define SNAPSHOT_PACKAGE_RECORD_SIZE (METHOD_SOURCE_NAME_LIMIT + PACKAGE_COMMENT_LIMIT)
#define SNAPSHOT_NAMED_OBJECT_RECORD_SIZE (2U + METHOD_SOURCE_NAME_LIMIT)
#define SNAPSHOT_LIVE_METHOD_SOURCE_RECORD_SIZE (8U + METHOD_SOURCE_NAME_LIMIT)
#define SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE 8U

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
#define BLOCK_CLOSURE_FIELD_SOURCE RECORZ_MVP_BLOCK_CLOSURE_FIELD_SOURCE
#define BLOCK_CLOSURE_FIELD_HOME_RECEIVER RECORZ_MVP_BLOCK_CLOSURE_FIELD_HOME_RECEIVER
#define BLOCK_CLOSURE_FIELD_LEXICAL0 RECORZ_MVP_BLOCK_CLOSURE_FIELD_LEXICAL0
#define BLOCK_CLOSURE_FIELD_LEXICAL1 RECORZ_MVP_BLOCK_CLOSURE_FIELD_LEXICAL1
#define CONTEXT_FIELD_SENDER RECORZ_MVP_CONTEXT_FIELD_SENDER
#define CONTEXT_FIELD_RECEIVER RECORZ_MVP_CONTEXT_FIELD_RECEIVER
#define CONTEXT_FIELD_DETAIL RECORZ_MVP_CONTEXT_FIELD_DETAIL
#define CONTEXT_FIELD_ALIVE RECORZ_MVP_CONTEXT_FIELD_ALIVE
#define COMPILED_METHOD_MAX_INSTRUCTIONS RECORZ_MVP_COMPILED_METHOD_MAX_INSTRUCTIONS

#define COMPILED_METHOD_OP_PUSH_GLOBAL RECORZ_MVP_COMPILED_METHOD_OP_PUSH_GLOBAL
#define COMPILED_METHOD_OP_PUSH_ROOT RECORZ_MVP_COMPILED_METHOD_OP_PUSH_ROOT
#define COMPILED_METHOD_OP_PUSH_ARGUMENT RECORZ_MVP_COMPILED_METHOD_OP_PUSH_ARGUMENT
#define COMPILED_METHOD_OP_PUSH_NIL RECORZ_MVP_COMPILED_METHOD_OP_PUSH_NIL
#define COMPILED_METHOD_OP_PUSH_LEXICAL RECORZ_MVP_COMPILED_METHOD_OP_PUSH_LEXICAL
#define COMPILED_METHOD_OP_PUSH_FIELD RECORZ_MVP_COMPILED_METHOD_OP_PUSH_FIELD
#define COMPILED_METHOD_OP_PUSH_SELF RECORZ_MVP_COMPILED_METHOD_OP_PUSH_SELF
#define COMPILED_METHOD_OP_PUSH_SMALL_INTEGER RECORZ_MVP_COMPILED_METHOD_OP_PUSH_SMALL_INTEGER
#define COMPILED_METHOD_OP_PUSH_STRING_LITERAL RECORZ_MVP_COMPILED_METHOD_OP_PUSH_STRING_LITERAL
#define COMPILED_METHOD_OP_SEND RECORZ_MVP_COMPILED_METHOD_OP_SEND
#define COMPILED_METHOD_OP_POP RECORZ_MVP_COMPILED_METHOD_OP_POP
#define COMPILED_METHOD_OP_RETURN_TOP RECORZ_MVP_COMPILED_METHOD_OP_RETURN_TOP
#define COMPILED_METHOD_OP_RETURN_RECEIVER RECORZ_MVP_COMPILED_METHOD_OP_RETURN_RECEIVER
#define COMPILED_METHOD_OP_STORE_LEXICAL RECORZ_MVP_COMPILED_METHOD_OP_STORE_LEXICAL
#define COMPILED_METHOD_OP_STORE_FIELD RECORZ_MVP_COMPILED_METHOD_OP_STORE_FIELD
#define COMPILED_METHOD_OP_JUMP RECORZ_MVP_COMPILED_METHOD_OP_JUMP
#define COMPILED_METHOD_OP_JUMP_IF_TRUE RECORZ_MVP_COMPILED_METHOD_OP_JUMP_IF_TRUE
#define COMPILED_METHOD_OP_JUMP_IF_FALSE RECORZ_MVP_COMPILED_METHOD_OP_JUMP_IF_FALSE
#define COMPILED_METHOD_OP_PUSH_THIS_CONTEXT RECORZ_MVP_COMPILED_METHOD_OP_PUSH_THIS_CONTEXT

#define BITMAP_STORAGE_FRAMEBUFFER RECORZ_MVP_BITMAP_STORAGE_FRAMEBUFFER
#define BITMAP_STORAGE_GLYPH_MONO RECORZ_MVP_BITMAP_STORAGE_GLYPH_MONO
#define BITMAP_STORAGE_HEAP_MONO 3U
#define MAX_OBJECT_KIND RECORZ_MVP_OBJECT_CONTEXT
#define MAX_SELECTOR_ID RECORZ_MVP_SELECTOR_ACCEPT_CURRENT
#define MAX_GLOBAL_ID RECORZ_MVP_GLOBAL_FALSE
#define SOURCE_EVAL_BINDING_LIMIT (MAX_SEND_ARGS + LEXICAL_LIMIT)
#define SOURCE_EVAL_ENV_LIMIT 32U
#define SOURCE_EVAL_HOME_CONTEXT_LIMIT 32U
#define SOURCE_EVAL_BLOCK_STATE_LIMIT 64U

#define WORKSPACE_VIEW_NONE 0U
#define WORKSPACE_VIEW_CLASSES 1U
#define WORKSPACE_VIEW_CLASS 2U
#define WORKSPACE_VIEW_OBJECT 3U
#define WORKSPACE_VIEW_METHODS 4U
#define WORKSPACE_VIEW_METHOD 5U
#define WORKSPACE_VIEW_CLASS_METHODS 6U
#define WORKSPACE_VIEW_CLASS_METHOD 7U
#define WORKSPACE_VIEW_CLASS_SOURCE 8U
#define WORKSPACE_VIEW_PROTOCOLS 9U
#define WORKSPACE_VIEW_PROTOCOL 10U
#define WORKSPACE_VIEW_CLASS_PROTOCOLS 11U
#define WORKSPACE_VIEW_CLASS_PROTOCOL 12U
#define WORKSPACE_VIEW_PACKAGE_SOURCE 13U
#define WORKSPACE_VIEW_PACKAGES 14U
#define WORKSPACE_VIEW_PACKAGE 15U

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

struct recorz_mvp_live_class_definition {
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    uint8_t has_superclass;
    char superclass_name[METHOD_SOURCE_NAME_LIMIT];
    char package_name[METHOD_SOURCE_NAME_LIMIT];
    char class_comment[CLASS_COMMENT_LIMIT];
    uint8_t instance_variable_count;
    char instance_variable_names[DYNAMIC_CLASS_IVAR_LIMIT][METHOD_SOURCE_NAME_LIMIT];
};

struct recorz_mvp_dynamic_class_definition {
    uint16_t class_handle;
    uint16_t superclass_handle;
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char package_name[METHOD_SOURCE_NAME_LIMIT];
    char class_comment[CLASS_COMMENT_LIMIT];
    uint8_t instance_variable_count;
    char instance_variable_names[DYNAMIC_CLASS_IVAR_LIMIT][METHOD_SOURCE_NAME_LIMIT];
};

struct recorz_mvp_live_package_definition {
    char package_name[METHOD_SOURCE_NAME_LIMIT];
    char package_comment[PACKAGE_COMMENT_LIMIT];
};

struct recorz_mvp_workspace_source_program {
    struct recorz_mvp_instruction instructions[WORKSPACE_SOURCE_INSTRUCTION_LIMIT];
    struct recorz_mvp_literal literals[WORKSPACE_SOURCE_LITERAL_LIMIT];
    char literal_texts[WORKSPACE_SOURCE_LITERAL_LIMIT][METHOD_SOURCE_CHUNK_LIMIT];
    char temporary_names[LEXICAL_LIMIT][METHOD_SOURCE_NAME_LIMIT];
    uint32_t instruction_count;
    uint32_t literal_count;
    uint16_t lexical_count;
};

struct recorz_mvp_named_object_binding {
    char name[METHOD_SOURCE_NAME_LIMIT];
    uint16_t object_handle;
};

struct recorz_mvp_live_method_source {
    uint16_t class_handle;
    uint8_t selector_id;
    uint8_t argument_count;
    char protocol_name[METHOD_SOURCE_NAME_LIMIT];
    uint16_t source_offset;
    uint16_t source_length;
};

struct recorz_mvp_live_string_literal {
    uint16_t class_handle;
    uint8_t selector_id;
    uint8_t argument_count;
    const char *text;
};

enum recorz_mvp_method_return_mode {
    RECORZ_MVP_METHOD_RETURN_RESULT = 1,
    RECORZ_MVP_METHOD_RETURN_RECEIVER = 2,
};

enum recorz_mvp_source_eval_result_kind {
    RECORZ_MVP_SOURCE_EVAL_VALUE = 1,
    RECORZ_MVP_SOURCE_EVAL_RETURN = 2,
};

struct recorz_mvp_source_eval_result {
    uint8_t kind;
    struct recorz_mvp_value value;
};

struct recorz_mvp_source_binding {
    char name[METHOD_SOURCE_NAME_LIMIT];
    struct recorz_mvp_value value;
};

struct recorz_mvp_source_lexical_environment {
    uint8_t in_use;
    uint8_t binding_count;
    int16_t parent_index;
    struct recorz_mvp_source_binding bindings[SOURCE_EVAL_BINDING_LIMIT];
};

struct recorz_mvp_source_home_context {
    uint8_t in_use;
    uint8_t alive;
    const struct recorz_mvp_heap_object *defining_class;
    struct recorz_mvp_value receiver;
    int16_t lexical_environment_index;
    uint16_t context_handle;
};

struct recorz_mvp_source_method_context {
    const struct recorz_mvp_heap_object *defining_class;
    struct recorz_mvp_value receiver;
    int16_t lexical_environment_index;
    int16_t home_context_index;
    uint16_t current_context_handle;
    uint8_t is_block;
};

struct recorz_mvp_runtime_block_state {
    uint8_t in_use;
    uint16_t block_handle;
    int16_t lexical_environment_index;
    int16_t home_context_index;
    const struct recorz_mvp_heap_object *defining_class;
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
static uint16_t selector_handles_by_id[MAX_SELECTOR_ID + 1U];
static uint16_t global_handles[MAX_GLOBAL_ID + 1U];
static struct recorz_mvp_dynamic_class_definition dynamic_classes[DYNAMIC_CLASS_LIMIT];
static struct recorz_mvp_live_package_definition live_packages[PACKAGE_LIMIT];
static struct recorz_mvp_named_object_binding named_objects[NAMED_OBJECT_LIMIT];
static struct recorz_mvp_live_method_source live_method_sources[LIVE_METHOD_SOURCE_LIMIT];
static struct recorz_mvp_live_string_literal live_string_literals[LIVE_STRING_LITERAL_LIMIT];
static char live_method_source_pool[LIVE_METHOD_SOURCE_POOL_LIMIT];
static uint16_t live_method_source_pool_used = 0U;
static uint16_t default_form_handle = 0U;
static uint16_t framebuffer_bitmap_handle = 0U;
static uint16_t next_dynamic_method_entry_execution_id = RECORZ_MVP_METHOD_ENTRY_COUNT;
static uint16_t dynamic_class_count = 0U;
static uint16_t package_count = 0U;
static uint16_t glyph_bitmap_handles[128];
static uint16_t glyph_fallback_handle = 0U;
static uint16_t transcript_layout_handle = 0U;
static uint16_t transcript_style_handle = 0U;
static uint16_t transcript_metrics_handle = 0U;
static uint16_t transcript_behavior_handle = 0U;
static struct recorz_mvp_source_lexical_environment source_eval_environments[SOURCE_EVAL_ENV_LIMIT];
static struct recorz_mvp_source_home_context source_eval_home_contexts[SOURCE_EVAL_HOME_CONTEXT_LIMIT];
static struct recorz_mvp_runtime_block_state source_eval_block_states[SOURCE_EVAL_BLOCK_STATE_LIMIT];
static uint16_t startup_hook_receiver_handle = 0U;
static uint16_t startup_hook_selector_id = 0U;
static uint32_t mono_bitmap_pool[MONO_BITMAP_LIMIT][MONO_BITMAP_MAX_HEIGHT];
static uint16_t mono_bitmap_count = 0U;
static uint16_t named_object_count = 0U;
static uint16_t live_method_source_count = 0U;
static uint16_t compiling_method_class_handle = 0U;
static uint8_t compiling_method_selector_id = 0U;
static uint8_t compiling_method_argument_count = 0U;
static uint32_t cursor_x = 0U;
static uint32_t cursor_y = 0U;
static char print_buffer[PRINT_BUFFER_SIZE];
static char workspace_target_buffer[METHOD_SOURCE_CHUNK_LIMIT];
static char kernel_source_io_buffer[8193];
static char runtime_string_pool[RUNTIME_STRING_POOL_LIMIT];
static uint32_t runtime_string_pool_offset = 0U;
static char snapshot_string_pool[SNAPSHOT_STRING_LIMIT];
static uint8_t snapshot_buffer[SNAPSHOT_BUFFER_LIMIT];

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
static void heap_set_field(uint16_t handle, uint8_t index, struct recorz_mvp_value value);
static void heap_set_class(uint16_t handle, uint16_t class_handle);
static uint32_t compiled_method_instruction_word(const struct recorz_mvp_heap_object *compiled_method, uint8_t index);
static uint16_t compiled_method_lexical_count(const struct recorz_mvp_heap_object *compiled_method);
static uint32_t text_length(const char *text);
static uint32_t text_left_margin(void);
static uint32_t char_width(void);
static uint32_t char_height(void);
static uint32_t text_line_spacing(void);
static uint32_t bitmap_width(const struct recorz_mvp_heap_object *bitmap);
static uint32_t bitmap_height(const struct recorz_mvp_heap_object *bitmap);
static const struct recorz_mvp_heap_object *bitmap_for_form(const struct recorz_mvp_heap_object *form);
static void workspace_evaluate_source(const char *source);
static uint8_t compiled_method_instruction_opcode(uint32_t instruction);
static uint8_t compiled_method_instruction_operand_a(uint32_t instruction);
static uint16_t compiled_method_instruction_operand_b(uint32_t instruction);
static uint8_t workspace_parse_method_target_name(
    const char *target_name,
    char class_name[],
    uint32_t class_name_size,
    char selector_name[],
    uint32_t selector_name_size
);
static uint8_t workspace_parse_protocol_target_name(
    const char *target_name,
    char class_name[],
    uint32_t class_name_size,
    char protocol_name[],
    uint32_t protocol_name_size
);
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
static const struct recorz_mvp_heap_object *lookup_class_by_name(const char *class_name);
static const struct recorz_mvp_heap_object *ensure_class_defined(
    const struct recorz_mvp_live_class_definition *definition
);
static const struct recorz_mvp_heap_object *class_object_for_kind(uint8_t kind);
static const struct recorz_mvp_heap_object *class_side_lookup_target(
    const struct recorz_mvp_heap_object *class_object
);
static void initialize_runtime_caches(void);
static void reset_runtime_state(void);
static void load_snapshot_state(const uint8_t *blob, uint32_t size);
static void emit_live_snapshot(void);
static void file_in_class_chunks_source(const char *source);
static void file_in_chunk_stream_source(const char *source);
static const char *file_out_class_source_by_name(const char *class_name);
static const char *file_out_package_source_by_name(const char *package_name);
static int compare_source_names(const char *left, const char *right);
static const char *runtime_string_allocate_copy(const char *text);
static struct recorz_mvp_value boolean_value(uint8_t condition);
static struct recorz_mvp_value object_value(uint16_t handle);
static struct recorz_mvp_value string_value(const char *text);
static uint16_t heap_allocate_seeded_class(uint8_t kind);
static void forget_live_string_literals(uint16_t class_handle, uint8_t selector_id, uint8_t argument_count);
static void append_text_checked(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *text
);
static void apply_external_file_in_blob(const uint8_t *blob, uint32_t size);
static const struct recorz_mvp_heap_object *default_form_object(void);
static void form_clear(const struct recorz_mvp_heap_object *form);
static void form_newline(const struct recorz_mvp_heap_object *form);
static void form_write_string(const struct recorz_mvp_heap_object *form, const char *text);

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
        case RECORZ_MVP_OP_STORE_FIELD:
            return "storeField";
        case RECORZ_MVP_OP_PUSH_SELF:
            return "pushSelf";
        case RECORZ_MVP_OP_PUSH_SMALL_INTEGER:
            return "pushSmallInteger";
        case RECORZ_MVP_OP_PUSH_STRING_LITERAL:
            return "pushStringLiteral";
        case RECORZ_MVP_OP_JUMP:
            return "jump";
        case RECORZ_MVP_OP_JUMP_IF_TRUE:
            return "jumpIfTrue";
        case RECORZ_MVP_OP_JUMP_IF_FALSE:
            return "jumpIfFalse";
        case RECORZ_MVP_OP_PUSH_BLOCK_LITERAL:
            return "pushBlockLiteral";
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
        case RECORZ_MVP_SELECTOR_CLASS_NAMED:
            return "classNamed:";
        case RECORZ_MVP_SELECTOR_VALUE:
            return "value";
        case RECORZ_MVP_SELECTOR_SET_VALUE:
            return "setValue:";
        case RECORZ_MVP_SELECTOR_DETAIL:
            return "detail";
        case RECORZ_MVP_SELECTOR_SET_DETAIL:
            return "setDetail:";
        case RECORZ_MVP_SELECTOR_REMEMBER_OBJECT_NAMED:
            return "rememberObject:named:";
        case RECORZ_MVP_SELECTOR_OBJECT_NAMED:
            return "objectNamed:";
        case RECORZ_MVP_SELECTOR_SAVE_SNAPSHOT:
            return "saveSnapshot";
        case RECORZ_MVP_SELECTOR_CONFIGURE_STARTUP_SELECTOR_NAMED:
            return "configureStartup:selectorNamed:";
        case RECORZ_MVP_SELECTOR_CLEAR_STARTUP:
            return "clearStartup";
        case RECORZ_MVP_SELECTOR_FILE_IN:
            return "fileIn:";
        case RECORZ_MVP_SELECTOR_BROWSE_CLASS_NAMED:
            return "browseClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_CLASSES:
            return "browseClasses";
        case RECORZ_MVP_SELECTOR_BROWSE_OBJECT_NAMED:
            return "browseObjectNamed:";
        case RECORZ_MVP_SELECTOR_REOPEN:
            return "reopen";
        case RECORZ_MVP_SELECTOR_EVALUATE:
            return "evaluate:";
        case RECORZ_MVP_SELECTOR_RERUN:
            return "rerun";
        case RECORZ_MVP_SELECTOR_CONTENTS:
            return "contents";
        case RECORZ_MVP_SELECTOR_SET_CONTENTS:
            return "setContents:";
        case RECORZ_MVP_SELECTOR_EVALUATE_CURRENT:
            return "evaluateCurrent";
        case RECORZ_MVP_SELECTOR_FILE_IN_CURRENT:
            return "fileInCurrent";
        case RECORZ_MVP_SELECTOR_BROWSE_METHODS_FOR_CLASS_NAMED:
            return "browseMethodsForClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_METHOD_OF_CLASS_NAMED:
            return "browseMethod:ofClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHODS_FOR_CLASS_NAMED:
            return "browseClassMethodsForClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_CLASS_METHOD_OF_CLASS_NAMED:
            return "browseClassMethod:ofClassNamed:";
        case RECORZ_MVP_SELECTOR_FILE_OUT_CLASS_NAMED:
            return "fileOutClassNamed:";
        case RECORZ_MVP_SELECTOR_MEMORY_REPORT:
            return "memoryReport";
        case RECORZ_MVP_SELECTOR_BROWSE_PROTOCOLS_FOR_CLASS_NAMED:
            return "browseProtocolsForClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_PROTOCOL_OF_CLASS_NAMED:
            return "browseProtocol:ofClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOLS_FOR_CLASS_NAMED:
            return "browseClassProtocolsForClassNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_CLASS_PROTOCOL_OF_CLASS_NAMED:
            return "browseClassProtocol:ofClassNamed:";
        case RECORZ_MVP_SELECTOR_FILE_OUT_PACKAGE_NAMED:
            return "fileOutPackageNamed:";
        case RECORZ_MVP_SELECTOR_BROWSE_PACKAGES:
            return "browsePackages";
        case RECORZ_MVP_SELECTOR_BROWSE_PACKAGE_NAMED:
            return "browsePackageNamed:";
        case RECORZ_MVP_SELECTOR_SUM:
            return "sum";
        case RECORZ_MVP_SELECTOR_SET_LEFT_RIGHT:
            return "setLeft:right:";
        case RECORZ_MVP_SELECTOR_TRUTH:
            return "truth";
        case RECORZ_MVP_SELECTOR_FALSITY:
            return "falsity";
        case RECORZ_MVP_SELECTOR_IF_TRUE:
            return "ifTrue:";
        case RECORZ_MVP_SELECTOR_IF_FALSE:
            return "ifFalse:";
        case RECORZ_MVP_SELECTOR_IF_TRUE_IF_FALSE:
            return "ifTrue:ifFalse:";
        case RECORZ_MVP_SELECTOR_EQUAL:
            return "=";
        case RECORZ_MVP_SELECTOR_LESS_THAN:
            return "<";
        case RECORZ_MVP_SELECTOR_GREATER_THAN:
            return ">";
        case RECORZ_MVP_SELECTOR_SENDER:
            return "sender";
        case RECORZ_MVP_SELECTOR_RECEIVER:
            return "receiver";
        case RECORZ_MVP_SELECTOR_ALIVE:
            return "alive";
        case RECORZ_MVP_SELECTOR_VALUE_ARG:
            return "value:";
        case RECORZ_MVP_SELECTOR_ACCEPT_CURRENT:
            return "acceptCurrent";
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
        case RECORZ_MVP_OBJECT_WORKSPACE:
            return "Workspace";
        case RECORZ_MVP_OBJECT_TRUE:
            return "True";
        case RECORZ_MVP_OBJECT_FALSE:
            return "False";
        case RECORZ_MVP_OBJECT_BLOCK_CLOSURE:
            return "BlockClosure";
        case RECORZ_MVP_OBJECT_CONTEXT:
            return "Context";
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

static void write_u16_le(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)(value & 0xFFU);
    bytes[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static void write_u32_le(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)(value & 0xFFU);
    bytes[1] = (uint8_t)((value >> 8U) & 0xFFU);
    bytes[2] = (uint8_t)((value >> 16U) & 0xFFU);
    bytes[3] = (uint8_t)((value >> 24U) & 0xFFU);
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

static uint8_t source_char_is_binary_selector(char ch) {
    return (uint8_t)(ch == '+' || ch == '-' || ch == '*' || ch == '=' || ch == '<' || ch == '>');
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
static const struct recorz_mvp_dynamic_class_definition *dynamic_class_definition_for_name(const char *class_name);
static struct recorz_mvp_dynamic_class_definition *mutable_dynamic_class_definition_for_handle(uint16_t class_handle);
static const struct recorz_mvp_live_method_source *live_method_source_for_selector_and_arity(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count
);
static const struct recorz_mvp_live_method_source *live_method_source_for_class_chain(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector_id,
    uint8_t argument_count,
    const struct recorz_mvp_heap_object **owner_class_out
);
static uint8_t source_text_contains_block_literal(const char *source);
static const char *source_parse_method_header(
    const char *source,
    char selector_name[METHOD_SOURCE_NAME_LIMIT],
    char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t *argument_count,
    const char **body_cursor_out
);
static uint16_t allocate_placeholder_live_source_compiled_method(void);
static struct recorz_mvp_source_eval_result source_eval_value_result(struct recorz_mvp_value value);
static struct recorz_mvp_source_eval_result source_eval_return_result(struct recorz_mvp_value value);
static int16_t source_allocate_lexical_environment(int16_t parent_index);
static int16_t source_allocate_home_context(
    const struct recorz_mvp_heap_object *defining_class,
    struct recorz_mvp_value receiver,
    int16_t lexical_environment_index,
    uint16_t sender_context_handle,
    const char *detail_text
);
static uint16_t allocate_source_context_object(
    uint16_t sender_context_handle,
    struct recorz_mvp_value receiver,
    const char *detail_text
);
static void source_append_binding(
    int16_t lexical_environment_index,
    const char *name,
    struct recorz_mvp_value value
);
static struct recorz_mvp_value *source_lookup_binding_cell(
    int16_t lexical_environment_index,
    const char *name
);
static struct recorz_mvp_source_eval_result source_execute_block_closure(
    const struct recorz_mvp_heap_object *object,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle
);
static struct recorz_mvp_source_eval_result source_evaluate_expression(
    struct recorz_mvp_source_method_context *context,
    const char *cursor,
    const char **cursor_out
);
static struct recorz_mvp_source_eval_result source_evaluate_statement_sequence(
    struct recorz_mvp_source_method_context *context,
    const char *source,
    uint8_t is_block
);
static void execute_live_source_method_with_sender(
    const struct recorz_mvp_heap_object *class_object,
    struct recorz_mvp_value receiver,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    const char *source,
    uint16_t sender_context_handle
);
static void perform_send_with_sender(
    struct recorz_mvp_value receiver,
    uint8_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle,
    const char *text
);
static void perform_send(
    struct recorz_mvp_value receiver,
    uint8_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    const char *text
);

static void source_copy_identifier(char destination[], uint32_t destination_size, const char *source) {
    uint32_t index = 0U;

    while (source[index] != '\0') {
        if (index + 1U >= destination_size) {
            machine_panic("KernelInstaller identifier copy exceeds buffer capacity");
        }
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static void initialize_live_package_definition(struct recorz_mvp_live_package_definition *definition) {
    definition->package_name[0] = '\0';
    definition->package_comment[0] = '\0';
}

static const char *source_skip_decimal_digits(const char *cursor) {
    if (*cursor < '0' || *cursor > '9') {
        return 0;
    }
    while (*cursor >= '0' && *cursor <= '9') {
        ++cursor;
    }
    return cursor;
}

static const char *source_parse_hash_identifier(
    const char *cursor,
    char buffer[],
    uint32_t buffer_size
) {
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '#') {
        return 0;
    }
    ++cursor;
    return source_parse_identifier(cursor, buffer, buffer_size);
}

static const char *source_copy_single_quoted_text(
    const char *cursor,
    char buffer[],
    uint32_t buffer_size
) {
    uint32_t length = 0U;

    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '\'') {
        return 0;
    }
    ++cursor;
    while (*cursor != '\0' && *cursor != '\'') {
        if (length + 1U >= buffer_size) {
            machine_panic("KernelInstaller quoted text exceeds buffer capacity");
        }
        buffer[length++] = *cursor++;
    }
    if (*cursor != '\'') {
        machine_panic("KernelInstaller quoted text is unterminated");
    }
    buffer[length] = '\0';
    return cursor + 1;
}

static uint8_t source_parse_identifier_list(
    const char *text,
    char names[][METHOD_SOURCE_NAME_LIMIT],
    uint8_t maximum_count
) {
    uint8_t count = 0U;
    const char *cursor = text;

    cursor = source_skip_horizontal_space(cursor);
    while (*cursor != '\0') {
        if (count >= maximum_count) {
            machine_panic("KernelInstaller instance variable count exceeds MVP object field limit");
        }
        cursor = source_parse_identifier(cursor, names[count], METHOD_SOURCE_NAME_LIMIT);
        if (cursor == 0) {
            machine_panic("KernelInstaller instance variable name is invalid");
        }
        ++count;
        cursor = source_skip_horizontal_space(cursor);
    }
    return count;
}

static void initialize_live_class_definition(struct recorz_mvp_live_class_definition *definition) {
    uint8_t index;

    definition->class_name[0] = '\0';
    definition->has_superclass = 0U;
    definition->superclass_name[0] = '\0';
    definition->package_name[0] = '\0';
    definition->class_comment[0] = '\0';
    definition->instance_variable_count = 0U;
    for (index = 0U; index < DYNAMIC_CLASS_IVAR_LIMIT; ++index) {
        definition->instance_variable_names[index][0] = '\0';
    }
}

static void source_parse_class_definition_from_chunk(
    const char *chunk,
    struct recorz_mvp_live_class_definition *definition
) {
    const char *cursor = chunk;
    char keyword[METHOD_SOURCE_NAME_LIMIT];

    initialize_live_class_definition(definition);
    if (!source_starts_with(cursor, "RecorzKernelClass:")) {
        machine_panic("KernelInstaller class chunk is missing a RecorzKernelClass header");
    }
    cursor += 18U;
    cursor = source_parse_hash_identifier(cursor, definition->class_name, sizeof(definition->class_name));
    if (cursor == 0) {
        machine_panic("KernelInstaller class chunk header has an invalid class name");
    }
    cursor = source_skip_horizontal_space(cursor);
    while (*cursor != '\0') {
        char instance_variables[METHOD_SOURCE_CHUNK_LIMIT];

        cursor = source_parse_identifier(cursor, keyword, sizeof(keyword));
        if (cursor == 0 || *cursor != ':') {
            machine_panic("KernelInstaller class chunk header has an invalid keyword");
        }
        ++cursor;
        if (source_names_equal(keyword, "superclass")) {
            cursor = source_parse_hash_identifier(cursor, definition->superclass_name, sizeof(definition->superclass_name));
            if (cursor == 0) {
                machine_panic("KernelInstaller class chunk superclass is invalid");
            }
            definition->has_superclass = 1U;
        } else if (source_names_equal(keyword, "package")) {
            cursor = source_copy_single_quoted_text(cursor, definition->package_name, sizeof(definition->package_name));
            if (cursor == 0) {
                machine_panic("KernelInstaller class chunk package is invalid");
            }
        } else if (source_names_equal(keyword, "comment")) {
            cursor = source_copy_single_quoted_text(cursor, definition->class_comment, sizeof(definition->class_comment));
            if (cursor == 0) {
                machine_panic("KernelInstaller class chunk comment is invalid");
            }
        } else if (source_names_equal(keyword, "instanceVariableNames")) {
            cursor = source_copy_single_quoted_text(cursor, instance_variables, sizeof(instance_variables));
            if (cursor == 0) {
                machine_panic("KernelInstaller class chunk instanceVariableNames is invalid");
            }
            definition->instance_variable_count = source_parse_identifier_list(
                instance_variables,
                definition->instance_variable_names,
                DYNAMIC_CLASS_IVAR_LIMIT
            );
        } else if (
            source_names_equal(keyword, "descriptorOrder") ||
            source_names_equal(keyword, "objectKindOrder") ||
            source_names_equal(keyword, "sourceBootOrder")
        ) {
            cursor = source_skip_horizontal_space(cursor);
            cursor = source_skip_decimal_digits(cursor);
            if (cursor == 0) {
                machine_panic("KernelInstaller class chunk numeric header value is invalid");
            }
        } else {
            machine_panic("KernelInstaller class chunk header uses an unsupported keyword");
        }
        cursor = source_skip_horizontal_space(cursor);
    }
}

static void source_parse_class_side_name_from_chunk(
    const char *chunk,
    char class_name[],
    uint32_t class_name_size
) {
    const char *cursor = chunk;

    if (!source_starts_with(cursor, "RecorzKernelClassSide:")) {
        machine_panic("KernelInstaller class-side chunk is missing a RecorzKernelClassSide header");
    }
    cursor += 22U;
    cursor = source_parse_hash_identifier(cursor, class_name, class_name_size);
    if (cursor == 0) {
        machine_panic("KernelInstaller class-side chunk header has an invalid class name");
    }
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '\0') {
        machine_panic("KernelInstaller class-side chunk has unexpected trailing text");
    }
}

static void source_parse_protocol_name_from_chunk(
    const char *chunk,
    char protocol_name[],
    uint32_t protocol_name_size
) {
    const char *cursor = chunk;

    if (!source_starts_with(cursor, "RecorzKernelProtocol:")) {
        machine_panic("KernelInstaller protocol chunk is missing a RecorzKernelProtocol header");
    }
    cursor += 21U;
    cursor = source_copy_single_quoted_text(cursor, protocol_name, protocol_name_size);
    if (cursor == 0) {
        machine_panic("KernelInstaller protocol chunk name is invalid");
    }
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '\0') {
        machine_panic("KernelInstaller protocol chunk has unexpected trailing text");
    }
}

static void source_parse_package_definition_from_chunk(
    const char *chunk,
    struct recorz_mvp_live_package_definition *definition,
    uint8_t *has_comment_out
) {
    const char *cursor = chunk;
    char keyword[METHOD_SOURCE_NAME_LIMIT];

    initialize_live_package_definition(definition);
    *has_comment_out = 0U;
    if (!source_starts_with(cursor, "RecorzKernelPackage:")) {
        machine_panic("KernelInstaller package chunk is missing a RecorzKernelPackage header");
    }
    cursor += (sizeof("RecorzKernelPackage:") - 1U);
    cursor = source_copy_single_quoted_text(cursor, definition->package_name, sizeof(definition->package_name));
    if (cursor == 0) {
        machine_panic("KernelInstaller package chunk name is invalid");
    }
    cursor = source_skip_horizontal_space(cursor);
    while (*cursor != '\0') {
        cursor = source_parse_identifier(cursor, keyword, sizeof(keyword));
        if (cursor == 0 || *cursor != ':') {
            machine_panic("KernelInstaller package chunk has an invalid keyword");
        }
        ++cursor;
        if (source_names_equal(keyword, "comment")) {
            cursor = source_copy_single_quoted_text(
                cursor,
                definition->package_comment,
                sizeof(definition->package_comment)
            );
            if (cursor == 0) {
                machine_panic("KernelInstaller package chunk comment is invalid");
            }
            *has_comment_out = 1U;
        } else {
            machine_panic("KernelInstaller package chunk header uses an unsupported keyword");
        }
        cursor = source_skip_horizontal_space(cursor);
    }
}

static const char *source_parse_do_it_chunk_body(const char *chunk) {
    const char *cursor = chunk;

    if (!source_starts_with(cursor, "RecorzKernelDoIt:")) {
        machine_panic("KernelInstaller do-it chunk is missing a RecorzKernelDoIt header");
    }
    cursor += 17U;
    if (*cursor == '\n') {
        ++cursor;
    } else {
        cursor = source_skip_horizontal_space(cursor);
    }
    cursor = source_skip_blank_lines(cursor);
    if (*cursor == '\0') {
        machine_panic("KernelInstaller do-it chunk has no executable source");
    }
    return cursor;
}

static const char *workspace_normalize_do_it_source(const char *source) {
    uint32_t offset = 0U;

    if (source == 0 || source[0] == '\0') {
        machine_panic("Workspace do-it source is empty");
    }
    if (source_starts_with(source, "RecorzKernelDoIt:")) {
        return source;
    }
    kernel_source_io_buffer[0] = '\0';
    append_text_checked(kernel_source_io_buffer, sizeof(kernel_source_io_buffer), &offset, "RecorzKernelDoIt:\n");
    append_text_checked(kernel_source_io_buffer, sizeof(kernel_source_io_buffer), &offset, source);
    return runtime_string_allocate_copy(kernel_source_io_buffer);
}

static const char *workspace_source_for_evaluation(const char *source) {
    if (source_starts_with(source, "RecorzKernelDoIt:")) {
        return source_parse_do_it_chunk_body(source);
    }
    return source;
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
    if (source_names_equal(name, "Workspace")) {
        return RECORZ_MVP_GLOBAL_WORKSPACE;
    }
    if (source_names_equal(name, "true")) {
        return RECORZ_MVP_GLOBAL_TRUE;
    }
    if (source_names_equal(name, "false")) {
        return RECORZ_MVP_GLOBAL_FALSE;
    }
    return 0U;
}

static uint8_t source_selector_id_for_name(const char *name) {
    uint8_t selector;

    for (selector = RECORZ_MVP_SELECTOR_SHOW;
         selector <= MAX_SELECTOR_ID;
         ++selector) {
        if (source_names_equal(name, selector_name(selector))) {
            return selector;
        }
    }
    return 0U;
}

static const char *source_skip_statement_space(const char *cursor) {
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
        ++cursor;
    }
    return cursor;
}

static uint32_t source_copy_next_statement(const char **cursor_ref, char buffer[], uint32_t buffer_size) {
    const char *cursor = source_skip_statement_space(*cursor_ref);
    const char *trimmed_start;
    const char *trimmed_end;
    uint32_t length = 0U;
    uint8_t in_string = 0U;
    uint32_t block_depth = 0U;

    while (*cursor == '.') {
        ++cursor;
        cursor = source_skip_statement_space(cursor);
    }
    if (*cursor == '\0') {
        *cursor_ref = cursor;
        buffer[0] = '\0';
        return 0U;
    }
    trimmed_start = cursor;
    while (*cursor != '\0') {
        if (*cursor == '\'') {
            in_string = (uint8_t)!in_string;
            ++cursor;
            continue;
        }
        if (!in_string && *cursor == '[') {
            ++block_depth;
            ++cursor;
            continue;
        }
        if (!in_string && *cursor == ']') {
            if (block_depth == 0U) {
                machine_panic("Workspace source statement has an unexpected ']'");
            }
            --block_depth;
            ++cursor;
            continue;
        }
        if (*cursor == '.' && !in_string && block_depth == 0U) {
            break;
        }
        ++cursor;
    }
    if (in_string) {
        machine_panic("Workspace source statement contains an unterminated string");
    }
    if (block_depth != 0U) {
        machine_panic("Workspace source statement contains an unterminated block");
    }
    trimmed_end = cursor;
    while (
        trimmed_end > trimmed_start &&
        (trimmed_end[-1] == ' ' || trimmed_end[-1] == '\t' ||
         trimmed_end[-1] == '\r' || trimmed_end[-1] == '\n')
    ) {
        --trimmed_end;
    }
    while (trimmed_start + length < trimmed_end) {
        if (length + 1U >= buffer_size) {
            machine_panic("Workspace source statement exceeds buffer capacity");
        }
        buffer[length] = trimmed_start[length];
        ++length;
    }
    buffer[length] = '\0';
    if (*cursor == '.') {
        ++cursor;
    }
    *cursor_ref = cursor;
    return length;
}

static const char *source_copy_bracket_body(
    const char *cursor,
    char buffer[],
    uint32_t buffer_size
) {
    uint32_t length = 0U;
    uint32_t block_depth = 1U;
    uint8_t in_string = 0U;

    cursor = source_skip_statement_space(cursor);
    if (*cursor != '[') {
        return 0;
    }
    ++cursor;
    while (*cursor != '\0') {
        char ch = *cursor++;

        if (ch == '\'') {
            in_string = (uint8_t)!in_string;
        } else if (!in_string && ch == '[') {
            ++block_depth;
        } else if (!in_string && ch == ']') {
            --block_depth;
            if (block_depth == 0U) {
                buffer[length] = '\0';
                return cursor;
            }
        }
        if (length + 1U >= buffer_size) {
            machine_panic("KernelInstaller block source exceeds buffer capacity");
        }
        buffer[length++] = ch;
    }
    machine_panic("KernelInstaller block source is missing ']'");
    return 0;
}

static const char *source_parse_small_integer(
    const char *cursor,
    int32_t *value_out
) {
    uint8_t negative = 0U;
    int32_t value = 0;

    if (*cursor == '-') {
        negative = 1U;
        ++cursor;
    }
    if (*cursor < '0' || *cursor > '9') {
        return 0;
    }
    while (*cursor >= '0' && *cursor <= '9') {
        value = (value * 10) + (int32_t)(*cursor - '0');
        ++cursor;
    }
    *value_out = negative ? -value : value;
    return cursor;
}

static uint8_t source_text_contains_block_literal(const char *source) {
    uint8_t in_string = 0U;

    if (source == 0) {
        return 0U;
    }
    while (*source != '\0') {
        if (*source == '\'') {
            in_string = (uint8_t)!in_string;
        } else if (!in_string && *source == '[') {
            return 1U;
        }
        ++source;
    }
    return 0U;
}

static uint8_t source_text_contains_identifier(const char *source, const char *identifier) {
    uint8_t in_string = 0U;
    char previous = '\0';

    if (source == 0 || identifier == 0 || identifier[0] == '\0') {
        return 0U;
    }
    while (*source != '\0') {
        if (*source == '\'') {
            in_string = (uint8_t)!in_string;
            ++source;
            continue;
        }
        if (!in_string &&
            source_char_is_identifier_start(*source) &&
            !source_char_is_identifier_char(previous)) {
            char token[METHOD_SOURCE_NAME_LIMIT];
            const char *token_cursor = source_parse_identifier(source, token, sizeof(token));

            if (token_cursor != 0 && source_names_equal(token, identifier)) {
                if (!source_char_is_identifier_char(*token_cursor)) {
                    return 1U;
                }
            }
        }
        previous = *source++;
    }
    return 0U;
}

static uint8_t source_line_contains_embedded_statement_separator(const char *line) {
    uint8_t in_string = 0U;

    while (*line != '\0') {
        if (*line == '\'') {
            in_string = (uint8_t)!in_string;
        } else if (!in_string && *line == '.') {
            const char *cursor = line + 1;

            while (*cursor == ' ' || *cursor == '\t') {
                ++cursor;
            }
            if (*cursor != '\0') {
                return 1U;
            }
        }
        ++line;
    }
    return 0U;
}

static uint8_t source_text_requires_live_evaluator(const char *source) {
    const char *cursor = source;
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint8_t body_line_count = 0U;

    if (source_text_contains_block_literal(source) ||
        source_text_contains_identifier(source, "thisContext")) {
        return 1U;
    }
    if (source_copy_trimmed_line(&cursor, line, sizeof(line)) == 0U) {
        return 0U;
    }
    while (source_copy_trimmed_line(&cursor, line, sizeof(line)) != 0U) {
        if (line[0] == '|' || source_line_contains_embedded_statement_separator(line)) {
            return 1U;
        }
        ++body_line_count;
        if (body_line_count > 1U) {
            return 1U;
        }
    }
    return 0U;
}

static const char *source_parse_method_header(
    const char *source,
    char selector_name[METHOD_SOURCE_NAME_LIMIT],
    char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t *argument_count,
    const char **body_cursor_out
) {
    char header_line[METHOD_SOURCE_LINE_LIMIT];
    char selector_part[METHOD_SOURCE_NAME_LIMIT];
    const char *cursor = source;
    const char *header_cursor;
    uint32_t selector_length = 0U;
    uint32_t part_length;

    if (source_copy_trimmed_line(&cursor, header_line, sizeof(header_line)) == 0U) {
        return 0;
    }
    header_cursor = source_parse_identifier(header_line, selector_name, METHOD_SOURCE_NAME_LIMIT);
    if (header_cursor == 0) {
        return 0;
    }
    while (selector_name[selector_length] != '\0') {
        ++selector_length;
    }
    *argument_count = 0U;
    header_cursor = source_skip_horizontal_space(header_cursor);
    if (*header_cursor == ':') {
        while (*header_cursor == ':') {
            if (*argument_count >= MAX_SEND_ARGS) {
                machine_panic("KernelInstaller source method has too many arguments");
            }
            if (selector_length + 1U >= METHOD_SOURCE_NAME_LIMIT) {
                machine_panic("KernelInstaller source method selector exceeds buffer capacity");
            }
            selector_name[selector_length++] = ':';
            selector_name[selector_length] = '\0';
            ++header_cursor;
            header_cursor = source_skip_horizontal_space(header_cursor);
            header_cursor = source_parse_identifier(
                header_cursor,
                argument_names[*argument_count],
                METHOD_SOURCE_NAME_LIMIT
            );
            if (header_cursor == 0) {
                machine_panic("KernelInstaller source method keyword header is missing an argument name");
            }
            ++(*argument_count);
            header_cursor = source_skip_horizontal_space(header_cursor);
            if (*header_cursor == '\0') {
                break;
            }
            header_cursor = source_parse_identifier(header_cursor, selector_part, sizeof(selector_part));
            if (header_cursor == 0) {
                machine_panic("KernelInstaller source method header has unexpected trailing text");
            }
            part_length = 0U;
            while (selector_part[part_length] != '\0') {
                if (selector_length + 1U >= METHOD_SOURCE_NAME_LIMIT) {
                    machine_panic("KernelInstaller source method selector exceeds buffer capacity");
                }
                selector_name[selector_length++] = selector_part[part_length++];
            }
            selector_name[selector_length] = '\0';
            header_cursor = source_skip_horizontal_space(header_cursor);
        }
    }
    if (*header_cursor != '\0') {
        machine_panic("KernelInstaller source method header has unexpected trailing text");
    }
    *body_cursor_out = cursor;
    return cursor;
}

static uint16_t allocate_placeholder_live_source_compiled_method(void) {
    uint32_t instruction_words[2];

    instruction_words[0] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_NIL, 0U, 0U);
    instruction_words[1] = encode_compiled_method_word(COMPILED_METHOD_OP_RETURN_TOP, 0U, 0U);
    return allocate_compiled_method_from_words(instruction_words, 2U);
}

static struct recorz_mvp_source_eval_result source_eval_value_result(struct recorz_mvp_value value) {
    struct recorz_mvp_source_eval_result result;

    result.kind = RECORZ_MVP_SOURCE_EVAL_VALUE;
    result.value = value;
    return result;
}

static struct recorz_mvp_source_eval_result source_eval_return_result(struct recorz_mvp_value value) {
    struct recorz_mvp_source_eval_result result;

    result.kind = RECORZ_MVP_SOURCE_EVAL_RETURN;
    result.value = value;
    return result;
}

static struct recorz_mvp_source_lexical_environment *source_lexical_environment_at(int16_t lexical_environment_index) {
    if (lexical_environment_index < 0 ||
        lexical_environment_index >= (int16_t)SOURCE_EVAL_ENV_LIMIT ||
        !source_eval_environments[lexical_environment_index].in_use) {
        machine_panic("source lexical environment index is invalid");
    }
    return &source_eval_environments[lexical_environment_index];
}

static struct recorz_mvp_source_home_context *source_home_context_at(int16_t home_context_index) {
    if (home_context_index < 0 ||
        home_context_index >= (int16_t)SOURCE_EVAL_HOME_CONTEXT_LIMIT ||
        !source_eval_home_contexts[home_context_index].in_use) {
        machine_panic("source home context index is invalid");
    }
    return &source_eval_home_contexts[home_context_index];
}

static int16_t source_allocate_lexical_environment(int16_t parent_index) {
    uint16_t env_index;
    struct recorz_mvp_source_lexical_environment *environment;
    uint16_t binding_index;

    for (env_index = 0U; env_index < SOURCE_EVAL_ENV_LIMIT; ++env_index) {
        if (!source_eval_environments[env_index].in_use) {
            environment = &source_eval_environments[env_index];
            environment->in_use = 1U;
            environment->binding_count = 0U;
            environment->parent_index = parent_index;
            for (binding_index = 0U; binding_index < SOURCE_EVAL_BINDING_LIMIT; ++binding_index) {
                environment->bindings[binding_index].name[0] = '\0';
                environment->bindings[binding_index].value = nil_value();
            }
            return (int16_t)env_index;
        }
    }
    machine_panic("source lexical environment pool overflow");
    return -1;
}

static int16_t source_allocate_home_context(
    const struct recorz_mvp_heap_object *defining_class,
    struct recorz_mvp_value receiver,
    int16_t lexical_environment_index,
    uint16_t sender_context_handle,
    const char *detail_text
) {
    uint16_t home_index;

    for (home_index = 0U; home_index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++home_index) {
        if (!source_eval_home_contexts[home_index].in_use) {
            source_eval_home_contexts[home_index].in_use = 1U;
            source_eval_home_contexts[home_index].alive = 1U;
            source_eval_home_contexts[home_index].defining_class = defining_class;
            source_eval_home_contexts[home_index].receiver = receiver;
            source_eval_home_contexts[home_index].lexical_environment_index = lexical_environment_index;
            source_eval_home_contexts[home_index].context_handle = allocate_source_context_object(
                sender_context_handle,
                receiver,
                detail_text
            );
            return (int16_t)home_index;
        }
    }
    machine_panic("source home context pool overflow");
    return -1;
}

static uint16_t allocate_source_context_object(
    uint16_t sender_context_handle,
    struct recorz_mvp_value receiver,
    const char *detail_text
) {
    uint16_t handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_CONTEXT);

    heap_set_field(
        handle,
        CONTEXT_FIELD_SENDER,
        sender_context_handle == 0U ? nil_value() : object_value(sender_context_handle)
    );
    heap_set_field(handle, CONTEXT_FIELD_RECEIVER, receiver);
    if (detail_text == 0 || detail_text[0] == '\0') {
        heap_set_field(handle, CONTEXT_FIELD_DETAIL, nil_value());
    } else {
        heap_set_field(
            handle,
            CONTEXT_FIELD_DETAIL,
            string_value(runtime_string_allocate_copy(detail_text))
        );
    }
    heap_set_field(handle, CONTEXT_FIELD_ALIVE, boolean_value(1U));
    return handle;
}

static void source_append_binding(
    int16_t lexical_environment_index,
    const char *name,
    struct recorz_mvp_value value
) {
    struct recorz_mvp_source_lexical_environment *environment = source_lexical_environment_at(lexical_environment_index);
    uint16_t binding_index;

    if (name == 0 || name[0] == '\0') {
        machine_panic("source lexical binding name is empty");
    }
    for (binding_index = 0U; binding_index < environment->binding_count; ++binding_index) {
        if (source_names_equal(environment->bindings[binding_index].name, name)) {
            machine_panic("source lexical binding is duplicated");
        }
    }
    if (environment->binding_count >= SOURCE_EVAL_BINDING_LIMIT) {
        machine_panic("source lexical binding limit exceeded");
    }
    source_copy_identifier(
        environment->bindings[environment->binding_count].name,
        sizeof(environment->bindings[environment->binding_count].name),
        name
    );
    environment->bindings[environment->binding_count].value = value;
    ++environment->binding_count;
}

static struct recorz_mvp_value *source_lookup_binding_cell(
    int16_t lexical_environment_index,
    const char *name
) {
    while (lexical_environment_index >= 0) {
        struct recorz_mvp_source_lexical_environment *environment =
            source_lexical_environment_at(lexical_environment_index);
        uint16_t binding_index;

        for (binding_index = 0U; binding_index < environment->binding_count; ++binding_index) {
            if (source_names_equal(environment->bindings[binding_index].name, name)) {
                return &environment->bindings[binding_index].value;
            }
        }
        lexical_environment_index = environment->parent_index;
    }
    return 0;
}

static uint8_t workspace_source_append_instruction(
    struct recorz_mvp_workspace_source_program *program,
    uint8_t opcode,
    uint8_t operand_a,
    uint16_t operand_b
) {
    if (program->instruction_count >= WORKSPACE_SOURCE_INSTRUCTION_LIMIT) {
        return 0U;
    }
    program->instructions[program->instruction_count].opcode = opcode;
    program->instructions[program->instruction_count].operand_a = operand_a;
    program->instructions[program->instruction_count].operand_b = operand_b;
    ++program->instruction_count;
    return 1U;
}

static uint16_t workspace_source_append_string_literal(
    struct recorz_mvp_workspace_source_program *program,
    const char *text
) {
    uint32_t index = program->literal_count;
    uint32_t char_index = 0U;

    if (index >= WORKSPACE_SOURCE_LITERAL_LIMIT) {
        machine_panic("Workspace source literal count exceeds capacity");
    }
    while (text[char_index] != '\0') {
        if (char_index + 1U >= METHOD_SOURCE_CHUNK_LIMIT) {
            machine_panic("Workspace source string literal exceeds capacity");
        }
        program->literal_texts[index][char_index] = text[char_index];
        ++char_index;
    }
    program->literal_texts[index][char_index] = '\0';
    program->literals[index].kind = RECORZ_MVP_LITERAL_STRING;
    program->literals[index].integer = 0;
    program->literals[index].string = program->literal_texts[index];
    ++program->literal_count;
    return (uint16_t)index;
}

static uint16_t workspace_source_append_small_integer_literal(
    struct recorz_mvp_workspace_source_program *program,
    int32_t value
) {
    uint32_t index = program->literal_count;

    if (index >= WORKSPACE_SOURCE_LITERAL_LIMIT) {
        machine_panic("Workspace source literal count exceeds capacity");
    }
    program->literals[index].kind = RECORZ_MVP_LITERAL_SMALL_INTEGER;
    program->literals[index].integer = value;
    program->literals[index].string = 0;
    ++program->literal_count;
    return (uint16_t)index;
}

static void workspace_source_patch_instruction(
    struct recorz_mvp_workspace_source_program *program,
    uint16_t instruction_index,
    uint8_t opcode,
    uint8_t operand_a,
    uint16_t operand_b
) {
    if (instruction_index >= program->instruction_count) {
        machine_panic("Workspace source instruction patch is out of range");
    }
    program->instructions[instruction_index].opcode = opcode;
    program->instructions[instruction_index].operand_a = operand_a;
    program->instructions[instruction_index].operand_b = operand_b;
}

static const char *workspace_compile_expression_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
);
static uint16_t workspace_temporary_index(
    const struct recorz_mvp_workspace_source_program *program,
    const char *name
);
static const char *workspace_parse_temporary_declarations(
    const char *source,
    struct recorz_mvp_workspace_source_program *program
);
static const char *workspace_compile_operand_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
);
static void workspace_compile_statement(
    const char *statement,
    struct recorz_mvp_workspace_source_program *program
);
static void workspace_compile_inline_block(
    const char *source,
    struct recorz_mvp_workspace_source_program *program
);
static const char *workspace_compile_conditional_expression_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
);

static const char *workspace_compile_binary_expression_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
) {
    char selector_buffer[2];
    const char *parsed_cursor;
    uint8_t selector_id;

    parsed_cursor = workspace_compile_operand_push(cursor, program);
    if (parsed_cursor == 0) {
        return 0;
    }
    cursor = source_skip_statement_space(parsed_cursor);
    while (source_char_is_binary_selector(*cursor)) {
        selector_buffer[0] = *cursor++;
        selector_buffer[1] = '\0';
        cursor = workspace_compile_operand_push(cursor, program);
        if (cursor == 0) {
            machine_panic("Workspace binary send is missing an argument");
        }
        selector_id = source_selector_id_for_name(selector_buffer);
        if (selector_id == 0U) {
            machine_panic("Workspace source statement uses an unknown binary selector");
        }
        if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_SEND, selector_id, 1U)) {
            machine_panic("Workspace source exceeds instruction capacity");
        }
        cursor = source_skip_statement_space(cursor);
    }
    return cursor;
}

static uint16_t workspace_temporary_index(
    const struct recorz_mvp_workspace_source_program *program,
    const char *name
) {
    uint16_t temporary_index;

    for (temporary_index = 0U; temporary_index < program->lexical_count; ++temporary_index) {
        if (program->temporary_names[temporary_index][0] != '\0' &&
            source_names_equal(program->temporary_names[temporary_index], name)) {
            return temporary_index;
        }
    }
    return 0xFFFFU;
}

static const char *workspace_parse_temporary_declarations(
    const char *source,
    struct recorz_mvp_workspace_source_program *program
) {
    const char *cursor = source_skip_horizontal_space(source);

    if (*cursor != '|') {
        machine_panic("Workspace source temporary declaration is invalid");
    }
    ++cursor;
    cursor = source_skip_horizontal_space(cursor);
    while (*cursor != '\0') {
        char name[METHOD_SOURCE_NAME_LIMIT];
        uint16_t existing_index;

        if (*cursor == '|') {
            ++cursor;
            return source_skip_statement_space(cursor);
        }
        cursor = source_parse_identifier(cursor, name, sizeof(name));
        if (cursor == 0) {
            machine_panic("Workspace source temporary declaration is invalid");
        }
        for (existing_index = 0U; existing_index < program->lexical_count; ++existing_index) {
            if (source_names_equal(program->temporary_names[existing_index], name)) {
                machine_panic("Workspace source temporary declaration repeats a temporary");
            }
        }
        if (program->lexical_count >= LEXICAL_LIMIT) {
            machine_panic("Workspace source temporary declaration exceeds lexical capacity");
        }
        {
            uint32_t name_index = 0U;

            while (name[name_index] != '\0') {
                if (name_index + 1U >= sizeof(program->temporary_names[program->lexical_count])) {
                    machine_panic("Workspace source temporary name exceeds capacity");
                }
                program->temporary_names[program->lexical_count][name_index] = name[name_index];
                ++name_index;
            }
            program->temporary_names[program->lexical_count][name_index] = '\0';
        }
        ++program->lexical_count;
        cursor = source_skip_horizontal_space(cursor);
    }
    machine_panic("Workspace source temporary declaration is missing a closing '|'");
    return 0;
}

static const char *workspace_compile_operand_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
) {
    char identifier[METHOD_SOURCE_NAME_LIMIT];
    char block_text[METHOD_SOURCE_CHUNK_LIMIT];
    char quoted_text[METHOD_SOURCE_CHUNK_LIMIT];
    int32_t small_integer = 0;
    const char *parsed_cursor;
    const char *selector_cursor;
    const char *after_selector;
    uint8_t global_id;
    uint16_t literal_index;
    uint16_t temporary_index;
    uint8_t selector_id;

    cursor = source_skip_statement_space(cursor);
    if (*cursor == '(') {
        ++cursor;
        parsed_cursor = workspace_compile_expression_push(cursor, program);
        if (parsed_cursor == 0) {
            machine_panic("Workspace parenthesized expression is invalid");
        }
        parsed_cursor = source_skip_statement_space(parsed_cursor);
        if (*parsed_cursor != ')') {
            machine_panic("Workspace parenthesized expression is missing ')'");
        }
        cursor = parsed_cursor + 1;
    } else if (*cursor == '[') {
        parsed_cursor = source_copy_bracket_body(cursor, block_text, sizeof(block_text));
        if (parsed_cursor == 0) {
            machine_panic("Workspace source block literal is invalid");
        }
        literal_index = workspace_source_append_string_literal(program, block_text);
        if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_BLOCK_LITERAL, 0U, literal_index)) {
            machine_panic("Workspace source exceeds instruction capacity");
        }
        cursor = parsed_cursor;
    } else if (*cursor == '\'') {
        parsed_cursor = source_copy_single_quoted_text(cursor, quoted_text, sizeof(quoted_text));
        if (parsed_cursor == 0) {
            machine_panic("Workspace source string literal is invalid");
        }
        literal_index = workspace_source_append_string_literal(program, quoted_text);
        if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_LITERAL, 0U, literal_index)) {
            machine_panic("Workspace source exceeds instruction capacity");
        }
        cursor = parsed_cursor;
    } else {
        parsed_cursor = source_parse_small_integer(cursor, &small_integer);
        if (parsed_cursor != 0) {
            literal_index = workspace_source_append_small_integer_literal(program, small_integer);
            if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_LITERAL, 0U, literal_index)) {
                machine_panic("Workspace source exceeds instruction capacity");
            }
            cursor = parsed_cursor;
        } else {
            parsed_cursor = source_parse_identifier(cursor, identifier, sizeof(identifier));
            if (parsed_cursor == 0) {
                return 0;
            }
            if (source_names_equal(identifier, "nil")) {
                if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_NIL, 0U, 0U)) {
                    machine_panic("Workspace source exceeds instruction capacity");
                }
            } else if (source_names_equal(identifier, "self")) {
                if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_SELF, 0U, 0U)) {
                    machine_panic("Workspace source exceeds instruction capacity");
                }
            } else if (source_names_equal(identifier, "thisContext")) {
                if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_THIS_CONTEXT, 0U, 0U)) {
                    machine_panic("Workspace source exceeds instruction capacity");
                }
            } else {
                temporary_index = workspace_temporary_index(program, identifier);
                if (temporary_index != 0xFFFFU) {
                    if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_LEXICAL, 0U, temporary_index)) {
                        machine_panic("Workspace source exceeds instruction capacity");
                    }
                } else {
                    global_id = source_global_id_for_name(identifier);
                    if (global_id == 0U) {
                        machine_panic("Workspace source uses an unknown global operand");
                    }
                    if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_GLOBAL, global_id, 0U)) {
                        machine_panic("Workspace source exceeds instruction capacity");
                    }
                }
            }
            cursor = parsed_cursor;
        }
    }
    cursor = source_skip_statement_space(cursor);
    while (source_char_is_identifier_start(*cursor)) {
        selector_cursor = source_parse_identifier(cursor, identifier, sizeof(identifier));
        if (selector_cursor == 0) {
            break;
        }
        after_selector = source_skip_statement_space(selector_cursor);
        if (*after_selector == ':') {
            break;
        }
        selector_id = source_selector_id_for_name(identifier);
        if (selector_id == 0U) {
            machine_panic("Workspace source statement uses an unknown unary selector");
        }
        if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_SEND, selector_id, 0U)) {
            machine_panic("Workspace source exceeds instruction capacity");
        }
        cursor = after_selector;
    }
    return cursor;
}

static const char *workspace_compile_expression_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
) {
    char selector_buffer[METHOD_SOURCE_NAME_LIMIT];
    char selector_part[METHOD_SOURCE_NAME_LIMIT];
    const char *part_cursor;
    const char *parsed_cursor;
    uint16_t argument_count = 0U;
    uint32_t selector_length = 0U;
    uint8_t selector_id;

    parsed_cursor = workspace_compile_binary_expression_push(cursor, program);
    if (parsed_cursor == 0) {
        return 0;
    }
    cursor = source_skip_statement_space(parsed_cursor);
    part_cursor = source_parse_identifier(cursor, selector_part, sizeof(selector_part));
    if (part_cursor == 0) {
        return cursor;
    }
    part_cursor = source_skip_statement_space(part_cursor);
    if (*part_cursor != ':') {
        return cursor;
    }
    if (source_names_equal(selector_part, "ifTrue") || source_names_equal(selector_part, "ifFalse")) {
        return workspace_compile_conditional_expression_push(cursor, program);
    }
    while (selector_part[selector_length] != '\0') {
        selector_buffer[selector_length] = selector_part[selector_length];
        ++selector_length;
    }
    selector_buffer[selector_length] = '\0';
    do {
        if (selector_length + 1U >= sizeof(selector_buffer)) {
            machine_panic("Workspace keyword selector exceeds capacity");
        }
        selector_buffer[selector_length++] = ':';
        selector_buffer[selector_length] = '\0';
        cursor = workspace_compile_binary_expression_push(part_cursor + 1, program);
        if (cursor == 0) {
            machine_panic("Workspace keyword send is missing an argument");
        }
        ++argument_count;
        cursor = source_skip_statement_space(cursor);
        part_cursor = source_parse_identifier(cursor, selector_part, sizeof(selector_part));
        if (part_cursor == 0) {
            break;
        }
        part_cursor = source_skip_statement_space(part_cursor);
        if (*part_cursor != ':') {
            break;
        }
        {
            uint32_t part_index = 0U;

            while (selector_part[part_index] != '\0') {
                if (selector_length + 1U >= sizeof(selector_buffer)) {
                    machine_panic("Workspace keyword selector exceeds capacity");
                }
                selector_buffer[selector_length++] = selector_part[part_index++];
            }
            selector_buffer[selector_length] = '\0';
        }
    } while (*part_cursor == ':');
    selector_id = source_selector_id_for_name(selector_buffer);
    if (selector_id == 0U) {
        machine_panic("Workspace source statement uses an unknown selector");
    }
    if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_SEND, selector_id, argument_count)) {
        machine_panic("Workspace source exceeds instruction capacity");
    }
    return cursor;
}

static void workspace_compile_inline_block(
    const char *source,
    struct recorz_mvp_workspace_source_program *program
) {
    const char *cursor = source;
    char statement[METHOD_SOURCE_CHUNK_LIMIT];
    uint8_t statement_count = 0U;
    uint8_t pending_value = 0U;

    if (*source_skip_statement_space(source) == ':') {
        machine_panic("Workspace blocks do not support block arguments yet");
    }
    while (source_copy_next_statement(&cursor, statement, sizeof(statement)) != 0U) {
        const char *trimmed = source_skip_statement_space(statement);

        if (trimmed[0] == '^') {
            machine_panic("Workspace blocks do not support return yet");
        }
        if (pending_value) {
            if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_POP, 0U, 0U)) {
                machine_panic("Workspace source exceeds instruction capacity");
            }
        }
        workspace_compile_statement(statement, program);
        pending_value = 1U;
        ++statement_count;
    }
    if (statement_count == 0U) {
        if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_NIL, 0U, 0U)) {
            machine_panic("Workspace source exceeds instruction capacity");
        }
    }
}

static const char *workspace_compile_conditional_expression_push(
    const char *cursor,
    struct recorz_mvp_workspace_source_program *program
) {
    char first_selector[METHOD_SOURCE_NAME_LIMIT];
    char second_selector[METHOD_SOURCE_NAME_LIMIT];
    char first_block[METHOD_SOURCE_CHUNK_LIMIT];
    char second_block[METHOD_SOURCE_CHUNK_LIMIT];
    const char *part_cursor;
    const char *after_selector;
    const char *after_first_block;
    const char *after_second_block;
    uint16_t branch_index;
    uint16_t jump_index;
    uint8_t first_is_true;
    uint8_t has_second_clause = 0U;

    part_cursor = source_parse_identifier(cursor, first_selector, sizeof(first_selector));
    if (part_cursor == 0) {
        machine_panic("Workspace conditional expression is missing a selector");
    }
    after_selector = source_skip_statement_space(part_cursor);
    if (*after_selector != ':') {
        machine_panic("Workspace conditional expression is missing ':'");
    }
    after_first_block = source_copy_bracket_body(after_selector + 1, first_block, sizeof(first_block));
    if (after_first_block == 0) {
        machine_panic("Workspace conditional expression is missing a block argument");
    }
    cursor = source_skip_statement_space(after_first_block);
    part_cursor = source_parse_identifier(cursor, second_selector, sizeof(second_selector));
    if (part_cursor != 0) {
        after_selector = source_skip_statement_space(part_cursor);
        if (*after_selector == ':' &&
            ((source_names_equal(first_selector, "ifTrue") && source_names_equal(second_selector, "ifFalse")) ||
             (source_names_equal(first_selector, "ifFalse") && source_names_equal(second_selector, "ifTrue")))) {
            after_second_block = source_copy_bracket_body(after_selector + 1, second_block, sizeof(second_block));
            if (after_second_block == 0) {
                machine_panic("Workspace conditional expression is missing a matching block argument");
            }
            cursor = after_second_block;
            has_second_clause = 1U;
        }
    }

    first_is_true = (uint8_t)source_names_equal(first_selector, "ifTrue");
    branch_index = program->instruction_count;
    if (!workspace_source_append_instruction(
            program,
            first_is_true ? RECORZ_MVP_OP_JUMP_IF_FALSE : RECORZ_MVP_OP_JUMP_IF_TRUE,
            0U,
            0U)) {
        machine_panic("Workspace source exceeds instruction capacity");
    }
    workspace_compile_inline_block(first_block, program);
    jump_index = program->instruction_count;
    if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_JUMP, 0U, 0U)) {
        machine_panic("Workspace source exceeds instruction capacity");
    }
    workspace_source_patch_instruction(
        program,
        branch_index,
        first_is_true ? RECORZ_MVP_OP_JUMP_IF_FALSE : RECORZ_MVP_OP_JUMP_IF_TRUE,
        0U,
        program->instruction_count
    );
    if (has_second_clause) {
        workspace_compile_inline_block(second_block, program);
    } else if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_NIL, 0U, 0U)) {
        machine_panic("Workspace source exceeds instruction capacity");
    }
    workspace_source_patch_instruction(program, jump_index, RECORZ_MVP_OP_JUMP, 0U, program->instruction_count);
    return cursor;
}

static void workspace_compile_statement(
    const char *statement,
    struct recorz_mvp_workspace_source_program *program
) {
    const char *cursor = statement;
    char receiver_name[METHOD_SOURCE_NAME_LIMIT];
    const char *assignment_cursor;
    uint16_t temporary_index;

    assignment_cursor = source_parse_identifier(cursor, receiver_name, sizeof(receiver_name));
    if (assignment_cursor != 0) {
        assignment_cursor = source_skip_horizontal_space(assignment_cursor);
        if (assignment_cursor[0] == ':' && assignment_cursor[1] == '=') {
            temporary_index = workspace_temporary_index(program, receiver_name);
            if (temporary_index == 0xFFFFU) {
                machine_panic("Workspace assignment target must be a temporary");
            }
            cursor = workspace_compile_expression_push(assignment_cursor + 2, program);
            if (cursor == 0) {
                machine_panic("Workspace assignment right-hand side is invalid");
            }
            if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_DUP, 0U, 0U) ||
                !workspace_source_append_instruction(program, RECORZ_MVP_OP_STORE_LEXICAL, 0U, temporary_index)) {
                machine_panic("Workspace source exceeds instruction capacity");
            }
        } else {
            cursor = workspace_compile_expression_push(cursor, program);
        }
    } else {
        cursor = workspace_compile_expression_push(cursor, program);
    }
    if (cursor == 0) {
        machine_panic("Workspace source statement is missing a receiver");
    }
    cursor = source_skip_statement_space(cursor);
    if (*cursor != '\0') {
        machine_panic("Workspace source statement has unexpected trailing text");
    }
}

static void build_workspace_source_program(
    const char *source,
    struct recorz_mvp_workspace_source_program *program
) {
    const char *cursor = source;
    char statement[METHOD_SOURCE_CHUNK_LIMIT];
    uint8_t statement_count = 0U;

    program->instruction_count = 0U;
    program->literal_count = 0U;
    program->lexical_count = 0U;
    {
        uint16_t temporary_index;

        for (temporary_index = 0U; temporary_index < LEXICAL_LIMIT; ++temporary_index) {
            program->temporary_names[temporary_index][0] = '\0';
        }
    }
    if (source == 0 || *source == '\0') {
        machine_panic("Workspace source is empty");
    }
    cursor = source_skip_statement_space(cursor);
    if (*cursor == '|') {
        cursor = workspace_parse_temporary_declarations(cursor, program);
    }
    while (source_copy_next_statement(&cursor, statement, sizeof(statement)) != 0U) {
        workspace_compile_statement(statement, program);
        if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_POP, 0U, 0U)) {
            machine_panic("Workspace source exceeds instruction capacity");
        }
        ++statement_count;
    }
    if (statement_count == 0U) {
        machine_panic("Workspace source contains no executable statements");
    }
    if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_PUSH_NIL, 0U, 0U) ||
        !workspace_source_append_instruction(program, RECORZ_MVP_OP_RETURN, 0U, 0U)) {
        machine_panic("Workspace source exceeds instruction capacity");
    }
}

static const struct recorz_mvp_dynamic_class_definition *dynamic_class_definition_for_name(const char *class_name) {
    uint16_t dynamic_index;

    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        if (source_names_equal(class_name, dynamic_classes[dynamic_index].class_name)) {
            return &dynamic_classes[dynamic_index];
        }
    }
    return 0;
}

static struct recorz_mvp_dynamic_class_definition *mutable_dynamic_class_definition_for_handle(uint16_t class_handle) {
    uint16_t dynamic_index;

    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        if (dynamic_classes[dynamic_index].class_handle == class_handle) {
            return &dynamic_classes[dynamic_index];
        }
    }
    return 0;
}

static const struct recorz_mvp_dynamic_class_definition *dynamic_class_definition_for_handle(uint16_t class_handle) {
    return (const struct recorz_mvp_dynamic_class_definition *)mutable_dynamic_class_definition_for_handle(class_handle);
}

static const struct recorz_mvp_live_package_definition *package_definition_for_name(const char *package_name) {
    uint16_t package_index;

    for (package_index = 0U; package_index < package_count; ++package_index) {
        if (source_names_equal(package_name, live_packages[package_index].package_name)) {
            return &live_packages[package_index];
        }
    }
    return 0;
}

static struct recorz_mvp_live_package_definition *mutable_package_definition_for_name(const char *package_name) {
    return (struct recorz_mvp_live_package_definition *)package_definition_for_name(package_name);
}

static void remember_package_definition(
    const char *package_name,
    const char *package_comment,
    uint8_t has_comment
) {
    struct recorz_mvp_live_package_definition *definition;

    if (package_name == 0 || package_name[0] == '\0') {
        return;
    }
    definition = mutable_package_definition_for_name(package_name);
    if (definition == 0) {
        if (package_count >= PACKAGE_LIMIT) {
            machine_panic("package registry overflow");
        }
        definition = &live_packages[package_count++];
        initialize_live_package_definition(definition);
        source_copy_identifier(definition->package_name, sizeof(definition->package_name), package_name);
    }
    if (has_comment) {
        source_copy_identifier(definition->package_comment, sizeof(definition->package_comment), package_comment);
    }
}

static uint16_t named_object_handle_for_name(const char *name) {
    uint16_t named_index;

    for (named_index = 0U; named_index < named_object_count; ++named_index) {
        if (source_names_equal(name, named_objects[named_index].name)) {
            return named_objects[named_index].object_handle;
        }
    }
    return 0U;
}

static void remember_named_object_handle(uint16_t object_handle, const char *name) {
    uint16_t named_index;

    if (object_handle == 0U || object_handle > heap_size) {
        machine_panic("named object handle is out of range");
    }
    if (name == 0 || *name == '\0') {
        machine_panic("named object name is empty");
    }
    for (named_index = 0U; named_index < named_object_count; ++named_index) {
        if (source_names_equal(name, named_objects[named_index].name)) {
            named_objects[named_index].object_handle = object_handle;
            return;
        }
    }
    if (named_object_count >= NAMED_OBJECT_LIMIT) {
        machine_panic("named object registry overflow");
    }
    source_copy_identifier(
        named_objects[named_object_count].name,
        sizeof(named_objects[named_object_count].name),
        name
    );
    named_objects[named_object_count].object_handle = object_handle;
    ++named_object_count;
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
    if (global_id == 0U || global_id > MAX_GLOBAL_ID || global_handles[global_id] == 0U) {
        machine_panic("unknown global in MVP VM");
    }
    return object_value(global_handles[global_id]);
}

static struct recorz_mvp_value top_level_receiver_value(void) {
    return global_value(RECORZ_MVP_GLOBAL_WORKSPACE);
}

static struct recorz_mvp_value boolean_value(uint8_t condition) {
    return global_value(condition ? RECORZ_MVP_GLOBAL_TRUE : RECORZ_MVP_GLOBAL_FALSE);
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

static uint8_t condition_value_is_true(struct recorz_mvp_value value) {
    if (value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("conditional branch expects a boolean object");
    }
    if ((uint16_t)value.integer == global_handles[RECORZ_MVP_GLOBAL_TRUE]) {
        return 1U;
    }
    if ((uint16_t)value.integer == global_handles[RECORZ_MVP_GLOBAL_FALSE]) {
        return 0U;
    }
    machine_panic("conditional branch expects true or false");
    return 0U;
}

static uint8_t value_equals(struct recorz_mvp_value left, struct recorz_mvp_value right) {
    if (left.kind != right.kind) {
        return 0U;
    }
    switch (left.kind) {
        case RECORZ_MVP_VALUE_NIL:
            return 1U;
        case RECORZ_MVP_VALUE_SMALL_INTEGER:
        case RECORZ_MVP_VALUE_OBJECT:
            return (uint8_t)(left.integer == right.integer);
        case RECORZ_MVP_VALUE_STRING:
            if (left.string == 0 || right.string == 0) {
                return (uint8_t)(left.string == right.string);
            }
            return source_names_equal(left.string, right.string);
    }
    machine_panic("unsupported value kind in equality check");
    return 0U;
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

static void append_memory_report_text(
    char buffer[],
    uint32_t *offset,
    const char *text
) {
    uint32_t index = 0U;

    while (text[index] != '\0') {
        if (*offset + 1U >= MEMORY_REPORT_BUFFER_SIZE) {
            machine_panic("KernelInstaller memory report exceeds buffer capacity");
        }
        buffer[(*offset)++] = text[index++];
    }
    buffer[*offset] = '\0';
}

static void append_memory_report_u32(
    char buffer[],
    uint32_t *offset,
    uint32_t value
) {
    render_small_integer((int32_t)value);
    append_memory_report_text(buffer, offset, print_buffer);
}

static void append_memory_report_line(
    char buffer[],
    uint32_t *offset,
    const char *label,
    uint32_t used,
    uint32_t limit
) {
    append_memory_report_text(buffer, offset, label);
    append_memory_report_text(buffer, offset, " ");
    append_memory_report_u32(buffer, offset, used);
    append_memory_report_text(buffer, offset, "/");
    append_memory_report_u32(buffer, offset, limit);
    append_memory_report_text(buffer, offset, "\n");
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

static uint16_t allocate_block_closure_from_source(
    const char *source_text,
    struct recorz_mvp_value home_receiver
) {
    uint16_t handle;

    if (source_text == 0) {
        machine_panic("block closure source is null");
    }
    handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_BLOCK_CLOSURE);
    heap_set_field(
        handle,
        BLOCK_CLOSURE_FIELD_SOURCE,
        string_value(runtime_string_allocate_copy(source_text))
    );
    heap_set_field(handle, BLOCK_CLOSURE_FIELD_HOME_RECEIVER, home_receiver);
    heap_set_field(handle, BLOCK_CLOSURE_FIELD_LEXICAL0, nil_value());
    heap_set_field(handle, BLOCK_CLOSURE_FIELD_LEXICAL1, nil_value());
    return handle;
}

static void source_register_block_state(
    uint16_t block_handle,
    const struct recorz_mvp_heap_object *defining_class,
    int16_t lexical_environment_index,
    int16_t home_context_index
) {
    uint16_t state_index;

    for (state_index = 0U; state_index < SOURCE_EVAL_BLOCK_STATE_LIMIT; ++state_index) {
        if (source_eval_block_states[state_index].in_use &&
            source_eval_block_states[state_index].block_handle == block_handle) {
            source_eval_block_states[state_index].defining_class = defining_class;
            source_eval_block_states[state_index].lexical_environment_index = lexical_environment_index;
            source_eval_block_states[state_index].home_context_index = home_context_index;
            return;
        }
    }
    for (state_index = 0U; state_index < SOURCE_EVAL_BLOCK_STATE_LIMIT; ++state_index) {
        if (!source_eval_block_states[state_index].in_use) {
            source_eval_block_states[state_index].in_use = 1U;
            source_eval_block_states[state_index].block_handle = block_handle;
            source_eval_block_states[state_index].defining_class = defining_class;
            source_eval_block_states[state_index].lexical_environment_index = lexical_environment_index;
            source_eval_block_states[state_index].home_context_index = home_context_index;
            return;
        }
    }
    machine_panic("source block state pool overflow");
}

static const struct recorz_mvp_runtime_block_state *source_block_state_for_handle(uint16_t block_handle) {
    uint16_t state_index;

    for (state_index = 0U; state_index < SOURCE_EVAL_BLOCK_STATE_LIMIT; ++state_index) {
        if (source_eval_block_states[state_index].in_use &&
            source_eval_block_states[state_index].block_handle == block_handle) {
            return &source_eval_block_states[state_index];
        }
    }
    return 0;
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
    if (selector == 0U || selector > MAX_SELECTOR_ID ||
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

static uint16_t compiled_method_lexical_count(const struct recorz_mvp_heap_object *compiled_method) {
    uint8_t instruction_index;
    uint16_t lexical_count = 0U;

    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("method entry implementation is not a compiled method");
    }
    for (instruction_index = 0U; instruction_index < compiled_method->field_count; ++instruction_index) {
        uint32_t instruction = compiled_method_instruction_word(compiled_method, instruction_index);
        uint8_t opcode = compiled_method_instruction_opcode(instruction);
        uint16_t lexical_index;

        if (opcode != COMPILED_METHOD_OP_PUSH_LEXICAL &&
            opcode != COMPILED_METHOD_OP_STORE_LEXICAL) {
            continue;
        }
        lexical_index = compiled_method_instruction_operand_b(instruction);
        if ((uint16_t)(lexical_index + 1U) > lexical_count) {
            lexical_count = (uint16_t)(lexical_index + 1U);
        }
    }
    return lexical_count;
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

static const struct recorz_mvp_heap_object *class_superclass_object_or_null(
    const struct recorz_mvp_heap_object *class_object
) {
    struct recorz_mvp_value superclass_value = heap_get_field(class_object, CLASS_FIELD_SUPERCLASS);

    if (superclass_value.kind == RECORZ_MVP_VALUE_NIL) {
        return 0;
    }
    if (superclass_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("class superclass is not a class object");
    }
    {
        const struct recorz_mvp_heap_object *superclass_object = heap_object_for_value(superclass_value);

        if (superclass_object->kind != RECORZ_MVP_OBJECT_CLASS) {
            machine_panic("class superclass is not a class object");
        }
        return superclass_object;
    }
}

static const struct recorz_mvp_heap_object *class_side_lookup_target(
    const struct recorz_mvp_heap_object *class_object
) {
    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("class-side lookup expects a class object");
    }
    return class_object_for_heap_object(class_object);
}

static const struct recorz_mvp_heap_object *ensure_dedicated_metaclass_for_class(
    const struct recorz_mvp_heap_object *class_object,
    const struct recorz_mvp_heap_object *superclass_object
) {
    const struct recorz_mvp_heap_object *shared_class_object = class_object_for_kind(RECORZ_MVP_OBJECT_CLASS);
    const struct recorz_mvp_heap_object *metaclass_object;
    uint16_t class_handle;
    uint16_t metaclass_handle;
    uint16_t metaclass_superclass_handle;

    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("metaclass creation expects a class object");
    }
    if (class_object == shared_class_object) {
        return class_object;
    }
    metaclass_superclass_handle = heap_handle_for_object(shared_class_object);
    if (superclass_object != 0) {
        metaclass_superclass_handle = heap_handle_for_object(class_side_lookup_target(superclass_object));
    }
    metaclass_object = class_side_lookup_target(class_object);
    class_handle = heap_handle_for_object(class_object);
    if (metaclass_object == shared_class_object) {
        metaclass_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_CLASS);
        heap_set_field(metaclass_handle, CLASS_FIELD_INSTANCE_KIND, small_integer_value((int32_t)RECORZ_MVP_OBJECT_CLASS));
        heap_set_field(metaclass_handle, CLASS_FIELD_METHOD_START, nil_value());
        heap_set_field(metaclass_handle, CLASS_FIELD_METHOD_COUNT, small_integer_value(0));
        heap_set_class(class_handle, metaclass_handle);
        metaclass_object = (const struct recorz_mvp_heap_object *)heap_object(metaclass_handle);
    } else {
        metaclass_handle = heap_handle_for_object(metaclass_object);
    }
    heap_set_field(
        metaclass_handle,
        CLASS_FIELD_SUPERCLASS,
        object_value(metaclass_superclass_handle)
    );
    return (const struct recorz_mvp_heap_object *)heap_object(metaclass_handle);
}

static const struct recorz_mvp_heap_object *lookup_builtin_method_descriptor(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector,
    uint16_t argument_count
) {
    const struct recorz_mvp_heap_object *selector_object;
    const struct recorz_mvp_heap_object *lookup_class = class_object;
    uint32_t lookup_depth = 0U;

    if (selector == 0U || selector > MAX_SELECTOR_ID) {
        machine_panic("selector id is out of range");
    }
    if (selector_handles_by_id[selector] == 0U) {
        return 0;
    }
    selector_object = (const struct recorz_mvp_heap_object *)heap_object(selector_handles_by_id[selector]);

    while (lookup_class != 0) {
        const struct recorz_mvp_heap_object *method_object = class_method_start_object(lookup_class);
        uint32_t method_count = class_method_count(lookup_class);
        uint32_t method_index;

        if (lookup_depth++ >= HEAP_LIMIT) {
            machine_panic("class superclass chain is invalid");
        }
        if (method_count != 0U) {
            if (method_object == 0) {
                machine_panic("class with methods is missing a method start");
            }
            for (method_index = 0U; method_index < method_count; ++method_index) {
                const struct recorz_mvp_heap_object *candidate = (const struct recorz_mvp_heap_object *)heap_object(
                    (uint16_t)(heap_handle_for_object(method_object) + method_index)
                );

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
        }
        lookup_class = class_superclass_object_or_null(lookup_class);
    }
    return 0;
}

static const struct recorz_mvp_heap_object *class_object_for_kind(uint8_t kind) {
    if (kind == 0U || kind > MAX_OBJECT_KIND || class_descriptor_handles_by_kind[kind] == 0U) {
        machine_panic("method update class kind is out of range");
    }
    return (const struct recorz_mvp_heap_object *)heap_object(class_descriptor_handles_by_kind[kind]);
}

static const char *class_name_for_object(const struct recorz_mvp_heap_object *class_object) {
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition =
        dynamic_class_definition_for_handle(heap_handle_for_object(class_object));

    if (dynamic_definition != 0) {
        return dynamic_definition->class_name;
    }
    return object_kind_name((uint8_t)class_instance_kind(class_object));
}

static uint8_t live_instance_field_count_for_class_with_depth(
    const struct recorz_mvp_heap_object *class_object,
    uint32_t depth
) {
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition =
        dynamic_class_definition_for_handle(heap_handle_for_object(class_object));
    const struct recorz_mvp_heap_object *superclass_object;
    uint32_t total_count = 0U;

    if (depth > DYNAMIC_CLASS_LIMIT) {
        machine_panic("class instance field chain is invalid");
    }
    superclass_object = class_superclass_object_or_null(class_object);
    if (superclass_object != 0) {
        total_count = live_instance_field_count_for_class_with_depth(superclass_object, depth + 1U);
    }
    if (dynamic_definition != 0) {
        total_count += dynamic_definition->instance_variable_count;
    }
    if (total_count > OBJECT_FIELD_LIMIT) {
        machine_panic("class instance field count exceeds object field capacity");
    }
    return (uint8_t)total_count;
}

static uint8_t live_instance_field_count_for_class(const struct recorz_mvp_heap_object *class_object) {
    return live_instance_field_count_for_class_with_depth(class_object, 0U);
}

static uint8_t class_field_index_for_name_with_depth(
    const struct recorz_mvp_heap_object *class_object,
    const char *field_name,
    uint8_t *field_index_out,
    uint32_t depth
) {
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition =
        dynamic_class_definition_for_handle(heap_handle_for_object(class_object));
    const struct recorz_mvp_heap_object *superclass_object = class_superclass_object_or_null(class_object);
    uint8_t inherited_field_count = 0U;
    uint8_t field_index;

    if (depth > DYNAMIC_CLASS_LIMIT) {
        machine_panic("class field lookup chain is invalid");
    }
    if (superclass_object != 0) {
        inherited_field_count = live_instance_field_count_for_class_with_depth(superclass_object, depth + 1U);
    }
    if (dynamic_definition == 0) {
        if (superclass_object == 0) {
            return 0U;
        }
        return class_field_index_for_name_with_depth(superclass_object, field_name, field_index_out, depth + 1U);
    }
    for (field_index = 0U; field_index < dynamic_definition->instance_variable_count; ++field_index) {
        if (source_names_equal(field_name, dynamic_definition->instance_variable_names[field_index])) {
            *field_index_out = (uint8_t)(inherited_field_count + field_index);
            return 1U;
        }
    }
    if (superclass_object == 0) {
        return 0U;
    }
    return class_field_index_for_name_with_depth(superclass_object, field_name, field_index_out, depth + 1U);
}

static uint8_t class_field_index_for_name(
    const struct recorz_mvp_heap_object *class_object,
    const char *field_name,
    uint8_t *field_index_out
) {
    return class_field_index_for_name_with_depth(class_object, field_name, field_index_out, 0U);
}

static const struct recorz_mvp_heap_object *default_form_object(void) {
    if (default_form_handle == 0U) {
        machine_panic("default form is not initialized");
    }
    return (const struct recorz_mvp_heap_object *)heap_object(default_form_handle);
}

static void validate_workspace_receiver(const struct recorz_mvp_heap_object *workspace_object) {
    const struct recorz_mvp_heap_object *workspace_class = class_object_for_heap_object(workspace_object);

    if (class_instance_kind(workspace_class) != RECORZ_MVP_OBJECT_WORKSPACE) {
        machine_panic("Workspace receiver class is invalid");
    }
}

static uint8_t workspace_current_view_kind_field_index(
    const struct recorz_mvp_heap_object *workspace_object
) {
    validate_workspace_receiver(workspace_object);
    return RECORZ_MVP_WORKSPACE_FIELD_CURRENT_VIEW_KIND;
}

static uint8_t workspace_current_target_name_field_index(
    const struct recorz_mvp_heap_object *workspace_object
) {
    validate_workspace_receiver(workspace_object);
    return RECORZ_MVP_WORKSPACE_FIELD_CURRENT_TARGET_NAME;
}

static uint8_t workspace_current_source_field_index(
    const struct recorz_mvp_heap_object *workspace_object
) {
    validate_workspace_receiver(workspace_object);
    return RECORZ_MVP_WORKSPACE_FIELD_CURRENT_SOURCE;
}

static uint8_t workspace_last_source_field_index(
    const struct recorz_mvp_heap_object *workspace_object
) {
    validate_workspace_receiver(workspace_object);
    return RECORZ_MVP_WORKSPACE_FIELD_LAST_SOURCE;
}

static void workspace_remember_view(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t view_kind,
    const char *target_name
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);

    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)view_kind)
    );
    if (target_name == 0 || target_name[0] == '\0') {
        heap_set_field(
            workspace_handle,
            workspace_current_target_name_field_index(workspace_object),
            nil_value()
        );
        return;
    }
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        string_value(target_name)
    );
}

static void workspace_remember_source(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *source
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);

    if (source == 0 || source[0] == '\0') {
        heap_set_field(
            workspace_handle,
            workspace_last_source_field_index(workspace_object),
            nil_value()
        );
        return;
    }
    heap_set_field(
        workspace_handle,
        workspace_last_source_field_index(workspace_object),
        string_value(source)
    );
}

static void workspace_remember_current_source(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *source
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);

    if (source == 0 || source[0] == '\0') {
        heap_set_field(
            workspace_handle,
            workspace_current_source_field_index(workspace_object),
            nil_value()
        );
        return;
    }
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        string_value(source)
    );
}

static struct recorz_mvp_value workspace_current_source_value(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint8_t field_index = workspace_current_source_field_index(workspace_object);

    if (workspace_object->field_count <= field_index) {
        return nil_value();
    }
    return heap_get_field(workspace_object, field_index);
}

static const struct recorz_mvp_heap_object *workspace_target_class_for_file_in(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;

    if (workspace_object->field_count <= workspace_current_view_kind_field_index(workspace_object)) {
        return 0;
    }
    view_kind_value = heap_get_field(workspace_object, workspace_current_view_kind_field_index(workspace_object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        return 0;
    }
    if (workspace_object->field_count <= workspace_current_target_name_field_index(workspace_object)) {
        return 0;
    }
    target_name_value = heap_get_field(workspace_object, workspace_current_target_name_field_index(workspace_object));
    if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        return 0;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS) {
        return lookup_class_by_name(target_name_value.string);
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_SOURCE) {
        return lookup_class_by_name(target_name_value.string);
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHODS) {
        return lookup_class_by_name(target_name_value.string);
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHOD) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char selector_name[METHOD_SOURCE_NAME_LIMIT];

        if (!workspace_parse_method_target_name(target_name_value.string, class_name, sizeof(class_name), selector_name, sizeof(selector_name))) {
            machine_panic("Workspace method target is invalid");
        }
        return lookup_class_by_name(class_name);
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHODS) {
        const struct recorz_mvp_heap_object *class_object = lookup_class_by_name(target_name_value.string);

        if (class_object == 0) {
            return 0;
        }
        return ensure_dedicated_metaclass_for_class(class_object, class_superclass_object_or_null(class_object));
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHOD) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char selector_name[METHOD_SOURCE_NAME_LIMIT];
        const struct recorz_mvp_heap_object *class_object;

        if (!workspace_parse_method_target_name(target_name_value.string, class_name, sizeof(class_name), selector_name, sizeof(selector_name))) {
            machine_panic("Workspace class-side method target is invalid");
        }
        class_object = lookup_class_by_name(class_name);
        if (class_object == 0) {
            return 0;
        }
        return ensure_dedicated_metaclass_for_class(class_object, class_superclass_object_or_null(class_object));
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PROTOCOLS) {
        return lookup_class_by_name(target_name_value.string);
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PROTOCOL) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char protocol_name[METHOD_SOURCE_NAME_LIMIT];

        if (!workspace_parse_protocol_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                protocol_name,
                sizeof(protocol_name))) {
            machine_panic("Workspace protocol target is invalid");
        }
        return lookup_class_by_name(class_name);
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOLS) {
        const struct recorz_mvp_heap_object *class_object = lookup_class_by_name(target_name_value.string);

        if (class_object == 0) {
            return 0;
        }
        return ensure_dedicated_metaclass_for_class(class_object, class_superclass_object_or_null(class_object));
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOL) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char protocol_name[METHOD_SOURCE_NAME_LIMIT];
        const struct recorz_mvp_heap_object *class_object;

        if (!workspace_parse_protocol_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                protocol_name,
                sizeof(protocol_name))) {
            machine_panic("Workspace class-side protocol target is invalid");
        }
        class_object = lookup_class_by_name(class_name);
        if (class_object == 0) {
            return 0;
        }
        return ensure_dedicated_metaclass_for_class(class_object, class_superclass_object_or_null(class_object));
    }
    return 0;
}

static const char *workspace_compose_method_target_name(
    const char *class_name,
    const char *selector_name
) {
    uint32_t offset = 0U;
    const char *part = class_name;

    if (class_name == 0 || class_name[0] == '\0' || selector_name == 0 || selector_name[0] == '\0') {
        machine_panic("Workspace method target is incomplete");
    }
    while (*part != '\0') {
        if (offset + 1U >= sizeof(workspace_target_buffer)) {
            machine_panic("Workspace method target exceeds buffer capacity");
        }
        workspace_target_buffer[offset++] = *part++;
    }
    if (offset + 2U >= sizeof(workspace_target_buffer)) {
        machine_panic("Workspace method target exceeds buffer capacity");
    }
    workspace_target_buffer[offset++] = '>';
    workspace_target_buffer[offset++] = '>';
    part = selector_name;
    while (*part != '\0') {
        if (offset + 1U >= sizeof(workspace_target_buffer)) {
            machine_panic("Workspace method target exceeds buffer capacity");
        }
        workspace_target_buffer[offset++] = *part++;
    }
    workspace_target_buffer[offset] = '\0';
    return workspace_target_buffer;
}

static uint8_t workspace_parse_method_target_name(
    const char *target_name,
    char class_name[],
    uint32_t class_name_size,
    char selector_name[],
    uint32_t selector_name_size
) {
    const char *separator = target_name;
    uint32_t class_length;
    uint32_t selector_length = 0U;
    uint32_t index;

    if (target_name == 0 || target_name[0] == '\0') {
        return 0U;
    }
    while (separator[0] != '\0') {
        if (separator[0] == '>' && separator[1] == '>') {
            break;
        }
        ++separator;
    }
    if (separator[0] == '\0') {
        return 0U;
    }
    class_length = (uint32_t)(separator - target_name);
    if (class_length == 0U || class_length + 1U > class_name_size) {
        return 0U;
    }
    for (index = 0U; index < class_length; ++index) {
        class_name[index] = target_name[index];
    }
    class_name[class_length] = '\0';
    separator += 2;
    while (separator[selector_length] != '\0') {
        if (selector_length + 1U >= selector_name_size) {
            return 0U;
        }
        selector_name[selector_length] = separator[selector_length];
        ++selector_length;
    }
    if (selector_length == 0U) {
        return 0U;
    }
    selector_name[selector_length] = '\0';
    return 1U;
}

static const char *workspace_compose_protocol_target_name(
    const char *class_name,
    const char *protocol_name
) {
    uint32_t offset = 0U;
    const char *part = class_name;

    if (class_name == 0 || class_name[0] == '\0' || protocol_name == 0 || protocol_name[0] == '\0') {
        machine_panic("Workspace protocol target is incomplete");
    }
    while (*part != '\0') {
        if (offset + 1U >= sizeof(workspace_target_buffer)) {
            machine_panic("Workspace protocol target exceeds buffer capacity");
        }
        workspace_target_buffer[offset++] = *part++;
    }
    if (offset + 2U >= sizeof(workspace_target_buffer)) {
        machine_panic("Workspace protocol target exceeds buffer capacity");
    }
    workspace_target_buffer[offset++] = '\n';
    part = protocol_name;
    while (*part != '\0') {
        if (offset + 1U >= sizeof(workspace_target_buffer)) {
            machine_panic("Workspace protocol target exceeds buffer capacity");
        }
        workspace_target_buffer[offset++] = *part++;
    }
    workspace_target_buffer[offset] = '\0';
    return workspace_target_buffer;
}

static uint8_t workspace_parse_protocol_target_name(
    const char *target_name,
    char class_name[],
    uint32_t class_name_size,
    char protocol_name[],
    uint32_t protocol_name_size
) {
    const char *separator = target_name;
    uint32_t class_length;
    uint32_t protocol_length = 0U;
    uint32_t index;

    if (target_name == 0 || target_name[0] == '\0') {
        return 0U;
    }
    while (separator[0] != '\0') {
        if (separator[0] == '\n') {
            break;
        }
        ++separator;
    }
    if (separator[0] == '\0') {
        return 0U;
    }
    class_length = (uint32_t)(separator - target_name);
    if (class_length == 0U || class_length + 1U > class_name_size) {
        return 0U;
    }
    for (index = 0U; index < class_length; ++index) {
        class_name[index] = target_name[index];
    }
    class_name[class_length] = '\0';
    separator += 1;
    while (separator[protocol_length] != '\0') {
        if (protocol_length + 1U >= protocol_name_size) {
            return 0U;
        }
        protocol_name[protocol_length] = separator[protocol_length];
        ++protocol_length;
    }
    if (protocol_length == 0U) {
        return 0U;
    }
    protocol_name[protocol_length] = '\0';
    return 1U;
}

static uint32_t workspace_visible_columns(const struct recorz_mvp_heap_object *form) {
    uint32_t left_margin = text_left_margin();
    uint32_t width = bitmap_width(bitmap_for_form(form));
    uint32_t char_columns;

    if (width <= left_margin || char_width() == 0U) {
        return 1U;
    }
    char_columns = (width - left_margin) / char_width();
    return char_columns == 0U ? 1U : char_columns;
}

static uint8_t workspace_has_line_capacity(const struct recorz_mvp_heap_object *form) {
    return cursor_y + char_height() < bitmap_height(bitmap_for_form(form));
}

static void workspace_newline(void) {
    cursor_x = text_left_margin();
    cursor_y += char_height() + text_line_spacing();
}

static void workspace_copy_display_text(
    char output[],
    uint32_t output_size,
    const char *text,
    uint32_t max_visible_chars
) {
    uint32_t index = 0U;
    uint32_t limit = output_size == 0U ? 0U : output_size - 1U;
    uint8_t truncated = 0U;

    if (max_visible_chars != 0U && max_visible_chars < limit) {
        limit = max_visible_chars;
    }
    while (text[index] != '\0') {
        char character = text[index];

        if (index >= limit) {
            truncated = 1U;
            break;
        }
        if (character >= 'a' && character <= 'z') {
            character = (char)(character - ('a' - 'A'));
        }
        output[index++] = character;
    }
    if (truncated && limit >= 3U) {
        if (index > limit - 3U) {
            index = limit - 3U;
        }
        output[index++] = '.';
        output[index++] = '.';
        output[index++] = '.';
    }
    output[index] = '\0';
}

static void workspace_write_label_and_text(
    const struct recorz_mvp_heap_object *form,
    const char *label,
    const char *text
) {
    char uppercase_buffer[METHOD_SOURCE_NAME_LIMIT];
    uint32_t columns;
    uint32_t label_columns;
    uint32_t text_columns;

    if (!workspace_has_line_capacity(form)) {
        return;
    }
    columns = workspace_visible_columns(form);
    label_columns = text_length(label) + 2U;
    text_columns = columns > label_columns ? columns - label_columns : 0U;
    workspace_copy_display_text(uppercase_buffer, sizeof(uppercase_buffer), text, text_columns);
    form_write_string(form, label);
    form_write_string(form, ": ");
    form_write_string(form, uppercase_buffer);
    workspace_newline();
}

static void workspace_write_text_line(
    const struct recorz_mvp_heap_object *form,
    const char *text
) {
    char uppercase_buffer[METHOD_SOURCE_NAME_LIMIT];

    if (!workspace_has_line_capacity(form)) {
        return;
    }
    workspace_copy_display_text(
        uppercase_buffer,
        sizeof(uppercase_buffer),
        text,
        workspace_visible_columns(form)
    );
    form_write_string(form, uppercase_buffer);
    workspace_newline();
}

static void workspace_write_label_and_integer(
    const struct recorz_mvp_heap_object *form,
    const char *label,
    uint32_t value
) {
    if (!workspace_has_line_capacity(form)) {
        return;
    }
    form_write_string(form, label);
    form_write_string(form, ": ");
    render_small_integer((int32_t)value);
    form_write_string(form, print_buffer);
    workspace_newline();
}

static const char *workspace_named_object_name_for_handle(uint16_t object_handle) {
    uint16_t named_index;

    for (named_index = 0U; named_index < named_object_count; ++named_index) {
        if (named_objects[named_index].object_handle == object_handle) {
            return named_objects[named_index].name;
        }
    }
    return 0;
}

static void workspace_write_label_and_value(
    const struct recorz_mvp_heap_object *form,
    const char *label,
    struct recorz_mvp_value value
) {
    if (value.kind == RECORZ_MVP_VALUE_NIL) {
        workspace_write_label_and_text(form, label, "nil");
        return;
    }
    if (value.kind == RECORZ_MVP_VALUE_STRING && value.string != 0) {
        workspace_write_label_and_text(form, label, value.string);
        return;
    }
    if (value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER) {
        workspace_write_label_and_integer(form, label, (uint32_t)value.integer);
        return;
    }
    if (value.kind == RECORZ_MVP_VALUE_OBJECT) {
        const char *named_object_name = workspace_named_object_name_for_handle((uint16_t)value.integer);

        if (named_object_name != 0) {
            workspace_write_label_and_text(form, label, named_object_name);
            return;
        }
        workspace_write_label_and_text(
            form,
            label,
            class_name_for_object(class_object_for_heap_object(heap_object_for_value(value)))
        );
        return;
    }
    machine_panic("Workspace browser encountered an unsupported value");
}

static void workspace_render_class_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_heap_object *class_object,
    const char *class_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *superclass_object = class_superclass_object_or_null(class_object);
    const char *superclass_name = superclass_object == 0 ? "nil" : class_name_for_object(superclass_object);
    const struct recorz_mvp_dynamic_class_definition *definition =
        dynamic_class_definition_for_handle(heap_handle_for_object(class_object));

    (void)workspace_object;
    form_clear(form);
    form_write_string(form, "WORKSPACE");
    form_newline(form);
    workspace_write_label_and_text(form, "CLASS", class_name);
    workspace_write_label_and_text(form, "SUPER", superclass_name);
    if (definition != 0 && definition->package_name[0] != '\0') {
        workspace_write_label_and_text(form, "PACKAGE", definition->package_name);
    }
    if (definition != 0 && definition->class_comment[0] != '\0') {
        workspace_write_label_and_text(form, "COMMENT", definition->class_comment);
    }
    workspace_write_label_and_integer(form, "SLOTS", live_instance_field_count_for_class(class_object));
    workspace_write_label_and_integer(form, "METHODS", class_method_count(class_object));
}

static uint32_t workspace_package_count(void) {
    return package_count;
}

static uint32_t workspace_package_class_count(
    const char *package_name
) {
    uint16_t dynamic_index;
    uint32_t count = 0U;

    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        if (source_names_equal(dynamic_classes[dynamic_index].package_name, package_name)) {
            ++count;
        }
    }
    return count;
}

static void workspace_render_package_list_browser(
    const struct recorz_mvp_heap_object *workspace_object
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT];
    uint16_t package_index;
    uint16_t sorted_count = 0U;

    (void)workspace_object;
    form_clear(form);
    form_write_string(form, "WORKSPACE");
    form_newline(form);
    form_write_string(form, "PACKAGES");
    form_newline(form);
    workspace_write_label_and_integer(form, "PACKAGES", workspace_package_count());
    if (workspace_package_count() == 0U) {
        workspace_write_text_line(form, "NO PACKAGES");
        return;
    }
    for (package_index = 0U; package_index < package_count; ++package_index) {
        uint16_t insert_index = sorted_count;

        while (insert_index != 0U &&
               compare_source_names(
                   live_packages[package_index].package_name,
                   sorted_packages[insert_index - 1U]->package_name) < 0) {
            sorted_packages[insert_index] = sorted_packages[insert_index - 1U];
            --insert_index;
        }
        sorted_packages[insert_index] = &live_packages[package_index];
        ++sorted_count;
    }
    for (package_index = 0U; package_index < sorted_count; ++package_index) {
        char line[METHOD_SOURCE_LINE_LIMIT];
        uint32_t line_offset = 0U;

        append_text_checked(line, sizeof(line), &line_offset, sorted_packages[package_index]->package_name);
        append_text_checked(line, sizeof(line), &line_offset, " :: ");
        render_small_integer((int32_t)workspace_package_class_count(sorted_packages[package_index]->package_name));
        append_text_checked(line, sizeof(line), &line_offset, print_buffer);
        workspace_write_text_line(form, line);
    }
}

static void workspace_render_package_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *package_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_live_package_definition *package_definition = package_definition_for_name(package_name);
    uint16_t dynamic_index;
    uint32_t class_count = workspace_package_class_count(package_name);

    (void)workspace_object;
    form_clear(form);
    workspace_write_label_and_text(form, "PACKAGE", package_name);
    if (package_definition != 0 && package_definition->package_comment[0] != '\0') {
        workspace_write_label_and_text(form, "COMMENT", package_definition->package_comment);
    }
    workspace_write_label_and_integer(form, "CLASSES", class_count);
    if (class_count == 0U) {
        workspace_write_text_line(form, "EMPTY PACKAGE");
        return;
    }
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        if (!source_names_equal(dynamic_classes[dynamic_index].package_name, package_name)) {
            continue;
        }
        workspace_write_text_line(form, dynamic_classes[dynamic_index].class_name);
    }
}

static void workspace_render_class_list_browser(
    const struct recorz_mvp_heap_object *workspace_object
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    uint8_t kind;
    uint16_t dynamic_index;
    uint32_t seeded_count = 0U;

    (void)workspace_object;
    form_clear(form);
    form_write_string(form, "WORKSPACE");
    form_newline(form);
    form_write_string(form, "CLASSES");
    form_newline(form);
    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] != 0U) {
            ++seeded_count;
        }
    }
    workspace_write_label_and_integer(form, "SEEDED", seeded_count);
    workspace_write_label_and_integer(form, "DYNAMIC", dynamic_class_count);
    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] == 0U) {
            continue;
        }
        workspace_write_text_line(form, object_kind_name(kind));
    }
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        workspace_write_text_line(form, dynamic_classes[dynamic_index].class_name);
    }
}

static void workspace_render_method_list_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_heap_object *class_object,
    const char *class_name,
    const char *side_label
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t method_index;

    (void)workspace_object;
    form_clear(form);
    workspace_write_label_and_text(form, "CLASS", class_name);
    workspace_write_label_and_text(form, "SIDE", side_label);
    workspace_write_label_and_integer(form, "METHODS", method_count);
    if (method_count == 0U) {
        workspace_write_text_line(form, "NO METHODS");
        return;
    }
    if (method_start_object == 0) {
        machine_panic("Workspace method list is missing a method start");
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );
        const struct recorz_mvp_live_method_source *source_record;
        char line[METHOD_SOURCE_LINE_LIMIT];
        uint32_t line_offset = 0U;

        source_record = live_method_source_for_selector_and_arity(
            heap_handle_for_object(class_object),
            (uint8_t)method_descriptor_selector(method_object),
            (uint8_t)method_descriptor_argument_count(method_object)
        );
        line[0] = '\0';
        if (source_record != 0 && source_record->protocol_name[0] != '\0') {
            append_text_checked(line, sizeof(line), &line_offset, source_record->protocol_name);
            append_text_checked(line, sizeof(line), &line_offset, " :: ");
        }
        append_text_checked(
            line,
            sizeof(line),
            &line_offset,
            selector_name((uint8_t)method_descriptor_selector(method_object))
        );
        workspace_write_text_line(form, line);
    }
}

static const char *workspace_protocol_name_for_method(
    const struct recorz_mvp_heap_object *class_object,
    const struct recorz_mvp_heap_object *method_object
) {
    const struct recorz_mvp_live_method_source *source_record = live_method_source_for_selector_and_arity(
        heap_handle_for_object(class_object),
        (uint8_t)method_descriptor_selector(method_object),
        (uint8_t)method_descriptor_argument_count(method_object)
    );

    if (source_record == 0 || source_record->protocol_name[0] == '\0') {
        return "UNFILED";
    }
    return source_record->protocol_name;
}

static uint32_t workspace_protocol_method_count(
    const struct recorz_mvp_heap_object *class_object,
    const char *protocol_name
) {
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t method_index;
    uint32_t count = 0U;

    if (method_count == 0U || method_start_object == 0) {
        return 0U;
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );

        if (source_names_equal(workspace_protocol_name_for_method(class_object, method_object), protocol_name)) {
            ++count;
        }
    }
    return count;
}

static uint8_t workspace_protocol_seen_earlier(
    const struct recorz_mvp_heap_object *class_object,
    const struct recorz_mvp_heap_object *method_start_object,
    uint32_t method_index,
    const char *protocol_name
) {
    uint32_t previous_index;

    for (previous_index = 0U; previous_index < method_index; ++previous_index) {
        const struct recorz_mvp_heap_object *previous_method_object =
            (const struct recorz_mvp_heap_object *)heap_object(
                (uint16_t)(heap_handle_for_object(method_start_object) + previous_index)
            );

        if (source_names_equal(
                workspace_protocol_name_for_method(class_object, previous_method_object),
                protocol_name)) {
            return 1U;
        }
    }
    return 0U;
}

static uint32_t workspace_protocol_count(
    const struct recorz_mvp_heap_object *class_object
) {
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t method_index;
    uint32_t count = 0U;

    if (method_count == 0U || method_start_object == 0) {
        return 0U;
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );
        const char *protocol_name = workspace_protocol_name_for_method(class_object, method_object);

        if (!workspace_protocol_seen_earlier(class_object, method_start_object, method_index, protocol_name)) {
            ++count;
        }
    }
    return count;
}

static void workspace_render_protocol_list_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_heap_object *class_object,
    const char *class_name,
    const char *side_label
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t protocol_count = workspace_protocol_count(class_object);
    uint32_t method_index;

    (void)workspace_object;
    form_clear(form);
    workspace_write_label_and_text(form, "CLASS", class_name);
    workspace_write_label_and_text(form, "SIDE", side_label);
    workspace_write_label_and_text(form, "VIEW", "PROTOCOLS");
    workspace_write_label_and_integer(form, "PROTOCOLS", protocol_count);
    if (method_count == 0U) {
        workspace_write_text_line(form, "NO METHODS");
        return;
    }
    if (method_start_object == 0) {
        machine_panic("Workspace protocol list is missing a method start");
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );
        const char *protocol_name = workspace_protocol_name_for_method(class_object, method_object);
        char line[METHOD_SOURCE_LINE_LIMIT];
        uint32_t line_offset = 0U;

        if (workspace_protocol_seen_earlier(class_object, method_start_object, method_index, protocol_name)) {
            continue;
        }
        append_text_checked(line, sizeof(line), &line_offset, protocol_name);
        append_text_checked(line, sizeof(line), &line_offset, " (");
        render_small_integer((int32_t)workspace_protocol_method_count(class_object, protocol_name));
        append_text_checked(line, sizeof(line), &line_offset, print_buffer);
        append_text_checked(line, sizeof(line), &line_offset, ")");
        workspace_write_text_line(form, line);
    }
}

static void workspace_render_protocol_method_list_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_heap_object *class_object,
    const char *class_name,
    const char *side_label,
    const char *protocol_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t protocol_method_count = workspace_protocol_method_count(class_object, protocol_name);
    uint32_t method_index;

    (void)workspace_object;
    form_clear(form);
    workspace_write_label_and_text(form, "CLASS", class_name);
    workspace_write_label_and_text(form, "SIDE", side_label);
    workspace_write_label_and_text(form, "PROTO", protocol_name);
    workspace_write_label_and_integer(form, "METHODS", protocol_method_count);
    if (method_count == 0U || protocol_method_count == 0U) {
        workspace_write_text_line(form, "NO METHODS");
        return;
    }
    if (method_start_object == 0) {
        machine_panic("Workspace protocol browser is missing a method start");
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );

        if (!source_names_equal(
                workspace_protocol_name_for_method(class_object, method_object),
                protocol_name)) {
            continue;
        }
        workspace_write_text_line(form, selector_name((uint8_t)method_descriptor_selector(method_object)));
    }
}

static void workspace_render_method_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *class_name,
    const char *selector_name_text,
    const char *side_label
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *cursor;
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint8_t wrote_line = 0U;

    form_clear(form);
    workspace_write_label_and_text(form, "CLASS", class_name);
    workspace_write_label_and_text(form, "SIDE", side_label);
    workspace_write_label_and_text(form, "METHOD", selector_name_text);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        workspace_write_text_line(form, "NO SOURCE BUFFER");
        return;
    }
    cursor = source_value.string;
    while (source_copy_trimmed_line(&cursor, line, sizeof(line)) != 0U) {
        workspace_write_text_line(form, line);
        wrote_line = 1U;
    }
    if (!wrote_line) {
        workspace_write_text_line(form, "EMPTY SOURCE");
    }
}

static void workspace_render_class_source_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *class_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *cursor;
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint8_t wrote_line = 0U;

    form_clear(form);
    workspace_write_label_and_text(form, "CLASS", class_name);
    workspace_write_label_and_text(form, "VIEW", "SOURCE");
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        workspace_write_text_line(form, "NO SOURCE BUFFER");
        return;
    }
    cursor = source_value.string;
    while (source_copy_trimmed_line(&cursor, line, sizeof(line)) != 0U) {
        workspace_write_text_line(form, line);
        wrote_line = 1U;
    }
    if (!wrote_line) {
        workspace_write_text_line(form, "EMPTY SOURCE");
    }
}

static void workspace_render_package_source_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *package_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *cursor;
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint8_t wrote_line = 0U;

    form_clear(form);
    workspace_write_label_and_text(form, "PACKAGE", package_name);
    workspace_write_label_and_text(form, "VIEW", "SOURCE");
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        workspace_write_text_line(form, "NO SOURCE BUFFER");
        return;
    }
    cursor = source_value.string;
    while (source_copy_trimmed_line(&cursor, line, sizeof(line)) != 0U) {
        workspace_write_text_line(form, line);
        wrote_line = 1U;
    }
    if (!wrote_line) {
        workspace_write_text_line(form, "EMPTY SOURCE");
    }
}

static void workspace_render_dynamic_object_fields(
    const struct recorz_mvp_heap_object *form,
    const struct recorz_mvp_heap_object *class_object,
    const struct recorz_mvp_heap_object *object,
    uint32_t depth
) {
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition =
        dynamic_class_definition_for_handle(heap_handle_for_object(class_object));
    const struct recorz_mvp_heap_object *superclass_object = class_superclass_object_or_null(class_object);
    uint8_t field_index;

    if (depth > DYNAMIC_CLASS_LIMIT) {
        machine_panic("Workspace object browser class chain is invalid");
    }
    if (superclass_object != 0) {
        workspace_render_dynamic_object_fields(form, superclass_object, object, depth + 1U);
    }
    if (dynamic_definition == 0) {
        return;
    }
    for (field_index = 0U; field_index < dynamic_definition->instance_variable_count; ++field_index) {
        uint8_t resolved_field_index;

        if (!class_field_index_for_name(
                class_object,
                dynamic_definition->instance_variable_names[field_index],
                &resolved_field_index)) {
            machine_panic("Workspace object browser could not resolve an instance variable");
        }
        workspace_write_label_and_value(
            form,
            dynamic_definition->instance_variable_names[field_index],
            heap_get_field(object, resolved_field_index)
        );
    }
}

static void workspace_render_object_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *object_name,
    const struct recorz_mvp_heap_object *object
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *class_object = class_object_for_heap_object(object);

    (void)workspace_object;
    form_clear(form);
    form_write_string(form, "WORKSPACE");
    form_newline(form);
    workspace_write_label_and_text(form, "OBJECT", object_name);
    workspace_write_label_and_text(form, "CLASS", class_name_for_object(class_object));
    workspace_write_label_and_integer(form, "SLOTS", object->field_count);
    workspace_render_dynamic_object_fields(form, class_object, object, 0U);
}

static void validate_dynamic_class_definition(
    const struct recorz_mvp_live_class_definition *definition,
    const struct recorz_mvp_dynamic_class_definition *existing_definition,
    const struct recorz_mvp_heap_object *superclass_object
) {
    uint8_t field_index;
    uint8_t inherited_field_index;

    if (existing_definition != 0) {
        if (existing_definition->instance_variable_count != definition->instance_variable_count) {
            machine_panic("KernelInstaller does not support changing dynamic class instance variables");
        }
        for (field_index = 0U; field_index < definition->instance_variable_count; ++field_index) {
            if (!source_names_equal(
                    existing_definition->instance_variable_names[field_index],
                    definition->instance_variable_names[field_index])) {
                machine_panic("KernelInstaller does not support renaming dynamic class instance variables");
            }
        }
    }
    for (field_index = 0U; field_index < definition->instance_variable_count; ++field_index) {
        uint8_t other_field_index;

        for (other_field_index = (uint8_t)(field_index + 1U);
             other_field_index < definition->instance_variable_count;
             ++other_field_index) {
            if (source_names_equal(
                    definition->instance_variable_names[field_index],
                    definition->instance_variable_names[other_field_index])) {
                machine_panic("KernelInstaller class chunk repeats an instance variable name");
            }
        }
        if (superclass_object != 0 &&
            class_field_index_for_name(superclass_object, definition->instance_variable_names[field_index], &inherited_field_index)) {
            machine_panic("KernelInstaller class chunk repeats an inherited instance variable name");
        }
    }
}

static uint32_t primitive_kind_for_heap_object(const struct recorz_mvp_heap_object *object) {
    return class_instance_kind(class_object_for_heap_object(object));
}

static void validate_compiled_method(
    const struct recorz_mvp_heap_object *compiled_method,
    uint32_t argument_count
) {
    uint8_t discovered_stack_depth[COMPILED_METHOD_MAX_INSTRUCTIONS];
    uint8_t pending_instruction_indices[COMPILED_METHOD_MAX_INSTRUCTIONS];
    uint8_t pending_count = 0U;
    uint8_t saw_return = 0U;
    uint8_t instruction_count;
    uint8_t instruction_index;

    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("compiled method entry implementation is not a compiled method");
    }
    if (compiled_method->field_count == 0U || compiled_method->field_count > COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("compiled method field count is invalid");
    }
    instruction_count = compiled_method->field_count;
    for (instruction_index = 0U; instruction_index < instruction_count; ++instruction_index) {
        discovered_stack_depth[instruction_index] = 0xFFU;
    }
    discovered_stack_depth[0] = 0U;
    pending_instruction_indices[pending_count++] = 0U;
    while (pending_count != 0U) {
        uint32_t instruction;
        uint8_t opcode;
        uint8_t operand_a;
        uint16_t operand_b;
        uint32_t send_count;
        uint8_t stack_depth;
        uint8_t next_depth = 0U;
        uint16_t target_pc;

        instruction_index = pending_instruction_indices[--pending_count];
        stack_depth = discovered_stack_depth[instruction_index];
        instruction = compiled_method_instruction_word(compiled_method, instruction_index);
        opcode = compiled_method_instruction_opcode(instruction);
        operand_a = compiled_method_instruction_operand_a(instruction);
        operand_b = compiled_method_instruction_operand_b(instruction);
        switch (opcode) {
            case COMPILED_METHOD_OP_PUSH_GLOBAL:
                if (operand_a < RECORZ_MVP_GLOBAL_TRANSCRIPT || operand_a > MAX_GLOBAL_ID) {
                    machine_panic("compiled method pushGlobal global id is out of range");
                }
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_ROOT:
                if (operand_a < RECORZ_MVP_SEED_ROOT_DEFAULT_FORM ||
                    operand_a > RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS) {
                    machine_panic("compiled method pushRoot root id is out of range");
                }
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_ARGUMENT:
                if (operand_a >= argument_count) {
                    machine_panic("compiled method pushArgument index is out of range");
                }
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_LEXICAL:
                if (operand_b >= LEXICAL_LIMIT) {
                    machine_panic("compiled method pushLexical index is out of range");
                }
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_NIL:
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_FIELD:
                if (operand_a >= OBJECT_FIELD_LIMIT) {
                    machine_panic("compiled method pushField index is out of range");
                }
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_SELF:
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_THIS_CONTEXT:
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_SMALL_INTEGER:
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_PUSH_STRING_LITERAL:
                if (operand_b == 0U || operand_b > LIVE_STRING_LITERAL_LIMIT) {
                    machine_panic("compiled method pushStringLiteral slot is out of range");
                }
                next_depth = (uint8_t)(stack_depth + 1U);
                break;
            case COMPILED_METHOD_OP_STORE_LEXICAL:
                if (operand_b >= LEXICAL_LIMIT) {
                    machine_panic("compiled method storeLexical index is out of range");
                }
                if (stack_depth == 0U) {
                    machine_panic("compiled method storeLexical stack underflow");
                }
                next_depth = (uint8_t)(stack_depth - 1U);
                break;
            case COMPILED_METHOD_OP_STORE_FIELD:
                if (operand_a >= OBJECT_FIELD_LIMIT) {
                    machine_panic("compiled method storeField index is out of range");
                }
                if (stack_depth == 0U) {
                    machine_panic("compiled method storeField stack underflow");
                }
                next_depth = (uint8_t)(stack_depth - 1U);
                break;
            case COMPILED_METHOD_OP_POP:
                if (stack_depth == 0U) {
                    machine_panic("compiled method pop stack underflow");
                }
                next_depth = (uint8_t)(stack_depth - 1U);
                break;
            case COMPILED_METHOD_OP_SEND:
                if (operand_a < RECORZ_MVP_SELECTOR_SHOW ||
                    operand_a > MAX_SELECTOR_ID) {
                    machine_panic("compiled method send selector is out of range");
                }
                send_count = operand_b;
                if (send_count > MAX_SEND_ARGS) {
                    machine_panic("compiled method send argument count is out of range");
                }
                if (stack_depth < send_count + 1U) {
                    machine_panic("compiled method send stack underflow");
                }
                next_depth = (uint8_t)(stack_depth - send_count);
                break;
            case COMPILED_METHOD_OP_JUMP:
                target_pc = operand_b;
                if (target_pc >= instruction_count) {
                    machine_panic("compiled method jump target is out of range");
                }
                if (discovered_stack_depth[target_pc] == 0xFFU) {
                    discovered_stack_depth[target_pc] = stack_depth;
                    pending_instruction_indices[pending_count++] = (uint8_t)target_pc;
                } else if (discovered_stack_depth[target_pc] != stack_depth) {
                    machine_panic("compiled method jump stack depth mismatch");
                }
                continue;
            case COMPILED_METHOD_OP_JUMP_IF_TRUE:
            case COMPILED_METHOD_OP_JUMP_IF_FALSE:
                if (stack_depth == 0U) {
                    machine_panic("compiled method conditional jump stack underflow");
                }
                target_pc = operand_b;
                if (target_pc >= instruction_count) {
                    machine_panic("compiled method conditional jump target is out of range");
                }
                next_depth = (uint8_t)(stack_depth - 1U);
                if (discovered_stack_depth[target_pc] == 0xFFU) {
                    discovered_stack_depth[target_pc] = next_depth;
                    pending_instruction_indices[pending_count++] = (uint8_t)target_pc;
                } else if (discovered_stack_depth[target_pc] != next_depth) {
                    machine_panic("compiled method conditional jump stack depth mismatch");
                }
                break;
            case COMPILED_METHOD_OP_RETURN_TOP:
                if (stack_depth == 0U) {
                    machine_panic("compiled method returnTop stack underflow");
                }
                saw_return = 1U;
                continue;
            case COMPILED_METHOD_OP_RETURN_RECEIVER:
                saw_return = 1U;
                continue;
            default:
                machine_panic("compiled method opcode is unknown");
        }
        if ((uint8_t)(instruction_index + 1U) >= instruction_count) {
            machine_panic("compiled method falls through without returning");
        }
        if (discovered_stack_depth[instruction_index + 1U] == 0xFFU) {
            discovered_stack_depth[instruction_index + 1U] = next_depth;
            pending_instruction_indices[pending_count++] = (uint8_t)(instruction_index + 1U);
        } else if (discovered_stack_depth[instruction_index + 1U] != next_depth) {
            machine_panic("compiled method control flow stack depth mismatch");
        }
    }
    if (!saw_return) {
        machine_panic("compiled method has no reachable return");
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
    const struct recorz_mvp_heap_object *class_object,
    uint8_t require_seeded_entry_ids
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
    if (start_handle == 0U || start_handle > heap_size) {
        machine_panic("class method start is out of range");
    }
    if ((uint32_t)start_handle + method_count - 1U > heap_size) {
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
            selector > MAX_SELECTOR_ID) {
            machine_panic("method descriptor selector is out of range");
        }
        if (argument_count > MAX_SEND_ARGS) {
            machine_panic("method descriptor argument count exceeds send capacity");
        }
        if (primitive_kind != class_kind) {
            machine_panic("method descriptor primitive kind does not match class instance kind");
        }
        if (entry == 0U) {
            machine_panic("method entry execution id is out of range");
        }
        if (require_seeded_entry_ids && entry >= RECORZ_MVP_METHOD_ENTRY_COUNT) {
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

static void validate_heap_class_graph(uint16_t object_count, uint8_t require_seeded_entry_ids) {
    uint16_t handle;

    for (handle = 1U; handle <= object_count; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);
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
    for (handle = 1U; handle <= object_count; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);

        if (object->kind == RECORZ_MVP_OBJECT_CLASS) {
            validate_class_method_table(object, require_seeded_entry_ids);
        }
    }
}

static void initialize_class_handle_cache(void) {
    uint16_t handle;
    uint16_t kind;

    for (kind = 0U; kind <= MAX_OBJECT_KIND; ++kind) {
        class_handles_by_kind[kind] = 0U;
        class_descriptor_handles_by_kind[kind] = 0U;
    }
    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);

        if (object->kind != RECORZ_MVP_OBJECT_CLASS) {
            continue;
        }
        kind = (uint16_t)class_instance_kind(object);
        if (kind == 0U || kind > MAX_OBJECT_KIND) {
            continue;
        }
        if (class_descriptor_handles_by_kind[kind] == 0U) {
            class_descriptor_handles_by_kind[kind] = handle;
            class_handles_by_kind[kind] = handle;
        }
    }
    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);

        if (object->kind > MAX_OBJECT_KIND || object->class_handle == 0U) {
            continue;
        }
        if (class_handles_by_kind[object->kind] == 0U) {
            class_handles_by_kind[object->kind] = object->class_handle;
        }
    }
}

static void initialize_selector_handle_cache(void) {
    uint16_t handle;
    uint16_t selector_id;

    for (selector_id = 0U; selector_id <= MAX_SELECTOR_ID; ++selector_id) {
        selector_handles_by_id[selector_id] = 0U;
    }
    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);

        if (object->kind != RECORZ_MVP_OBJECT_SELECTOR) {
            continue;
        }
        selector_id = (uint16_t)selector_object_selector_id(object);
        if (selector_handles_by_id[selector_id] != 0U) {
            machine_panic("seed contains duplicate selector objects");
        }
        selector_handles_by_id[selector_id] = handle;
    }
}

static void initialize_runtime_caches(void) {
    initialize_class_handle_cache();
    initialize_selector_handle_cache();
}

static void reset_runtime_state(void) {
    uint16_t global_index;
    uint16_t dynamic_index;
    uint16_t package_index;
    uint16_t named_index;
    uint16_t code_index;

    heap_size = 0U;
    mono_bitmap_count = 0U;
    next_dynamic_method_entry_execution_id = RECORZ_MVP_METHOD_ENTRY_COUNT;
    dynamic_class_count = 0U;
    package_count = 0U;
    named_object_count = 0U;
    live_method_source_count = 0U;
    default_form_handle = 0U;
    framebuffer_bitmap_handle = 0U;
    glyph_fallback_handle = 0U;
    transcript_layout_handle = 0U;
    transcript_style_handle = 0U;
    transcript_metrics_handle = 0U;
    transcript_behavior_handle = 0U;
    startup_hook_receiver_handle = 0U;
    startup_hook_selector_id = 0U;
    cursor_x = 0U;
    cursor_y = 0U;
    live_method_source_pool_used = 0U;
    live_method_source_pool[0] = '\0';
    runtime_string_pool_offset = 0U;
    runtime_string_pool[0] = '\0';
    snapshot_string_pool[0] = '\0';
    for (global_index = 0U; global_index <= MAX_GLOBAL_ID; ++global_index) {
        global_handles[global_index] = 0U;
    }
    for (dynamic_index = 0U; dynamic_index < DYNAMIC_CLASS_LIMIT; ++dynamic_index) {
        uint8_t ivar_index;

        dynamic_classes[dynamic_index].class_handle = 0U;
        dynamic_classes[dynamic_index].superclass_handle = 0U;
        dynamic_classes[dynamic_index].class_name[0] = '\0';
        dynamic_classes[dynamic_index].package_name[0] = '\0';
        dynamic_classes[dynamic_index].class_comment[0] = '\0';
        dynamic_classes[dynamic_index].instance_variable_count = 0U;
        for (ivar_index = 0U; ivar_index < DYNAMIC_CLASS_IVAR_LIMIT; ++ivar_index) {
            dynamic_classes[dynamic_index].instance_variable_names[ivar_index][0] = '\0';
        }
    }
    for (package_index = 0U; package_index < PACKAGE_LIMIT; ++package_index) {
        live_packages[package_index].package_name[0] = '\0';
        live_packages[package_index].package_comment[0] = '\0';
    }
    for (named_index = 0U; named_index < NAMED_OBJECT_LIMIT; ++named_index) {
        named_objects[named_index].name[0] = '\0';
        named_objects[named_index].object_handle = 0U;
    }
    for (named_index = 0U; named_index < LIVE_STRING_LITERAL_LIMIT; ++named_index) {
        live_string_literals[named_index].class_handle = 0U;
        live_string_literals[named_index].selector_id = 0U;
        live_string_literals[named_index].argument_count = 0U;
        live_string_literals[named_index].text = 0;
    }
    for (named_index = 0U; named_index < LIVE_METHOD_SOURCE_LIMIT; ++named_index) {
        live_method_sources[named_index].class_handle = 0U;
        live_method_sources[named_index].selector_id = 0U;
        live_method_sources[named_index].argument_count = 0U;
        live_method_sources[named_index].protocol_name[0] = '\0';
        live_method_sources[named_index].source_offset = 0U;
        live_method_sources[named_index].source_length = 0U;
    }
    for (code_index = 0U; code_index < 128U; ++code_index) {
        glyph_bitmap_handles[code_index] = 0U;
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
    uint8_t echo_serial = (uint8_t)(bitmap_storage_kind(bitmap_for_form(form)) == BITMAP_STORAGE_FRAMEBUFFER);

    cursor_x = text_left_margin();
    cursor_y += char_height() + text_line_spacing();
    if (transcript_clear_on_overflow() && cursor_y + char_height() >= bitmap_height(bitmap_for_form(form))) {
        form_clear(form);
    }
    if (echo_serial) {
        machine_putc('\n');
    }
}

static void form_write_string(const struct recorz_mvp_heap_object *form, const char *text) {
    uint32_t form_width = bitmap_width(bitmap_for_form(form));
    uint8_t echo_serial = (uint8_t)(bitmap_storage_kind(bitmap_for_form(form)) == BITMAP_STORAGE_FRAMEBUFFER);

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
        if (echo_serial) {
            machine_putc(*text);
        }
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

    if (seed->object_count > HEAP_LIMIT) {
        machine_panic("seed object count exceeds heap capacity");
    }

    reset_runtime_state();
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
    validate_heap_class_graph(heap_size, 1U);
    initialize_runtime_caches();

    for (global_index = 0U; global_index <= MAX_GLOBAL_ID; ++global_index) {
        global_handles[global_index] = 0U;
    }
    for (global_index = RECORZ_MVP_GLOBAL_TRANSCRIPT; global_index <= MAX_GLOBAL_ID; ++global_index) {
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

static uint32_t text_length(const char *text) {
    uint32_t length = 0U;

    if (text == 0) {
        return 0U;
    }
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

static uint8_t runtime_string_pool_contains(const char *text, uint32_t *offset_out) {
    uint32_t offset;

    if (text == 0) {
        return 0U;
    }
    if (text < runtime_string_pool || text >= runtime_string_pool + runtime_string_pool_offset) {
        return 0U;
    }
    offset = (uint32_t)(text - runtime_string_pool);
    if (offset_out != 0) {
        *offset_out = offset;
    }
    return 1U;
}

static void runtime_string_mark_live_value(
    struct recorz_mvp_value value,
    uint8_t live_starts[],
    uint32_t live_starts_size
) {
    uint32_t offset;

    if (value.kind != RECORZ_MVP_VALUE_STRING ||
        !runtime_string_pool_contains(value.string, &offset)) {
        return;
    }
    live_starts[offset >> 3U] |= (uint8_t)(1U << (offset & 7U));
    (void)live_starts_size;
}

static uint8_t runtime_string_live_start_is_marked(
    const uint8_t live_starts[],
    uint32_t offset
) {
    return (uint8_t)((live_starts[offset >> 3U] & (uint8_t)(1U << (offset & 7U))) != 0U);
}

static void runtime_string_rewrite_live_value(
    struct recorz_mvp_value *value,
    const char *old_text,
    const char *new_text
) {
    if (value->kind == RECORZ_MVP_VALUE_STRING && value->string == old_text) {
        value->string = new_text;
    }
}

static void runtime_string_rewrite_references(const char *old_text, const char *new_text) {
    uint32_t stack_index;
    uint16_t handle;
    uint16_t literal_index;

    for (stack_index = 0U; stack_index < stack_size; ++stack_index) {
        runtime_string_rewrite_live_value(&stack[stack_index], old_text, new_text);
    }
    for (handle = 1U; handle <= heap_size; ++handle) {
        struct recorz_mvp_heap_object *object = heap_object(handle);
        uint8_t field_index;

        for (field_index = 0U; field_index < object->field_count; ++field_index) {
            runtime_string_rewrite_live_value(&object->fields[field_index], old_text, new_text);
        }
    }
    runtime_string_rewrite_live_value(&panic_send_receiver, old_text, new_text);
    for (stack_index = 0U; stack_index < MAX_SEND_ARGS; ++stack_index) {
        runtime_string_rewrite_live_value(&panic_send_arguments[stack_index], old_text, new_text);
    }
    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        if (live_string_literals[literal_index].text == old_text) {
            live_string_literals[literal_index].text = new_text;
        }
    }
}

static void runtime_string_compact_live_references(void) {
    uint8_t live_starts[(RUNTIME_STRING_POOL_LIMIT + 7U) / 8U];
    uint32_t write_offset = 0U;
    uint32_t read_offset;
    uint32_t live_starts_index;
    uint32_t stack_index;
    uint16_t handle;
    uint16_t literal_index;

    for (live_starts_index = 0U; live_starts_index < sizeof(live_starts); ++live_starts_index) {
        live_starts[live_starts_index] = 0U;
    }
    for (stack_index = 0U; stack_index < stack_size; ++stack_index) {
        runtime_string_mark_live_value(stack[stack_index], live_starts, sizeof(live_starts));
    }
    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = heap_object(handle);
        uint8_t field_index;

        for (field_index = 0U; field_index < object->field_count; ++field_index) {
            runtime_string_mark_live_value(object->fields[field_index], live_starts, sizeof(live_starts));
        }
    }
    runtime_string_mark_live_value(panic_send_receiver, live_starts, sizeof(live_starts));
    for (stack_index = 0U; stack_index < MAX_SEND_ARGS; ++stack_index) {
        runtime_string_mark_live_value(panic_send_arguments[stack_index], live_starts, sizeof(live_starts));
    }
    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        if (live_string_literals[literal_index].text != 0) {
            runtime_string_mark_live_value(
                string_value(live_string_literals[literal_index].text),
                live_starts,
                sizeof(live_starts)
            );
        }
    }

    read_offset = 0U;
    while (read_offset < runtime_string_pool_offset) {
        uint32_t string_length = text_length(runtime_string_pool + read_offset);

        if (runtime_string_live_start_is_marked(live_starts, read_offset)) {
            const char *old_text = runtime_string_pool + read_offset;
            const char *new_text = runtime_string_pool + write_offset;
            uint32_t char_index;

            if (write_offset != read_offset) {
                for (char_index = 0U; char_index <= string_length; ++char_index) {
                    runtime_string_pool[write_offset + char_index] =
                        runtime_string_pool[read_offset + char_index];
                }
                runtime_string_rewrite_references(old_text, new_text);
            }
            write_offset += string_length + 1U;
        }
        read_offset += string_length + 1U;
    }
    runtime_string_pool_offset = write_offset;
    if (runtime_string_pool_offset < RUNTIME_STRING_POOL_LIMIT) {
        runtime_string_pool[runtime_string_pool_offset] = '\0';
    }
}

static const char *runtime_string_allocate_copy(const char *text) {
    uint32_t length;
    uint32_t index;
    uint32_t start;

    if (text == 0) {
        machine_panic("runtime string source is null");
    }
    length = text_length(text);
    if (length + 1U > RUNTIME_STRING_POOL_LIMIT) {
        machine_panic("runtime string exceeds pool capacity");
    }
    if (runtime_string_pool_offset + length + 1U > RUNTIME_STRING_POOL_LIMIT) {
        runtime_string_compact_live_references();
    }
    if (runtime_string_pool_offset + length + 1U > RUNTIME_STRING_POOL_LIMIT) {
        machine_panic("runtime string pool overflow");
    }
    start = runtime_string_pool_offset;
    for (index = 0U; index < length; ++index) {
        runtime_string_pool[start + index] = text[index];
    }
    runtime_string_pool[start + length] = '\0';
    runtime_string_pool_offset += length + 1U;
    return runtime_string_pool + start;
}

static const char *live_method_source_text(const struct recorz_mvp_live_method_source *source_record) {
    if (source_record->source_length == 0U) {
        return "";
    }
    if (source_record->source_offset + source_record->source_length >= LIVE_METHOD_SOURCE_POOL_LIMIT) {
        machine_panic("live method source offset is out of range");
    }
    if (live_method_source_pool[source_record->source_offset + source_record->source_length] != '\0') {
        machine_panic("live method source is not terminated");
    }
    return live_method_source_pool + source_record->source_offset;
}

static void repack_live_method_source_pool(void) {
    uint16_t source_index;
    uint16_t write_offset = 0U;

    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];
        uint16_t source_offset = source_record->source_offset;
        uint16_t source_length = source_record->source_length;
        uint16_t char_index;

        if (source_length == 0U) {
            source_record->source_offset = 0U;
            continue;
        }
        if (source_offset + source_length >= LIVE_METHOD_SOURCE_POOL_LIMIT) {
            machine_panic("live method source offset is out of range");
        }
        if (write_offset + source_length + 1U > LIVE_METHOD_SOURCE_POOL_LIMIT) {
            machine_panic("live method source pool overflow");
        }
        for (char_index = 0U; char_index <= source_length; ++char_index) {
            live_method_source_pool[write_offset + char_index] =
                live_method_source_pool[source_offset + char_index];
        }
        source_record->source_offset = write_offset;
        write_offset += source_length + 1U;
    }
    live_method_source_pool_used = write_offset;
    if (live_method_source_pool_used < LIVE_METHOD_SOURCE_POOL_LIMIT) {
        live_method_source_pool[live_method_source_pool_used] = '\0';
    }
}

static struct recorz_mvp_live_method_source *mutable_live_method_source_for(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count
) {
    uint16_t source_index;

    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];

        if (source_record->class_handle == class_handle &&
            source_record->selector_id == selector_id &&
            source_record->argument_count == argument_count) {
            return source_record;
        }
    }
    return 0;
}

static const struct recorz_mvp_live_method_source *live_method_source_for(
    uint16_t class_handle,
    uint8_t selector_id
) {
    uint16_t source_index;

    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        const struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];

        if (source_record->class_handle == class_handle &&
            source_record->selector_id == selector_id) {
            return source_record;
        }
    }
    return 0;
}

static void forget_live_method_source(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count
) {
    uint16_t source_index;

    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];
        uint16_t move_index;

        if (source_record->class_handle != class_handle ||
            source_record->selector_id != selector_id ||
            source_record->argument_count != argument_count) {
            continue;
        }
        for (move_index = source_index + 1U; move_index < live_method_source_count; ++move_index) {
            live_method_sources[move_index - 1U].class_handle = live_method_sources[move_index].class_handle;
            live_method_sources[move_index - 1U].selector_id = live_method_sources[move_index].selector_id;
            live_method_sources[move_index - 1U].argument_count = live_method_sources[move_index].argument_count;
            source_copy_identifier(
                live_method_sources[move_index - 1U].protocol_name,
                sizeof(live_method_sources[move_index - 1U].protocol_name),
                live_method_sources[move_index].protocol_name
            );
            live_method_sources[move_index - 1U].source_offset = live_method_sources[move_index].source_offset;
            live_method_sources[move_index - 1U].source_length = live_method_sources[move_index].source_length;
        }
        --live_method_source_count;
        repack_live_method_source_pool();
        return;
    }
}

static void remember_live_method_source(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count,
    const char *protocol_name,
    const char *source
) {
    struct recorz_mvp_live_method_source *source_record;
    uint32_t length;
    uint32_t start;

    if (source == 0 || source[0] == '\0') {
        forget_live_method_source(class_handle, selector_id, argument_count);
        return;
    }
    length = text_length(source);
    if (length + 1U > METHOD_SOURCE_CHUNK_LIMIT) {
        machine_panic("live method source exceeds chunk capacity");
    }
    source_record = mutable_live_method_source_for(class_handle, selector_id, argument_count);
    if (source_record != 0) {
        uint32_t reused_bytes = (uint32_t)source_record->source_length + 1U;

        if (live_method_source_pool_used - reused_bytes + length + 1U > LIVE_METHOD_SOURCE_POOL_LIMIT) {
            machine_panic("live method source pool overflow");
        }
        forget_live_method_source(class_handle, selector_id, argument_count);
    } else if (live_method_source_count >= LIVE_METHOD_SOURCE_LIMIT) {
        machine_panic("live method source registry overflow");
    }
    if (live_method_source_pool_used + length + 1U > LIVE_METHOD_SOURCE_POOL_LIMIT) {
        machine_panic("live method source pool overflow");
    }
    source_record = &live_method_sources[live_method_source_count++];
    source_record->class_handle = class_handle;
    source_record->selector_id = selector_id;
    source_record->argument_count = argument_count;
    source_copy_identifier(source_record->protocol_name, sizeof(source_record->protocol_name), protocol_name);
    source_record->source_offset = live_method_source_pool_used;
    source_record->source_length = (uint16_t)length;
    start = live_method_source_pool_used;
    live_method_source_pool_used = (uint16_t)(live_method_source_pool_used + length + 1U);
    while (*source != '\0') {
        live_method_source_pool[start++] = *source++;
    }
    live_method_source_pool[start] = '\0';
}

static void forget_live_string_literals(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count
) {
    uint16_t literal_index;

    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        if (live_string_literals[literal_index].text == 0 ||
            live_string_literals[literal_index].class_handle != class_handle ||
            live_string_literals[literal_index].selector_id != selector_id ||
            live_string_literals[literal_index].argument_count != argument_count) {
            continue;
        }
        live_string_literals[literal_index].class_handle = 0U;
        live_string_literals[literal_index].selector_id = 0U;
        live_string_literals[literal_index].argument_count = 0U;
        live_string_literals[literal_index].text = 0;
    }
}

static uint16_t remember_live_string_literal(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count,
    const char *text
) {
    uint16_t literal_index;
    uint16_t free_slot = 0U;

    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        if (live_string_literals[literal_index].text == 0) {
            if (free_slot == 0U) {
                free_slot = (uint16_t)(literal_index + 1U);
            }
            continue;
        }
        if (live_string_literals[literal_index].class_handle == class_handle &&
            live_string_literals[literal_index].selector_id == selector_id &&
            live_string_literals[literal_index].argument_count == argument_count &&
            source_names_equal(live_string_literals[literal_index].text, text)) {
            return (uint16_t)(literal_index + 1U);
        }
    }
    if (free_slot == 0U) {
        machine_panic("live string literal registry overflow");
    }
    literal_index = (uint16_t)(free_slot - 1U);
    live_string_literals[literal_index].class_handle = class_handle;
    live_string_literals[literal_index].selector_id = selector_id;
    live_string_literals[literal_index].argument_count = argument_count;
    live_string_literals[literal_index].text = runtime_string_allocate_copy(text);
    return free_slot;
}

static uint16_t current_live_string_literal_count(void) {
    uint16_t literal_index;
    uint16_t count = 0U;

    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        if (live_string_literals[literal_index].text != 0) {
            ++count;
        }
    }
    return count;
}

static uint32_t current_live_string_literal_byte_count(void) {
    uint16_t literal_index;
    uint32_t byte_count = 0U;

    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        if (live_string_literals[literal_index].text != 0) {
            byte_count += text_length(live_string_literals[literal_index].text) + 1U;
        }
    }
    return byte_count;
}

static uint32_t snapshot_string_storage_size(struct recorz_mvp_value value) {
    if (value.kind != RECORZ_MVP_VALUE_STRING || value.string == 0) {
        return 0U;
    }
    return text_length(value.string) + 1U;
}

static uint32_t current_snapshot_string_byte_count(void) {
    uint32_t string_byte_count = 0U;
    uint16_t handle;

    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);
        uint8_t field_index;

        for (field_index = 0U; field_index < object->field_count; ++field_index) {
            string_byte_count += snapshot_string_storage_size(object->fields[field_index]);
        }
    }
    return string_byte_count;
}

static uint32_t current_snapshot_live_method_source_byte_count(void) {
    return live_method_source_pool_used;
}

static uint32_t snapshot_total_size(
    uint32_t string_byte_count,
    uint32_t live_method_source_byte_count,
    uint32_t live_string_literal_byte_count,
    uint16_t live_string_literal_count
) {
    return SNAPSHOT_HEADER_SIZE +
           (heap_size * SNAPSHOT_OBJECT_SIZE) +
           (MAX_GLOBAL_ID * 2U) +
           (RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS * 2U) +
           (128U * 2U) +
           ((uint32_t)dynamic_class_count * SNAPSHOT_DYNAMIC_CLASS_RECORD_SIZE) +
           ((uint32_t)package_count * SNAPSHOT_PACKAGE_RECORD_SIZE) +
           ((uint32_t)named_object_count * SNAPSHOT_NAMED_OBJECT_RECORD_SIZE) +
           ((uint32_t)live_method_source_count * SNAPSHOT_LIVE_METHOD_SOURCE_RECORD_SIZE) +
           live_method_source_byte_count +
           ((uint32_t)live_string_literal_count * SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE) +
           live_string_literal_byte_count +
           ((uint32_t)mono_bitmap_count * MONO_BITMAP_MAX_HEIGHT * 4U) +
           string_byte_count;
}

static const char *kernel_memory_report_text(void) {
    char buffer[MEMORY_REPORT_BUFFER_SIZE];
    uint32_t offset = 0U;
    uint32_t snapshot_string_bytes = current_snapshot_string_byte_count();
    uint32_t live_method_source_bytes = current_snapshot_live_method_source_byte_count();
    uint32_t live_string_literal_bytes = current_live_string_literal_byte_count();
    uint16_t live_string_literal_count = current_live_string_literal_count();
    uint32_t snapshot_size = snapshot_total_size(
        snapshot_string_bytes,
        live_method_source_bytes,
        live_string_literal_bytes,
        live_string_literal_count
    );

    buffer[0] = '\0';
    append_memory_report_text(buffer, &offset, "MEMORY\n");
    append_memory_report_text(buffer, &offset, "PROFILE ");
    append_memory_report_text(buffer, &offset, RECORZ_MVP_PROFILE_NAME);
    append_memory_report_text(buffer, &offset, "\n");
    append_memory_report_line(buffer, &offset, "HEAP", heap_size, HEAP_LIMIT);
    append_memory_report_line(buffer, &offset, "DCLS", dynamic_class_count, DYNAMIC_CLASS_LIMIT);
    append_memory_report_line(buffer, &offset, "PKGS", package_count, PACKAGE_LIMIT);
    append_memory_report_line(buffer, &offset, "NOBJ", named_object_count, NAMED_OBJECT_LIMIT);
    append_memory_report_line(buffer, &offset, "MSRC", live_method_source_count, LIVE_METHOD_SOURCE_LIMIT);
    append_memory_report_line(
        buffer,
        &offset,
        "MSRP",
        live_method_source_bytes,
        LIVE_METHOD_SOURCE_POOL_LIMIT
    );
    append_memory_report_line(buffer, &offset, "LSTR", live_string_literal_count, LIVE_STRING_LITERAL_LIMIT);
    append_memory_report_line(
        buffer,
        &offset,
        "RSTR",
        runtime_string_pool_offset,
        RUNTIME_STRING_POOL_LIMIT
    );
    append_memory_report_line(
        buffer,
        &offset,
        "SSTR",
        snapshot_string_bytes,
        SNAPSHOT_STRING_LIMIT
    );
    append_memory_report_line(buffer, &offset, "SNAP", snapshot_size, SNAPSHOT_BUFFER_LIMIT);
    append_memory_report_line(buffer, &offset, "MONO", mono_bitmap_count, MONO_BITMAP_LIMIT);
    return runtime_string_allocate_copy(buffer);
}

static void snapshot_encode_value(
    uint8_t *slot,
    struct recorz_mvp_value value,
    uint8_t *string_section,
    uint32_t string_section_size,
    uint32_t *string_offset
) {
    uint32_t index;

    slot[0] = value.kind;
    slot[1] = 0U;
    write_u16_le(slot + 2U, 0U);
    write_u32_le(slot + 4U, (uint32_t)value.integer);
    if (value.kind != RECORZ_MVP_VALUE_STRING) {
        return;
    }
    if (value.string == 0) {
        machine_panic("snapshot cannot encode a null string");
    }
    {
        uint32_t length = text_length(value.string);

        if (length > 65535U) {
            machine_panic("snapshot string exceeds maximum encodable length");
        }
        if (*string_offset + length + 1U > string_section_size) {
            machine_panic("snapshot string section overflow");
        }
        write_u16_le(slot + 2U, (uint16_t)length);
        write_u32_le(slot + 4U, *string_offset);
        for (index = 0U; index < length; ++index) {
            string_section[*string_offset + index] = (uint8_t)value.string[index];
        }
        string_section[*string_offset + length] = 0U;
        *string_offset += length + 1U;
    }
}

static struct recorz_mvp_value snapshot_decode_value(
    const uint8_t *slot,
    uint32_t string_byte_count
) {
    uint8_t kind = slot[0];
    uint16_t aux = read_u16_le(slot + 2U);
    int32_t value = (int32_t)read_u32_le(slot + 4U);

    switch (kind) {
        case RECORZ_MVP_VALUE_NIL:
            return nil_value();
        case RECORZ_MVP_VALUE_OBJECT:
            if (value <= 0 || (uint32_t)value > heap_size) {
                machine_panic("snapshot object reference is out of range");
            }
            return object_value((uint16_t)value);
        case RECORZ_MVP_VALUE_SMALL_INTEGER:
            return small_integer_value(value);
        case RECORZ_MVP_VALUE_STRING: {
            uint32_t offset = (uint32_t)value;

            if (offset + aux >= string_byte_count) {
                machine_panic("snapshot string value is out of range");
            }
            if (snapshot_string_pool[offset + aux] != '\0') {
                machine_panic("snapshot string value is not terminated");
            }
            return string_value(snapshot_string_pool + offset);
        }
    }
    machine_panic("snapshot value kind is unknown");
    return nil_value();
}

static void emit_snapshot_hex_byte(uint8_t value) {
    static const char digits[] = "0123456789abcdef";

    machine_putc(digits[(value >> 4U) & 0x0FU]);
    machine_putc(digits[value & 0x0FU]);
}

static void emit_live_snapshot(void) {
    uint32_t string_byte_count;
    uint32_t live_method_source_byte_count;
    uint32_t live_string_literal_byte_count;
    uint16_t live_string_literal_count;
    uint16_t handle;
    uint16_t dynamic_index;
    uint16_t package_index;
    uint16_t named_index;
    uint16_t live_method_index;
    uint16_t literal_index;
    uint32_t row;
    uint32_t total_size;
    uint32_t offset;
    uint32_t string_offset = 0U;
    uint8_t *string_section;

    string_byte_count = current_snapshot_string_byte_count();
    live_method_source_byte_count = current_snapshot_live_method_source_byte_count();
    live_string_literal_byte_count = current_live_string_literal_byte_count();
    live_string_literal_count = current_live_string_literal_count();
    total_size = snapshot_total_size(
        string_byte_count,
        live_method_source_byte_count,
        live_string_literal_byte_count,
        live_string_literal_count
    );
    if (total_size > SNAPSHOT_BUFFER_LIMIT) {
        machine_panic("snapshot exceeds buffer capacity");
    }
    offset = 0U;
    snapshot_buffer[offset++] = SNAPSHOT_MAGIC_0;
    snapshot_buffer[offset++] = SNAPSHOT_MAGIC_1;
    snapshot_buffer[offset++] = SNAPSHOT_MAGIC_2;
    snapshot_buffer[offset++] = SNAPSHOT_MAGIC_3;
    write_u16_le(snapshot_buffer + offset, SNAPSHOT_VERSION);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, heap_size);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, dynamic_class_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, package_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, named_object_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, mono_bitmap_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, next_dynamic_method_entry_execution_id);
    offset += 2U;
    write_u32_le(snapshot_buffer + offset, string_byte_count);
    offset += 4U;
    write_u16_le(snapshot_buffer + offset, (uint16_t)cursor_x);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, (uint16_t)cursor_y);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, startup_hook_receiver_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, startup_hook_selector_id);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, live_method_source_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, (uint16_t)live_method_source_byte_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, live_string_literal_count);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, (uint16_t)live_string_literal_byte_count);
    offset += 2U;
    write_u32_le(snapshot_buffer + offset, total_size);
    offset += 4U;
    string_section = snapshot_buffer + (total_size - string_byte_count);

    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(handle);
        uint8_t field_index;

        snapshot_buffer[offset++] = object->kind;
        snapshot_buffer[offset++] = object->field_count;
        write_u16_le(snapshot_buffer + offset, object->class_handle);
        offset += 2U;
        for (field_index = 0U; field_index < OBJECT_FIELD_LIMIT; ++field_index) {
            snapshot_encode_value(
                snapshot_buffer + offset,
                object->fields[field_index],
                string_section,
                string_byte_count,
                &string_offset
            );
            offset += SNAPSHOT_VALUE_SIZE;
        }
    }
    for (handle = RECORZ_MVP_GLOBAL_TRANSCRIPT; handle <= MAX_GLOBAL_ID; ++handle) {
        write_u16_le(snapshot_buffer + offset, global_handles[handle]);
        offset += 2U;
    }
    write_u16_le(snapshot_buffer + offset, default_form_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, framebuffer_bitmap_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, transcript_behavior_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, transcript_layout_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, transcript_style_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, transcript_metrics_handle);
    offset += 2U;
    for (handle = 0U; handle < 128U; ++handle) {
        write_u16_le(snapshot_buffer + offset, glyph_bitmap_handles[handle]);
        offset += 2U;
    }
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        const struct recorz_mvp_dynamic_class_definition *definition = &dynamic_classes[dynamic_index];
        uint8_t ivar_index;
        uint32_t name_index;

        write_u16_le(snapshot_buffer + offset, definition->class_handle);
        offset += 2U;
        write_u16_le(snapshot_buffer + offset, definition->superclass_handle);
        offset += 2U;
        snapshot_buffer[offset++] = definition->instance_variable_count;
        snapshot_buffer[offset++] = 0U;
        snapshot_buffer[offset++] = 0U;
        snapshot_buffer[offset++] = 0U;
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)definition->class_name[name_index];
        }
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)definition->package_name[name_index];
        }
        for (name_index = 0U; name_index < CLASS_COMMENT_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)definition->class_comment[name_index];
        }
        for (ivar_index = 0U; ivar_index < DYNAMIC_CLASS_IVAR_LIMIT; ++ivar_index) {
            for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
                snapshot_buffer[offset++] =
                    (uint8_t)definition->instance_variable_names[ivar_index][name_index];
            }
        }
    }
    for (package_index = 0U; package_index < package_count; ++package_index) {
        uint32_t name_index;

        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)live_packages[package_index].package_name[name_index];
        }
        for (name_index = 0U; name_index < PACKAGE_COMMENT_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)live_packages[package_index].package_comment[name_index];
        }
    }
    for (named_index = 0U; named_index < named_object_count; ++named_index) {
        uint32_t name_index;

        write_u16_le(snapshot_buffer + offset, named_objects[named_index].object_handle);
        offset += 2U;
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)named_objects[named_index].name[name_index];
        }
    }
    for (live_method_index = 0U; live_method_index < live_method_source_count; ++live_method_index) {
        const struct recorz_mvp_live_method_source *source_record = &live_method_sources[live_method_index];
        uint32_t name_index;

        write_u16_le(snapshot_buffer + offset, source_record->class_handle);
        offset += 2U;
        snapshot_buffer[offset++] = source_record->selector_id;
        snapshot_buffer[offset++] = source_record->argument_count;
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            snapshot_buffer[offset++] = (uint8_t)source_record->protocol_name[name_index];
        }
        write_u16_le(snapshot_buffer + offset, source_record->source_offset);
        offset += 2U;
        write_u16_le(snapshot_buffer + offset, source_record->source_length);
        offset += 2U;
    }
    for (row = 0U; row < live_method_source_byte_count; ++row) {
        snapshot_buffer[offset++] = (uint8_t)live_method_source_pool[row];
    }
    for (literal_index = 0U; literal_index < LIVE_STRING_LITERAL_LIMIT; ++literal_index) {
        const struct recorz_mvp_live_string_literal *literal_record = &live_string_literals[literal_index];
        uint32_t text_index;
        uint32_t text_length_value;

        if (literal_record->text == 0) {
            continue;
        }
        write_u16_le(snapshot_buffer + offset, (uint16_t)(literal_index + 1U));
        offset += 2U;
        write_u16_le(snapshot_buffer + offset, literal_record->class_handle);
        offset += 2U;
        snapshot_buffer[offset++] = literal_record->selector_id;
        snapshot_buffer[offset++] = literal_record->argument_count;
        text_length_value = text_length(literal_record->text);
        write_u16_le(snapshot_buffer + offset, (uint16_t)text_length_value);
        offset += 2U;
        for (text_index = 0U; text_index <= text_length_value; ++text_index) {
            snapshot_buffer[offset++] = (uint8_t)literal_record->text[text_index];
        }
    }
    for (handle = 0U; handle < mono_bitmap_count; ++handle) {
        for (row = 0U; row < MONO_BITMAP_MAX_HEIGHT; ++row) {
            write_u32_le(snapshot_buffer + offset, mono_bitmap_pool[handle][row]);
            offset += 4U;
        }
    }
    if (string_offset != string_byte_count || offset != total_size - string_byte_count) {
        machine_panic("snapshot encoding size mismatch");
    }

    machine_puts("recorz-snapshot-begin ");
    panic_put_u32(total_size);
    machine_putc('\n');
    for (offset = 0U; offset < total_size; offset += 32U) {
        uint32_t chunk_size = total_size - offset;
        uint32_t chunk_index;

        if (chunk_size > 32U) {
            chunk_size = 32U;
        }
        machine_puts("recorz-snapshot-data ");
        for (chunk_index = 0U; chunk_index < chunk_size; ++chunk_index) {
            emit_snapshot_hex_byte(snapshot_buffer[offset + chunk_index]);
        }
        machine_putc('\n');
    }
    machine_puts("recorz-snapshot-end\n");
}

static void load_snapshot_state(const uint8_t *blob, uint32_t size) {
    uint16_t object_count;
    uint16_t dynamic_count;
    uint16_t saved_package_count;
    uint16_t saved_named_object_count;
    uint16_t saved_live_method_source_count;
    uint16_t saved_live_method_source_byte_count;
    uint16_t saved_live_string_literal_count;
    uint16_t saved_live_string_literal_byte_count;
    uint16_t saved_mono_bitmap_count;
    uint16_t saved_next_dynamic_method_entry_execution_id;
    uint32_t string_byte_count;
    uint16_t saved_cursor_x;
    uint16_t saved_cursor_y;
    uint16_t saved_startup_hook_receiver_handle;
    uint16_t saved_startup_hook_selector_id;
    uint32_t expected_size;
    uint32_t string_section_offset;
    uint32_t offset;
    uint16_t handle;
    uint16_t dynamic_index;
    uint16_t named_index;
    uint32_t row;

    if (size < SNAPSHOT_HEADER_SIZE) {
        machine_panic("snapshot is too small");
    }
    if (blob[0] != SNAPSHOT_MAGIC_0 || blob[1] != SNAPSHOT_MAGIC_1 ||
        blob[2] != SNAPSHOT_MAGIC_2 || blob[3] != SNAPSHOT_MAGIC_3) {
        machine_panic("snapshot magic mismatch");
    }
    if (read_u16_le(blob + 4U) != SNAPSHOT_VERSION) {
        machine_panic("snapshot version mismatch");
    }
    object_count = read_u16_le(blob + 6U);
    dynamic_count = read_u16_le(blob + 8U);
    saved_package_count = read_u16_le(blob + 10U);
    saved_named_object_count = read_u16_le(blob + 12U);
    saved_mono_bitmap_count = read_u16_le(blob + 14U);
    saved_next_dynamic_method_entry_execution_id = read_u16_le(blob + 16U);
    string_byte_count = read_u32_le(blob + 18U);
    saved_cursor_x = read_u16_le(blob + 22U);
    saved_cursor_y = read_u16_le(blob + 24U);
    saved_startup_hook_receiver_handle = read_u16_le(blob + 26U);
    saved_startup_hook_selector_id = read_u16_le(blob + 28U);
    saved_live_method_source_count = read_u16_le(blob + 30U);
    saved_live_method_source_byte_count = read_u16_le(blob + 32U);
    saved_live_string_literal_count = read_u16_le(blob + 34U);
    saved_live_string_literal_byte_count = read_u16_le(blob + 36U);
    expected_size = read_u32_le(blob + 38U);
    if (object_count == 0U || object_count > HEAP_LIMIT) {
        machine_panic("snapshot object count exceeds heap capacity");
    }
    if (dynamic_count > DYNAMIC_CLASS_LIMIT) {
        machine_panic("snapshot dynamic class count exceeds capacity");
    }
    if (saved_package_count > PACKAGE_LIMIT) {
        machine_panic("snapshot package count exceeds capacity");
    }
    if (saved_named_object_count > NAMED_OBJECT_LIMIT) {
        machine_panic("snapshot named object count exceeds capacity");
    }
    if (saved_live_method_source_count > LIVE_METHOD_SOURCE_LIMIT) {
        machine_panic("snapshot live method source count exceeds capacity");
    }
    if (saved_live_method_source_byte_count > LIVE_METHOD_SOURCE_POOL_LIMIT) {
        machine_panic("snapshot live method source pool exceeds capacity");
    }
    if (saved_live_string_literal_count > LIVE_STRING_LITERAL_LIMIT) {
        machine_panic("snapshot live string literal count exceeds capacity");
    }
    if (saved_mono_bitmap_count > MONO_BITMAP_LIMIT) {
        machine_panic("snapshot mono bitmap count exceeds capacity");
    }
    if (string_byte_count > SNAPSHOT_STRING_LIMIT) {
        machine_panic("snapshot string section exceeds capacity");
    }
    if (expected_size != size) {
        machine_panic("snapshot size mismatch");
    }
    string_section_offset = SNAPSHOT_HEADER_SIZE +
                            ((uint32_t)object_count * SNAPSHOT_OBJECT_SIZE) +
                            (MAX_GLOBAL_ID * 2U) +
                            (RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS * 2U) +
                            (128U * 2U) +
                            ((uint32_t)dynamic_count * SNAPSHOT_DYNAMIC_CLASS_RECORD_SIZE) +
                            ((uint32_t)saved_package_count * SNAPSHOT_PACKAGE_RECORD_SIZE) +
                            ((uint32_t)saved_named_object_count * SNAPSHOT_NAMED_OBJECT_RECORD_SIZE) +
                            ((uint32_t)saved_live_method_source_count * SNAPSHOT_LIVE_METHOD_SOURCE_RECORD_SIZE) +
                            saved_live_method_source_byte_count +
                            ((uint32_t)saved_live_string_literal_count * SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE) +
                            saved_live_string_literal_byte_count +
                            ((uint32_t)saved_mono_bitmap_count * MONO_BITMAP_MAX_HEIGHT * 4U);
    if (string_section_offset + string_byte_count != size) {
        machine_panic("snapshot string section size mismatch");
    }

    reset_runtime_state();
    for (row = 0U; row < string_byte_count; ++row) {
        snapshot_string_pool[row] = (char)blob[string_section_offset + row];
    }
    if (string_byte_count < SNAPSHOT_STRING_LIMIT) {
        snapshot_string_pool[string_byte_count] = '\0';
    }
    for (handle = 1U; handle <= object_count; ++handle) {
        uint8_t kind = blob[SNAPSHOT_HEADER_SIZE + ((uint32_t)(handle - 1U) * SNAPSHOT_OBJECT_SIZE)];
        (void)heap_allocate(kind);
    }
    offset = SNAPSHOT_HEADER_SIZE;
    for (handle = 1U; handle <= object_count; ++handle) {
        struct recorz_mvp_heap_object *object = heap_object(handle);
        uint8_t field_index;

        object->kind = blob[offset++];
        object->field_count = blob[offset++];
        object->class_handle = read_u16_le(blob + offset);
        offset += 2U;
        if (object->field_count > OBJECT_FIELD_LIMIT) {
            machine_panic("snapshot field count exceeds object field capacity");
        }
        if (object->class_handle == 0U || object->class_handle > object_count) {
            machine_panic("snapshot class handle is out of range");
        }
        for (field_index = 0U; field_index < OBJECT_FIELD_LIMIT; ++field_index) {
            object->fields[field_index] = snapshot_decode_value(blob + offset, string_byte_count);
            offset += SNAPSHOT_VALUE_SIZE;
        }
    }
    for (handle = RECORZ_MVP_GLOBAL_TRANSCRIPT; handle <= MAX_GLOBAL_ID; ++handle) {
        global_handles[handle] = read_u16_le(blob + offset);
        if (global_handles[handle] == 0U || global_handles[handle] > object_count) {
            machine_panic("snapshot global handle is out of range");
        }
        offset += 2U;
    }
    default_form_handle = read_u16_le(blob + offset);
    offset += 2U;
    framebuffer_bitmap_handle = read_u16_le(blob + offset);
    offset += 2U;
    transcript_behavior_handle = read_u16_le(blob + offset);
    offset += 2U;
    transcript_layout_handle = read_u16_le(blob + offset);
    offset += 2U;
    transcript_style_handle = read_u16_le(blob + offset);
    offset += 2U;
    transcript_metrics_handle = read_u16_le(blob + offset);
    offset += 2U;
    if (default_form_handle == 0U || default_form_handle > object_count ||
        framebuffer_bitmap_handle == 0U || framebuffer_bitmap_handle > object_count ||
        transcript_behavior_handle == 0U || transcript_behavior_handle > object_count ||
        transcript_layout_handle == 0U || transcript_layout_handle > object_count ||
        transcript_style_handle == 0U || transcript_style_handle > object_count ||
        transcript_metrics_handle == 0U || transcript_metrics_handle > object_count) {
        machine_panic("snapshot root handle is out of range");
    }
    for (handle = 0U; handle < 128U; ++handle) {
        glyph_bitmap_handles[handle] = read_u16_le(blob + offset);
        if (glyph_bitmap_handles[handle] != 0U && glyph_bitmap_handles[handle] > object_count) {
            machine_panic("snapshot glyph handle is out of range");
        }
        offset += 2U;
    }
    dynamic_class_count = dynamic_count;
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        struct recorz_mvp_dynamic_class_definition *definition = &dynamic_classes[dynamic_index];
        uint8_t ivar_index;
        uint32_t name_index;

        definition->class_handle = read_u16_le(blob + offset);
        offset += 2U;
        definition->superclass_handle = read_u16_le(blob + offset);
        offset += 2U;
        definition->instance_variable_count = blob[offset++];
        offset += 3U;
        if (definition->class_handle == 0U || definition->class_handle > object_count ||
            definition->superclass_handle == 0U || definition->superclass_handle > object_count) {
            machine_panic("snapshot dynamic class handle is out of range");
        }
        if (definition->instance_variable_count > DYNAMIC_CLASS_IVAR_LIMIT) {
            machine_panic("snapshot dynamic class instance variable count is out of range");
        }
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            definition->class_name[name_index] = (char)blob[offset++];
        }
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            definition->package_name[name_index] = (char)blob[offset++];
        }
        for (name_index = 0U; name_index < CLASS_COMMENT_LIMIT; ++name_index) {
            definition->class_comment[name_index] = (char)blob[offset++];
        }
        for (ivar_index = 0U; ivar_index < DYNAMIC_CLASS_IVAR_LIMIT; ++ivar_index) {
            for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
                definition->instance_variable_names[ivar_index][name_index] = (char)blob[offset++];
            }
        }
    }
    package_count = saved_package_count;
    for (dynamic_index = 0U; dynamic_index < package_count; ++dynamic_index) {
        uint32_t name_index;

        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            live_packages[dynamic_index].package_name[name_index] = (char)blob[offset++];
        }
        for (name_index = 0U; name_index < PACKAGE_COMMENT_LIMIT; ++name_index) {
            live_packages[dynamic_index].package_comment[name_index] = (char)blob[offset++];
        }
    }
    named_object_count = saved_named_object_count;
    for (named_index = 0U; named_index < named_object_count; ++named_index) {
        uint32_t name_index;

        named_objects[named_index].object_handle = read_u16_le(blob + offset);
        offset += 2U;
        if (named_objects[named_index].object_handle == 0U ||
            named_objects[named_index].object_handle > object_count) {
            machine_panic("snapshot named object handle is out of range");
        }
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            named_objects[named_index].name[name_index] = (char)blob[offset++];
        }
    }
    live_method_source_count = saved_live_method_source_count;
    for (named_index = 0U; named_index < live_method_source_count; ++named_index) {
        uint32_t name_index;

        live_method_sources[named_index].class_handle = read_u16_le(blob + offset);
        offset += 2U;
        live_method_sources[named_index].selector_id = blob[offset++];
        live_method_sources[named_index].argument_count = blob[offset++];
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            live_method_sources[named_index].protocol_name[name_index] = (char)blob[offset++];
        }
        live_method_sources[named_index].source_offset = read_u16_le(blob + offset);
        offset += 2U;
        live_method_sources[named_index].source_length = read_u16_le(blob + offset);
        offset += 2U;
        if (live_method_sources[named_index].class_handle == 0U ||
            live_method_sources[named_index].class_handle > object_count) {
            machine_panic("snapshot live method source class handle is out of range");
        }
        if (live_method_sources[named_index].source_offset + live_method_sources[named_index].source_length >=
            saved_live_method_source_byte_count) {
            machine_panic("snapshot live method source is out of range");
        }
    }
    live_method_source_pool_used = saved_live_method_source_byte_count;
    for (row = 0U; row < saved_live_method_source_byte_count; ++row) {
        live_method_source_pool[row] = (char)blob[offset++];
    }
    if (saved_live_method_source_byte_count < LIVE_METHOD_SOURCE_POOL_LIMIT) {
        live_method_source_pool[saved_live_method_source_byte_count] = '\0';
    }
    for (named_index = 0U; named_index < live_method_source_count; ++named_index) {
        const struct recorz_mvp_live_method_source *source_record = &live_method_sources[named_index];

        if (live_method_source_pool[source_record->source_offset + source_record->source_length] != '\0') {
            machine_panic("snapshot live method source is not terminated");
        }
    }
    for (named_index = 0U; named_index < LIVE_STRING_LITERAL_LIMIT; ++named_index) {
        live_string_literals[named_index].class_handle = 0U;
        live_string_literals[named_index].selector_id = 0U;
        live_string_literals[named_index].argument_count = 0U;
        live_string_literals[named_index].text = 0;
    }
    for (named_index = 0U; named_index < saved_live_string_literal_count; ++named_index) {
        uint16_t slot_id = read_u16_le(blob + offset);
        uint16_t class_handle = read_u16_le(blob + offset + 2U);
        uint8_t selector_id = blob[offset + 4U];
        uint8_t argument_count = blob[offset + 5U];
        uint16_t text_length_value = read_u16_le(blob + offset + 6U);
        char literal_text[METHOD_SOURCE_CHUNK_LIMIT];
        uint32_t text_index;

        offset += SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE;
        if (slot_id == 0U || slot_id > LIVE_STRING_LITERAL_LIMIT) {
            machine_panic("snapshot live string literal slot is out of range");
        }
        if (class_handle == 0U || class_handle > object_count) {
            machine_panic("snapshot live string literal class handle is out of range");
        }
        if (text_length_value + 1U > METHOD_SOURCE_CHUNK_LIMIT) {
            machine_panic("snapshot live string literal exceeds chunk capacity");
        }
        for (text_index = 0U; text_index < text_length_value; ++text_index) {
            literal_text[text_index] = (char)blob[offset++];
        }
        literal_text[text_length_value] = '\0';
        if (blob[offset++] != 0U) {
            machine_panic("snapshot live string literal is not terminated");
        }
        live_string_literals[slot_id - 1U].class_handle = class_handle;
        live_string_literals[slot_id - 1U].selector_id = selector_id;
        live_string_literals[slot_id - 1U].argument_count = argument_count;
        live_string_literals[slot_id - 1U].text = runtime_string_allocate_copy(literal_text);
    }
    mono_bitmap_count = saved_mono_bitmap_count;
    for (handle = 0U; handle < mono_bitmap_count; ++handle) {
        for (row = 0U; row < MONO_BITMAP_MAX_HEIGHT; ++row) {
            mono_bitmap_pool[handle][row] = read_u32_le(blob + offset);
            offset += 4U;
        }
    }
    if (offset != string_section_offset) {
        machine_panic("snapshot fixed section size mismatch");
    }
    next_dynamic_method_entry_execution_id = saved_next_dynamic_method_entry_execution_id;
    cursor_x = saved_cursor_x;
    cursor_y = saved_cursor_y;
    startup_hook_receiver_handle = saved_startup_hook_receiver_handle;
    startup_hook_selector_id = saved_startup_hook_selector_id;
    validate_heap_class_graph(heap_size, 0U);
    initialize_runtime_caches();
    if ((startup_hook_receiver_handle == 0U) != (startup_hook_selector_id == 0U)) {
        machine_panic("snapshot startup hook is incomplete");
    }
    if (startup_hook_receiver_handle != 0U) {
        if (startup_hook_receiver_handle > object_count) {
            machine_panic("snapshot startup hook receiver is out of range");
        }
        if (startup_hook_selector_id == 0U || startup_hook_selector_id > MAX_SELECTOR_ID) {
            machine_panic("snapshot startup hook selector is out of range");
        }
    }
    {
        struct recorz_mvp_value fallback_value =
            heap_get_field(transcript_behavior_object(), TEXT_BEHAVIOR_FIELD_FALLBACK_BITMAP);
        const struct recorz_mvp_heap_object *fallback_bitmap = heap_object_for_value(fallback_value);

        if (fallback_bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
            machine_panic("snapshot fallback bitmap is not a bitmap");
        }
        glyph_fallback_handle = (uint16_t)fallback_value.integer;
    }
    if (heap_get_field(heap_object(default_form_handle), FORM_FIELD_BITS).kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)heap_get_field(heap_object(default_form_handle), FORM_FIELD_BITS).integer != framebuffer_bitmap_handle) {
        machine_panic("snapshot default form does not point at the framebuffer bitmap");
    }
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
    const char (*lexical_names)[METHOD_SOURCE_NAME_LIMIT];
    const struct recorz_mvp_heap_object *block_defining_class;
    int16_t home_context_index;
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
    uint8_t field_count;
    uint8_t field_index;

    (void)receiver;
    (void)arguments;
    (void)text;
    if (object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("Class new expects a class receiver");
    }
    instance_handle = heap_allocate(RECORZ_MVP_OBJECT_OBJECT);
    heap_set_class(instance_handle, heap_handle_for_object(object));
    field_count = live_instance_field_count_for_class(object);
    for (field_index = 0U; field_index < field_count; ++field_index) {
        heap_set_field(instance_handle, field_index, nil_value());
    }
    push(object_value(instance_handle));
}

static void execute_entry_kernel_installer_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller classNamed: expects a source string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("KernelInstaller classNamed: could not resolve class");
    }
    push(object_value(heap_handle_for_object(class_object)));
}

static void execute_entry_kernel_installer_file_out_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller fileOutClassNamed: expects a class name string");
    }
    push(string_value(file_out_class_source_by_name(arguments[0].string)));
}

static uint8_t compile_source_operand_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *name,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    uint16_t argument_index;
    uint16_t temporary_index;
    uint8_t field_index;
    uint8_t global_id;

    if (*instruction_count >= COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("KernelInstaller source method exceeds compiled method capacity");
    }
    for (argument_index = 0U; argument_index < argument_count; ++argument_index) {
        if (argument_names[argument_index][0] != '\0' &&
            source_names_equal(name, argument_names[argument_index])) {
            instruction_words[(*instruction_count)++] = encode_compiled_method_word(
                COMPILED_METHOD_OP_PUSH_ARGUMENT,
                (uint8_t)argument_index,
                0U
            );
            return 1U;
        }
    }
    for (temporary_index = 0U; temporary_index < temporary_count; ++temporary_index) {
        if (temporary_names[temporary_index][0] != '\0' &&
            source_names_equal(name, temporary_names[temporary_index])) {
            instruction_words[(*instruction_count)++] = encode_compiled_method_word(
                COMPILED_METHOD_OP_PUSH_LEXICAL,
                0U,
                temporary_index
            );
            return 1U;
        }
    }
    if (class_field_index_for_name(class_object, name, &field_index)) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_FIELD, field_index, 0U);
        return 1U;
    }
    global_id = source_global_id_for_name(name);
    if (global_id != 0U) {
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_PUSH_GLOBAL, global_id, 0U);
        return 1U;
    }
    return 0U;
}

static void compile_source_append_instruction(
    uint32_t instruction_words[],
    uint8_t *instruction_count,
    uint8_t opcode,
    uint8_t operand_a,
    uint16_t operand_b
) {
    if (*instruction_count >= COMPILED_METHOD_MAX_INSTRUCTIONS) {
        machine_panic("KernelInstaller source method exceeds compiled method capacity");
    }
    instruction_words[(*instruction_count)++] = encode_compiled_method_word(opcode, operand_a, operand_b);
}

static void compile_source_patch_instruction(
    uint32_t instruction_words[],
    uint8_t instruction_count,
    uint8_t instruction_index,
    uint8_t opcode,
    uint8_t operand_a,
    uint16_t operand_b
) {
    if (instruction_index >= instruction_count) {
        machine_panic("KernelInstaller source method patch is out of range");
    }
    instruction_words[instruction_index] = encode_compiled_method_word(opcode, operand_a, operand_b);
}

static const char *compile_source_expression_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
);
static uint8_t compile_source_statement_line(
    const struct recorz_mvp_heap_object *class_object,
    const char *line,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count,
    uint8_t *produces_value
);
static void compile_source_return_line(
    const struct recorz_mvp_heap_object *class_object,
    const char *line,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
);
static void compile_source_inline_block(
    const struct recorz_mvp_heap_object *class_object,
    const char *source,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
);
static const char *compile_source_conditional_expression_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
);
static const char *compile_source_primary_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
);

static const char *compile_source_binary_expression_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    char selector_name_buffer[2];
    const char *parsed_cursor;
    uint8_t selector_id;

    parsed_cursor = compile_source_primary_push(
        class_object,
        cursor,
        argument_names,
        argument_count,
        temporary_names,
        temporary_count,
        instruction_words,
        instruction_count
    );
    if (parsed_cursor == 0) {
        return 0;
    }
    cursor = source_skip_horizontal_space(parsed_cursor);
    while (source_char_is_binary_selector(*cursor)) {
        selector_name_buffer[0] = *cursor++;
        selector_name_buffer[1] = '\0';
        cursor = compile_source_primary_push(
            class_object,
            cursor,
            argument_names,
            argument_count,
            temporary_names,
            temporary_count,
            instruction_words,
            instruction_count
        );
        if (cursor == 0) {
            machine_panic("KernelInstaller source method binary send is missing an argument");
        }
        selector_id = source_selector_id_for_name(selector_name_buffer);
        if (selector_id == 0U) {
            machine_panic("KernelInstaller source method uses an unknown binary selector");
        }
        compile_source_append_instruction(
            instruction_words,
            instruction_count,
            COMPILED_METHOD_OP_SEND,
            selector_id,
            1U
        );
        cursor = source_skip_horizontal_space(cursor);
    }
    return cursor;
}

static const char *compile_source_primary_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    int32_t small_integer;
    char quoted_text[METHOD_SOURCE_CHUNK_LIMIT];
    char token[METHOD_SOURCE_NAME_LIMIT];
    const char *token_cursor;
    uint8_t selector_id;

    cursor = source_skip_horizontal_space(cursor);
    if (*cursor == '(') {
        ++cursor;
        token_cursor = compile_source_expression_push(
            class_object,
            cursor,
            argument_names,
            argument_count,
            temporary_names,
            temporary_count,
            instruction_words,
            instruction_count
        );
        if (token_cursor == 0) {
            machine_panic("KernelInstaller source method parenthesized expression is invalid");
        }
        token_cursor = source_skip_horizontal_space(token_cursor);
        if (*token_cursor != ')') {
            machine_panic("KernelInstaller source method parenthesized expression is missing ')'");
        }
        cursor = token_cursor + 1;
    } else if (*cursor == '\'') {
        uint16_t literal_slot;

        token_cursor = source_copy_single_quoted_text(cursor, quoted_text, sizeof(quoted_text));
        if (token_cursor == 0) {
            machine_panic("KernelInstaller source method string literal is invalid");
        }
        if (compiling_method_class_handle == 0U) {
            machine_panic("KernelInstaller source method string literal has no active method context");
        }
        literal_slot = remember_live_string_literal(
            compiling_method_class_handle,
            compiling_method_selector_id,
            compiling_method_argument_count,
            quoted_text
        );
        compile_source_append_instruction(
            instruction_words,
            instruction_count,
            COMPILED_METHOD_OP_PUSH_STRING_LITERAL,
            0U,
            literal_slot
        );
        cursor = token_cursor;
    } else {
        token_cursor = source_parse_small_integer(cursor, &small_integer);
        if (token_cursor != 0) {
            if (small_integer < -32768 || small_integer > 32767) {
                machine_panic("KernelInstaller source method small integer literal is out of range");
            }
            compile_source_append_instruction(
                instruction_words,
                instruction_count,
                COMPILED_METHOD_OP_PUSH_SMALL_INTEGER,
                0U,
                (uint16_t)(int16_t)small_integer
            );
            cursor = token_cursor;
        } else {
            token_cursor = source_parse_identifier(cursor, token, sizeof(token));
            if (token_cursor == 0) {
                return 0;
            }
            if (source_names_equal(token, "nil")) {
                compile_source_append_instruction(
                    instruction_words,
                    instruction_count,
                    COMPILED_METHOD_OP_PUSH_NIL,
                    0U,
                    0U
                );
            } else if (source_names_equal(token, "self")) {
                compile_source_append_instruction(
                    instruction_words,
                    instruction_count,
                    COMPILED_METHOD_OP_PUSH_SELF,
                    0U,
                    0U
                );
            } else if (!compile_source_operand_push(
                    class_object,
                    token,
                    argument_names,
                    argument_count,
                    temporary_names,
                    temporary_count,
                    instruction_words,
                    instruction_count)) {
                return 0;
            }
            cursor = token_cursor;
        }
    }
    cursor = source_skip_horizontal_space(cursor);
    while (source_char_is_identifier_start(*cursor)) {
        const char *selector_cursor = source_parse_identifier(cursor, token, sizeof(token));
        const char *after_selector;

        if (selector_cursor == 0) {
            break;
        }
        after_selector = source_skip_horizontal_space(selector_cursor);
        if (*after_selector == ':') {
            break;
        }
        selector_id = source_selector_id_for_name(token);
        if (selector_id == 0U) {
            machine_panic("KernelInstaller source method uses an unknown unary selector");
        }
        compile_source_append_instruction(
            instruction_words,
            instruction_count,
            COMPILED_METHOD_OP_SEND,
            selector_id,
            0U
        );
        cursor = after_selector;
    }
    return cursor;
}

static const char *compile_source_expression_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    char selector_name_buffer[METHOD_SOURCE_NAME_LIMIT];
    char selector_part[METHOD_SOURCE_NAME_LIMIT];
    const char *part_cursor;
    const char *parsed_cursor;
    uint16_t keyword_argument_count = 0U;
    uint32_t selector_length = 0U;
    uint8_t selector_id;

    parsed_cursor = compile_source_binary_expression_push(
        class_object,
        cursor,
        argument_names,
        argument_count,
        temporary_names,
        temporary_count,
        instruction_words,
        instruction_count
    );
    if (parsed_cursor == 0) {
        return 0;
    }
    cursor = source_skip_horizontal_space(parsed_cursor);
    part_cursor = source_parse_identifier(cursor, selector_part, sizeof(selector_part));
    if (part_cursor == 0) {
        return cursor;
    }
    part_cursor = source_skip_horizontal_space(part_cursor);
    if (*part_cursor != ':') {
        return cursor;
    }
    if (source_names_equal(selector_part, "ifTrue") || source_names_equal(selector_part, "ifFalse")) {
        return compile_source_conditional_expression_push(
            class_object,
            cursor,
            argument_names,
            argument_count,
            temporary_names,
            temporary_count,
            instruction_words,
            instruction_count
        );
    }
    while (selector_part[selector_length] != '\0') {
        selector_name_buffer[selector_length] = selector_part[selector_length];
        ++selector_length;
    }
    selector_name_buffer[selector_length] = '\0';
    do {
        if (selector_length + 1U >= sizeof(selector_name_buffer)) {
            machine_panic("KernelInstaller keyword selector exceeds buffer capacity");
        }
        selector_name_buffer[selector_length++] = ':';
        selector_name_buffer[selector_length] = '\0';
        cursor = compile_source_binary_expression_push(
            class_object,
            part_cursor + 1,
            argument_names,
            argument_count,
            temporary_names,
            temporary_count,
            instruction_words,
            instruction_count
        );
        if (cursor == 0) {
            machine_panic("KernelInstaller source method keyword send is missing an argument");
        }
        ++keyword_argument_count;
        cursor = source_skip_horizontal_space(cursor);
        part_cursor = source_parse_identifier(cursor, selector_part, sizeof(selector_part));
        if (part_cursor == 0) {
            break;
        }
        part_cursor = source_skip_horizontal_space(part_cursor);
        if (*part_cursor != ':') {
            break;
        }
        {
            uint32_t part_index = 0U;

            while (selector_part[part_index] != '\0') {
                if (selector_length + 1U >= sizeof(selector_name_buffer)) {
                    machine_panic("KernelInstaller keyword selector exceeds buffer capacity");
                }
                selector_name_buffer[selector_length++] = selector_part[part_index++];
            }
            selector_name_buffer[selector_length] = '\0';
        }
    } while (*part_cursor == ':');
    selector_id = source_selector_id_for_name(selector_name_buffer);
    if (selector_id == 0U) {
        machine_panic("KernelInstaller source method uses an unknown keyword selector");
    }
    compile_source_append_instruction(
        instruction_words,
        instruction_count,
        COMPILED_METHOD_OP_SEND,
        selector_id,
        keyword_argument_count
    );
    return cursor;
}

static void compile_source_inline_block(
    const struct recorz_mvp_heap_object *class_object,
    const char *source,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    const char *cursor = source;
    char statement[METHOD_SOURCE_CHUNK_LIMIT];
    uint8_t statement_count = 0U;
    uint8_t pending_statement_value = 0U;

    if (*source_skip_horizontal_space(source) == ':') {
        machine_panic("KernelInstaller conditional blocks do not support block arguments yet");
    }
    while (source_copy_next_statement(&cursor, statement, sizeof(statement)) != 0U) {
        uint8_t produces_value = 0U;
        const char *trimmed = source_skip_horizontal_space(statement);

        if (pending_statement_value) {
            compile_source_append_instruction(
                instruction_words,
                instruction_count,
                COMPILED_METHOD_OP_POP,
                0U,
                0U
            );
            pending_statement_value = 0U;
        }
        if (trimmed[0] == '^') {
            compile_source_return_line(
                class_object,
                statement,
                argument_names,
                argument_count,
                temporary_names,
                temporary_count,
                instruction_words,
                instruction_count
            );
            if (source_copy_next_statement(&cursor, statement, sizeof(statement)) != 0U) {
                machine_panic("KernelInstaller conditional block has unexpected trailing statements after a return");
            }
            return;
        }
        if (!compile_source_statement_line(
                class_object,
                statement,
                argument_names,
                argument_count,
                temporary_names,
                temporary_count,
                instruction_words,
                instruction_count,
                &produces_value
            )) {
            machine_panic("KernelInstaller conditional block statement is unsupported");
        }
        pending_statement_value = produces_value;
        ++statement_count;
    }
    if (statement_count == 0U || !pending_statement_value) {
        compile_source_append_instruction(
            instruction_words,
            instruction_count,
            COMPILED_METHOD_OP_PUSH_NIL,
            0U,
            0U
        );
    }
}

static const char *compile_source_conditional_expression_push(
    const struct recorz_mvp_heap_object *class_object,
    const char *cursor,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    char first_selector[METHOD_SOURCE_NAME_LIMIT];
    char second_selector[METHOD_SOURCE_NAME_LIMIT];
    char first_block[METHOD_SOURCE_CHUNK_LIMIT];
    char second_block[METHOD_SOURCE_CHUNK_LIMIT];
    const char *part_cursor;
    const char *after_selector;
    const char *after_first_block;
    const char *after_second_block;
    uint8_t first_is_true;
    uint8_t branch_instruction_index;
    uint8_t jump_instruction_index;
    uint8_t has_second_clause = 0U;

    part_cursor = source_parse_identifier(cursor, first_selector, sizeof(first_selector));
    if (part_cursor == 0) {
        machine_panic("KernelInstaller conditional expression is missing a selector");
    }
    after_selector = source_skip_horizontal_space(part_cursor);
    if (*after_selector != ':') {
        machine_panic("KernelInstaller conditional expression is missing ':'");
    }
    after_first_block = source_copy_bracket_body(after_selector + 1, first_block, sizeof(first_block));
    if (after_first_block == 0) {
        machine_panic("KernelInstaller conditional expression is missing a block argument");
    }
    cursor = source_skip_horizontal_space(after_first_block);
    part_cursor = source_parse_identifier(cursor, second_selector, sizeof(second_selector));
    if (part_cursor != 0) {
        after_selector = source_skip_horizontal_space(part_cursor);
        if (*after_selector == ':' &&
            ((source_names_equal(first_selector, "ifTrue") && source_names_equal(second_selector, "ifFalse")) ||
             (source_names_equal(first_selector, "ifFalse") && source_names_equal(second_selector, "ifTrue")))) {
            after_second_block = source_copy_bracket_body(after_selector + 1, second_block, sizeof(second_block));
            if (after_second_block == 0) {
                machine_panic("KernelInstaller conditional expression is missing a matching block argument");
            }
            cursor = after_second_block;
            has_second_clause = 1U;
        }
    }

    first_is_true = (uint8_t)source_names_equal(first_selector, "ifTrue");
    branch_instruction_index = *instruction_count;
    compile_source_append_instruction(
        instruction_words,
        instruction_count,
        first_is_true ? COMPILED_METHOD_OP_JUMP_IF_FALSE : COMPILED_METHOD_OP_JUMP_IF_TRUE,
        0U,
        0U
    );
    compile_source_inline_block(
        class_object,
        first_block,
        argument_names,
        argument_count,
        temporary_names,
        temporary_count,
        instruction_words,
        instruction_count
    );
    jump_instruction_index = *instruction_count;
    compile_source_append_instruction(
        instruction_words,
        instruction_count,
        COMPILED_METHOD_OP_JUMP,
        0U,
        0U
    );
    compile_source_patch_instruction(
        instruction_words,
        *instruction_count,
        branch_instruction_index,
        first_is_true ? COMPILED_METHOD_OP_JUMP_IF_FALSE : COMPILED_METHOD_OP_JUMP_IF_TRUE,
        0U,
        *instruction_count
    );
    if (has_second_clause) {
        compile_source_inline_block(
            class_object,
            second_block,
            argument_names,
            argument_count,
            temporary_names,
            temporary_count,
            instruction_words,
            instruction_count
        );
    } else {
        compile_source_append_instruction(
            instruction_words,
            instruction_count,
            COMPILED_METHOD_OP_PUSH_NIL,
            0U,
            0U
        );
    }
    compile_source_patch_instruction(
        instruction_words,
        *instruction_count,
        jump_instruction_index,
        COMPILED_METHOD_OP_JUMP,
        0U,
        *instruction_count
    );
    return cursor;
}

static uint8_t compile_source_statement_line(
    const struct recorz_mvp_heap_object *class_object,
    const char *line,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count,
    uint8_t *produces_value
) {
    char receiver_name[METHOD_SOURCE_NAME_LIMIT];
    const char *cursor = line;

    *produces_value = 0U;
    cursor = source_skip_horizontal_space(cursor);
    if (source_names_equal(cursor, "")) {
        return 1U;
    }
    cursor = source_parse_identifier(cursor, receiver_name, sizeof(receiver_name));
    if (cursor != 0) {
        cursor = source_skip_horizontal_space(cursor);
    }
    if (cursor != 0 && cursor[0] == ':' && cursor[1] == '=') {
        uint8_t field_index;
        uint16_t temporary_index;

        cursor += 2;
        cursor = source_skip_horizontal_space(cursor);
        cursor = compile_source_expression_push(
            class_object,
            cursor,
            argument_names,
            argument_count,
            temporary_names,
            temporary_count,
            instruction_words,
            instruction_count
        );
        if (cursor == 0) {
            machine_panic("KernelInstaller source method assignment right-hand side is invalid");
        }
        cursor = source_skip_horizontal_space(cursor);
        if (*cursor != '.' || cursor[1] != '\0') {
            machine_panic("KernelInstaller source method assignment has unexpected trailing text");
        }
        for (temporary_index = 0U; temporary_index < temporary_count; ++temporary_index) {
            if (temporary_names[temporary_index][0] != '\0' &&
                source_names_equal(receiver_name, temporary_names[temporary_index])) {
                compile_source_append_instruction(
                    instruction_words,
                    instruction_count,
                    COMPILED_METHOD_OP_STORE_LEXICAL,
                    0U,
                    temporary_index
                );
                return 1U;
            }
        }
        if (!class_field_index_for_name(class_object, receiver_name, &field_index)) {
            machine_panic("KernelInstaller source method assignment target must be a temporary or an instance variable");
        }
        compile_source_append_instruction(
            instruction_words,
            instruction_count,
            COMPILED_METHOD_OP_STORE_FIELD,
            field_index,
            0U
        );
        return 1U;
    }
    cursor = compile_source_expression_push(
        class_object,
        line,
        argument_names,
        argument_count,
        temporary_names,
        temporary_count,
        instruction_words,
        instruction_count
    );
    if (cursor == 0) {
        machine_panic("KernelInstaller source method uses an unsupported receiver expression");
    }
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '\0' && !(*cursor == '.' && cursor[1] == '\0')) {
        machine_panic("KernelInstaller source method statement has unexpected trailing text");
    }
    *produces_value = 1U;
    return 1U;
}

static void compile_source_return_line(
    const struct recorz_mvp_heap_object *class_object,
    const char *line,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint32_t instruction_words[],
    uint8_t *instruction_count
) {
    const char *cursor = source_skip_horizontal_space(line);

    if (*cursor != '^') {
        machine_panic("KernelInstaller source method return is invalid");
    }
    ++cursor;
    cursor = source_skip_horizontal_space(cursor);
    if (source_names_equal(cursor, "self")) {
        if (*instruction_count >= COMPILED_METHOD_MAX_INSTRUCTIONS) {
            machine_panic("KernelInstaller source method exceeds compiled method capacity");
        }
        instruction_words[(*instruction_count)++] = encode_compiled_method_word(COMPILED_METHOD_OP_RETURN_RECEIVER, 0U, 0U);
        return;
    }
    cursor = compile_source_expression_push(
        class_object,
        cursor,
        argument_names,
        argument_count,
        temporary_names,
        temporary_count,
        instruction_words,
        instruction_count
    );
    if (cursor == 0) {
        machine_panic("KernelInstaller source method return expression is unsupported");
    }
    cursor = source_skip_horizontal_space(cursor);
    if (*cursor != '\0') {
        machine_panic("KernelInstaller source method return has unexpected trailing text");
    }
    compile_source_append_instruction(
        instruction_words,
        instruction_count,
        COMPILED_METHOD_OP_RETURN_TOP,
        0U,
        0U
    );
}

static void compile_source_parse_temporary_declarations(
    const struct recorz_mvp_heap_object *class_object,
    const char *line,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t *temporary_count
) {
    const char *cursor = source_skip_horizontal_space(line);

    if (*cursor != '|') {
        machine_panic("KernelInstaller source temporary declaration is invalid");
    }
    ++cursor;
    cursor = source_skip_horizontal_space(cursor);
    *temporary_count = 0U;
    while (*cursor != '\0') {
        uint16_t existing_index;
        uint8_t field_index;

        if (*cursor == '|') {
            ++cursor;
            cursor = source_skip_horizontal_space(cursor);
            if (*cursor != '\0') {
                machine_panic("KernelInstaller source temporary declaration has unexpected trailing text");
            }
            return;
        }
        if (*temporary_count >= LEXICAL_LIMIT) {
            machine_panic("KernelInstaller source method has too many temporaries");
        }
        cursor = source_parse_identifier(
            cursor,
            temporary_names[*temporary_count],
            sizeof(temporary_names[*temporary_count])
        );
        if (cursor == 0) {
            machine_panic("KernelInstaller source temporary declaration is invalid");
        }
        for (existing_index = 0U; existing_index < argument_count; ++existing_index) {
            if (source_names_equal(temporary_names[*temporary_count], argument_names[existing_index])) {
                machine_panic("KernelInstaller source temporary repeats an argument name");
            }
        }
        for (existing_index = 0U; existing_index < *temporary_count; ++existing_index) {
            if (source_names_equal(temporary_names[*temporary_count], temporary_names[existing_index])) {
                machine_panic("KernelInstaller source temporary declaration repeats a temporary");
            }
        }
        if (class_field_index_for_name(class_object, temporary_names[*temporary_count], &field_index)) {
            machine_panic("KernelInstaller source temporary repeats an instance variable name");
        }
        ++(*temporary_count);
        cursor = source_skip_horizontal_space(cursor);
    }
    machine_panic("KernelInstaller source temporary declaration is missing '|'");
}

static uint16_t compile_source_method_and_allocate(
    const struct recorz_mvp_heap_object *class_object,
    const char *source,
    uint8_t *selector_id_out,
    uint16_t *argument_count_out
) {
    char header_line[METHOD_SOURCE_LINE_LIMIT];
    char statement_line[METHOD_SOURCE_LINE_LIMIT];
    char trailing_line[METHOD_SOURCE_LINE_LIMIT];
    char selector_name_buffer[METHOD_SOURCE_NAME_LIMIT];
    char selector_part_buffer[METHOD_SOURCE_NAME_LIMIT];
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    char temporary_names[LEXICAL_LIMIT][METHOD_SOURCE_NAME_LIMIT];
    const char *cursor = source;
    const char *header_cursor;
    uint32_t instruction_words[COMPILED_METHOD_MAX_INSTRUCTIONS];
    uint8_t instruction_count = 0U;
    uint8_t selector_id;
    uint16_t argument_count = 0U;
    uint16_t temporary_count = 0U;
    uint8_t found_return = 0U;
    uint8_t pending_statement_value = 0U;
    uint32_t selector_length = 0U;
    uint32_t part_length = 0U;
    uint8_t have_pending_line = 0U;

    if (source == 0 || *source == '\0') {
        machine_panic("KernelInstaller source method string is empty");
    }
    if (source_text_requires_live_evaluator(source)) {
        const char *body_cursor;

        if (source_parse_method_header(
                source,
                selector_name_buffer,
                argument_names,
                &argument_count,
                &body_cursor) == 0) {
            machine_panic("KernelInstaller source method header is invalid");
        }
        (void)body_cursor;
        selector_id = source_selector_id_for_name(selector_name_buffer);
        if (selector_id == 0U) {
            machine_panic("KernelInstaller source method uses an unknown selector");
        }
        *selector_id_out = selector_id;
        *argument_count_out = argument_count;
        forget_live_string_literals(heap_handle_for_object(class_object), selector_id, (uint8_t)argument_count);
        return allocate_placeholder_live_source_compiled_method();
    }
    for (argument_count = 0U; argument_count < MAX_SEND_ARGS; ++argument_count) {
        argument_names[argument_count][0] = '\0';
    }
    for (temporary_count = 0U; temporary_count < LEXICAL_LIMIT; ++temporary_count) {
        temporary_names[temporary_count][0] = '\0';
    }
    argument_count = 0U;
    temporary_count = 0U;
    if (source_copy_trimmed_line(&cursor, header_line, sizeof(header_line)) == 0U) {
        machine_panic("KernelInstaller source method is missing a header");
    }
    header_cursor = source_parse_identifier(header_line, selector_name_buffer, sizeof(selector_name_buffer));
    if (header_cursor == 0) {
        machine_panic("KernelInstaller source method header is invalid");
    }
    header_cursor = source_skip_horizontal_space(header_cursor);
    while (selector_name_buffer[selector_length] != '\0') {
        ++selector_length;
    }
    if (*header_cursor == ':') {
        while (*header_cursor == ':') {
            if (argument_count >= MAX_SEND_ARGS) {
                machine_panic("KernelInstaller source method has too many arguments");
            }
            if (selector_length + 1U >= sizeof(selector_name_buffer)) {
                machine_panic("KernelInstaller source method selector exceeds buffer capacity");
            }
            selector_name_buffer[selector_length++] = ':';
            selector_name_buffer[selector_length] = '\0';
            ++header_cursor;
            header_cursor = source_skip_horizontal_space(header_cursor);
            header_cursor = source_parse_identifier(
                header_cursor,
                argument_names[argument_count],
                sizeof(argument_names[argument_count])
            );
            if (header_cursor == 0) {
                machine_panic("KernelInstaller source method keyword header is missing an argument name");
            }
            ++argument_count;
            header_cursor = source_skip_horizontal_space(header_cursor);
            if (*header_cursor == '\0') {
                break;
            }
            header_cursor = source_parse_identifier(header_cursor, selector_part_buffer, sizeof(selector_part_buffer));
            if (header_cursor == 0) {
                machine_panic("KernelInstaller source method header has unexpected trailing text");
            }
            part_length = 0U;
            while (selector_part_buffer[part_length] != '\0') {
                if (selector_length + 1U >= sizeof(selector_name_buffer)) {
                    machine_panic("KernelInstaller source method selector exceeds buffer capacity");
                }
                selector_name_buffer[selector_length++] = selector_part_buffer[part_length++];
            }
            selector_name_buffer[selector_length] = '\0';
            header_cursor = source_skip_horizontal_space(header_cursor);
        }
        if (*header_cursor != '\0') {
            machine_panic("KernelInstaller source method header has unexpected trailing text");
        }
    } else if (*header_cursor != '\0') {
        machine_panic("KernelInstaller source method header has unexpected trailing text");
    }
    selector_id = source_selector_id_for_name(selector_name_buffer);
    if (selector_id == 0U) {
        machine_panic("KernelInstaller source method uses an unknown selector");
    }
    forget_live_string_literals(heap_handle_for_object(class_object), selector_id, (uint8_t)argument_count);
    compiling_method_class_handle = heap_handle_for_object(class_object);
    compiling_method_selector_id = selector_id;
    compiling_method_argument_count = (uint8_t)argument_count;
    if (source_copy_trimmed_line(&cursor, statement_line, sizeof(statement_line)) != 0U) {
        const char *trimmed_line = source_skip_horizontal_space(statement_line);

        if (trimmed_line[0] == '|') {
            compile_source_parse_temporary_declarations(
                class_object,
                statement_line,
                argument_names,
                argument_count,
                temporary_names,
                &temporary_count
            );
        } else {
            have_pending_line = 1U;
        }
    }
    while (have_pending_line || source_copy_trimmed_line(&cursor, statement_line, sizeof(statement_line)) != 0U) {
        uint8_t produces_value = 0U;
        const char *trimmed_line = source_skip_horizontal_space(statement_line);

        have_pending_line = 0U;

        if (pending_statement_value) {
            const char *return_cursor = source_skip_horizontal_space(trimmed_line + 1);

            if (!(trimmed_line[0] == '^' && source_names_equal(return_cursor, "self"))) {
                compile_source_append_instruction(
                    instruction_words,
                    &instruction_count,
                    COMPILED_METHOD_OP_POP,
                    0U,
                    0U
                );
            }
            pending_statement_value = 0U;
        }

        if (trimmed_line[0] == '^') {
            compile_source_return_line(
                class_object,
                statement_line,
                argument_names,
                argument_count,
                temporary_names,
                temporary_count,
                instruction_words,
                &instruction_count
            );
            found_return = 1U;
            break;
        }
        if (!compile_source_statement_line(
                class_object,
                statement_line,
                argument_names,
                argument_count,
                temporary_names,
                temporary_count,
                instruction_words,
                &instruction_count,
                &produces_value
            )) {
            machine_panic("KernelInstaller source method statement is unsupported");
        }
        pending_statement_value = produces_value;
    }
    if (!found_return) {
        machine_panic("KernelInstaller source method is missing a return");
    }
    if (source_copy_trimmed_line(&cursor, trailing_line, sizeof(trailing_line)) != 0U) {
        machine_panic("KernelInstaller source method has unexpected trailing lines");
    }
    compiling_method_class_handle = 0U;
    compiling_method_selector_id = 0U;
    compiling_method_argument_count = 0U;
    *selector_id_out = selector_id;
    *argument_count_out = argument_count;
    return allocate_compiled_method_from_words(instruction_words, instruction_count);
}

static uint8_t source_environment_has_local_binding(int16_t lexical_environment_index, const char *name) {
    struct recorz_mvp_source_lexical_environment *environment = source_lexical_environment_at(lexical_environment_index);
    uint16_t binding_index;

    for (binding_index = 0U; binding_index < environment->binding_count; ++binding_index) {
        if (source_names_equal(environment->bindings[binding_index].name, name)) {
            return 1U;
        }
    }
    return 0U;
}

static struct recorz_mvp_value source_read_identifier(
    struct recorz_mvp_source_method_context *context,
    const char *name
) {
    struct recorz_mvp_value *binding_cell;
    uint8_t field_index;
    uint8_t global_id;

    if (source_names_equal(name, "self")) {
        return context->receiver;
    }
    if (source_names_equal(name, "nil")) {
        return nil_value();
    }
    if (source_names_equal(name, "true")) {
        return global_value(RECORZ_MVP_GLOBAL_TRUE);
    }
    if (source_names_equal(name, "false")) {
        return global_value(RECORZ_MVP_GLOBAL_FALSE);
    }
    if (source_names_equal(name, "thisContext")) {
        if (context->current_context_handle == 0U) {
            machine_panic("live source thisContext has no active context object");
        }
        return object_value(context->current_context_handle);
    }
    binding_cell = source_lookup_binding_cell(context->lexical_environment_index, name);
    if (binding_cell != 0) {
        return *binding_cell;
    }
    if (context->receiver.kind == RECORZ_MVP_VALUE_OBJECT &&
        context->defining_class != 0 &&
        class_field_index_for_name(context->defining_class, name, &field_index)) {
        return heap_get_field(heap_object_for_value(context->receiver), field_index);
    }
    global_id = source_global_id_for_name(name);
    if (global_id != 0U) {
        return global_value(global_id);
    }
    machine_panic("live source identifier is unknown");
    return nil_value();
}

static void source_write_identifier(
    struct recorz_mvp_source_method_context *context,
    const char *name,
    struct recorz_mvp_value value
) {
    struct recorz_mvp_value *binding_cell = source_lookup_binding_cell(context->lexical_environment_index, name);
    uint8_t field_index;

    if (binding_cell != 0) {
        *binding_cell = value;
        return;
    }
    if (context->receiver.kind == RECORZ_MVP_VALUE_OBJECT &&
        context->defining_class != 0 &&
        class_field_index_for_name(context->defining_class, name, &field_index)) {
        heap_set_field((uint16_t)context->receiver.integer, field_index, value);
        return;
    }
    machine_panic("live source assignment target is unknown");
}

static const char *source_parse_temporary_declarations(
    struct recorz_mvp_source_method_context *context,
    const char *source
) {
    struct recorz_mvp_source_lexical_environment *environment =
        source_lexical_environment_at(context->lexical_environment_index);
    const char *cursor = source_skip_horizontal_space(source);

    if (*cursor != '|') {
        machine_panic("live source temporary declaration is invalid");
    }
    ++cursor;
    cursor = source_skip_horizontal_space(cursor);
    while (*cursor != '\0') {
        char name[METHOD_SOURCE_NAME_LIMIT];
        uint8_t field_index;

        if (*cursor == '|') {
            ++cursor;
            return source_skip_statement_space(cursor);
        }
        cursor = source_parse_identifier(cursor, name, sizeof(name));
        if (cursor == 0) {
            machine_panic("live source temporary declaration is invalid");
        }
        if (source_environment_has_local_binding(context->lexical_environment_index, name)) {
            machine_panic("live source temporary declaration repeats a local binding");
        }
        if (context->defining_class != 0 && class_field_index_for_name(context->defining_class, name, &field_index)) {
            machine_panic("live source temporary declaration repeats an instance variable");
        }
        if (environment->binding_count >= SOURCE_EVAL_BINDING_LIMIT) {
            machine_panic("live source temporary declaration exceeds binding capacity");
        }
        source_append_binding(context->lexical_environment_index, name, nil_value());
        cursor = source_skip_horizontal_space(cursor);
    }
    machine_panic("live source temporary declaration is missing a closing '|'");
    return 0;
}

static const char *source_parse_block_header(
    const char *source,
    char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t *argument_count_out
) {
    const char *cursor = source_skip_statement_space(source);

    *argument_count_out = 0U;
    while (*cursor == ':') {
        ++cursor;
        cursor = source_parse_identifier(cursor, argument_names[*argument_count_out], METHOD_SOURCE_NAME_LIMIT);
        if (cursor == 0) {
            machine_panic("block parameter name is invalid");
        }
        ++(*argument_count_out);
        if (*argument_count_out > MAX_SEND_ARGS) {
            machine_panic("block parameter count exceeds send capacity");
        }
        cursor = source_skip_statement_space(cursor);
    }
    if (*argument_count_out != 0U) {
        if (*cursor != '|') {
            machine_panic("block parameter list is missing '|'");
        }
        ++cursor;
    }
    return cursor;
}

static struct recorz_mvp_source_eval_result source_send_message(
    struct recorz_mvp_source_method_context *context,
    struct recorz_mvp_value receiver,
    const char *selector_name,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[]
) {
    const struct recorz_mvp_heap_object *source_owner_class = 0;
    const struct recorz_mvp_live_method_source *source_record;
    uint8_t selector_id;

    if (receiver.kind == RECORZ_MVP_VALUE_OBJECT &&
        primitive_kind_for_heap_object(heap_object_for_value(receiver)) == RECORZ_MVP_OBJECT_BLOCK_CLOSURE &&
        ((source_names_equal(selector_name, "value") && argument_count == 0U) ||
         (source_names_equal(selector_name, "value:") && argument_count == 1U))) {
        return source_execute_block_closure(
            heap_object_for_value(receiver),
            argument_count,
            arguments,
            context->current_context_handle
        );
    }
    if ((source_names_equal(selector_name, "ifTrue:") && argument_count == 1U) ||
        (source_names_equal(selector_name, "ifFalse:") && argument_count == 1U) ||
        (source_names_equal(selector_name, "ifTrue:ifFalse:") && argument_count == 2U)) {
        uint8_t condition_is_true = condition_value_is_true(receiver);
        uint16_t chosen_index = 0xFFFFU;

        if (source_names_equal(selector_name, "ifTrue:")) {
            chosen_index = condition_is_true ? 0U : 0xFFFFU;
        } else if (source_names_equal(selector_name, "ifFalse:")) {
            chosen_index = condition_is_true ? 0xFFFFU : 0U;
        } else {
            chosen_index = condition_is_true ? 0U : 1U;
        }
        if (chosen_index == 0xFFFFU) {
            return source_eval_value_result(nil_value());
        }
        if (arguments[chosen_index].kind != RECORZ_MVP_VALUE_OBJECT ||
            primitive_kind_for_heap_object(heap_object_for_value(arguments[chosen_index])) != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
            machine_panic("conditional send expects a block closure argument");
        }
        return source_execute_block_closure(
            heap_object_for_value(arguments[chosen_index]),
            0U,
            0,
            context->current_context_handle
        );
    }
    selector_id = source_selector_id_for_name(selector_name);
    if (selector_id == 0U) {
        machine_panic("live source send uses an unknown selector");
    }
    if (receiver.kind == RECORZ_MVP_VALUE_OBJECT) {
        source_record = live_method_source_for_class_chain(
            class_object_for_heap_object(heap_object_for_value(receiver)),
            selector_id,
            (uint8_t)argument_count,
            &source_owner_class
        );
        if (source_record != 0 && source_text_requires_live_evaluator(live_method_source_text(source_record))) {
            execute_live_source_method_with_sender(
                source_owner_class,
                receiver,
                argument_count,
                arguments,
                live_method_source_text(source_record),
                context->current_context_handle
            );
            return source_eval_value_result(pop_value());
        }
    }
    perform_send_with_sender(
        receiver,
        selector_id,
        argument_count,
        arguments,
        context->current_context_handle,
        0
    );
    return source_eval_value_result(pop_value());
}

static struct recorz_mvp_source_eval_result source_evaluate_primary_expression(
    struct recorz_mvp_source_method_context *context,
    const char *cursor,
    const char **cursor_out
) {
    struct recorz_mvp_source_eval_result result;
    const char *parsed_cursor;
    char token[METHOD_SOURCE_NAME_LIMIT];
    char block_source[METHOD_SOURCE_CHUNK_LIMIT];
    int32_t small_integer;

    cursor = source_skip_horizontal_space(cursor);
    if (*cursor == '(') {
        result = source_evaluate_expression(context, cursor + 1, &parsed_cursor);
        if (result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
            return result;
        }
        parsed_cursor = source_skip_horizontal_space(parsed_cursor);
        if (*parsed_cursor != ')') {
            machine_panic("live source parenthesized expression is missing ')'");
        }
        cursor = parsed_cursor + 1;
    } else if (*cursor == '[') {
        uint16_t block_handle;

        parsed_cursor = source_copy_bracket_body(cursor, block_source, sizeof(block_source));
        if (parsed_cursor == 0) {
            machine_panic("live source block literal is invalid");
        }
        block_handle = allocate_block_closure_from_source(block_source, context->receiver);
        source_register_block_state(
            block_handle,
            context->defining_class,
            context->lexical_environment_index,
            context->home_context_index
        );
        result = source_eval_value_result(object_value(block_handle));
        cursor = parsed_cursor;
    } else if (*cursor == '\'') {
        char quoted_text[METHOD_SOURCE_CHUNK_LIMIT];

        parsed_cursor = source_copy_single_quoted_text(cursor, quoted_text, sizeof(quoted_text));
        if (parsed_cursor == 0) {
            machine_panic("live source string literal is invalid");
        }
        result = source_eval_value_result(string_value(runtime_string_allocate_copy(quoted_text)));
        cursor = parsed_cursor;
    } else {
        parsed_cursor = source_parse_small_integer(cursor, &small_integer);
        if (parsed_cursor != 0) {
            result = source_eval_value_result(small_integer_value(small_integer));
            cursor = parsed_cursor;
        } else {
            parsed_cursor = source_parse_identifier(cursor, token, sizeof(token));
            if (parsed_cursor == 0) {
                machine_panic("live source primary expression is invalid");
            }
            result = source_eval_value_result(source_read_identifier(context, token));
            cursor = parsed_cursor;
        }
    }

    cursor = source_skip_horizontal_space(cursor);
    while (source_char_is_identifier_start(*cursor)) {
        char selector_name[METHOD_SOURCE_NAME_LIMIT];
        const char *selector_cursor = source_parse_identifier(cursor, selector_name, sizeof(selector_name));
        const char *after_selector;

        if (selector_cursor == 0) {
            break;
        }
        after_selector = source_skip_horizontal_space(selector_cursor);
        if (*after_selector == ':') {
            break;
        }
        result = source_send_message(context, result.value, selector_name, 0U, 0);
        if (result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
            return result;
        }
        cursor = after_selector;
    }
    *cursor_out = cursor;
    return result;
}

static struct recorz_mvp_source_eval_result source_evaluate_binary_expression(
    struct recorz_mvp_source_method_context *context,
    const char *cursor,
    const char **cursor_out
) {
    struct recorz_mvp_source_eval_result receiver_result;

    receiver_result = source_evaluate_primary_expression(context, cursor, &cursor);
    if (receiver_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
        return receiver_result;
    }
    cursor = source_skip_horizontal_space(cursor);
    while (source_char_is_binary_selector(*cursor)) {
        char selector_name[2];
        struct recorz_mvp_source_eval_result argument_result;

        selector_name[0] = *cursor++;
        selector_name[1] = '\0';
        argument_result = source_evaluate_primary_expression(context, cursor, &cursor);
        if (argument_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
            return argument_result;
        }
        receiver_result = source_send_message(context, receiver_result.value, selector_name, 1U, &argument_result.value);
        if (receiver_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
            return receiver_result;
        }
        cursor = source_skip_horizontal_space(cursor);
    }
    *cursor_out = cursor;
    return receiver_result;
}

static struct recorz_mvp_source_eval_result source_evaluate_expression(
    struct recorz_mvp_source_method_context *context,
    const char *cursor,
    const char **cursor_out
) {
    char selector_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_part[METHOD_SOURCE_NAME_LIMIT];
    struct recorz_mvp_source_eval_result receiver_result;
    struct recorz_mvp_value arguments[MAX_SEND_ARGS];
    const char *part_cursor;
    uint16_t argument_count = 0U;
    uint32_t selector_length = 0U;

    receiver_result = source_evaluate_binary_expression(context, cursor, &cursor);
    if (receiver_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
        return receiver_result;
    }
    cursor = source_skip_horizontal_space(cursor);
    part_cursor = source_parse_identifier(cursor, selector_part, sizeof(selector_part));
    if (part_cursor == 0) {
        *cursor_out = cursor;
        return receiver_result;
    }
    part_cursor = source_skip_horizontal_space(part_cursor);
    if (*part_cursor != ':') {
        *cursor_out = cursor;
        return receiver_result;
    }
    while (selector_part[selector_length] != '\0') {
        selector_name[selector_length] = selector_part[selector_length];
        ++selector_length;
    }
    selector_name[selector_length] = '\0';
    do {
        struct recorz_mvp_source_eval_result argument_result;
        uint32_t part_index = 0U;

        if (argument_count >= MAX_SEND_ARGS) {
            machine_panic("live source send exceeds argument capacity");
        }
        if (selector_length + 1U >= sizeof(selector_name)) {
            machine_panic("live source selector exceeds capacity");
        }
        selector_name[selector_length++] = ':';
        selector_name[selector_length] = '\0';
        argument_result = source_evaluate_binary_expression(context, part_cursor + 1, &cursor);
        if (argument_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
            return argument_result;
        }
        arguments[argument_count++] = argument_result.value;
        cursor = source_skip_horizontal_space(cursor);
        part_cursor = source_parse_identifier(cursor, selector_part, sizeof(selector_part));
        if (part_cursor == 0) {
            break;
        }
        part_cursor = source_skip_horizontal_space(part_cursor);
        if (*part_cursor != ':') {
            break;
        }
        while (selector_part[part_index] != '\0') {
            if (selector_length + 1U >= sizeof(selector_name)) {
                machine_panic("live source selector exceeds capacity");
            }
            selector_name[selector_length++] = selector_part[part_index++];
        }
        selector_name[selector_length] = '\0';
    } while (*part_cursor == ':');
    *cursor_out = cursor;
    return source_send_message(context, receiver_result.value, selector_name, argument_count, arguments);
}

static struct recorz_mvp_source_eval_result source_execute_block_closure(
    const struct recorz_mvp_heap_object *object,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle
) {
    const struct recorz_mvp_runtime_block_state *block_state;
    const struct recorz_mvp_heap_object *defining_class = 0;
    struct recorz_mvp_source_method_context context;
    struct recorz_mvp_source_eval_result result;
    struct recorz_mvp_value source_value;
    struct recorz_mvp_value home_receiver;
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    uint16_t parsed_argument_count = 0U;
    const char *body_cursor;
    int16_t lexical_environment_index;
    uint16_t argument_index;

    if (primitive_kind_for_heap_object(object) != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
        machine_panic("source block execution expects a block closure");
    }
    source_value = heap_get_field(object, BLOCK_CLOSURE_FIELD_SOURCE);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING || source_value.string == 0) {
        machine_panic("block closure source field is invalid");
    }
    home_receiver = heap_get_field(object, BLOCK_CLOSURE_FIELD_HOME_RECEIVER);
    block_state = source_block_state_for_handle(heap_handle_for_object(object));
    if (block_state != 0) {
        lexical_environment_index = source_allocate_lexical_environment(block_state->lexical_environment_index);
        defining_class = block_state->defining_class;
    } else {
        lexical_environment_index = source_allocate_lexical_environment(-1);
        if (home_receiver.kind == RECORZ_MVP_VALUE_OBJECT) {
            defining_class = class_object_for_heap_object(heap_object_for_value(home_receiver));
        } else {
            defining_class = class_object_for_kind(RECORZ_MVP_OBJECT_OBJECT);
        }
    }
    body_cursor = source_parse_block_header(source_value.string, argument_names, &parsed_argument_count);
    if (parsed_argument_count != argument_count) {
        machine_panic("block argument count does not match activation");
    }
    for (argument_index = 0U; argument_index < argument_count; ++argument_index) {
        source_append_binding(lexical_environment_index, argument_names[argument_index], arguments[argument_index]);
    }
    context.defining_class = defining_class;
    context.receiver = home_receiver;
    context.lexical_environment_index = lexical_environment_index;
    context.home_context_index = block_state == 0 ? -1 : block_state->home_context_index;
    context.current_context_handle = allocate_source_context_object(
        sender_context_handle,
        home_receiver,
        "<block>"
    );
    context.is_block = 1U;
    result = source_evaluate_statement_sequence(&context, body_cursor, 1U);
    heap_set_field(context.current_context_handle, CONTEXT_FIELD_ALIVE, boolean_value(0U));
    if (result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
        if (context.home_context_index < 0) {
            machine_panic("block attempted a non-local return without a live home context");
        }
        if (!source_home_context_at(context.home_context_index)->alive) {
            machine_panic("block attempted a non-local return to a dead home context");
        }
    }
    return result;
}

static struct recorz_mvp_source_eval_result source_evaluate_statement_sequence(
    struct recorz_mvp_source_method_context *context,
    const char *source,
    uint8_t is_block
) {
    const char *cursor = source_skip_statement_space(source);
    char statement[METHOD_SOURCE_CHUNK_LIMIT];
    struct recorz_mvp_source_eval_result last_result = source_eval_value_result(nil_value());
    uint8_t saw_statement = 0U;

    (void)is_block;
    if (*cursor == '|') {
        cursor = source_parse_temporary_declarations(context, cursor);
    }
    while (source_copy_next_statement(&cursor, statement, sizeof(statement)) != 0U) {
        const char *trimmed = source_skip_statement_space(statement);

        if (trimmed[0] == '\0') {
            continue;
        }
        if (trimmed[0] == '^') {
            const char *return_cursor;

            last_result = source_evaluate_expression(context, trimmed + 1, &return_cursor);
            if (last_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
                return last_result;
            }
            return_cursor = source_skip_horizontal_space(return_cursor);
            if (*return_cursor != '\0') {
                machine_panic("live source return has unexpected trailing text");
            }
            return source_eval_return_result(last_result.value);
        }
        {
            char binding_name[METHOD_SOURCE_NAME_LIMIT];
            const char *binding_cursor = source_parse_identifier(trimmed, binding_name, sizeof(binding_name));

            if (binding_cursor != 0) {
                binding_cursor = source_skip_horizontal_space(binding_cursor);
                if (binding_cursor[0] == ':' && binding_cursor[1] == '=') {
                    const char *expression_cursor;

                    last_result = source_evaluate_expression(context, binding_cursor + 2, &expression_cursor);
                    if (last_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
                        return last_result;
                    }
                    expression_cursor = source_skip_horizontal_space(expression_cursor);
                    if (*expression_cursor != '\0') {
                        machine_panic("live source assignment has unexpected trailing text");
                    }
                    source_write_identifier(context, binding_name, last_result.value);
                    saw_statement = 1U;
                    continue;
                }
            }
        }
        last_result = source_evaluate_expression(context, trimmed, &trimmed);
        if (last_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
            return last_result;
        }
        trimmed = source_skip_horizontal_space(trimmed);
        if (*trimmed != '\0') {
            machine_panic("live source statement has unexpected trailing text");
        }
        saw_statement = 1U;
    }
    if (!saw_statement) {
        return source_eval_value_result(nil_value());
    }
    return last_result;
}

static void execute_live_source_method_with_sender(
    const struct recorz_mvp_heap_object *class_object,
    struct recorz_mvp_value receiver,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    const char *source,
    uint16_t sender_context_handle
) {
    struct recorz_mvp_source_method_context context;
    struct recorz_mvp_source_eval_result result;
    char selector_name[METHOD_SOURCE_NAME_LIMIT];
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    uint16_t parsed_argument_count;
    const char *body_cursor;
    int16_t lexical_environment_index;
    int16_t home_context_index;
    uint16_t argument_index;

    if (source == 0 || source[0] == '\0') {
        machine_panic("live source method is empty");
    }
    if (source_parse_method_header(
            source,
            selector_name,
            argument_names,
            &parsed_argument_count,
            &body_cursor) == 0) {
        machine_panic("live source method header is invalid");
    }
    if (parsed_argument_count != argument_count) {
        machine_panic("live source method argument count does not match send");
    }
    lexical_environment_index = source_allocate_lexical_environment(-1);
    for (argument_index = 0U; argument_index < argument_count; ++argument_index) {
        source_append_binding(lexical_environment_index, argument_names[argument_index], arguments[argument_index]);
    }
    home_context_index = source_allocate_home_context(
        class_object,
        receiver,
        lexical_environment_index,
        sender_context_handle,
        selector_name
    );
    context.defining_class = class_object;
    context.receiver = receiver;
    context.lexical_environment_index = lexical_environment_index;
    context.home_context_index = home_context_index;
    context.current_context_handle = source_home_context_at(home_context_index)->context_handle;
    context.is_block = 0U;
    result = source_evaluate_statement_sequence(&context, body_cursor, 0U);
    source_home_context_at(home_context_index)->alive = 0U;
    heap_set_field(context.current_context_handle, CONTEXT_FIELD_ALIVE, boolean_value(0U));
    push(result.value);
}

static void file_in_method_chunks_on_class(
    const char *source,
    const struct recorz_mvp_heap_object *class_object
) {
    const char *cursor = source;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    char current_protocol[METHOD_SOURCE_NAME_LIMIT];
    uint8_t installed_method_count = 0U;

    if (source == 0 || *source == '\0') {
        machine_panic("KernelInstaller method chunk source is empty");
    }
    current_protocol[0] = '\0';
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        uint16_t compiled_method_handle;
        uint8_t selector_id;
        uint16_t argument_count;

        if (source_starts_with(chunk, "RecorzKernelClass:") ||
            source_starts_with(chunk, "RecorzKernelClassSide:")) {
            current_protocol[0] = '\0';
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelProtocol:")) {
            source_parse_protocol_name_from_chunk(chunk, current_protocol, sizeof(current_protocol));
            continue;
        }
        compiled_method_handle = compile_source_method_and_allocate(class_object, chunk, &selector_id, &argument_count);
        validate_compiled_method(heap_object(compiled_method_handle), argument_count);
        install_compiled_method_update(class_object, selector_id, argument_count, compiled_method_handle);
        remember_live_method_source(
            heap_handle_for_object(class_object),
            selector_id,
            (uint8_t)argument_count,
            current_protocol,
            chunk
        );
        ++installed_method_count;
    }
    if (installed_method_count == 0U) {
        machine_panic("KernelInstaller fileInMethodChunks:onClass: found no installable method chunks");
    }
}

static void install_method_chunk_on_class(
    const struct recorz_mvp_heap_object *class_object,
    const char *protocol_name,
    const char *chunk
) {
    uint16_t compiled_method_handle;
    uint8_t selector_id;
    uint16_t argument_count;

    compiled_method_handle = compile_source_method_and_allocate(class_object, chunk, &selector_id, &argument_count);
    validate_compiled_method(heap_object(compiled_method_handle), argument_count);
    install_compiled_method_update(class_object, selector_id, argument_count, compiled_method_handle);
    remember_live_method_source(
        heap_handle_for_object(class_object),
        selector_id,
        (uint8_t)argument_count,
        protocol_name,
        chunk
    );
}

static void install_method_source_on_class(
    const struct recorz_mvp_heap_object *class_object,
    const char *source
) {
    uint16_t compiled_method_handle;
    uint8_t selector_id;
    uint16_t argument_count;

    compiled_method_handle = compile_source_method_and_allocate(class_object, source, &selector_id, &argument_count);
    validate_compiled_method(heap_object(compiled_method_handle), argument_count);
    install_compiled_method_update(class_object, selector_id, argument_count, compiled_method_handle);
    remember_live_method_source(heap_handle_for_object(class_object), selector_id, (uint8_t)argument_count, "", source);
}

static const struct recorz_mvp_heap_object *lookup_class_by_name(const char *class_name) {
    uint8_t kind;
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition;

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
    dynamic_definition = dynamic_class_definition_for_name(class_name);
    if (dynamic_definition != 0) {
        return (const struct recorz_mvp_heap_object *)heap_object(dynamic_definition->class_handle);
    }
    return 0;
}

static void copy_dynamic_class_definition(
    struct recorz_mvp_dynamic_class_definition *destination,
    const struct recorz_mvp_live_class_definition *source,
    uint16_t class_handle,
    uint16_t superclass_handle
) {
    uint8_t ivar_index;

    destination->class_handle = class_handle;
    destination->superclass_handle = superclass_handle;
    source_copy_identifier(destination->class_name, sizeof(destination->class_name), source->class_name);
    source_copy_identifier(destination->package_name, sizeof(destination->package_name), source->package_name);
    source_copy_identifier(destination->class_comment, sizeof(destination->class_comment), source->class_comment);
    destination->instance_variable_count = source->instance_variable_count;
    for (ivar_index = 0U; ivar_index < DYNAMIC_CLASS_IVAR_LIMIT; ++ivar_index) {
        destination->instance_variable_names[ivar_index][0] = '\0';
    }
    for (ivar_index = 0U; ivar_index < source->instance_variable_count; ++ivar_index) {
        source_copy_identifier(
            destination->instance_variable_names[ivar_index],
            sizeof(destination->instance_variable_names[ivar_index]),
            source->instance_variable_names[ivar_index]
        );
    }
}

static const struct recorz_mvp_heap_object *ensure_class_defined(
    const struct recorz_mvp_live_class_definition *definition
) {
    const struct recorz_mvp_heap_object *class_object = lookup_class_by_name(definition->class_name);
    struct recorz_mvp_dynamic_class_definition *dynamic_definition = 0;
    const struct recorz_mvp_dynamic_class_definition *existing_dynamic_definition = 0;
    struct recorz_mvp_live_class_definition superclass_definition;
    const struct recorz_mvp_heap_object *superclass_object;
    uint16_t class_handle;
    const char *resolved_superclass_name;

    if (definition->class_name[0] == '\0') {
        machine_panic("dynamic class definition is missing a class name");
    }
    if (class_object != 0) {
        dynamic_definition = mutable_dynamic_class_definition_for_handle(heap_handle_for_object(class_object));
        if (dynamic_definition == 0) {
            if (definition->instance_variable_count != 0U) {
                machine_panic("KernelInstaller does not support redefining seeded class instance variables");
            }
            if (definition->package_name[0] != '\0') {
                machine_panic("KernelInstaller does not support attaching packages to seeded classes");
            }
            if (definition->class_comment[0] != '\0') {
                machine_panic("KernelInstaller does not support attaching comments to seeded classes");
            }
            if (definition->has_superclass) {
                resolved_superclass_name = class_name_for_object(
                    heap_object_for_value(heap_get_field(class_object, CLASS_FIELD_SUPERCLASS))
                );
                if (!source_names_equal(definition->superclass_name, resolved_superclass_name)) {
                    machine_panic("KernelInstaller seeded class superclass does not match class chunk");
                }
            }
            return class_object;
        }
        existing_dynamic_definition = dynamic_definition;
        if (!definition->has_superclass &&
            definition->package_name[0] == '\0' &&
            definition->instance_variable_count == 0U &&
            definition->class_comment[0] == '\0') {
            return class_object;
        }
    }
    if (class_object == 0) {
        if (dynamic_class_count >= DYNAMIC_CLASS_LIMIT) {
            machine_panic("dynamic class registry overflow");
        }
        class_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_CLASS);
        heap_set_field(class_handle, CLASS_FIELD_INSTANCE_KIND, small_integer_value((int32_t)RECORZ_MVP_OBJECT_OBJECT));
        heap_set_field(class_handle, CLASS_FIELD_METHOD_START, nil_value());
        heap_set_field(class_handle, CLASS_FIELD_METHOD_COUNT, small_integer_value(0));
        dynamic_definition = &dynamic_classes[dynamic_class_count++];
        class_object = (const struct recorz_mvp_heap_object *)heap_object(class_handle);
        machine_puts("recorz qemu-riscv32 mvp: created class ");
        machine_puts(definition->class_name);
        machine_putc('\n');
    } else {
        class_handle = heap_handle_for_object(class_object);
    }

    if (definition->has_superclass) {
        initialize_live_class_definition(&superclass_definition);
        source_copy_identifier(
            superclass_definition.class_name,
            sizeof(superclass_definition.class_name),
            definition->superclass_name
        );
        superclass_object = ensure_class_defined(&superclass_definition);
    } else {
        superclass_object = class_object_for_kind(RECORZ_MVP_OBJECT_OBJECT);
    }
    validate_dynamic_class_definition(definition, existing_dynamic_definition, superclass_object);
    heap_set_field(
        class_handle,
        CLASS_FIELD_SUPERCLASS,
        object_value(heap_handle_for_object(superclass_object))
    );
    (void)ensure_dedicated_metaclass_for_class(
        (const struct recorz_mvp_heap_object *)heap_object(class_handle),
        superclass_object
    );
    copy_dynamic_class_definition(
        dynamic_definition,
        definition,
        class_handle,
        heap_handle_for_object(superclass_object)
    );
    remember_package_definition(dynamic_definition->package_name, "", 0U);
    return (const struct recorz_mvp_heap_object *)heap_object(class_handle);
}

static uint32_t live_method_source_count_for_class_handle(uint16_t class_handle) {
    uint16_t source_index;
    uint32_t count = 0U;

    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        if (live_method_sources[source_index].class_handle == class_handle) {
            ++count;
        }
    }
    return count;
}

static void append_text_checked(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *text
) {
    uint32_t index = 0U;

    while (text[index] != '\0') {
        if (*offset + 1U >= buffer_size) {
            machine_panic("class file-out text exceeds buffer capacity");
        }
        buffer[(*offset)++] = text[index++];
    }
    buffer[*offset] = '\0';
}

static void append_char_checked(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    char ch
) {
    if (*offset + 1U >= buffer_size) {
        machine_panic("class file-out text exceeds buffer capacity");
    }
    buffer[(*offset)++] = ch;
    buffer[*offset] = '\0';
}

static int compare_method_source_records(
    const struct recorz_mvp_live_method_source *left,
    const struct recorz_mvp_live_method_source *right
) {
    const char *left_protocol = left->protocol_name;
    const char *right_protocol = right->protocol_name;
    const char *left_selector = selector_name(left->selector_id);
    const char *right_selector = selector_name(right->selector_id);
    uint32_t index = 0U;

    while (left_protocol[index] != '\0' && right_protocol[index] != '\0') {
        if (left_protocol[index] < right_protocol[index]) {
            return -1;
        }
        if (left_protocol[index] > right_protocol[index]) {
            return 1;
        }
        ++index;
    }
    if (left_protocol[index] == '\0' && right_protocol[index] != '\0') {
        return -1;
    }
    if (left_protocol[index] != '\0' && right_protocol[index] == '\0') {
        return 1;
    }
    index = 0U;
    while (left_selector[index] != '\0' && right_selector[index] != '\0') {
        if (left_selector[index] < right_selector[index]) {
            return -1;
        }
        if (left_selector[index] > right_selector[index]) {
            return 1;
        }
        ++index;
    }
    if (left_selector[index] == '\0' && right_selector[index] != '\0') {
        return -1;
    }
    if (left_selector[index] != '\0' && right_selector[index] == '\0') {
        return 1;
    }
    if (left->argument_count < right->argument_count) {
        return -1;
    }
    if (left->argument_count > right->argument_count) {
        return 1;
    }
    return 0;
}

static int compare_source_names(const char *left, const char *right) {
    uint32_t index = 0U;

    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] < right[index]) {
            return -1;
        }
        if (left[index] > right[index]) {
            return 1;
        }
        ++index;
    }
    if (left[index] == '\0' && right[index] != '\0') {
        return -1;
    }
    if (left[index] != '\0' && right[index] == '\0') {
        return 1;
    }
    return 0;
}

static void append_chunk_text(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *chunk_text,
    uint8_t *wrote_any_chunk
) {
    append_text_checked(buffer, buffer_size, offset, chunk_text);
    append_text_checked(buffer, buffer_size, offset, "\n!\n");
    *wrote_any_chunk = 1U;
}

static void append_protocol_chunk(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *protocol_name,
    uint8_t *wrote_any_chunk
) {
    append_text_checked(buffer, buffer_size, offset, "RecorzKernelProtocol: '");
    append_text_checked(buffer, buffer_size, offset, protocol_name);
    append_text_checked(buffer, buffer_size, offset, "'\n!\n");
    *wrote_any_chunk = 1U;
}

static void append_dynamic_class_header_source(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const struct recorz_mvp_dynamic_class_definition *definition
) {
    uint8_t ivar_index;

    append_text_checked(buffer, buffer_size, offset, "RecorzKernelClass: #");
    append_text_checked(buffer, buffer_size, offset, definition->class_name);
    append_text_checked(buffer, buffer_size, offset, " superclass: #");
    append_text_checked(
        buffer,
        buffer_size,
        offset,
        class_name_for_object(heap_object(definition->superclass_handle))
    );
    if (definition->package_name[0] != '\0') {
        append_text_checked(buffer, buffer_size, offset, " package: '");
        append_text_checked(buffer, buffer_size, offset, definition->package_name);
        append_char_checked(buffer, buffer_size, offset, '\'');
    }
    if (definition->class_comment[0] != '\0') {
        append_text_checked(buffer, buffer_size, offset, " comment: '");
        append_text_checked(buffer, buffer_size, offset, definition->class_comment);
        append_char_checked(buffer, buffer_size, offset, '\'');
    }
    append_text_checked(buffer, buffer_size, offset, " instanceVariableNames: '");
    for (ivar_index = 0U; ivar_index < definition->instance_variable_count; ++ivar_index) {
        if (ivar_index != 0U) {
            append_char_checked(buffer, buffer_size, offset, ' ');
        }
        append_text_checked(buffer, buffer_size, offset, definition->instance_variable_names[ivar_index]);
    }
    append_char_checked(buffer, buffer_size, offset, '\'');
}

static void append_live_method_source_chunks(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    uint16_t class_handle,
    uint8_t *wrote_any_chunk
) {
    const struct recorz_mvp_live_method_source *sorted_sources[LIVE_METHOD_SOURCE_LIMIT];
    char current_protocol[METHOD_SOURCE_NAME_LIMIT];
    uint16_t source_index;
    uint16_t sorted_count = 0U;

    current_protocol[0] = '\0';
    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        const struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];
        uint16_t insert_index;

        if (source_record->class_handle != class_handle) {
            continue;
        }
        insert_index = sorted_count;
        while (insert_index != 0U &&
               compare_method_source_records(source_record, sorted_sources[insert_index - 1U]) < 0) {
            sorted_sources[insert_index] = sorted_sources[insert_index - 1U];
            --insert_index;
        }
        sorted_sources[insert_index] = source_record;
        ++sorted_count;
    }
    for (source_index = 0U; source_index < sorted_count; ++source_index) {
        if (!source_names_equal(current_protocol, sorted_sources[source_index]->protocol_name)) {
            source_copy_identifier(current_protocol, sizeof(current_protocol), sorted_sources[source_index]->protocol_name);
            if (current_protocol[0] != '\0') {
                append_protocol_chunk(buffer, buffer_size, offset, current_protocol, wrote_any_chunk);
            }
        }
        append_chunk_text(
            buffer,
            buffer_size,
            offset,
            live_method_source_text(sorted_sources[source_index]),
            wrote_any_chunk
        );
    }
}

static const struct recorz_mvp_live_method_source *live_method_source_for_selector_and_arity(
    uint16_t class_handle,
    uint8_t selector_id,
    uint8_t argument_count
) {
    uint16_t source_index;

    for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
        const struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];

        if (source_record->class_handle == class_handle &&
            source_record->selector_id == selector_id &&
            source_record->argument_count == argument_count) {
            return source_record;
        }
    }
    return 0;
}

static const char *file_out_class_source_by_name(const char *class_name) {
    const struct recorz_mvp_heap_object *class_object;
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition;
    const struct recorz_mvp_heap_object *metaclass_object;
    uint16_t class_handle;
    uint16_t metaclass_handle;
    uint32_t offset = 0U;
    uint8_t wrote_any_chunk = 0U;

    class_object = lookup_class_by_name(class_name);
    if (class_object == 0) {
        machine_panic("KernelInstaller fileOutClassNamed: could not resolve class");
    }
    class_handle = heap_handle_for_object(class_object);
    dynamic_definition = dynamic_class_definition_for_handle(class_handle);
    if (dynamic_definition == 0) {
        machine_panic("KernelInstaller fileOutClassNamed: only live classes support file-out in the MVP");
    }
    if (live_method_source_count_for_class_handle(class_handle) != class_method_count(class_object)) {
        machine_panic("KernelInstaller fileOutClassNamed: class contains methods without live source");
    }
    kernel_source_io_buffer[0] = '\0';
    append_dynamic_class_header_source(
        kernel_source_io_buffer,
        sizeof(kernel_source_io_buffer),
        &offset,
        dynamic_definition
    );
    append_text_checked(kernel_source_io_buffer, sizeof(kernel_source_io_buffer), &offset, "\n!\n");
    append_live_method_source_chunks(
        kernel_source_io_buffer,
        sizeof(kernel_source_io_buffer),
        &offset,
        class_handle,
        &wrote_any_chunk
    );

    metaclass_object = class_side_lookup_target(class_object);
    metaclass_handle = heap_handle_for_object(metaclass_object);
    if (live_method_source_count_for_class_handle(metaclass_handle) != class_method_count(metaclass_object)) {
        machine_panic("KernelInstaller fileOutClassNamed: metaclass contains methods without live source");
    }
    if (class_method_count(metaclass_object) != 0U) {
        append_text_checked(
            kernel_source_io_buffer,
            sizeof(kernel_source_io_buffer),
            &offset,
            "RecorzKernelClassSide: #"
        );
        append_text_checked(
            kernel_source_io_buffer,
            sizeof(kernel_source_io_buffer),
            &offset,
            dynamic_definition->class_name
        );
        append_text_checked(kernel_source_io_buffer, sizeof(kernel_source_io_buffer), &offset, "\n!\n");
        append_live_method_source_chunks(
            kernel_source_io_buffer,
            sizeof(kernel_source_io_buffer),
            &offset,
            metaclass_handle,
            &wrote_any_chunk
        );
    }
    return runtime_string_allocate_copy(kernel_source_io_buffer);
}

static const char *file_out_package_source_by_name(const char *package_name) {
    static char package_file_out_buffer[8192];
    const struct recorz_mvp_live_package_definition *package_definition;
    const struct recorz_mvp_dynamic_class_definition *sorted_definitions[DYNAMIC_CLASS_LIMIT];
    uint16_t sorted_count = 0U;
    uint16_t dynamic_index;
    uint32_t offset = 0U;

    if (package_name == 0 || package_name[0] == '\0') {
        machine_panic("KernelInstaller fileOutPackageNamed: package name is empty");
    }
    package_definition = package_definition_for_name(package_name);
    if (package_definition == 0) {
        machine_panic("KernelInstaller fileOutPackageNamed: could not resolve package");
    }
    package_file_out_buffer[0] = '\0';
    append_text_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, "RecorzKernelPackage: '");
    append_text_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, package_name);
    append_char_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, '\'');
    if (package_definition->package_comment[0] != '\0') {
        append_text_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, " comment: '");
        append_text_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, package_definition->package_comment);
        append_char_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, '\'');
    }
    append_text_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, "\n!\n");
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        const struct recorz_mvp_dynamic_class_definition *definition = &dynamic_classes[dynamic_index];
        uint16_t insert_index;

        if (!source_names_equal(definition->package_name, package_name)) {
            continue;
        }
        insert_index = sorted_count;
        while (insert_index != 0U &&
               compare_source_names(
                   definition->class_name,
                   sorted_definitions[insert_index - 1U]->class_name) < 0) {
            sorted_definitions[insert_index] = sorted_definitions[insert_index - 1U];
            --insert_index;
        }
        sorted_definitions[insert_index] = definition;
        ++sorted_count;
    }
    for (dynamic_index = 0U; dynamic_index < sorted_count; ++dynamic_index) {
        const struct recorz_mvp_dynamic_class_definition *definition = sorted_definitions[dynamic_index];
        const char *class_source = file_out_class_source_by_name(definition->class_name);

        append_text_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, class_source);
        if (dynamic_index + 1U < sorted_count &&
            package_file_out_buffer[offset - 1U] != '\n') {
            append_char_checked(package_file_out_buffer, sizeof(package_file_out_buffer), &offset, '\n');
        }
    }
    return runtime_string_allocate_copy(package_file_out_buffer);
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
    if (selector_id == 0U || selector_id > MAX_SELECTOR_ID) {
        machine_panic("KernelInstaller selectorId is out of range");
    }
    if (argument_count > MAX_SEND_ARGS) {
        machine_panic("KernelInstaller argumentCount exceeds send capacity");
    }
    validate_compiled_method(compiled_method, argument_count);
    forget_live_method_source(heap_handle_for_object(class_object), (uint8_t)selector_id, (uint8_t)argument_count);
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
    install_method_source_on_class(class_object, arguments[0].string);
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
    uint8_t found_class = 0U;

    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller fileInClassChunks: expects a source string");
    }
    cursor = arguments[0].string;
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        if (source_starts_with(chunk, "RecorzKernelPackage:")) {
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            struct recorz_mvp_live_class_definition definition;

            source_parse_class_definition_from_chunk(chunk, &definition);
            source_copy_identifier(class_name, sizeof(class_name), definition.class_name);
            found_class = 1U;
            break;
        }
        if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
            source_parse_class_side_name_from_chunk(chunk, class_name, sizeof(class_name));
            found_class = 1U;
            break;
        }
        machine_panic("KernelInstaller fileInClassChunks: source is missing an initial class header");
    }
    if (!found_class) {
        machine_panic("KernelInstaller fileInClassChunks: source is missing an initial class header");
    }
    file_in_class_chunks_source(arguments[0].string);
    class_object = lookup_class_by_name(class_name);
    if (class_object == 0) {
        machine_panic("KernelInstaller fileInClassChunks: class could not be resolved after install");
    }
    push(object_value(heap_handle_for_object(class_object)));
}

static void file_in_class_chunks_source(const char *source) {
    file_in_chunk_stream_source(source);
}

static void file_in_chunk_stream_source(const char *source) {
    const char *cursor = source;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    const struct recorz_mvp_heap_object *class_object = 0;
    const struct recorz_mvp_heap_object *install_class_object = 0;
    char current_package[METHOD_SOURCE_NAME_LIMIT];
    char current_protocol[METHOD_SOURCE_NAME_LIMIT];
    uint8_t package_chunk_count = 0U;
    uint8_t class_header_count = 0U;
    uint8_t do_it_chunk_count = 0U;

    if (source == 0 || *source == '\0') {
        machine_panic("KernelInstaller fileInClassChunks: source is empty");
    }
    current_package[0] = '\0';
    current_protocol[0] = '\0';
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        if (source_starts_with(chunk, "RecorzKernelPackage:")) {
            struct recorz_mvp_live_package_definition package_definition;
            uint8_t has_comment;

            source_parse_package_definition_from_chunk(chunk, &package_definition, &has_comment);
            source_copy_identifier(current_package, sizeof(current_package), package_definition.package_name);
            remember_package_definition(package_definition.package_name, package_definition.package_comment, has_comment);
            current_protocol[0] = '\0';
            ++package_chunk_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            struct recorz_mvp_live_class_definition definition;

            source_parse_class_definition_from_chunk(chunk, &definition);
            if (definition.package_name[0] == '\0' && current_package[0] != '\0') {
                source_copy_identifier(definition.package_name, sizeof(definition.package_name), current_package);
            }
            class_object = ensure_class_defined(&definition);
            install_class_object = class_object;
            current_protocol[0] = '\0';
            ++class_header_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
            char class_name[METHOD_SOURCE_NAME_LIMIT];

            source_parse_class_side_name_from_chunk(chunk, class_name, sizeof(class_name));
            class_object = lookup_class_by_name(class_name);
            if (class_object == 0) {
                machine_panic("KernelInstaller class-side chunk could not resolve class");
            }
            install_class_object = ensure_dedicated_metaclass_for_class(
                class_object,
                class_superclass_object_or_null(class_object)
            );
            current_protocol[0] = '\0';
            ++class_header_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelProtocol:")) {
            if (install_class_object == 0) {
                machine_panic("KernelInstaller protocol chunk has no active class side");
            }
            source_parse_protocol_name_from_chunk(chunk, current_protocol, sizeof(current_protocol));
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelDoIt:")) {
            workspace_evaluate_source(source_parse_do_it_chunk_body(chunk));
            current_protocol[0] = '\0';
            ++do_it_chunk_count;
            continue;
        }
        if (install_class_object == 0) {
            machine_panic("KernelInstaller file-in stream is missing an initial RecorzKernelClass chunk");
        }
        install_method_chunk_on_class(install_class_object, current_protocol, chunk);
    }
    if (class_header_count == 0U && do_it_chunk_count == 0U && package_chunk_count == 0U) {
        machine_panic("KernelInstaller file-in stream contains no package, class, or do-it chunks");
    }
}

static void apply_external_file_in_blob(const uint8_t *blob, uint32_t size) {
    uint32_t index;

    if (blob == 0 || size == 0U) {
        return;
    }
    if (size >= sizeof(kernel_source_io_buffer)) {
        machine_panic("external file-in payload exceeds buffer capacity");
    }
    for (index = 0U; index < size; ++index) {
        if (blob[index] == '\0') {
            machine_panic("external file-in payload contains an unexpected NUL byte");
        }
        kernel_source_io_buffer[index] = (char)blob[index];
    }
    kernel_source_io_buffer[size] = '\0';
    file_in_chunk_stream_source(kernel_source_io_buffer);
}

static void execute_entry_kernel_installer_remember_object_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller rememberObject:named: expects an object");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("KernelInstaller rememberObject:named: expects a name string");
    }
    remember_named_object_handle((uint16_t)arguments[0].integer, arguments[1].string);
    push(receiver);
}

static void execute_entry_kernel_installer_object_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t object_handle;

    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller objectNamed: expects a name string");
    }
    object_handle = named_object_handle_for_name(arguments[0].string);
    if (object_handle == 0U) {
        machine_panic("KernelInstaller objectNamed: could not resolve object");
    }
    push(object_value(object_handle));
}

static void execute_entry_kernel_installer_save_snapshot(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)arguments;
    (void)text;
    emit_live_snapshot();
    push(receiver);
}

static void execute_entry_kernel_installer_file_out_package_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("KernelInstaller fileOutPackageNamed: expects a package name string");
    }
    push(string_value(file_out_package_source_by_name(arguments[0].string)));
}

static void execute_entry_kernel_installer_memory_report(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(string_value(kernel_memory_report_text()));
}

static void execute_entry_kernel_installer_configure_startup_selector_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint8_t selector_id;

    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller configureStartup:selectorNamed: expects an object");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("KernelInstaller configureStartup:selectorNamed: expects a selector name");
    }
    selector_id = source_selector_id_for_name(arguments[1].string);
    if (selector_id == 0U) {
        machine_panic("KernelInstaller configureStartup:selectorNamed: selector is not declared");
    }
    startup_hook_receiver_handle = (uint16_t)arguments[0].integer;
    startup_hook_selector_id = selector_id;
    push(receiver);
}

static void execute_entry_kernel_installer_clear_startup(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)arguments;
    (void)text;
    startup_hook_receiver_handle = 0U;
    startup_hook_selector_id = 0U;
    push(receiver);
}

static void execute_entry_workspace_file_in(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace fileIn: expects a chunk stream string");
    }
    file_in_chunk_stream_source(arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_file_out_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const char *source;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace fileOutClassNamed: expects a class name string");
    }
    source = file_out_class_source_by_name(arguments[0].string);
    workspace_remember_current_source(object, source);
    workspace_remember_view(object, WORKSPACE_VIEW_CLASS_SOURCE, arguments[0].string);
    workspace_render_class_source_browser(object, arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_file_out_package_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const char *source;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace fileOutPackageNamed: expects a package name string");
    }
    source = file_out_package_source_by_name(arguments[0].string);
    workspace_remember_current_source(object, source);
    workspace_remember_view(object, WORKSPACE_VIEW_PACKAGE_SOURCE, arguments[0].string);
    workspace_render_package_source_browser(object, arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_contents(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)receiver;
    (void)arguments;
    (void)text;
    push(workspace_current_source_value(object));
}

static void execute_entry_workspace_set_contents(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace setContents: expects a source string");
    }
    workspace_remember_current_source(object, arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_evaluate(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const char *chunk_source;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace evaluate: expects a source string");
    }
    chunk_source = workspace_normalize_do_it_source(arguments[0].string);
    workspace_remember_current_source(object, chunk_source);
    workspace_remember_source(object, chunk_source);
    workspace_evaluate_source(workspace_source_for_evaluation(chunk_source));
    push(receiver);
}

static void execute_entry_workspace_evaluate_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value source_value;
    const char *chunk_source;

    (void)arguments;
    (void)text;
    source_value = workspace_current_source_value(object);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        machine_panic("Workspace evaluateCurrent has no current source");
    }
    chunk_source = workspace_normalize_do_it_source(source_value.string);
    workspace_remember_current_source(object, chunk_source);
    workspace_remember_source(object, chunk_source);
    workspace_evaluate_source(workspace_source_for_evaluation(chunk_source));
    push(receiver);
}

static void execute_entry_workspace_file_in_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value source_value;
    const struct recorz_mvp_heap_object *class_object;

    (void)arguments;
    (void)text;
    source_value = workspace_current_source_value(object);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        machine_panic("Workspace fileInCurrent has no current source");
    }
    if (source_starts_with(source_value.string, "RecorzKernelClass:") ||
        source_starts_with(source_value.string, "RecorzKernelClassSide:") ||
        source_starts_with(source_value.string, "RecorzKernelPackage:") ||
        source_starts_with(source_value.string, "RecorzKernelDoIt:")) {
        file_in_chunk_stream_source(source_value.string);
        push(receiver);
        return;
    }
    class_object = workspace_target_class_for_file_in(object);
    if (class_object == 0) {
        machine_panic("Workspace fileInCurrent has no target class");
    }
    install_method_source_on_class(class_object, source_value.string);
    push(receiver);
}

static void execute_entry_workspace_accept_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value source_value;
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const struct recorz_mvp_heap_object *class_object;
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_text[METHOD_SOURCE_NAME_LIMIT];

    (void)arguments;
    (void)text;
    source_value = workspace_current_source_value(object);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        machine_panic("Workspace acceptCurrent has no current source");
    }
    if (object->field_count <= workspace_current_view_kind_field_index(object)) {
        machine_panic("Workspace acceptCurrent has no current browser target");
    }
    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Workspace acceptCurrent requires a method browser target");
    }
    if ((uint32_t)view_kind_value.integer != WORKSPACE_VIEW_METHOD &&
        (uint32_t)view_kind_value.integer != WORKSPACE_VIEW_CLASS_METHOD) {
        machine_panic("Workspace acceptCurrent requires a method browser target");
    }
    class_object = workspace_target_class_for_file_in(object);
    if (class_object == 0) {
        machine_panic("Workspace acceptCurrent could not resolve the target class");
    }
    if (object->field_count <= workspace_current_target_name_field_index(object)) {
        machine_panic("Workspace acceptCurrent is missing the current method target");
    }
    target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
    if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        machine_panic("Workspace acceptCurrent is missing the current method target");
    }
    if (!workspace_parse_method_target_name(
            target_name_value.string,
            class_name,
            sizeof(class_name),
            selector_name_text,
            sizeof(selector_name_text))) {
        machine_panic("Workspace acceptCurrent current method target is invalid");
    }
    install_method_source_on_class(class_object, source_value.string);
    workspace_render_method_browser(
        object,
        class_name,
        selector_name_text,
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHOD ? "INST" : "CLASS"
    );
    push(receiver);
}

static void execute_entry_workspace_browse_classes(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_remember_view(object, WORKSPACE_VIEW_CLASSES, 0);
    workspace_render_class_list_browser(object);
    push(receiver);
}

static void execute_entry_workspace_browse_packages(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_remember_view(object, WORKSPACE_VIEW_PACKAGES, 0);
    workspace_render_package_list_browser(object);
    push(receiver);
}

static void execute_entry_workspace_browse_methods_for_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseMethodsForClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("Workspace browseMethodsForClassNamed: could not resolve class");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_METHODS, arguments[0].string);
    workspace_render_method_list_browser(object, class_object, arguments[0].string, "INST");
    push(receiver);
}

static void execute_entry_workspace_browse_package_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browsePackageNamed: expects a package name string");
    }
    if (package_definition_for_name(arguments[0].string) == 0) {
        machine_panic("Workspace browsePackageNamed: could not resolve package");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_PACKAGE, arguments[0].string);
    workspace_render_package_browser(object, arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_browse_protocols_for_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseProtocolsForClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("Workspace browseProtocolsForClassNamed: could not resolve class");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_PROTOCOLS, arguments[0].string);
    workspace_render_protocol_list_browser(object, class_object, arguments[0].string, "INST");
    push(receiver);
}

static void execute_entry_workspace_browse_protocol_of_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseProtocol:ofClassNamed: expects a protocol name string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("Workspace browseProtocol:ofClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[1].string);
    if (class_object == 0) {
        machine_panic("Workspace browseProtocol:ofClassNamed: could not resolve class");
    }
    workspace_remember_view(
        object,
        WORKSPACE_VIEW_PROTOCOL,
        workspace_compose_protocol_target_name(arguments[1].string, arguments[0].string)
    );
    workspace_render_protocol_method_list_browser(
        object,
        class_object,
        arguments[1].string,
        "INST",
        arguments[0].string
    );
    push(receiver);
}

static void execute_entry_workspace_browse_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("Workspace browseClassNamed: could not resolve class");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_CLASS, arguments[0].string);
    workspace_render_class_browser(object, class_object, arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_browse_object_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t object_handle;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseObjectNamed: expects an object name string");
    }
    object_handle = named_object_handle_for_name(arguments[0].string);
    if (object_handle == 0U) {
        machine_panic("Workspace browseObjectNamed: could not resolve object");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_OBJECT, arguments[0].string);
    workspace_render_object_browser(object, arguments[0].string, heap_object(object_handle));
    push(receiver);
}

static void execute_entry_workspace_browse_method_of_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    const struct recorz_mvp_live_method_source *source_record;
    uint8_t selector_id;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseMethod:ofClassNamed: expects a selector name string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("Workspace browseMethod:ofClassNamed: expects a class name string");
    }
    selector_id = source_selector_id_for_name(arguments[0].string);
    if (selector_id == 0U) {
        machine_panic("Workspace browseMethod:ofClassNamed: selector is not declared");
    }
    class_object = lookup_class_by_name(arguments[1].string);
    if (class_object == 0) {
        machine_panic("Workspace browseMethod:ofClassNamed: could not resolve class");
    }
    if (lookup_builtin_method_descriptor(class_object, selector_id, 0U) == 0 &&
        lookup_builtin_method_descriptor(class_object, selector_id, 1U) == 0) {
        machine_panic("Workspace browseMethod:ofClassNamed: could not resolve method");
    }
    source_record = live_method_source_for(heap_handle_for_object(class_object), selector_id);
    if (source_record != 0) {
        workspace_remember_current_source(
            object,
            live_method_source_text(source_record)
        );
    }
    workspace_remember_view(
        object,
        WORKSPACE_VIEW_METHOD,
        workspace_compose_method_target_name(arguments[1].string, arguments[0].string)
    );
    workspace_render_method_browser(object, arguments[1].string, arguments[0].string, "INST");
    push(receiver);
}

static void execute_entry_workspace_browse_class_methods_for_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseClassMethodsForClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("Workspace browseClassMethodsForClassNamed: could not resolve class");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_CLASS_METHODS, arguments[0].string);
    workspace_render_method_list_browser(
        object,
        class_side_lookup_target(class_object),
        arguments[0].string,
        "CLASS"
    );
    push(receiver);
}

static void execute_entry_workspace_browse_class_protocols_for_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseClassProtocolsForClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("Workspace browseClassProtocolsForClassNamed: could not resolve class");
    }
    workspace_remember_view(object, WORKSPACE_VIEW_CLASS_PROTOCOLS, arguments[0].string);
    workspace_render_protocol_list_browser(
        object,
        class_side_lookup_target(class_object),
        arguments[0].string,
        "CLASS"
    );
    push(receiver);
}

static void execute_entry_workspace_browse_class_protocol_of_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseClassProtocol:ofClassNamed: expects a protocol name string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("Workspace browseClassProtocol:ofClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[1].string);
    if (class_object == 0) {
        machine_panic("Workspace browseClassProtocol:ofClassNamed: could not resolve class");
    }
    workspace_remember_view(
        object,
        WORKSPACE_VIEW_CLASS_PROTOCOL,
        workspace_compose_protocol_target_name(arguments[1].string, arguments[0].string)
    );
    workspace_render_protocol_method_list_browser(
        object,
        class_side_lookup_target(class_object),
        arguments[1].string,
        "CLASS",
        arguments[0].string
    );
    push(receiver);
}

static void execute_entry_workspace_browse_class_method_of_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    const struct recorz_mvp_heap_object *metaclass_object;
    const struct recorz_mvp_live_method_source *source_record;
    uint8_t selector_id;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace browseClassMethod:ofClassNamed: expects a selector name string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("Workspace browseClassMethod:ofClassNamed: expects a class name string");
    }
    selector_id = source_selector_id_for_name(arguments[0].string);
    if (selector_id == 0U) {
        machine_panic("Workspace browseClassMethod:ofClassNamed: selector is not declared");
    }
    class_object = lookup_class_by_name(arguments[1].string);
    if (class_object == 0) {
        machine_panic("Workspace browseClassMethod:ofClassNamed: could not resolve class");
    }
    metaclass_object = class_side_lookup_target(class_object);
    if (lookup_builtin_method_descriptor(metaclass_object, selector_id, 0U) == 0 &&
        lookup_builtin_method_descriptor(metaclass_object, selector_id, 1U) == 0) {
        machine_panic("Workspace browseClassMethod:ofClassNamed: could not resolve class-side method");
    }
    source_record = live_method_source_for(heap_handle_for_object(metaclass_object), selector_id);
    if (source_record != 0) {
        workspace_remember_current_source(
            object,
            live_method_source_text(source_record)
        );
    }
    workspace_remember_view(
        object,
        WORKSPACE_VIEW_CLASS_METHOD,
        workspace_compose_method_target_name(arguments[1].string, arguments[0].string)
    );
    workspace_render_method_browser(object, arguments[1].string, arguments[0].string, "CLASS");
    push(receiver);
}

static void execute_entry_workspace_rerun(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value source_value;

    (void)arguments;
    (void)text;
    source_value = heap_get_field(object, workspace_last_source_field_index(object));
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        machine_panic("Workspace rerun has no remembered source");
    }
    workspace_evaluate_source(workspace_source_for_evaluation(source_value.string));
    push(receiver);
}

static void execute_entry_workspace_reopen(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *class_object;

    (void)arguments;
    (void)text;
    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        form_clear(form);
        form_write_string(form, "WORKSPACE");
        form_newline(form);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASSES) {
        workspace_render_class_list_browser(object);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGES) {
        workspace_render_package_list_browser(object);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHODS) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered method-list class");
        }
        class_object = lookup_class_by_name(target_name_value.string);
        if (class_object == 0) {
            machine_panic("Workspace reopen could not resolve the remembered method-list class");
        }
        workspace_render_method_list_browser(object, class_object, target_name_value.string, "INST");
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PROTOCOLS) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered protocol-list class");
        }
        class_object = lookup_class_by_name(target_name_value.string);
        if (class_object == 0) {
            machine_panic("Workspace reopen could not resolve the remembered protocol-list class");
        }
        workspace_render_protocol_list_browser(object, class_object, target_name_value.string, "INST");
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PROTOCOL) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char protocol_name[METHOD_SOURCE_NAME_LIMIT];

        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered protocol target");
        }
        if (!workspace_parse_protocol_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                protocol_name,
                sizeof(protocol_name))) {
            machine_panic("Workspace reopen remembered protocol target is invalid");
        }
        class_object = lookup_class_by_name(class_name);
        if (class_object == 0) {
            machine_panic("Workspace reopen could not resolve the remembered protocol class");
        }
        workspace_render_protocol_method_list_browser(object, class_object, class_name, "INST", protocol_name);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHODS) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered class-side method-list class");
        }
        class_object = lookup_class_by_name(target_name_value.string);
        if (class_object == 0) {
            machine_panic("Workspace reopen could not resolve the remembered class-side method-list class");
        }
        workspace_render_method_list_browser(
            object,
            class_side_lookup_target(class_object),
            target_name_value.string,
            "CLASS"
        );
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOLS) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered class-side protocol-list class");
        }
        class_object = lookup_class_by_name(target_name_value.string);
        if (class_object == 0) {
            machine_panic("Workspace reopen could not resolve the remembered class-side protocol-list class");
        }
        workspace_render_protocol_list_browser(
            object,
            class_side_lookup_target(class_object),
            target_name_value.string,
            "CLASS"
        );
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOL) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char protocol_name[METHOD_SOURCE_NAME_LIMIT];

        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered class-side protocol target");
        }
        if (!workspace_parse_protocol_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                protocol_name,
                sizeof(protocol_name))) {
            machine_panic("Workspace reopen remembered class-side protocol target is invalid");
        }
        class_object = lookup_class_by_name(class_name);
        if (class_object == 0) {
            machine_panic("Workspace reopen could not resolve the remembered class-side protocol class");
        }
        workspace_render_protocol_method_list_browser(
            object,
            class_side_lookup_target(class_object),
            class_name,
            "CLASS",
            protocol_name
        );
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_OBJECT) {
        uint16_t object_handle;

        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered object name");
        }
        object_handle = named_object_handle_for_name(target_name_value.string);
        if (object_handle == 0U) {
            machine_panic("Workspace reopen could not resolve the remembered object");
        }
        workspace_render_object_browser(object, target_name_value.string, heap_object(object_handle));
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_SOURCE) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered class source target");
        }
        workspace_render_class_source_browser(object, target_name_value.string);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered package source target");
        }
        workspace_render_package_source_browser(object, target_name_value.string);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE) {
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered package target");
        }
        if (package_definition_for_name(target_name_value.string) == 0) {
            machine_panic("Workspace reopen could not resolve the remembered package");
        }
        workspace_render_package_browser(object, target_name_value.string);
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHOD) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char selector_name_text[METHOD_SOURCE_NAME_LIMIT];

        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered method target");
        }
        if (!workspace_parse_method_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                selector_name_text,
                sizeof(selector_name_text))) {
            machine_panic("Workspace reopen remembered method target is invalid");
        }
        workspace_render_method_browser(object, class_name, selector_name_text, "INST");
        push(receiver);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHOD) {
        char class_name[METHOD_SOURCE_NAME_LIMIT];
        char selector_name_text[METHOD_SOURCE_NAME_LIMIT];

        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace reopen is missing the remembered class-side method target");
        }
        if (!workspace_parse_method_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                selector_name_text,
                sizeof(selector_name_text))) {
            machine_panic("Workspace reopen remembered class-side method target is invalid");
        }
        workspace_render_method_browser(object, class_name, selector_name_text, "CLASS");
        push(receiver);
        return;
    }
    target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
    if ((uint32_t)view_kind_value.integer != WORKSPACE_VIEW_CLASS ||
        target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        form_clear(form);
        form_write_string(form, "WORKSPACE");
        form_newline(form);
        push(receiver);
        return;
    }
    class_object = lookup_class_by_name(target_name_value.string);
    if (class_object == 0) {
        machine_panic("Workspace reopen could not resolve the remembered class");
    }
    workspace_render_class_browser(object, class_object, target_name_value.string);
    push(receiver);
}

static const recorz_mvp_method_entry_handler primitive_binding_handlers[RECORZ_MVP_PRIMITIVE_COUNT] = {
    RECORZ_MVP_GENERATED_PRIMITIVE_BINDING_HANDLERS
};

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

static uint8_t executable_uses_this_context(const struct recorz_mvp_executable *executable) {
    uint32_t instruction_index;

    for (instruction_index = 0U; instruction_index < executable->instruction_count; ++instruction_index) {
        struct recorz_mvp_instruction instruction =
            executable->read_instruction(executable->instruction_source, instruction_index);

        if (instruction.opcode == RECORZ_MVP_OP_PUSH_THIS_CONTEXT) {
            return 1U;
        }
    }
    return 0U;
}

static uint16_t allocate_compiled_activation_context_if_needed(
    const struct recorz_mvp_executable *executable,
    uint16_t sender_context_handle,
    struct recorz_mvp_value receiver,
    const char *detail_text
) {
    if (sender_context_handle == 0U && !executable_uses_this_context(executable)) {
        return 0U;
    }
    return allocate_source_context_object(sender_context_handle, receiver, detail_text);
}

static int16_t ensure_executable_lexical_environment(
    const struct recorz_mvp_executable *executable,
    const struct recorz_mvp_value lexical[],
    int16_t lexical_environment_index
) {
    uint16_t lexical_index;

    if (lexical_environment_index >= 0 ||
        executable->lexical_names == 0 ||
        executable->lexical_count == 0U) {
        return lexical_environment_index;
    }
    lexical_environment_index = source_allocate_lexical_environment(-1);
    for (lexical_index = 0U; lexical_index < executable->lexical_count; ++lexical_index) {
        source_append_binding(
            lexical_environment_index,
            executable->lexical_names[lexical_index],
            lexical[lexical_index]
        );
    }
    return lexical_environment_index;
}

static void mark_context_dead(uint16_t context_handle) {
    if (context_handle != 0U) {
        heap_set_field(context_handle, CONTEXT_FIELD_ALIVE, boolean_value(0U));
    }
}

static void execute_executable(
    const struct recorz_mvp_executable *executable,
    const struct recorz_mvp_heap_object *receiver_object,
    struct recorz_mvp_value receiver,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t current_context_handle
) {
    struct recorz_mvp_value activation_stack[STACK_LIMIT];
    struct recorz_mvp_value lexical[LEXICAL_LIMIT];
    struct recorz_mvp_source_lexical_environment *shared_lexical_environment = 0;
    uint32_t activation_stack_size = 0U;
    uint32_t lexical_index;
    uint32_t pc = 0U;
    int16_t shared_lexical_environment_index = -1;

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
                if (shared_lexical_environment != 0) {
                    activation_push(
                        activation_stack,
                        &activation_stack_size,
                        shared_lexical_environment->bindings[instruction.operand_b].value
                    );
                } else {
                    activation_push(activation_stack, &activation_stack_size, lexical[instruction.operand_b]);
                }
                break;
            case RECORZ_MVP_OP_STORE_LEXICAL:
                if ((uint32_t)instruction.operand_b >= executable->lexical_count) {
                    machine_panic("lexical write out of range");
                }
                if (shared_lexical_environment != 0) {
                    shared_lexical_environment->bindings[instruction.operand_b].value =
                        activation_pop(activation_stack, &activation_stack_size);
                } else {
                    lexical[instruction.operand_b] = activation_pop(activation_stack, &activation_stack_size);
                }
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
            case RECORZ_MVP_OP_PUSH_SELF:
                activation_push(activation_stack, &activation_stack_size, receiver);
                break;
            case RECORZ_MVP_OP_PUSH_THIS_CONTEXT:
                if (current_context_handle == 0U) {
                    machine_panic("thisContext requires an activation context");
                }
                activation_push(activation_stack, &activation_stack_size, object_value(current_context_handle));
                break;
            case RECORZ_MVP_OP_PUSH_SMALL_INTEGER:
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                    small_integer_value((int16_t)instruction.operand_b)
                );
                break;
            case RECORZ_MVP_OP_PUSH_STRING_LITERAL:
                if (instruction.operand_b == 0U ||
                    instruction.operand_b > LIVE_STRING_LITERAL_LIMIT ||
                    live_string_literals[instruction.operand_b - 1U].text == 0) {
                    machine_panic("string literal slot is out of range");
                }
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                    string_value(live_string_literals[instruction.operand_b - 1U].text)
                );
                break;
            case RECORZ_MVP_OP_PUSH_BLOCK_LITERAL:
                {
                    uint16_t block_handle;
                    const struct recorz_mvp_heap_object *block_defining_class = executable->block_defining_class;

                if ((uint32_t)instruction.operand_b >= executable->literal_count) {
                    machine_panic("block literal is out of range");
                }
                if (executable->literals[instruction.operand_b].kind != RECORZ_MVP_LITERAL_STRING ||
                    executable->literals[instruction.operand_b].string == 0) {
                    machine_panic("block literal source must be a string");
                }
                    block_handle = allocate_block_closure_from_source(
                        executable->literals[instruction.operand_b].string,
                        receiver
                    );
                    if (block_defining_class == 0) {
                        if (receiver.kind == RECORZ_MVP_VALUE_OBJECT) {
                            block_defining_class = class_object_for_heap_object(heap_object_for_value(receiver));
                        } else {
                            block_defining_class = class_object_for_kind(RECORZ_MVP_OBJECT_OBJECT);
                        }
                    }
                    shared_lexical_environment_index = ensure_executable_lexical_environment(
                        executable,
                        lexical,
                        shared_lexical_environment_index
                    );
                    if (shared_lexical_environment_index >= 0) {
                        shared_lexical_environment =
                            source_lexical_environment_at(shared_lexical_environment_index);
                    }
                    source_register_block_state(
                        block_handle,
                        block_defining_class,
                        shared_lexical_environment_index,
                        executable->home_context_index
                    );
                activation_push(
                    activation_stack,
                    &activation_stack_size,
                        object_value(block_handle)
                );
                break;
                }
            case RECORZ_MVP_OP_JUMP:
                if (instruction.operand_b >= executable->instruction_count) {
                    machine_panic("jump target is out of range");
                }
                pc = instruction.operand_b;
                break;
            case RECORZ_MVP_OP_JUMP_IF_TRUE:
            case RECORZ_MVP_OP_JUMP_IF_FALSE: {
                uint8_t condition_is_true;

                if (instruction.operand_b >= executable->instruction_count) {
                    machine_panic("conditional jump target is out of range");
                }
                condition_is_true = condition_value_is_true(
                    activation_pop(activation_stack, &activation_stack_size)
                );
                if ((instruction.opcode == RECORZ_MVP_OP_JUMP_IF_TRUE && condition_is_true) ||
                    (instruction.opcode == RECORZ_MVP_OP_JUMP_IF_FALSE && !condition_is_true)) {
                    pc = instruction.operand_b;
                }
                break;
            }
            case RECORZ_MVP_OP_STORE_FIELD:
                if (receiver_object == 0) {
                    machine_panic("storeField requires a receiver object");
                }
                heap_set_field(
                    heap_handle_for_object(receiver_object),
                    instruction.operand_a,
                    activation_pop(activation_stack, &activation_stack_size)
                );
                break;
            case RECORZ_MVP_OP_SEND: {
                struct recorz_mvp_value send_arguments[MAX_SEND_ARGS];
                struct recorz_mvp_value send_receiver;
                struct recorz_mvp_source_eval_result source_result;
                const struct recorz_mvp_runtime_block_state *block_state = 0;
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
                if (send_receiver.kind == RECORZ_MVP_VALUE_OBJECT &&
                    primitive_kind_for_heap_object(heap_object_for_value(send_receiver)) == RECORZ_MVP_OBJECT_BLOCK_CLOSURE &&
                    ((instruction.operand_a == RECORZ_MVP_SELECTOR_VALUE && instruction.operand_b == 0U) ||
                     (instruction.operand_a == RECORZ_MVP_SELECTOR_VALUE_ARG && instruction.operand_b == 1U))) {
                    block_state = source_block_state_for_handle(heap_handle_for_object(heap_object_for_value(send_receiver)));
                    source_result = source_execute_block_closure(
                        heap_object_for_value(send_receiver),
                        instruction.operand_b,
                        send_arguments,
                        current_context_handle
                    );
                    if (source_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
                        if (block_state == 0 ||
                            block_state->home_context_index != executable->home_context_index ||
                            executable->home_context_index < 0) {
                            machine_panic("block non-local return crossed an unsupported executable boundary");
                        }
                        push(source_result.value);
                        return;
                    }
                    activation_push(activation_stack, &activation_stack_size, source_result.value);
                    break;
                }
                if ((instruction.operand_a == RECORZ_MVP_SELECTOR_IF_TRUE && instruction.operand_b == 1U) ||
                    (instruction.operand_a == RECORZ_MVP_SELECTOR_IF_FALSE && instruction.operand_b == 1U) ||
                    (instruction.operand_a == RECORZ_MVP_SELECTOR_IF_TRUE_IF_FALSE && instruction.operand_b == 2U)) {
                    uint8_t condition_is_true = condition_value_is_true(send_receiver);
                    uint16_t chosen_index = 0xFFFFU;

                    if (instruction.operand_a == RECORZ_MVP_SELECTOR_IF_TRUE) {
                        chosen_index = condition_is_true ? 0U : 0xFFFFU;
                    } else if (instruction.operand_a == RECORZ_MVP_SELECTOR_IF_FALSE) {
                        chosen_index = condition_is_true ? 0xFFFFU : 0U;
                    } else {
                        chosen_index = condition_is_true ? 0U : 1U;
                    }
                    if (chosen_index == 0xFFFFU) {
                        activation_push(activation_stack, &activation_stack_size, nil_value());
                        break;
                    }
                    if (send_arguments[chosen_index].kind != RECORZ_MVP_VALUE_OBJECT ||
                        primitive_kind_for_heap_object(heap_object_for_value(send_arguments[chosen_index])) != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
                        machine_panic("conditional send expects a block closure argument");
                    }
                    block_state = source_block_state_for_handle(
                        heap_handle_for_object(heap_object_for_value(send_arguments[chosen_index]))
                    );
                    source_result = source_execute_block_closure(
                        heap_object_for_value(send_arguments[chosen_index]),
                        0U,
                        0,
                        current_context_handle
                    );
                    if (source_result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
                        if (block_state == 0 ||
                            block_state->home_context_index != executable->home_context_index ||
                            executable->home_context_index < 0) {
                            machine_panic("block non-local return crossed an unsupported executable boundary");
                        }
                        push(source_result.value);
                        return;
                    }
                    activation_push(activation_stack, &activation_stack_size, source_result.value);
                    break;
                }
                perform_send_with_sender(
                    send_receiver,
                    instruction.operand_a,
                    instruction.operand_b,
                    send_arguments,
                    current_context_handle,
                    0
                );
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

static void workspace_evaluate_source(const char *source) {
    struct recorz_mvp_workspace_source_program program;
    struct recorz_mvp_executable executable = {
        .instruction_source = program.instructions,
        .read_instruction = read_program_instruction,
        .instruction_count = 0U,
        .literals = program.literals,
        .literal_count = 0U,
        .lexical_count = 0U,
        .home_context_index = -1,
    };
    struct recorz_mvp_value workspace_receiver = top_level_receiver_value();
    const struct recorz_mvp_heap_object *workspace_receiver_object = heap_object_for_value(workspace_receiver);
    uint32_t stack_size_before = stack_size;
    int16_t home_context_index = source_allocate_home_context(
        class_object_for_heap_object(workspace_receiver_object),
        workspace_receiver,
        -1,
        0U,
        "<workspace>"
    );
    uint16_t context_handle = source_home_context_at(home_context_index)->context_handle;

    build_workspace_source_program(source, &program);
    executable.instruction_count = program.instruction_count;
    executable.literal_count = program.literal_count;
    executable.lexical_count = program.lexical_count;
    executable.lexical_names = program.temporary_names;
    executable.block_defining_class = class_object_for_heap_object(workspace_receiver_object);
    executable.home_context_index = home_context_index;
    execute_executable(&executable, workspace_receiver_object, workspace_receiver, 0U, 0, context_handle);
    source_home_context_at(home_context_index)->alive = 0U;
    mark_context_dead(context_handle);
    if (stack_size != stack_size_before + 1U) {
        machine_panic("Workspace source execution did not return exactly one value");
    }
    (void)pop_value();
}

static void execute_block_closure_with_sender(
    const struct recorz_mvp_heap_object *object,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle
) {
    struct recorz_mvp_source_eval_result result =
        source_execute_block_closure(object, argument_count, arguments, sender_context_handle);

    if (result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
        machine_panic("block non-local return requires a live source caller");
    }
    push(result.value);
}

static void dispatch_conditional_block_send_with_sender(
    uint8_t selector,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle
) {
    uint8_t condition_is_true = condition_value_is_true(receiver);
    uint16_t chosen_index = 0xFFFFU;

    if (selector == RECORZ_MVP_SELECTOR_IF_TRUE) {
        chosen_index = condition_is_true ? 0U : 0xFFFFU;
    } else if (selector == RECORZ_MVP_SELECTOR_IF_FALSE) {
        chosen_index = condition_is_true ? 0xFFFFU : 0U;
    } else if (selector == RECORZ_MVP_SELECTOR_IF_TRUE_IF_FALSE) {
        chosen_index = condition_is_true ? 0U : 1U;
    } else {
        machine_panic("conditional dispatch received an unknown selector");
    }
    if (chosen_index == 0xFFFFU) {
        push(nil_value());
        return;
    }
    if (arguments[chosen_index].kind != RECORZ_MVP_VALUE_OBJECT ||
        primitive_kind_for_heap_object(heap_object_for_value(arguments[chosen_index])) != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
        machine_panic("conditional send expects a block closure argument");
    }
    execute_block_closure_with_sender(
        heap_object_for_value(arguments[chosen_index]),
        0U,
        0,
        sender_context_handle
    );
}

static void execute_compiled_method_with_sender(
    const struct recorz_mvp_heap_object *receiver_object,
    struct recorz_mvp_value receiver,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    uint8_t selector,
    uint16_t sender_context_handle,
    const struct recorz_mvp_heap_object *compiled_method
) {
    const struct recorz_mvp_executable executable = {
        .instruction_source = compiled_method,
        .read_instruction = read_compiled_method_instruction,
        .instruction_count = compiled_method->field_count,
        .literals = 0,
        .literal_count = 0U,
        .lexical_count = compiled_method_lexical_count(compiled_method),
        .home_context_index = -1,
    };
    uint16_t context_handle;

    if (compiled_method->kind != RECORZ_MVP_OBJECT_COMPILED_METHOD) {
        machine_panic("method entry implementation is not a compiled method");
    }
    context_handle = allocate_compiled_activation_context_if_needed(
        &executable,
        sender_context_handle,
        receiver,
        selector_name(selector)
    );
    execute_executable(&executable, receiver_object, receiver, argument_count, arguments, context_handle);
    mark_context_dead(context_handle);
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
        selector > MAX_SELECTOR_ID) {
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
    forget_live_string_literals(heap_handle_for_object(class_object), (uint8_t)selector, (uint8_t)argument_count);
    forget_live_method_source(heap_handle_for_object(class_object), (uint8_t)selector, (uint8_t)argument_count);
    install_compiled_method_update(class_object, (uint8_t)selector, argument_count, compiled_method_handle);
}

static const struct recorz_mvp_live_method_source *live_method_source_for_class_chain(
    const struct recorz_mvp_heap_object *class_object,
    uint8_t selector_id,
    uint8_t argument_count,
    const struct recorz_mvp_heap_object **owner_class_out
) {
    const struct recorz_mvp_heap_object *lookup_class = class_object;
    uint32_t depth = 0U;

    while (lookup_class != 0) {
        const struct recorz_mvp_live_method_source *source_record = live_method_source_for_selector_and_arity(
            heap_handle_for_object(lookup_class),
            selector_id,
            argument_count
        );

        if (source_record != 0) {
            if (owner_class_out != 0) {
                *owner_class_out = lookup_class;
            }
            return source_record;
        }
        if (depth++ >= HEAP_LIMIT) {
            machine_panic("class superclass chain is invalid");
        }
        lookup_class = class_superclass_object_or_null(lookup_class);
    }
    return 0;
}

static void dispatch_heap_object_send(
    const struct recorz_mvp_heap_object *object,
    uint8_t selector,
    uint16_t argument_count,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle,
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    const struct recorz_mvp_heap_object *method_object;
    const struct recorz_mvp_heap_object *entry_object;
    const struct recorz_mvp_heap_object *implementation_object;
    const struct recorz_mvp_heap_object *source_owner_class = 0;
    const struct recorz_mvp_live_method_source *source_record;
    struct recorz_mvp_value implementation_value;
    recorz_mvp_method_entry_handler handler;
    uint32_t entry;
    uint32_t primitive_binding_id;

    if (selector == RECORZ_MVP_SELECTOR_CLASS) {
        push(object_value(object->class_handle));
        return;
    }
    if ((selector == RECORZ_MVP_SELECTOR_IF_TRUE && argument_count == 1U) ||
        (selector == RECORZ_MVP_SELECTOR_IF_FALSE && argument_count == 1U) ||
        (selector == RECORZ_MVP_SELECTOR_IF_TRUE_IF_FALSE && argument_count == 2U)) {
        dispatch_conditional_block_send_with_sender(selector, receiver, arguments, sender_context_handle);
        return;
    }
    if (primitive_kind_for_heap_object(object) == RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
        if ((selector == RECORZ_MVP_SELECTOR_VALUE && argument_count == 0U) ||
            (selector == RECORZ_MVP_SELECTOR_VALUE_ARG && argument_count == 1U)) {
            execute_block_closure_with_sender(object, argument_count, arguments, sender_context_handle);
            return;
        }
        machine_panic("BlockClosure only understands value/value:");
    }
    class_object = class_object_for_heap_object(object);
    source_record = live_method_source_for_class_chain(
        class_object,
        selector,
        (uint8_t)argument_count,
        &source_owner_class
    );
    if (source_record != 0 && source_text_requires_live_evaluator(live_method_source_text(source_record))) {
        execute_live_source_method_with_sender(
            source_owner_class,
            receiver,
            argument_count,
            arguments,
            live_method_source_text(source_record),
            sender_context_handle
        );
        return;
    }
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
        execute_compiled_method_with_sender(
            object,
            receiver,
            argument_count,
            arguments,
            selector,
            sender_context_handle,
            implementation_object
        );
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

static void perform_send_with_sender(
    struct recorz_mvp_value receiver,
    uint8_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle,
    const char *text
) {
    const struct recorz_mvp_heap_object *object = 0;

    if (selector == RECORZ_MVP_SELECTOR_EQUAL) {
        if (send_argument_count != 1U) {
            machine_panic("= expects one argument");
        }
        push(boolean_value(value_equals(receiver, arguments[0])));
        return;
    }
    if (selector == RECORZ_MVP_SELECTOR_LESS_THAN || selector == RECORZ_MVP_SELECTOR_GREATER_THAN) {
        if (send_argument_count != 1U) {
            machine_panic("comparison expects one argument");
        }
        if (receiver.kind != RECORZ_MVP_VALUE_SMALL_INTEGER ||
            arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
            machine_panic("comparison expects small integer operands");
        }
        push(boolean_value(
            selector == RECORZ_MVP_SELECTOR_LESS_THAN
                ? (receiver.integer < arguments[0].integer)
                : (receiver.integer > arguments[0].integer)
        ));
        return;
    }
    if (selector == RECORZ_MVP_SELECTOR_SHOW || selector == RECORZ_MVP_SELECTOR_WRITE_STRING) {
        if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
            machine_panic("text send expects a string literal");
        }
        text = arguments[0].string;
    }

    if (receiver.kind == RECORZ_MVP_VALUE_OBJECT) {
        object = heap_object_for_value(receiver);
        dispatch_heap_object_send(
            object,
            selector,
            send_argument_count,
            receiver,
            arguments,
            sender_context_handle,
            text
        );
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

static void perform_send(
    struct recorz_mvp_value receiver,
    uint8_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    perform_send_with_sender(receiver, selector, send_argument_count, arguments, 0U, text);
}

static void run_startup_hook_if_configured(void) {
    struct recorz_mvp_value receiver;

    if (startup_hook_receiver_handle == 0U || startup_hook_selector_id == 0U) {
        return;
    }
    if (startup_hook_receiver_handle > heap_size) {
        machine_panic("startup hook receiver is out of range");
    }
    if (startup_hook_selector_id > MAX_SELECTOR_ID) {
        machine_panic("startup hook selector is out of range");
    }
    receiver = object_value(startup_hook_receiver_handle);
    stack_size = 0U;
    panic_phase = "startup";
    perform_send(receiver, (uint8_t)startup_hook_selector_id, 0U, 0, 0);
    if (stack_size == 0U) {
        machine_panic("startup hook did not return a value");
    }
    --stack_size;
}

void recorz_mvp_vm_run(
    const struct recorz_mvp_program *program,
    const struct recorz_mvp_seed *seed,
    const uint8_t *method_update_blob,
    uint32_t method_update_size,
    const uint8_t *file_in_blob,
    uint32_t file_in_size,
    const uint8_t *snapshot_blob,
    uint32_t snapshot_size
) {
    struct recorz_mvp_executable executable = {
        .instruction_source = program->instructions,
        .read_instruction = read_program_instruction,
        .instruction_count = program->instruction_count,
        .literals = program->literals,
        .literal_count = program->literal_count,
        .lexical_count = program->lexical_count,
        .lexical_names = program->lexical_names,
        .home_context_index = -1,
    };
    struct recorz_mvp_value top_level_receiver;
    const struct recorz_mvp_heap_object *top_level_receiver_object;
    uint16_t context_handle;
    int16_t home_context_index;

    stack_size = 0U;
    panic_phase = "bootstrap";
    panic_pc = 0U;
    panic_have_instruction = 0U;
    panic_have_send = 0U;
    machine_set_panic_hook(vm_panic_hook);
    if (snapshot_blob != 0 && snapshot_size != 0U) {
        panic_phase = "snapshot";
        load_snapshot_state(snapshot_blob, snapshot_size);
        machine_puts("recorz qemu-riscv32 mvp: loaded snapshot\n");
    } else {
        initialize_roots(seed);
    }
    if (method_update_blob != 0 && method_update_size != 0U) {
        panic_phase = "update";
        apply_method_update_payload(method_update_blob, method_update_size);
        machine_puts("recorz qemu-riscv32 mvp: applied method update\n");
    }
    if (file_in_blob != 0 && file_in_size != 0U) {
        panic_phase = "file-in";
        apply_external_file_in_blob(file_in_blob, file_in_size);
        machine_puts("recorz qemu-riscv32 mvp: applied external file-in\n");
    }
    run_startup_hook_if_configured();
    top_level_receiver = top_level_receiver_value();
    top_level_receiver_object = heap_object_for_value(top_level_receiver);
    home_context_index = source_allocate_home_context(
        class_object_for_heap_object(top_level_receiver_object),
        top_level_receiver,
        -1,
        0U,
        "<program>"
    );
    context_handle = source_home_context_at(home_context_index)->context_handle;
    executable.block_defining_class = class_object_for_heap_object(top_level_receiver_object);
    executable.home_context_index = home_context_index;
    execute_executable(&executable, top_level_receiver_object, top_level_receiver, 0U, 0, context_handle);
    source_home_context_at(home_context_index)->alive = 0U;
    mark_context_dead(context_handle);
    panic_phase = "return";
    machine_set_panic_hook(0);
}
