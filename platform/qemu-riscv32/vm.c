#include "vm.h"

#include <stdint.h>

#include "display.h"
#include "machine.h"
#include "../shared/font5x7.h"

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
#define METHOD_SOURCE_NAME_LIMIT 96U
#define CLASS_COMMENT_LIMIT 128U
#define PACKAGE_COMMENT_LIMIT CLASS_COMMENT_LIMIT
#define METHOD_SOURCE_CHUNK_LIMIT 2048U
#define PACKAGE_SOURCE_BUFFER_LIMIT 65536U
#define FILE_IN_SOURCE_BUFFER_LIMIT 131072U
#define REGENERATED_SOURCE_BUFFER_LIMIT 65536U
#define VIEW_CONTENT_HORIZONTAL_PADDING 24U
#define VIEW_CONTENT_VERTICAL_PADDING 24U
#define VIEW_CONTENT_INSET 12U
#define WORKSPACE_SOURCE_VIEW_LEFT 40U
#define WORKSPACE_SOURCE_VIEW_TOP 136U
#define WORKSPACE_SOURCE_VIEW_WIDTH 920U
#define WORKSPACE_SOURCE_VIEW_HEIGHT 456U
#define BROWSER_LIST_VIEW_LEFT 40U
#define BROWSER_LIST_VIEW_TOP 136U
#define BROWSER_LIST_VIEW_WIDTH 280U
#define BROWSER_LIST_VIEW_HEIGHT 456U
#define BROWSER_SOURCE_VIEW_LEFT 336U
#define BROWSER_SOURCE_VIEW_TOP 136U
#define BROWSER_SOURCE_VIEW_WIDTH 624U
#define BROWSER_SOURCE_VIEW_HEIGHT 456U
#define STATUS_VIEW_LEFT 40U
#define STATUS_VIEW_TOP 608U
#define STATUS_VIEW_WIDTH 904U
#define STATUS_VIEW_HEIGHT 72U
#define WORKSPACE_INPUT_MONITOR_STATE_LIMIT METHOD_SOURCE_CHUNK_LIMIT
#define WORKSPACE_INPUT_MONITOR_STATUS_LIMIT 64U
#define WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT 1024U
#define WORKSPACE_SOURCE_INSTRUCTION_LIMIT 64U
#define WORKSPACE_SOURCE_LITERAL_LIMIT 16U
#define DYNAMIC_CLASS_LIMIT RECORZ_MVP_DYNAMIC_CLASS_LIMIT
#define PACKAGE_LIMIT DYNAMIC_CLASS_LIMIT
#define DYNAMIC_CLASS_IVAR_LIMIT OBJECT_FIELD_LIMIT
#define NAMED_OBJECT_LIMIT RECORZ_MVP_NAMED_OBJECT_LIMIT
#define LIVE_METHOD_SOURCE_LIMIT RECORZ_MVP_LIVE_METHOD_SOURCE_LIMIT
#define LIVE_METHOD_SOURCE_POOL_LIMIT RECORZ_MVP_LIVE_METHOD_SOURCE_POOL_LIMIT
#define LIVE_PACKAGE_DO_IT_SOURCE_LIMIT 64U
#define LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT 32768U
#define LIVE_STRING_LITERAL_LIMIT 64U
#define RUNTIME_STRING_POOL_LIMIT RECORZ_MVP_RUNTIME_STRING_POOL_LIMIT
#define SNAPSHOT_STRING_LIMIT RECORZ_MVP_SNAPSHOT_STRING_LIMIT
#define SNAPSHOT_BUFFER_LIMIT RECORZ_MVP_SNAPSHOT_BUFFER_LIMIT

#define SNAPSHOT_MAGIC_0 'R'
#define SNAPSHOT_MAGIC_1 'C'
#define SNAPSHOT_MAGIC_2 'Z'
#define SNAPSHOT_MAGIC_3 'T'
#define SNAPSHOT_VERSION 7U
#define DEBUG_DUMP_RENDER_COUNTERS_BYTE 0x1fU
#define GC_TEMP_ROOT_LIMIT 8U
#define SNAPSHOT_HEADER_SIZE 52U
#define SNAPSHOT_VALUE_SIZE 8U
#define SNAPSHOT_OBJECT_SIZE (4U + (OBJECT_FIELD_LIMIT * SNAPSHOT_VALUE_SIZE))
#define SNAPSHOT_DYNAMIC_CLASS_RECORD_SIZE \
    (8U + (2U * METHOD_SOURCE_NAME_LIMIT) + CLASS_COMMENT_LIMIT + (DYNAMIC_CLASS_IVAR_LIMIT * METHOD_SOURCE_NAME_LIMIT))
#define SNAPSHOT_PACKAGE_RECORD_SIZE (METHOD_SOURCE_NAME_LIMIT + PACKAGE_COMMENT_LIMIT)
#define SNAPSHOT_NAMED_OBJECT_RECORD_SIZE (2U + METHOD_SOURCE_NAME_LIMIT)
#define SNAPSHOT_LIVE_METHOD_SOURCE_RECORD_SIZE (9U + METHOD_SOURCE_NAME_LIMIT)
#define SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE 9U

#define FORM_FIELD_BITS RECORZ_MVP_FORM_FIELD_BITS
#define BITMAP_FIELD_WIDTH RECORZ_MVP_BITMAP_FIELD_WIDTH
#define BITMAP_FIELD_HEIGHT RECORZ_MVP_BITMAP_FIELD_HEIGHT
#define BITMAP_FIELD_STORAGE_KIND RECORZ_MVP_BITMAP_FIELD_STORAGE_KIND
#define BITMAP_FIELD_STORAGE_ID RECORZ_MVP_BITMAP_FIELD_STORAGE_ID
#define TEXT_LAYOUT_FIELD_MARGINS RECORZ_MVP_TEXT_LAYOUT_FIELD_MARGINS
#define TEXT_LAYOUT_FIELD_PIXEL_SCALE RECORZ_MVP_TEXT_LAYOUT_FIELD_SCALE
#define TEXT_LAYOUT_FIELD_LINE_SPACING RECORZ_MVP_TEXT_LAYOUT_FIELD_LINE_SPACING
#define TEXT_LAYOUT_FIELD_FLOW RECORZ_MVP_TEXT_LAYOUT_FIELD_FLOW
#define TEXT_MARGINS_FIELD_LEFT RECORZ_MVP_TEXT_MARGINS_FIELD_LEFT
#define TEXT_MARGINS_FIELD_TOP RECORZ_MVP_TEXT_MARGINS_FIELD_TOP
#define TEXT_MARGINS_FIELD_RIGHT RECORZ_MVP_TEXT_MARGINS_FIELD_RIGHT
#define TEXT_MARGINS_FIELD_BOTTOM RECORZ_MVP_TEXT_MARGINS_FIELD_BOTTOM
#define TEXT_FLOW_FIELD_WRAP_WIDTH RECORZ_MVP_TEXT_FLOW_FIELD_WRAP_WIDTH
#define TEXT_FLOW_FIELD_TAB_WIDTH RECORZ_MVP_TEXT_FLOW_FIELD_TAB_WIDTH
#define TEXT_FLOW_FIELD_LINE_BREAK_MODE RECORZ_MVP_TEXT_FLOW_FIELD_LINE_BREAK_MODE
#define TEXT_CURSOR_FIELD_INDEX RECORZ_MVP_TEXT_CURSOR_FIELD_INDEX
#define TEXT_CURSOR_FIELD_LINE RECORZ_MVP_TEXT_CURSOR_FIELD_LINE
#define TEXT_CURSOR_FIELD_COLUMN RECORZ_MVP_TEXT_CURSOR_FIELD_COLUMN
#define TEXT_CURSOR_FIELD_TOP_LINE RECORZ_MVP_TEXT_CURSOR_FIELD_TOP_LINE
#define WORKSPACE_TOOL_FIELD_STATUS_TEXT RECORZ_MVP_WORKSPACE_TOOL_FIELD_STATUS_TEXT
#define WORKSPACE_TOOL_FIELD_FEEDBACK_TEXT RECORZ_MVP_WORKSPACE_TOOL_FIELD_FEEDBACK_TEXT
#define WORKSPACE_TOOL_FIELD_NAME RECORZ_MVP_WORKSPACE_TOOL_FIELD_NAME
#define WORKSPACE_TOOL_FIELD_VISIBLE_ORIGIN RECORZ_MVP_WORKSPACE_TOOL_FIELD_VISIBLE_ORIGIN
#define WORKSPACE_VISIBLE_ORIGIN_FIELD_TOP_LINE RECORZ_MVP_WORKSPACE_VISIBLE_ORIGIN_FIELD_TOP_LINE
#define WORKSPACE_VISIBLE_ORIGIN_FIELD_LEFT_COLUMN RECORZ_MVP_WORKSPACE_VISIBLE_ORIGIN_FIELD_LEFT_COLUMN
#define TEXT_SELECTION_FIELD_START_LINE RECORZ_MVP_TEXT_SELECTION_FIELD_START_LINE
#define TEXT_SELECTION_FIELD_START_COLUMN RECORZ_MVP_TEXT_SELECTION_FIELD_START_COLUMN
#define TEXT_SELECTION_FIELD_END_LINE RECORZ_MVP_TEXT_SELECTION_FIELD_END_LINE
#define TEXT_SELECTION_FIELD_END_COLUMN RECORZ_MVP_TEXT_SELECTION_FIELD_END_COLUMN
#define TEXT_STYLE_FIELD_BACKGROUND_COLOR RECORZ_MVP_TEXT_STYLE_FIELD_BACKGROUND_COLOR
#define TEXT_STYLE_FIELD_FOREGROUND_COLOR RECORZ_MVP_TEXT_STYLE_FIELD_FOREGROUND_COLOR
#define STYLED_TEXT_FIELD_TEXT RECORZ_MVP_STYLED_TEXT_FIELD_TEXT
#define STYLED_TEXT_FIELD_STYLE RECORZ_MVP_STYLED_TEXT_FIELD_STYLE
#define TEXT_METRICS_FIELD_CELL_WIDTH RECORZ_MVP_TEXT_METRICS_FIELD_CELL_WIDTH
#define TEXT_METRICS_FIELD_CELL_HEIGHT RECORZ_MVP_TEXT_METRICS_FIELD_CELL_HEIGHT
#define TEXT_METRICS_FIELD_BASELINE RECORZ_MVP_TEXT_METRICS_FIELD_BASELINE
#define TEXT_METRICS_FIELD_VERTICAL_METRICS RECORZ_MVP_TEXT_METRICS_FIELD_VERTICAL_METRICS
#define TEXT_VERTICAL_METRICS_FIELD_ASCENT RECORZ_MVP_TEXT_VERTICAL_METRICS_FIELD_ASCENT
#define TEXT_VERTICAL_METRICS_FIELD_DESCENT RECORZ_MVP_TEXT_VERTICAL_METRICS_FIELD_DESCENT
#define TEXT_VERTICAL_METRICS_FIELD_LINE_HEIGHT RECORZ_MVP_TEXT_VERTICAL_METRICS_FIELD_LINE_HEIGHT
#define TEXT_BEHAVIOR_FIELD_FALLBACK_BITMAP RECORZ_MVP_TEXT_BEHAVIOR_FIELD_FALLBACK_GLYPH
#define TEXT_BEHAVIOR_FIELD_CLEAR_ON_OVERFLOW RECORZ_MVP_TEXT_BEHAVIOR_FIELD_CLEAR_ON_OVERFLOW
#define FONT_FIELD_GLYPHS RECORZ_MVP_FONT_FIELD_GLYPHS
#define FONT_FIELD_METRICS RECORZ_MVP_FONT_FIELD_METRICS
#define FONT_FIELD_BEHAVIOR RECORZ_MVP_FONT_FIELD_BEHAVIOR
#define FONT_FIELD_POINT_SIZE RECORZ_MVP_FONT_FIELD_POINT_SIZE
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
#define TEST_RUNNER_FIELD_PASSED RECORZ_MVP_TEST_RUNNER_FIELD_PASSED
#define TEST_RUNNER_FIELD_FAILED RECORZ_MVP_TEST_RUNNER_FIELD_FAILED
#define TEST_RUNNER_FIELD_TOTAL RECORZ_MVP_TEST_RUNNER_FIELD_TOTAL
#define TEST_RUNNER_FIELD_LAST_LABEL RECORZ_MVP_TEST_RUNNER_FIELD_LAST_LABEL
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
#define CURSOR_FIELD_BITS RECORZ_MVP_CURSOR_FIELD_BITS
#define CURSOR_FIELD_HOTSPOT_X RECORZ_MVP_CURSOR_FIELD_HOTSPOT_X
#define CURSOR_FIELD_HOTSPOT_Y RECORZ_MVP_CURSOR_FIELD_HOTSPOT_Y
#define CHARACTER_SCANNER_FIELD_STOP RECORZ_MVP_CHARACTER_SCANNER_FIELD_STOP
#define CHARACTER_SCANNER_FIELD_INDEX RECORZ_MVP_CHARACTER_SCANNER_FIELD_INDEX
#define CHARACTER_SCANNER_FIELD_X RECORZ_MVP_CHARACTER_SCANNER_FIELD_X
#define CHARACTER_SCANNER_FIELD_Y RECORZ_MVP_CHARACTER_SCANNER_FIELD_Y

#define BITMAP_STORAGE_FRAMEBUFFER RECORZ_MVP_BITMAP_STORAGE_FRAMEBUFFER
#define BITMAP_STORAGE_GLYPH_MONO RECORZ_MVP_BITMAP_STORAGE_GLYPH_MONO
#define BITMAP_STORAGE_HEAP_MONO 3U
#define RECORZ_MVP_TRANSFER_RULE_COPY 0U
#define RECORZ_MVP_TRANSFER_RULE_OVER 1U
#define CHARACTER_SCANNER_STOP_END_OF_RUN 0U
#define CHARACTER_SCANNER_STOP_RIGHT_MARGIN 1U
#define CHARACTER_SCANNER_STOP_TAB 2U
#define CHARACTER_SCANNER_STOP_NEWLINE 3U
#define CHARACTER_SCANNER_STOP_CONTROL 4U
#define CHARACTER_SCANNER_STOP_SELECTION 5U
#define CHARACTER_SCANNER_STOP_CURSOR 6U
#define MAX_OBJECT_KIND RECORZ_MVP_OBJECT_WORKSPACE_TOOL
#define MAX_SELECTOR_ID RECORZ_MVP_SELECTOR_CLASS_NAMES_VISIBLE_FROM_COUNT
#define MAX_GLOBAL_ID RECORZ_MVP_GLOBAL_WORKSPACE_SELECTION
#define SOURCE_EVAL_BINDING_LIMIT (MAX_SEND_ARGS + LEXICAL_LIMIT)
#if defined(RECORZ_MVP_PROFILE_DEV)
#define SOURCE_EVAL_ENV_LIMIT 4096U
#define SOURCE_EVAL_HOME_CONTEXT_LIMIT 4096U
#define SOURCE_EVAL_BLOCK_STATE_LIMIT 4096U
#else
#define SOURCE_EVAL_ENV_LIMIT 256U
#define SOURCE_EVAL_HOME_CONTEXT_LIMIT 256U
#define SOURCE_EVAL_BLOCK_STATE_LIMIT 512U
#endif

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
#define WORKSPACE_VIEW_REGENERATED_BOOT_SOURCE 16U
#define WORKSPACE_VIEW_REGENERATED_KERNEL_SOURCE 17U
#define WORKSPACE_VIEW_INPUT_MONITOR 18U
#define WORKSPACE_VIEW_REGENERATED_FILE_IN_SOURCE 19U
#define WORKSPACE_VIEW_OPENING_MENU 20U
#define WORKSPACE_VIEW_INTERACTIVE_CLASSES 21U

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
    uint16_t selector_id;
    uint8_t argument_count;
    char protocol_name[METHOD_SOURCE_NAME_LIMIT];
    uint16_t source_offset;
    uint16_t source_length;
};

struct recorz_mvp_live_package_do_it_source {
    char package_name[METHOD_SOURCE_NAME_LIMIT];
    uint16_t source_offset;
    uint16_t source_length;
};

struct recorz_mvp_live_string_literal {
    uint16_t class_handle;
    uint16_t selector_id;
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
    uint16_t selector_id;
    uint8_t argument_count;
    uint8_t is_block;
};

struct recorz_mvp_runtime_block_state {
    int16_t lexical_environment_index;
    int16_t home_context_index;
    const struct recorz_mvp_heap_object *defining_class;
};

static struct recorz_mvp_value stack[STACK_LIMIT];
static struct recorz_mvp_heap_object heap[HEAP_LIMIT];
static uint32_t stack_size = 0U;
static uint16_t heap_size = 0U;
static uint16_t heap_live_count = 0U;
static uint16_t heap_high_water_mark = 0U;
static uint16_t class_handles_by_kind[MAX_OBJECT_KIND + 1U];
static uint16_t class_descriptor_handles_by_kind[MAX_OBJECT_KIND + 1U];
static uint16_t selector_handles_by_id[MAX_SELECTOR_ID + 1U];
static uint16_t global_handles[MAX_GLOBAL_ID + 1U];
static struct recorz_mvp_dynamic_class_definition dynamic_classes[DYNAMIC_CLASS_LIMIT];
static struct recorz_mvp_live_package_definition live_packages[PACKAGE_LIMIT];
static struct recorz_mvp_named_object_binding named_objects[NAMED_OBJECT_LIMIT];
static struct recorz_mvp_live_method_source live_method_sources[LIVE_METHOD_SOURCE_LIMIT];
static struct recorz_mvp_live_package_do_it_source live_package_do_it_sources[LIVE_PACKAGE_DO_IT_SOURCE_LIMIT];
static struct recorz_mvp_live_string_literal live_string_literals[LIVE_STRING_LITERAL_LIMIT];
static char live_method_source_pool[LIVE_METHOD_SOURCE_POOL_LIMIT];
static uint16_t live_method_source_pool_used = 0U;
static char live_package_do_it_source_pool[LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT];
static uint16_t live_package_do_it_source_count = 0U;
static uint16_t live_package_do_it_source_pool_used = 0U;
static uint16_t default_form_handle = 0U;
static uint16_t active_display_form_handle = 0U;
static uint16_t active_cursor_handle = 0U;
static uint8_t active_cursor_visible = 0U;
static uint32_t active_cursor_screen_x = 0U;
static uint32_t active_cursor_screen_y = 0U;
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
static uint16_t transcript_font_handle = 0U;
static struct recorz_mvp_source_lexical_environment source_eval_environments[SOURCE_EVAL_ENV_LIMIT];
static struct recorz_mvp_source_home_context source_eval_home_contexts[SOURCE_EVAL_HOME_CONTEXT_LIMIT];
static uint16_t startup_hook_receiver_handle = 0U;
static uint16_t startup_hook_selector_id = 0U;
static uint32_t mono_bitmap_pool[MONO_BITMAP_LIMIT][MONO_BITMAP_MAX_HEIGHT];
static uint16_t mono_bitmap_count = 0U;
static uint16_t named_object_count = 0U;
static uint16_t live_method_source_count = 0U;
static uint16_t compiling_method_class_handle = 0U;
static uint16_t compiling_method_selector_id = 0U;
static uint8_t compiling_method_argument_count = 0U;
static uint32_t cursor_x = 0U;
static uint32_t cursor_y = 0U;
static char print_buffer[PRINT_BUFFER_SIZE];
static char workspace_target_buffer[METHOD_SOURCE_CHUNK_LIMIT];
static char workspace_input_monitor_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char workspace_input_monitor_cursor_state[WORKSPACE_INPUT_MONITOR_STATE_LIMIT];
static char workspace_input_monitor_status[WORKSPACE_INPUT_MONITOR_STATUS_LIMIT];
static char workspace_input_monitor_feedback[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];
static char workspace_edit_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 2U];
static char workspace_editor_source_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char workspace_surface_list_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char workspace_surface_editor_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char workspace_surface_source_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char workspace_surface_status_buffer[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];
static uint8_t workspace_input_monitor_capture_enabled = 0U;
static uint8_t workspace_tool_status_dirty = 0U;
static uint8_t workspace_tool_feedback_dirty = 0U;
static uint32_t render_counter_editor_full_redraws = 0U;
static uint32_t render_counter_editor_pane_redraws = 0U;
static uint32_t render_counter_editor_cursor_redraws = 0U;
static uint32_t render_counter_editor_scroll_redraws = 0U;
static uint32_t render_counter_editor_status_redraws = 0U;
static uint32_t render_counter_browser_full_redraws = 0U;
static uint32_t render_counter_browser_list_redraws = 0U;
static char kernel_source_io_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char package_source_io_buffer[PACKAGE_SOURCE_BUFFER_LIMIT + 1U];
static char file_in_source_io_buffer[FILE_IN_SOURCE_BUFFER_LIMIT + 1U];
static char regenerated_source_io_buffer[REGENERATED_SOURCE_BUFFER_LIMIT];
static char runtime_string_pool[RUNTIME_STRING_POOL_LIMIT];
static uint32_t runtime_string_pool_offset = 0U;
static uint8_t gc_mark_bits[(HEAP_LIMIT + 7U) / 8U];
static uint16_t gc_temp_roots[GC_TEMP_ROOT_LIMIT];
static uint8_t gc_temp_root_count = 0U;
static uint32_t gc_collection_count = 0U;
static uint32_t gc_last_reclaimed_count = 0U;
static uint32_t gc_total_reclaimed_count = 0U;
static uintptr_t gc_stack_base_address = 0U;
static uint8_t gc_bootstrap_file_in_active = 0U;
static char snapshot_string_pool[SNAPSHOT_STRING_LIMIT];
static uint8_t snapshot_buffer[SNAPSHOT_BUFFER_LIMIT];
static uint8_t booted_from_snapshot = 0U;

static uint16_t seeded_handles[HEAP_LIMIT];
static const char *panic_phase = "idle";
static uint32_t panic_pc = 0U;
static struct recorz_mvp_instruction panic_instruction;
static uint8_t panic_have_instruction = 0U;
static uint8_t panic_have_send = 0U;
static uint16_t panic_send_selector = 0U;
static uint16_t panic_send_argument_count = 0U;
static struct recorz_mvp_value panic_send_receiver;
static struct recorz_mvp_value panic_send_arguments[MAX_SEND_ARGS];

static struct recorz_mvp_value nil_value(void);
static uint32_t small_integer_u32(struct recorz_mvp_value value, const char *message);
static int32_t small_integer_i32(struct recorz_mvp_value value, const char *message);
static void heap_set_field(uint16_t handle, uint8_t index, struct recorz_mvp_value value);
static void heap_set_class(uint16_t handle, uint16_t class_handle);
static const struct recorz_mvp_heap_object *heap_object_for_value(struct recorz_mvp_value value);
static struct recorz_mvp_value heap_get_field(const struct recorz_mvp_heap_object *object, uint8_t index);
static const struct recorz_mvp_heap_object *class_object_for_heap_object(const struct recorz_mvp_heap_object *object);
static uint32_t compiled_method_instruction_word(const struct recorz_mvp_heap_object *compiled_method, uint8_t index);
static uint16_t compiled_method_lexical_count(const struct recorz_mvp_heap_object *compiled_method);
static uint32_t text_length(const char *text);
static uint32_t text_left_margin(void);
static uint32_t text_right_margin(void);
static uint32_t text_bottom_margin(void);
static uint32_t text_wrap_limit_x_for_form_width(uint32_t form_width);
static uint32_t char_width(void);
static uint32_t char_height(void);
static uint32_t text_line_spacing(void);
static uint32_t text_line_height(void);
static uint32_t bitmap_width(const struct recorz_mvp_heap_object *bitmap);
static uint32_t bitmap_height(const struct recorz_mvp_heap_object *bitmap);
static const struct recorz_mvp_heap_object *bitmap_for_form(const struct recorz_mvp_heap_object *form);
static struct recorz_mvp_value workspace_evaluate_source(const char *source);
static struct recorz_mvp_value workspace_current_source_value(
    const struct recorz_mvp_heap_object *workspace_object
);
static uint8_t compiled_method_instruction_opcode(uint32_t instruction);
static uint16_t compiled_method_instruction_operand_a(uint32_t instruction);
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
    uint16_t selector,
    uint16_t argument_count,
    uint16_t compiled_method_handle
);
static void install_method_source_on_class(
    const struct recorz_mvp_heap_object *class_object,
    const char *source
);
static void install_method_chunk_on_class(
    const struct recorz_mvp_heap_object *class_object,
    const char *protocol_name,
    const char *chunk
);
static void file_in_class_source_on_existing_class(
    const char *source,
    const struct recorz_mvp_heap_object *class_object
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
static const char *file_out_class_source_text(const char *class_name, char buffer[], uint32_t buffer_size);
static const char *file_out_class_source_by_name(const char *class_name);
static const char *file_out_package_source_text(const char *package_name, char buffer[], uint32_t buffer_size);
static const char *file_out_package_source_by_name(const char *package_name);
static uint16_t workspace_collect_sorted_packages(
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT]
);
static void emit_regenerated_boot_source(const struct recorz_mvp_heap_object *workspace_object);
static void emit_regenerated_kernel_source(void);
static const char *regenerated_boot_source_text(const struct recorz_mvp_heap_object *workspace_object);
static const char *regenerated_kernel_source_text(void);
static const char *regenerated_file_in_source_text(void);
static const char *development_home_initial_source_text(void);
static int compare_source_names(const char *left, const char *right);
static const char *runtime_string_allocate_copy(const char *text);
static const char *runtime_string_intern_copy(const char *text);
static void runtime_string_compact_live_references(void);
static struct recorz_mvp_value boolean_value(uint8_t condition);
static struct recorz_mvp_value object_value(uint16_t handle);
static struct recorz_mvp_value string_value(const char *text);
static uint16_t heap_allocate_seeded_class(uint8_t kind);
static uint16_t heap_allocate_seeded_class_run(uint8_t kind, uint16_t count);
static uint8_t heap_handle_is_live(uint16_t handle);
static void forget_live_string_literals(uint16_t class_handle, uint16_t selector_id, uint8_t argument_count);
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
static void form_fill_rect_color(
    const struct recorz_mvp_heap_object *form,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
);
static void form_copy_rect(
    const struct recorz_mvp_heap_object *form,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
);
static void form_draw_line_color(
    const struct recorz_mvp_heap_object *form,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint32_t color
);
static void form_draw_code_point_at_with_colors(
    const struct recorz_mvp_heap_object *form,
    uint8_t code_point,
    uint32_t x,
    uint32_t y,
    uint32_t foreground_color,
    uint32_t background_color
);
static void form_write_code_point_with_colors(
    const struct recorz_mvp_heap_object *form,
    uint8_t code_point,
    uint32_t foreground_color,
    uint32_t background_color
);
static void bitblt_copy_mono_bitmap_to_form(
    const struct recorz_mvp_heap_object *source_bitmap,
    const struct recorz_mvp_heap_object *dest_form,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t copy_width,
    uint32_t copy_height,
    uint32_t dest_x,
    uint32_t dest_y,
    uint32_t scale,
    uint32_t one_color,
    uint32_t zero_color,
    uint8_t transfer_rule
);
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
    uint8_t transfer_rule
);
static uint8_t bitblt_transfer_rule_uses_transparent_zero(uint8_t transfer_rule);
static uint8_t normalize_bitblt_source_region(
    const struct recorz_mvp_heap_object *source_bitmap,
    uint32_t *source_x,
    uint32_t *source_y,
    uint32_t *copy_width,
    uint32_t *copy_height
);
static void draw_mono_bitmap_line(
    struct recorz_mvp_heap_object *bitmap,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint8_t value
);
static uint32_t text_background_color(void);
static void active_cursor_move_to(uint32_t x, uint32_t y);
static void active_cursor_set_visible(uint8_t visible);
static uint8_t active_cursor_is_visible(void);
static void draw_active_cursor_on_form(const struct recorz_mvp_heap_object *form);
static uint32_t bitmap_storage_kind(const struct recorz_mvp_heap_object *bitmap);
static uint32_t text_pixel_scale(void);
static uint32_t text_foreground_color(void);
static uint32_t workspace_input_monitor_cursor_index(const struct recorz_mvp_heap_object *workspace_object);
static void workspace_input_monitor_set_status(const char *text);
static void workspace_input_monitor_set_feedback_text(const char *text);
static void workspace_input_monitor_cursor_line_and_column(
    const char *text,
    uint32_t cursor_index,
    uint32_t *line_out,
    uint32_t *column_out
);
static void workspace_sync_input_monitor_text_state(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line
);
static uint32_t workspace_input_monitor_top_line(const struct recorz_mvp_heap_object *workspace_object);
static const char *workspace_input_monitor_feedback_tail_start(
    const char *text,
    uint32_t line_capacity
);
static uint8_t workspace_parse_input_monitor_state(
    const char *text,
    uint32_t *cursor_index_out,
    uint32_t *top_line_out,
    uint32_t *saved_view_kind_out,
    char *saved_target_out,
    uint32_t saved_target_out_size,
    char *status_out,
    uint32_t status_out_size,
    char *feedback_out,
    uint32_t feedback_out_size
);
static uint8_t workspace_input_monitor_draw_output_from_image(
    const struct recorz_mvp_heap_object *form,
    const char *status,
    const char *feedback,
    uint32_t output_line_capacity
);
static void workspace_store_input_monitor_state_with_context(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line,
    uint32_t saved_view_kind,
    const char *saved_target_name
);
static void workspace_store_input_monitor_state_with_context_text_only(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line,
    uint32_t saved_view_kind,
    const char *saved_target_name
);
static void workspace_store_input_monitor_state(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line
);
static void workspace_store_input_monitor_cursor_index(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index
);
static void workspace_move_input_monitor_cursor_up(const struct recorz_mvp_heap_object *workspace_object);
static void workspace_move_input_monitor_cursor_down(const struct recorz_mvp_heap_object *workspace_object);
static void workspace_move_input_monitor_cursor_left(const struct recorz_mvp_heap_object *workspace_object);
static void workspace_move_input_monitor_cursor_right(const struct recorz_mvp_heap_object *workspace_object);
static void workspace_move_input_monitor_cursor_to_line_start(
    const struct recorz_mvp_heap_object *workspace_object
);
static void workspace_move_input_monitor_cursor_to_line_end(
    const struct recorz_mvp_heap_object *workspace_object
);
static struct recorz_mvp_heap_object *workspace_cursor_object(void);
static uint32_t workspace_cursor_line_value(void);
static uint32_t workspace_cursor_column_value(void);
static uint32_t workspace_visible_origin_top_line_value(void);
static uint32_t workspace_visible_origin_left_column_value(void);
static uint8_t workspace_input_monitor_accept_context(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t *view_kind_out,
    char target_name_out[],
    uint32_t target_name_out_size
);
static void workspace_accept_current_in_place(
    const struct recorz_mvp_heap_object *object
);
static uint8_t workspace_resolve_source_browser_target(
    uint32_t view_kind,
    const char *target_name,
    uint32_t *normalized_view_kind_out,
    char normalized_target_name_out[],
    uint32_t normalized_target_name_out_size,
    const char **source_out
);
static void workspace_run_current_tests_in_place(
    const struct recorz_mvp_heap_object *object
);
static void workspace_accept_input_monitor_buffer(
    const struct recorz_mvp_heap_object *workspace_object
);
static const struct recorz_mvp_heap_object *workspace_global_object(void);
static uint8_t workspace_send_tool_selector(uint16_t selector);
static void workspace_reopen_in_place(
    const struct recorz_mvp_heap_object *object
);
static uint8_t workspace_browse_input_monitor_context(
    const struct recorz_mvp_heap_object *workspace_object
);
static struct recorz_mvp_heap_object *heap_object(uint16_t handle);
static uint16_t heap_handle_for_object(const struct recorz_mvp_heap_object *object);
static struct recorz_mvp_value perform_send_and_pop_result(
    struct recorz_mvp_value receiver,
    uint16_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    const char *text
);
static uint16_t gc_collect_now(void);

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

static uint8_t workspace_view_kind_uses_image_session(uint32_t view_kind) {
    switch (view_kind) {
        case WORKSPACE_VIEW_METHOD:
        case WORKSPACE_VIEW_CLASS_METHOD:
        case WORKSPACE_VIEW_CLASS_SOURCE:
        case WORKSPACE_VIEW_PACKAGE_SOURCE:
        case WORKSPACE_VIEW_OPENING_MENU:
        case WORKSPACE_VIEW_PACKAGES:
        case WORKSPACE_VIEW_INTERACTIVE_CLASSES:
        case WORKSPACE_VIEW_REGENERATED_BOOT_SOURCE:
        case WORKSPACE_VIEW_REGENERATED_KERNEL_SOURCE:
        case WORKSPACE_VIEW_INPUT_MONITOR:
        case WORKSPACE_VIEW_REGENERATED_FILE_IN_SOURCE:
            return 1U;
        default:
            return 0U;
    }
}

static uint32_t workspace_image_session_mode_for_view_kind(uint32_t view_kind) {
    if (view_kind == WORKSPACE_VIEW_OPENING_MENU) {
        return 0U;
    }
    if (view_kind == WORKSPACE_VIEW_PACKAGES) {
        return 2U;
    }
    if (view_kind == WORKSPACE_VIEW_INTERACTIVE_CLASSES) {
        return 3U;
    }
    return 1U;
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

static void render_counters_reset(void) {
    render_counter_editor_full_redraws = 0U;
    render_counter_editor_pane_redraws = 0U;
    render_counter_editor_cursor_redraws = 0U;
    render_counter_editor_scroll_redraws = 0U;
    render_counter_editor_status_redraws = 0U;
    render_counter_browser_full_redraws = 0U;
    render_counter_browser_list_redraws = 0U;
}

static void render_counters_dump(void) {
    machine_puts("recorz-render-counters ");
    machine_puts("editor_full=");
    panic_put_u32(render_counter_editor_full_redraws);
    machine_puts(" editor_pane=");
    panic_put_u32(render_counter_editor_pane_redraws);
    machine_puts(" editor_cursor=");
    panic_put_u32(render_counter_editor_cursor_redraws);
    machine_puts(" editor_scroll=");
    panic_put_u32(render_counter_editor_scroll_redraws);
    machine_puts(" editor_status=");
    panic_put_u32(render_counter_editor_status_redraws);
    machine_puts(" browser_full=");
    panic_put_u32(render_counter_browser_full_redraws);
    machine_puts(" browser_list=");
    panic_put_u32(render_counter_browser_list_redraws);
    machine_puts("\n");
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

static const char *selector_name(uint16_t selector) {
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
        case RECORZ_MVP_SELECTOR_RUN_CLASS_NAMED:
            return "runClassNamed:";
        case RECORZ_MVP_SELECTOR_RUN_PACKAGE_NAMED:
            return "runPackageNamed:";
        case RECORZ_MVP_SELECTOR_PASSED:
            return "passed";
        case RECORZ_MVP_SELECTOR_FAILED:
            return "failed";
        case RECORZ_MVP_SELECTOR_TOTAL:
            return "total";
        case RECORZ_MVP_SELECTOR_LAST_LABEL:
            return "lastLabel";
        case RECORZ_MVP_SELECTOR_TEST_PASS:
            return "testPass";
        case RECORZ_MVP_SELECTOR_TEST_FAIL:
            return "testFail";
        case RECORZ_MVP_SELECTOR_SAVE_AND_REOPEN:
            return "saveAndReopen";
        case RECORZ_MVP_SELECTOR_SAVE_AND_RERUN:
            return "saveAndRerun";
        case RECORZ_MVP_SELECTOR_SAVE_RECOVERY_SNAPSHOT:
            return "saveRecoverySnapshot";
        case RECORZ_MVP_SELECTOR_RECOVER_LAST_SOURCE:
            return "recoverLastSource";
        case RECORZ_MVP_SELECTOR_EMIT_REGENERATED_BOOT_SOURCE:
            return "emitRegeneratedBootSource";
        case RECORZ_MVP_SELECTOR_BROWSE_REGENERATED_BOOT_SOURCE:
            return "browseRegeneratedBootSource";
        case RECORZ_MVP_SELECTOR_BROWSE_REGENERATED_KERNEL_SOURCE:
            return "browseRegeneratedKernelSource";
        case RECORZ_MVP_SELECTOR_BROWSE_REGENERATED_FILE_IN_SOURCE:
            return "browseRegeneratedFileInSource";
        case RECORZ_MVP_SELECTOR_DEVELOPMENT_HOME:
            return "developmentHome";
        case RECORZ_MVP_SELECTOR_BROWSE_PACKAGES_INTERACTIVE:
            return "browsePackagesInteractive";
        case RECORZ_MVP_SELECTOR_GLYPHS:
            return "glyphs";
        case RECORZ_MVP_SELECTOR_METRICS:
            return "metrics";
        case RECORZ_MVP_SELECTOR_BEHAVIOR:
            return "behavior";
        case RECORZ_MVP_SELECTOR_POINT_SIZE:
            return "pointSize";
        case RECORZ_MVP_SELECTOR_FONT:
            return "font";
        case RECORZ_MVP_SELECTOR_CELL_WIDTH:
            return "cellWidth";
        case RECORZ_MVP_SELECTOR_CELL_HEIGHT:
            return "cellHeight";
        case RECORZ_MVP_SELECTOR_BASELINE:
            return "baseline";
        case RECORZ_MVP_SELECTOR_VERTICAL_METRICS:
            return "verticalMetrics";
        case RECORZ_MVP_SELECTOR_ASCENT:
            return "ascent";
        case RECORZ_MVP_SELECTOR_DESCENT:
            return "descent";
        case RECORZ_MVP_SELECTOR_LINE_HEIGHT:
            return "lineHeight";
        case RECORZ_MVP_SELECTOR_FOREGROUND_COLOR:
            return "foregroundColor";
        case RECORZ_MVP_SELECTOR_BACKGROUND_COLOR:
            return "backgroundColor";
        case RECORZ_MVP_SELECTOR_WITH_TEXT:
            return "withText:";
        case RECORZ_MVP_SELECTOR_TEXT:
            return "text";
        case RECORZ_MVP_SELECTOR_STYLE:
            return "style";
        case RECORZ_MVP_SELECTOR_WRITE_STYLED_TEXT:
            return "writeStyledText:";
        case RECORZ_MVP_SELECTOR_LAYOUT:
            return "layout";
        case RECORZ_MVP_SELECTOR_MARGINS:
            return "margins";
        case RECORZ_MVP_SELECTOR_FLOW:
            return "flow";
        case RECORZ_MVP_SELECTOR_LEFT:
            return "left";
        case RECORZ_MVP_SELECTOR_TOP:
            return "top";
        case RECORZ_MVP_SELECTOR_RIGHT:
            return "right";
        case RECORZ_MVP_SELECTOR_BOTTOM:
            return "bottom";
        case RECORZ_MVP_SELECTOR_SCALE:
            return "scale";
        case RECORZ_MVP_SELECTOR_LINE_SPACING:
            return "lineSpacing";
        case RECORZ_MVP_SELECTOR_WRAP_WIDTH:
            return "wrapWidth";
        case RECORZ_MVP_SELECTOR_TAB_WIDTH:
            return "tabWidth";
        case RECORZ_MVP_SELECTOR_LINE_BREAK_MODE:
            return "lineBreakMode";
        case RECORZ_MVP_SELECTOR_CURSOR:
            return "cursor";
        case RECORZ_MVP_SELECTOR_SELECTION:
            return "selection";
        case RECORZ_MVP_SELECTOR_INDEX:
            return "index";
        case RECORZ_MVP_SELECTOR_LINE:
            return "line";
        case RECORZ_MVP_SELECTOR_COLUMN:
            return "column";
        case RECORZ_MVP_SELECTOR_TOP_LINE:
            return "topLine";
        case RECORZ_MVP_SELECTOR_START_LINE:
            return "startLine";
        case RECORZ_MVP_SELECTOR_START_COLUMN:
            return "startColumn";
        case RECORZ_MVP_SELECTOR_END_LINE:
            return "endLine";
        case RECORZ_MVP_SELECTOR_END_COLUMN:
            return "endColumn";
        case RECORZ_MVP_SELECTOR_SIZE:
            return "size";
        case RECORZ_MVP_SELECTOR_DRAW_STYLED_TEXT_ON_FORM:
            return "drawStyledText:onForm:";
        case RECORZ_MVP_SELECTOR_TEXT_INDEX_X_Y_STYLE_ON_FORM:
            return "text:index:x:y:style:onForm:";
        case RECORZ_MVP_SELECTOR_CA:
            return "ca";
        case RECORZ_MVP_SELECTOR_LA:
            return "la";
        case RECORZ_MVP_SELECTOR_TA:
            return "ta";
        case RECORZ_MVP_SELECTOR_LM:
            return "lm";
        case RECORZ_MVP_SELECTOR_W:
            return "w:";
        case RECORZ_MVP_SELECTOR_D_X_Y_S_F:
            return "d:x:y:s:f:";
        case RECORZ_MVP_SELECTOR_WRITE_CODE_POINT_COLOR:
            return "writeCodePoint:color:";
        case RECORZ_MVP_SELECTOR_TEXT_INDEX_STYLE_ON_FORM:
            return "text:index:style:onForm:";
        case RECORZ_MVP_SELECTOR_BE_DISPLAY:
            return "beDisplay";
        case RECORZ_MVP_SELECTOR_CURRENT:
            return "current";
        case RECORZ_MVP_SELECTOR_HOTSPOT_X:
            return "hotspotX";
        case RECORZ_MVP_SELECTOR_HOTSPOT_Y:
            return "hotspotY";
        case RECORZ_MVP_SELECTOR_FROM_BITS_HOTSPOT_X_HOTSPOT_Y:
            return "fromBits:hotspotX:hotspotY:";
        case RECORZ_MVP_SELECTOR_BE_CURSOR:
            return "beCursor";
        case RECORZ_MVP_SELECTOR_SHOW_CURSOR:
            return "showCursor";
        case RECORZ_MVP_SELECTOR_HIDE_CURSOR:
            return "hideCursor";
        case RECORZ_MVP_SELECTOR_CURSOR_VISIBLE:
            return "cursorVisible";
        case RECORZ_MVP_SELECTOR_CURSOR_X:
            return "cursorX";
        case RECORZ_MVP_SELECTOR_CURSOR_Y:
            return "cursorY";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_TO_X_Y:
            return "moveCursorToX:y:";
        case RECORZ_MVP_SELECTOR_SCAN_TEXT_INDEX_X_Y_STYLE_ON_FORM_RIGHT_SELECTION_BOUNDARY_CURSOR_BOUNDARY:
            return "scanText:index:x:y:style:onForm:right:selectionBoundary:cursorBoundary:";
        case RECORZ_MVP_SELECTOR_STOP:
            return "stop";
        case RECORZ_MVP_SELECTOR_X:
            return "x";
        case RECORZ_MVP_SELECTOR_Y:
            return "y";
        case RECORZ_MVP_SELECTOR_DRAW_LINE_ON_FORM_FROM_X_FROM_Y_TO_X_TO_Y_COLOR:
            return "drawLineOnForm:fromX:fromY:toX:toY:color:";
        case RECORZ_MVP_SELECTOR_TEXT_CURSOR_X:
            return "textCursorX";
        case RECORZ_MVP_SELECTOR_TEXT_CURSOR_Y:
            return "textCursorY";
        case RECORZ_MVP_SELECTOR_MOVE_TEXT_CURSOR_TO_X_Y:
            return "moveTextCursorToX:y:";
        case RECORZ_MVP_SELECTOR_LEFT_MARGIN:
            return "leftMargin";
        case RECORZ_MVP_SELECTOR_TOP_MARGIN:
            return "topMargin";
        case RECORZ_MVP_SELECTOR_RIGHT_MARGIN:
            return "rightMargin";
        case RECORZ_MVP_SELECTOR_BOTTOM_MARGIN:
            return "bottomMargin";
        case RECORZ_MVP_SELECTOR_CHARACTER_WIDTH:
            return "characterWidth";
        case RECORZ_MVP_SELECTOR_CHARACTER_HEIGHT:
            return "characterHeight";
        case RECORZ_MVP_SELECTOR_LINE_ADVANCE:
            return "lineAdvance";
        case RECORZ_MVP_SELECTOR_WRAP_LIMIT_FOR_FORM_WIDTH:
            return "wrapLimitForFormWidth:";
        case RECORZ_MVP_SELECTOR_NEXT_TAB_X_FROM:
            return "nextTabXFrom:";
        case RECORZ_MVP_SELECTOR_NEXT_TAB_X_FROM_STEP:
            return "nextTabXFrom:step:";
        case RECORZ_MVP_SELECTOR_MOVE_TO_INDEX_LINE_COLUMN_TOP_LINE:
            return "moveToIndex:line:column:topLine:";
        case RECORZ_MVP_SELECTOR_COLLAPSE_TO_LINE_COLUMN:
            return "collapseToLine:column:";
        case RECORZ_MVP_SELECTOR_SYNC_CURSOR_TEXT_INDEX_TOP_VISIBLE:
            return "syncCursorText:index:top:visible:";
        case RECORZ_MVP_SELECTOR_SYNC_REMAINING_INDEX_LINE_COLUMN_TARGET_TOP_VISIBLE_CURSOR_SELECTION_TEXT:
            return "syncRemaining:index:line:column:target:top:visible:cursor:selection:text:";
        case RECORZ_MVP_SELECTOR_FINISH_CURSOR_SELECTION_INDEX_LINE_COLUMN_TOP_VISIBLE:
            return "finishCursor:selection:index:line:column:top:visible:";
        case RECORZ_MVP_SELECTOR_ADJUST_TOP_FOR_LINE_REQUESTED_VISIBLE:
            return "adjustTopForLine:requested:visible:";
        case RECORZ_MVP_SELECTOR_DRAW_SOURCE_ON_FORM_VISIBLE:
            return "drawSource:onForm:visible:";
        case RECORZ_MVP_SELECTOR_DRAW_TEXT_INDEX_LINE_TOP_REMAINING_ON_FORM_OPEN:
            return "drawText:index:line:top:remaining:onForm:open:";
        case RECORZ_MVP_SELECTOR_PAD_BLANK_ON_FORM:
            return "padBlank:onForm:";
        case RECORZ_MVP_SELECTOR_DRAW_CURSOR_ON_FORM:
            return "drawCursorOnForm:";
        case RECORZ_MVP_SELECTOR_DRAW_STATUS_FEEDBACK_ON_FORM_LINES:
            return "drawStatus:feedback:onForm:lines:";
        case RECORZ_MVP_SELECTOR_DRAW_FEEDBACK_INDEX_ON_FORM_REMAINING:
            return "drawFeedback:index:onForm:remaining:";
        case RECORZ_MVP_SELECTOR_DRAW_FEEDBACK_LINE_INDEX_ON_FORM_REMAINING:
            return "drawFeedbackLine:index:onForm:remaining:";
        case RECORZ_MVP_SELECTOR_PAD_FEEDBACK_ON_FORM:
            return "padFeedback:onForm:";
        case RECORZ_MVP_SELECTOR_SET_POINT_SIZE:
            return "setPointSize:";
        case RECORZ_MVP_SELECTOR_SCALE_LINE_SPACING:
            return "scale:lineSpacing:";
        case RECORZ_MVP_SELECTOR_LEFT_TOP_RIGHT_BOTTOM:
            return "left:top:right:bottom:";
        case RECORZ_MVP_SELECTOR_USE_PRIMARY_TEXT_MODE:
            return "usePrimaryTextMode";
        case RECORZ_MVP_SELECTOR_USE_COMFORT_TEXT_MODE:
            return "useComfortTextMode";
        case RECORZ_MVP_SELECTOR_SET_CELL_WIDTH_CELL_HEIGHT_BASELINE:
            return "setCellWidth:cellHeight:baseline:";
        case RECORZ_MVP_SELECTOR_SET_ASCENT_DESCENT_LINE_HEIGHT:
            return "setAscent:descent:lineHeight:";
        case RECORZ_MVP_SELECTOR_SET_WRAP_WIDTH_TAB_WIDTH_LINE_BREAK_MODE:
            return "setWrapWidth:tabWidth:lineBreakMode:";
        case RECORZ_MVP_SELECTOR_DRAW_TEXT_INDEX_LINE_COLUMN_TOP_REMAINING_ON_FORM_OPEN:
            return "drawText:index:line:column:top:remaining:onForm:open:";
        case RECORZ_MVP_SELECTOR_SELECTION_COVERS_LINE_COLUMN:
            return "selectionCoversLine:column:";
        case RECORZ_MVP_SELECTOR_SET_START_LINE_START_COLUMN_END_LINE_END_COLUMN:
            return "setStartLine:startColumn:endLine:endColumn:";
        case RECORZ_MVP_SELECTOR_CONTAINS_LINE_COLUMN:
            return "containsLine:column:";
        case RECORZ_MVP_SELECTOR_DRAW_TAIL_ON_FORM_OPEN:
            return "drawTail:onForm:open:";
        case RECORZ_MVP_SELECTOR_DRAW_CODE_TEXT_NEXT_INDEX_LINE_COLUMN_TOP_REMAINING_ON_FORM:
            return "drawCode:text:nextIndex:line:column:top:remaining:onForm:";
        case RECORZ_MVP_SELECTOR_DRAW_SKIPPED_CODE_TEXT_NEXT_INDEX_LINE_COLUMN_TOP_REMAINING_ON_FORM:
            return "drawSkippedCode:text:nextIndex:line:column:top:remaining:onForm:";
        case RECORZ_MVP_SELECTOR_UNDERLINE_CELL_LINE_COLUMN_ON_FORM:
            return "underlineCellLine:column:onForm:";
        case RECORZ_MVP_SELECTOR_MOVE_SOURCE_CURSOR_FOR_LINE_COLUMN_TOP:
            return "moveSourceCursorForLine:column:top:";
        case RECORZ_MVP_SELECTOR_DRAW_SELECTED_CODE_LINE_COLUMN_ON_FORM:
            return "drawSelectedCode:line:column:onForm:";
        case RECORZ_MVP_SELECTOR_SET_LEFT_TOP_WIDTH_HEIGHT:
            return "setLeft:top:width:height:";
        case RECORZ_MVP_SELECTOR_CONTAINS_X_Y:
            return "containsX:y:";
        case RECORZ_MVP_SELECTOR_SET_BOUNDS_TITLE:
            return "setBounds:title:";
        case RECORZ_MVP_SELECTOR_INVALIDATE:
            return "invalidate";
        case RECORZ_MVP_SELECTOR_VALIDATE:
            return "validate";
        case RECORZ_MVP_SELECTOR_NEEDS_REDRAW:
            return "needsRedraw";
        case RECORZ_MVP_SELECTOR_TAKE_FOCUS:
            return "takeFocus";
        case RECORZ_MVP_SELECTOR_LOSE_FOCUS:
            return "loseFocus";
        case RECORZ_MVP_SELECTOR_HAS_FOCUS:
            return "hasFocus";
        case RECORZ_MVP_SELECTOR_REDRAW_ON_FORM:
            return "redrawOnForm:";
        case RECORZ_MVP_SELECTOR_DRAW_BORDER_ON_FORM_COLOR:
            return "drawBorderOnForm:color:";
        case RECORZ_MVP_SELECTOR_DRAW_TITLE_ON_FORM:
            return "drawTitleOnForm:";
        case RECORZ_MVP_SELECTOR_BROWSE_INTERACTIVE_VIEWS:
            return "browseInteractiveViews";
        case RECORZ_MVP_SELECTOR_SET_PRIMARY_SECONDARY:
            return "setPrimary:secondary:";
        case RECORZ_MVP_SELECTOR_HANDLE_BYTE_ON_FORM:
            return "handleByte:onForm:";
        case RECORZ_MVP_SELECTOR_SWITCH_FOCUS:
            return "switchFocus";
        case RECORZ_MVP_SELECTOR_DISPATCH_COMMAND_ON_FORM:
            return "dispatchCommandOnForm:";
        case RECORZ_MVP_SELECTOR_PRINT_CURRENT:
            return "printCurrent";
        case RECORZ_MVP_SELECTOR_BOUNDS:
            return "bounds";
        case RECORZ_MVP_SELECTOR_SET_BOUNDS:
            return "setBounds:";
        case RECORZ_MVP_SELECTOR_SET_HORIZONTAL_BOUNDS_OFFSET_GAP:
            return "setHorizontalBounds:offset:gap:";
        case RECORZ_MVP_SELECTOR_SET_VERTICAL_BOUNDS_OFFSET_GAP:
            return "setVerticalBounds:offset:gap:";
        case RECORZ_MVP_SELECTOR_ARRANGE:
            return "arrange";
        case RECORZ_MVP_SELECTOR_APPLY_PRIMARY_SECONDARY:
            return "applyPrimary:secondary:";
        case RECORZ_MVP_SELECTOR_PRIMARY_BOUNDS:
            return "primaryBounds";
        case RECORZ_MVP_SELECTOR_SECONDARY_BOUNDS:
            return "secondaryBounds";
        case RECORZ_MVP_SELECTOR_SET_VIEW_TEXT:
            return "setView:text:";
        case RECORZ_MVP_SELECTOR_SET_VIEW_ITEMS_SELECTED_INDEX_TOP_LINE:
            return "setView:items:selectedIndex:topLine:";
        case RECORZ_MVP_SELECTOR_SET_VIEW_TEXT_CURSOR_SELECTION:
            return "setView:text:cursor:selection:";
        case RECORZ_MVP_SELECTOR_SET_VIEW_STATUS_FEEDBACK:
            return "setView:status:feedback:";
        case RECORZ_MVP_SELECTOR_SET_VIEW_COMMANDS_SELECTED_INDEX:
            return "setView:commands:selectedIndex:";
        case RECORZ_MVP_SELECTOR_DRAW_SOURCE_STATUS_FEEDBACK_ON_FORM:
            return "drawSource:status:feedback:onForm:";
        case RECORZ_MVP_SELECTOR_DRAW_WORKSPACE_CONTENT_ON_FORM_BOUNDS:
            return "drawWorkspaceContentOnForm:bounds:";
        case RECORZ_MVP_SELECTOR_DRAW_WORKSPACE_STATUS_ON_FORM_BOUNDS:
            return "drawWorkspaceStatusOnForm:bounds:";
        case RECORZ_MVP_SELECTOR_DRAW_LIST_SOURCE_TITLE_STATUS_ON_FORM:
            return "drawList:source:title:status:onForm:";
        case RECORZ_MVP_SELECTOR_DRAW_BROWSER_HEADER_TITLE_ON_FORM_BOUNDS:
            return "drawBrowserHeaderTitle:onForm:bounds:";
        case RECORZ_MVP_SELECTOR_DRAW_BROWSER_LIST_ON_FORM_BOUNDS:
            return "drawBrowserListOnForm:bounds:";
        case RECORZ_MVP_SELECTOR_DRAW_BROWSER_SOURCE_ON_FORM_BOUNDS:
            return "drawBrowserSourceOnForm:bounds:";
        case RECORZ_MVP_SELECTOR_DRAW_BROWSER_STATUS_ON_FORM_BOUNDS:
            return "drawBrowserStatusOnForm:bounds:";
        case RECORZ_MVP_SELECTOR_SEED_BOOT_CONTENTS:
            return "seedBootContents:";
        case RECORZ_MVP_SELECTOR_BROWSE_INTERACTIVE_INPUT:
            return "browseInteractiveInput";
        case RECORZ_MVP_SELECTOR_INTERACTIVE_INPUT_MONITOR:
            return "interactiveInputMonitor";
        case RECORZ_MVP_SELECTOR_EDIT_METHOD_OF_CLASS_NAMED:
            return "editMethod:ofClassNamed:";
        case RECORZ_MVP_SELECTOR_EDIT_PACKAGE_NAMED:
            return "editPackageNamed:";
        case RECORZ_MVP_SELECTOR_REVERT_CURRENT:
            return "revertCurrent";
        case RECORZ_MVP_SELECTOR_RUN_CURRENT_TESTS:
            return "runCurrentTests";
        case RECORZ_MVP_SELECTOR_EDIT_CURRENT:
            return "editCurrent";
        case RECORZ_MVP_SELECTOR_CURRENT_VIEW_KIND:
            return "currentViewKind";
        case RECORZ_MVP_SELECTOR_SET_CURRENT_VIEW_KIND:
            return "setCurrentViewKind:";
        case RECORZ_MVP_SELECTOR_CURRENT_TARGET_NAME:
            return "currentTargetName";
        case RECORZ_MVP_SELECTOR_SET_CURRENT_TARGET_NAME:
            return "setCurrentTargetName:";
        case RECORZ_MVP_SELECTOR_PACKAGE_COUNT:
            return "packageCount";
        case RECORZ_MVP_SELECTOR_PACKAGE_NAME_AT:
            return "packageNameAt:";
        case RECORZ_MVP_SELECTOR_PACKAGE_NAMES_VISIBLE_FROM_COUNT:
            return "packageNamesVisibleFrom:count:";
        case RECORZ_MVP_SELECTOR_CLASS_COUNT:
            return "classCount";
        case RECORZ_MVP_SELECTOR_CLASS_NAME_AT:
            return "classNameAt:";
        case RECORZ_MVP_SELECTOR_CLASS_NAMES_VISIBLE_FROM_COUNT:
            return "classNamesVisibleFrom:count:";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_LEFT:
            return "moveCursorLeft";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_RIGHT:
            return "moveCursorRight";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_UP:
            return "moveCursorUp";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_DOWN:
            return "moveCursorDown";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_TO_LINE_START:
            return "moveCursorToLineStart";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_TO_LINE_END:
            return "moveCursorToLineEnd";
        case RECORZ_MVP_SELECTOR_INSERT_CODE_POINT:
            return "insertCodePoint:";
        case RECORZ_MVP_SELECTOR_DELETE_BACKWARD:
            return "deleteBackward";
        case RECORZ_MVP_SELECTOR_STATUS_TEXT:
            return "statusText";
        case RECORZ_MVP_SELECTOR_FEEDBACK_TEXT:
            return "feedbackText";
        case RECORZ_MVP_SELECTOR_DRAW_SELECTION_ON_FORM:
            return "drawSelectionOnForm:";
        case RECORZ_MVP_SELECTOR_OPEN_ON_MODE:
            return "openOn:mode:";
        case RECORZ_MVP_SELECTOR_REDRAW_BROWSER_ON_FORM:
            return "redrawBrowserOnForm:";
        case RECORZ_MVP_SELECTOR_REDRAW_EDITOR_ON_FORM:
            return "redrawEditorOnForm:";
        case RECORZ_MVP_SELECTOR_VISIBLE_EDITOR_LINES:
            return "visibleEditorLines";
        case RECORZ_MVP_SELECTOR_VISIBLE_LIST_LINES:
            return "visibleListLines";
        case RECORZ_MVP_SELECTOR_SELECTED_INDEX:
            return "selectedIndex";
        case RECORZ_MVP_SELECTOR_LIST_TOP_LINE:
            return "listTopLine";
        case RECORZ_MVP_SELECTOR_MOVE_LIST_SELECTION_BY:
            return "moveListSelectionBy:";
        case RECORZ_MVP_SELECTOR_SELECTED_PACKAGE_NAME:
            return "selectedPackageName";
        case RECORZ_MVP_SELECTOR_UPDATE_TOOL_STATUS:
            return "updateToolStatus";
        case RECORZ_MVP_SELECTOR_CURRENT_STATUS_TEXT:
            return "currentStatusText";
        case RECORZ_MVP_SELECTOR_CURRENT_FEEDBACK_TEXT:
            return "currentFeedbackText";
        case RECORZ_MVP_SELECTOR_DRAW_VIEW_STATE_ON_FORM:
            return "drawViewStateOnForm:";
        case RECORZ_MVP_SELECTOR_MOVE_LIST_SELECTION_PAGE_BY:
            return "moveListSelectionPageBy:";
        case RECORZ_MVP_SELECTOR_SOURCE_EDITOR_VIEW_ACTIVE:
            return "sourceEditorViewActive";
        case RECORZ_MVP_SELECTOR_SOURCE_EDITOR_STATUS_TEXT:
            return "sourceEditorStatusText";
        case RECORZ_MVP_SELECTOR_SCROLL_EDITOR_UP_REMAINING:
            return "scrollEditorUpRemaining:";
        case RECORZ_MVP_SELECTOR_SCROLL_EDITOR_DOWN_REMAINING:
            return "scrollEditorDownRemaining:";
        case RECORZ_MVP_SELECTOR_SCROLL_EDITOR_PAGE_BY:
            return "scrollEditorPageBy:";
        case RECORZ_MVP_SELECTOR_HANDLE_ESCAPE_BYTE:
            return "handleEscapeByte:";
        case RECORZ_MVP_SELECTOR_HANDLE_BROWSER_BYTE:
            return "handleBrowserByte:";
        case RECORZ_MVP_SELECTOR_OPEN_SELECTED_PACKAGE_SOURCE:
            return "openSelectedPackageSource";
        case RECORZ_MVP_SELECTOR_HANDLE_EDITOR_BYTE_TOOL:
            return "handleEditorByte:tool:";
        case RECORZ_MVP_SELECTOR_SYNC_EDITOR_CURSOR:
            return "syncEditorCursor";
        case RECORZ_MVP_SELECTOR_VISIBLE_CONTENTS_TOP_LINES_COLUMNS:
            return "visibleContentsTop:lines:columns:";
        case RECORZ_MVP_SELECTOR_VISIBLE_EDITOR_COLUMNS:
            return "visibleEditorColumns";
        case RECORZ_MVP_SELECTOR_DRAW_EDITOR_CURSOR_ON_FORM:
            return "drawEditorCursorOnForm:";
        case RECORZ_MVP_SELECTOR_WORKSPACE_NAME:
            return "workspaceName";
        case RECORZ_MVP_SELECTOR_SET_WORKSPACE_NAME:
            return "setWorkspaceName:";
        case RECORZ_MVP_SELECTOR_VISIBLE_LEFT_COLUMN:
            return "visibleLeftColumn";
        case RECORZ_MVP_SELECTOR_SET_VISIBLE_LEFT_COLUMN:
            return "setVisibleLeftColumn:";
        case RECORZ_MVP_SELECTOR_VISIBLE_CONTENTS_TOP_LEFT_LINES_COLUMNS:
            return "visibleContentsTop:left:lines:columns:";
        case RECORZ_MVP_SELECTOR_ENSURE_EDITOR_CURSOR_VISIBLE:
            return "ensureEditorCursorVisible";
        case RECORZ_MVP_SELECTOR_SET_STATUS_TEXT:
            return "setStatusText:";
        case RECORZ_MVP_SELECTOR_SET_FEEDBACK_TEXT:
            return "setFeedbackText:";
        case RECORZ_MVP_SELECTOR_VISIBLE_TOP_LINE:
            return "visibleTopLine";
        case RECORZ_MVP_SELECTOR_SET_VISIBLE_ORIGIN_TOP_LEFT:
            return "setVisibleOriginTop:left:";
        case RECORZ_MVP_SELECTOR_IS_MODIFIED:
            return "isModified";
        case RECORZ_MVP_SELECTOR_CURRENT_TARGET_LABEL:
            return "currentTargetLabel";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_TO_VISIBLE_TOP:
            return "moveCursorToVisibleTop";
        case RECORZ_MVP_SELECTOR_MOVE_CURSOR_TO_VISIBLE_BOTTOM:
            return "moveCursorToVisibleBottom";
        case RECORZ_MVP_SELECTOR_MOVE_LIST_SELECTION_TO_VISIBLE_TOP:
            return "moveListSelectionToVisibleTop";
        case RECORZ_MVP_SELECTOR_MOVE_LIST_SELECTION_TO_VISIBLE_BOTTOM:
            return "moveListSelectionToVisibleBottom";
        case RECORZ_MVP_SELECTOR_CURRENT_HEADER_TEXT:
            return "currentHeaderText";
        case RECORZ_MVP_SELECTOR_NAMED_OBJECT_OR_NIL:
            return "namedObjectOrNil:";
        case RECORZ_MVP_SELECTOR_ACCEPT_COMMAND_BYTE:
            return "acceptCommandByte";
        case RECORZ_MVP_SELECTOR_REVERT_COMMAND_BYTE:
            return "revertCommandByte";
        case RECORZ_MVP_SELECTOR_RUN_TESTS_COMMAND_BYTE:
            return "runTestsCommandByte";
        case RECORZ_MVP_SELECTOR_PRINT_COMMAND_BYTE:
            return "printCommandByte";
        case RECORZ_MVP_SELECTOR_DO_IT_COMMAND_BYTE:
            return "doItCommandByte";
        case RECORZ_MVP_SELECTOR_ALTERNATE_DO_IT_COMMAND_BYTE:
            return "alternateDoItCommandByte";
        case RECORZ_MVP_SELECTOR_SAVE_AND_REOPEN_COMMAND_BYTE:
            return "saveAndReopenCommandByte";
        case RECORZ_MVP_SELECTOR_SAVE_RECOVERY_SNAPSHOT_COMMAND_BYTE:
            return "saveRecoverySnapshotCommandByte";
        case RECORZ_MVP_SELECTOR_RETURN_TO_BROWSER_COMMAND_BYTE:
            return "returnToBrowserCommandByte";
        case RECORZ_MVP_SELECTOR_SOURCE_EDITOR_VIEW_KIND:
            return "sourceEditorViewKind:";
        case RECORZ_MVP_SELECTOR_DISPATCH_WORKSPACE_COMMAND_ON_FORM:
            return "dispatchWorkspaceCommandOnForm:";
        case RECORZ_MVP_SELECTOR_DISPATCH_SOURCE_EDITOR_COMMAND_ON_FORM:
            return "dispatchSourceEditorCommandOnForm:";
        case RECORZ_MVP_SELECTOR_VISIBLE_ORIGIN:
            return "visibleOrigin";
        case RECORZ_MVP_SELECTOR_REMEMBER_PLAIN_WORKSPACE_STATE_CONTENTS:
            return "rememberPlainWorkspaceStateContents:";
        case RECORZ_MVP_SELECTOR_RESTORE_PLAIN_WORKSPACE_STATE:
            return "restorePlainWorkspaceState";
        case RECORZ_MVP_SELECTOR_SET_CONTENTS_CURSOR_SELECTION_VISIBLE_ORIGIN:
            return "setContents:cursor:selection:visibleOrigin:";
        case RECORZ_MVP_SELECTOR_COLLAPSE_SELECTION_TO_CURSOR:
            return "collapseSelectionToCursor";
        case RECORZ_MVP_SELECTOR_ENSURE_EDITOR_CURSOR_VISIBLE_FOR_LINES_VISIBLE_COLUMNS:
            return "ensureEditorCursorVisibleForLines:visibleColumns:";
        case RECORZ_MVP_SELECTOR_HANDLE_EDITOR_ARROW_BYTE_VISIBLE_LINES_VISIBLE_COLUMNS:
            return "handleEditorArrowByte:visibleLines:visibleColumns:";
        case RECORZ_MVP_SELECTOR_HANDLE_EDITOR_INPUT_BYTE_VISIBLE_LINES_VISIBLE_COLUMNS:
            return "handleEditorInputByte:visibleLines:visibleColumns:";
        case RECORZ_MVP_SELECTOR_FINISH_EDITOR_INTERACTION_FOR_LINES_VISIBLE_COLUMNS_REDRAW_CODE:
            return "finishEditorInteractionForLines:visibleColumns:redrawCode:";
        case RECORZ_MVP_SELECTOR_REMEMBER_CURRENT_PLAIN_WORKSPACE_STATE_ON_TOOL:
            return "rememberCurrentPlainWorkspaceStateOnTool:";
        case RECORZ_MVP_SELECTOR_REMEMBER_PLAIN_WORKSPACE_STATE_IF_NEEDED:
            return "rememberPlainWorkspaceStateIfNeeded";
        case RECORZ_MVP_SELECTOR_PACKAGE_SOURCE_TO_OPEN_FOR_PRIOR_TARGET:
            return "packageSourceToOpenFor:priorTarget:";
        case RECORZ_MVP_SELECTOR_BROWSER_RETURN_VIEW_KIND:
            return "browserReturnViewKind";
        case RECORZ_MVP_SELECTOR_SET_BROWSER_RETURN_VIEW_KIND:
            return "setBrowserReturnViewKind:";
        case RECORZ_MVP_SELECTOR_BROWSER_LIST_VIEW_ACTIVE:
            return "browserListViewActive";
        case RECORZ_MVP_SELECTOR_OPENING_MENU_OPTION_COUNT:
            return "openingMenuOptionCount";
        case RECORZ_MVP_SELECTOR_CURRENT_LIST_COUNT:
            return "currentListCount";
        case RECORZ_MVP_SELECTOR_PREPARE_LIST_SELECTION_FOR_COUNT:
            return "prepareListSelectionForCount:";
        case RECORZ_MVP_SELECTOR_PREPARE_BROWSER_STATE_FOR_MODE:
            return "prepareBrowserStateForMode:";
        case RECORZ_MVP_SELECTOR_CURRENT_BROWSER_ITEMS_VISIBLE_FROM_COUNT:
            return "currentBrowserItemsVisibleFrom:count:";
        case RECORZ_MVP_SELECTOR_OPENING_MENU_ITEMS_VISIBLE_FROM_COUNT:
            return "openingMenuItemsVisibleFrom:count:";
        case RECORZ_MVP_SELECTOR_CURRENT_BROWSER_HEADER_TEXT:
            return "currentBrowserHeaderText";
        case RECORZ_MVP_SELECTOR_CURRENT_BROWSER_SOURCE_TEXT:
            return "currentBrowserSourceText";
        case RECORZ_MVP_SELECTOR_SELECTED_CLASS_NAME:
            return "selectedClassName";
        case RECORZ_MVP_SELECTOR_OPEN_SELECTED_OPENING_MENU_ITEM:
            return "openSelectedOpeningMenuItem";
        case RECORZ_MVP_SELECTOR_OPEN_SELECTED_CLASS_SOURCE:
            return "openSelectedClassSource";
        case RECORZ_MVP_SELECTOR_CLASS_SOURCE_TO_OPEN_FOR_PRIOR_TARGET:
            return "classSourceToOpenFor:priorTarget:";
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
        case RECORZ_MVP_OBJECT_TEXT_VERTICAL_METRICS:
            return "TextVerticalMetrics";
        case RECORZ_MVP_OBJECT_STYLED_TEXT:
            return "StyledText";
        case RECORZ_MVP_OBJECT_TEXT_MARGINS:
            return "TextMargins";
        case RECORZ_MVP_OBJECT_TEXT_FLOW:
            return "TextFlow";
        case RECORZ_MVP_OBJECT_TEXT_CURSOR:
            return "TextCursor";
        case RECORZ_MVP_OBJECT_TEXT_SELECTION:
            return "TextSelection";
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
        case RECORZ_MVP_OBJECT_TEST_RUNNER:
            return "TestRunner";
        case RECORZ_MVP_OBJECT_FONT:
            return "Font";
        case RECORZ_MVP_OBJECT_CURSOR:
            return "Cursor";
        case RECORZ_MVP_OBJECT_CURSOR_FACTORY:
            return "CursorFactory";
        case RECORZ_MVP_OBJECT_CHARACTER_SCANNER:
            return "CharacterScanner";
        case RECORZ_MVP_OBJECT_WORKSPACE_TOOL:
            return "WorkspaceTool";
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
    uint16_t selector,
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

static uint8_t source_char_is_live_binary_selector(char ch) {
    return (uint8_t)(source_char_is_binary_selector(ch) || ch == '/' || ch == '%');
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

static uint8_t parse_decimal_u32_prefix(
    const char *text,
    const char **cursor_out,
    uint32_t *value_out
) {
    uint32_t value = 0U;
    uint8_t saw_digit = 0U;

    while (*text >= '0' && *text <= '9') {
        saw_digit = 1U;
        value = (value * 10U) + (uint32_t)(*text - '0');
        ++text;
    }
    if (!saw_digit) {
        return 0U;
    }
    *cursor_out = text;
    *value_out = value;
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

static uint8_t source_copy_raw_line(const char **cursor_ref, char buffer[], uint32_t buffer_size) {
    const char *cursor = *cursor_ref;
    uint32_t length = 0U;

    if (*cursor == '\0') {
        buffer[0] = '\0';
        return 0U;
    }
    while (*cursor != '\0' && *cursor != '\n' && *cursor != '\r') {
        if (length + 1U >= buffer_size) {
            break;
        }
        buffer[length++] = *cursor++;
    }
    buffer[length] = '\0';
    if (*cursor == '\r') {
        ++cursor;
    }
    if (*cursor == '\n') {
        ++cursor;
    }
    *cursor_ref = cursor;
    return 1U;
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
static const char *class_name_for_object(const struct recorz_mvp_heap_object *class_object);
static const struct recorz_mvp_live_method_source *live_method_source_for(
    uint16_t class_handle,
    uint16_t selector_id
);
static const char *live_method_source_text(const struct recorz_mvp_live_method_source *source_record);
static const struct recorz_mvp_live_method_source *live_method_source_for_selector_and_arity(
    uint16_t class_handle,
    uint16_t selector_id,
    uint8_t argument_count
);
static const struct recorz_mvp_live_method_source *live_method_source_for_class_chain(
    const struct recorz_mvp_heap_object *class_object,
    uint16_t selector_id,
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
    const char *detail_text,
    uint8_t allocate_context_object
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
static struct recorz_mvp_source_eval_result source_execute_inline_block_source(
    struct recorz_mvp_source_method_context *context,
    const char *source
);
static struct recorz_mvp_source_eval_result source_evaluate_conditional_expression(
    struct recorz_mvp_source_method_context *context,
    struct recorz_mvp_source_eval_result receiver_result,
    const char *cursor,
    const char **cursor_out
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
    uint16_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    uint16_t sender_context_handle,
    const char *text
);
static void perform_send(
    struct recorz_mvp_value receiver,
    uint16_t selector,
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
    if (source_names_equal(name, "TestRunner")) {
        return RECORZ_MVP_GLOBAL_TEST_RUNNER;
    }
    if (source_names_equal(name, "Cursor")) {
        return RECORZ_MVP_GLOBAL_CURSOR;
    }
    if (source_names_equal(name, "WorkspaceCursor")) {
        return RECORZ_MVP_GLOBAL_WORKSPACE_CURSOR;
    }
    if (source_names_equal(name, "WorkspaceSelection")) {
        return RECORZ_MVP_GLOBAL_WORKSPACE_SELECTION;
    }
    return 0U;
}

static uint8_t display_source_root_id_for_selector(
    const char *selector_name,
    uint32_t *root_id_out
) {
    if (source_names_equal(selector_name, "defaultForm")) {
        *root_id_out = RECORZ_MVP_SEED_ROOT_DEFAULT_FORM;
        return 1U;
    }
    if (source_names_equal(selector_name, "font")) {
        *root_id_out = RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT;
        return 1U;
    }
    if (source_names_equal(selector_name, "style")) {
        *root_id_out = RECORZ_MVP_SEED_ROOT_TRANSCRIPT_STYLE;
        return 1U;
    }
    if (source_names_equal(selector_name, "layout")) {
        *root_id_out = RECORZ_MVP_SEED_ROOT_TRANSCRIPT_LAYOUT;
        return 1U;
    }
    return 0U;
}

static uint16_t source_selector_id_for_name(const char *name) {
    uint16_t selector;

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
    const char *detail_text,
    uint8_t allocate_context_object
) {
    uint16_t home_index;

    for (home_index = 0U; home_index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++home_index) {
        if (!source_eval_home_contexts[home_index].in_use) {
            source_eval_home_contexts[home_index].in_use = 1U;
            source_eval_home_contexts[home_index].alive = 1U;
            source_eval_home_contexts[home_index].defining_class = defining_class;
            source_eval_home_contexts[home_index].receiver = receiver;
            source_eval_home_contexts[home_index].lexical_environment_index = lexical_environment_index;
            source_eval_home_contexts[home_index].context_handle =
                allocate_context_object != 0U ?
                    allocate_source_context_object(
                        sender_context_handle,
                        receiver,
                        detail_text
                    ) :
                    0U;
            return (int16_t)home_index;
        }
    }
    machine_panic("source home context pool overflow");
    return -1;
}

static uint8_t source_block_state_references_home_context(int16_t home_context_index) {
    uint16_t handle;

    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = &heap[handle - 1U];
        struct recorz_mvp_value home_context_value;

        if (object->kind != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
            continue;
        }
        home_context_value = object->fields[BLOCK_CLOSURE_FIELD_LEXICAL1];
        if (home_context_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
            (int16_t)small_integer_i32(
                home_context_value,
                "block closure home context state is not a small integer"
            ) == home_context_index) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t source_block_state_references_lexical_environment(int16_t lexical_environment_index) {
    uint16_t handle;

    for (handle = 1U; handle <= heap_size; ++handle) {
        const struct recorz_mvp_heap_object *object = &heap[handle - 1U];
        struct recorz_mvp_value lexical_environment_value;

        if (object->kind != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
            continue;
        }
        lexical_environment_value = object->fields[BLOCK_CLOSURE_FIELD_LEXICAL0];
        if (lexical_environment_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
            (int16_t)small_integer_i32(
                lexical_environment_value,
                "block closure lexical environment state is not a small integer"
            ) == lexical_environment_index) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t source_home_context_references_lexical_environment(int16_t lexical_environment_index) {
    uint16_t home_index;

    for (home_index = 0U; home_index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++home_index) {
        if (source_eval_home_contexts[home_index].in_use &&
            source_eval_home_contexts[home_index].lexical_environment_index == lexical_environment_index) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t source_child_environment_references_parent(int16_t lexical_environment_index) {
    uint16_t env_index;

    for (env_index = 0U; env_index < SOURCE_EVAL_ENV_LIMIT; ++env_index) {
        if (source_eval_environments[env_index].in_use &&
            source_eval_environments[env_index].parent_index == lexical_environment_index) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t source_lexical_environment_is_referenced(int16_t lexical_environment_index) {
    return (uint8_t)(
        source_block_state_references_lexical_environment(lexical_environment_index) ||
        source_home_context_references_lexical_environment(lexical_environment_index) ||
        source_child_environment_references_parent(lexical_environment_index)
    );
}

static void source_release_single_lexical_environment_if_unused(int16_t lexical_environment_index) {
    struct recorz_mvp_source_lexical_environment *environment;
    uint16_t binding_index;

    if (lexical_environment_index < 0 ||
        lexical_environment_index >= (int16_t)SOURCE_EVAL_ENV_LIMIT ||
        !source_eval_environments[lexical_environment_index].in_use ||
        source_lexical_environment_is_referenced(lexical_environment_index)) {
        return;
    }
    environment = &source_eval_environments[lexical_environment_index];
    environment->in_use = 0U;
    environment->binding_count = 0U;
    environment->parent_index = -1;
    for (binding_index = 0U; binding_index < SOURCE_EVAL_BINDING_LIMIT; ++binding_index) {
        environment->bindings[binding_index].name[0] = '\0';
        environment->bindings[binding_index].value = nil_value();
    }
}

static void source_release_home_context_if_unused(int16_t home_context_index) {
    struct recorz_mvp_source_home_context *home_context;

    if (home_context_index < 0 ||
        home_context_index >= (int16_t)SOURCE_EVAL_HOME_CONTEXT_LIMIT ||
        !source_eval_home_contexts[home_context_index].in_use) {
        return;
    }
    home_context = &source_eval_home_contexts[home_context_index];
    if (home_context->alive || source_block_state_references_home_context(home_context_index)) {
        return;
    }
    home_context->in_use = 0U;
    home_context->alive = 0U;
    home_context->defining_class = 0;
    home_context->receiver = nil_value();
    home_context->lexical_environment_index = -1;
    home_context->context_handle = 0U;
}

static void source_release_lexical_environment_chain_if_unused(int16_t lexical_environment_index) {
    while (lexical_environment_index >= 0 &&
           lexical_environment_index < (int16_t)SOURCE_EVAL_ENV_LIMIT &&
           source_eval_environments[lexical_environment_index].in_use) {
        struct recorz_mvp_source_lexical_environment *environment =
            &source_eval_environments[lexical_environment_index];
        int16_t parent_index = environment->parent_index;
 
        if (source_lexical_environment_is_referenced(lexical_environment_index)) {
            return;
        }
        source_release_single_lexical_environment_if_unused(lexical_environment_index);
        lexical_environment_index = parent_index;
    }
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
            string_value(runtime_string_intern_copy(detail_text))
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
    uint16_t operand_a,
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
    program->literals[index].string = runtime_string_allocate_copy(program->literal_texts[index]);
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
    uint16_t operand_a,
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
    uint16_t selector_id;

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
    uint16_t selector_id;

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
    uint16_t selector_id;

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
    uint8_t pending_value = 0U;

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
        machine_panic("Workspace source contains no executable statements");
    }
    if (!workspace_source_append_instruction(program, RECORZ_MVP_OP_RETURN, 0U, 0U)) {
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

    if (!heap_handle_is_live(object_handle)) {
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

static const struct recorz_mvp_heap_object *workspace_global_object(void) {
    uint16_t workspace_handle = global_handles[RECORZ_MVP_GLOBAL_WORKSPACE];

    if (!heap_handle_is_live(workspace_handle)) {
        machine_panic("Workspace global is unavailable");
    }
    return heap_object(workspace_handle);
}

static uint8_t workspace_send_tool_selector(uint16_t selector) {
    uint16_t tool_handle = named_object_handle_for_name("BootWorkspaceTool");

    if (tool_handle == 0U) {
        return 0U;
    }
    perform_send_and_pop_result(object_value(tool_handle), selector, 0U, 0, 0U);
    return 1U;
}

static struct recorz_mvp_heap_object *workspace_tool_object_or_null(void) {
    uint16_t tool_handle = named_object_handle_for_name("BootWorkspaceTool");

    if (tool_handle == 0U || !heap_handle_is_live(tool_handle)) {
        return 0;
    }
    return (struct recorz_mvp_heap_object *)heap_object(tool_handle);
}

static struct recorz_mvp_heap_object *workspace_visible_origin_object_or_null(void) {
    struct recorz_mvp_heap_object *tool_object = workspace_tool_object_or_null();
    struct recorz_mvp_value origin_value;
    uint16_t origin_handle;

    if (tool_object == 0) {
        return 0;
    }
    origin_value = heap_get_field(tool_object, WORKSPACE_TOOL_FIELD_VISIBLE_ORIGIN);
    if (origin_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        return 0;
    }
    origin_handle = (uint16_t)origin_value.integer;
    if (!heap_handle_is_live(origin_handle)) {
        return 0;
    }
    return (struct recorz_mvp_heap_object *)heap_object(origin_handle);
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
    if (global_id == 0U || global_id > MAX_GLOBAL_ID || !heap_handle_is_live(global_handles[global_id])) {
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
            return object_value(active_display_form_handle);
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
        case RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT:
            return object_value(transcript_font_handle);
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

static void append_memory_report_stat(
    char buffer[],
    uint32_t *offset,
    const char *label,
    uint32_t value
) {
    append_memory_report_text(buffer, offset, label);
    append_memory_report_text(buffer, offset, " ");
    append_memory_report_u32(buffer, offset, value);
    append_memory_report_text(buffer, offset, "\n");
}

static uint8_t heap_handle_is_live(uint16_t handle) {
    return (uint8_t)(handle != 0U && handle <= heap_size && heap[handle - 1U].kind != 0U);
}

static void heap_reset_slot(uint16_t handle) {
    struct recorz_mvp_heap_object *object;
    uint32_t field_index;

    if (handle == 0U || handle > HEAP_LIMIT) {
        machine_panic("heap slot handle is out of range");
    }
    object = &heap[handle - 1U];
    object->kind = 0U;
    object->field_count = 0U;
    object->class_handle = 0U;
    for (field_index = 0U; field_index < OBJECT_FIELD_LIMIT; ++field_index) {
        object->fields[field_index] = nil_value();
    }
}

static uint8_t gc_mark_bit_is_set(uint16_t handle) {
    uint16_t slot = (uint16_t)(handle - 1U);

    return (uint8_t)((gc_mark_bits[slot >> 3U] & (uint8_t)(1U << (slot & 7U))) != 0U);
}

static void gc_set_mark_bit(uint16_t handle) {
    uint16_t slot = (uint16_t)(handle - 1U);

    gc_mark_bits[slot >> 3U] |= (uint8_t)(1U << (slot & 7U));
}

static void gc_clear_mark_bits(void) {
    uint32_t index;

    for (index = 0U; index < sizeof(gc_mark_bits); ++index) {
        gc_mark_bits[index] = 0U;
    }
}

static void gc_push_temp_root(uint16_t handle) {
    if (gc_temp_root_count >= GC_TEMP_ROOT_LIMIT) {
        machine_panic("garbage collector temporary root stack overflow");
    }
    gc_temp_roots[gc_temp_root_count++] = handle;
}

static void gc_pop_temp_root(void) {
    if (gc_temp_root_count == 0U) {
        machine_panic("garbage collector temporary root stack underflow");
    }
    --gc_temp_root_count;
    gc_temp_roots[gc_temp_root_count] = 0U;
}

static void gc_mark_handle_if_live(uint16_t handle);

static void gc_mark_value_if_live(struct recorz_mvp_value value) {
    if (value.kind == RECORZ_MVP_VALUE_OBJECT) {
        gc_mark_handle_if_live((uint16_t)value.integer);
    }
}

static void gc_mark_class_method_table_if_live(const struct recorz_mvp_heap_object *class_object) {
    struct recorz_mvp_value method_start_value;
    uint32_t method_count;
    uint16_t start_handle;
    uint16_t method_offset;

    if (class_object->kind != RECORZ_MVP_OBJECT_CLASS) {
        return;
    }
    method_count = small_integer_u32(
        heap_get_field(class_object, CLASS_FIELD_METHOD_COUNT),
        "GC class method count is not a small integer"
    );
    if (method_count == 0U) {
        return;
    }
    method_start_value = heap_get_field(class_object, CLASS_FIELD_METHOD_START);
    if (method_start_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("GC class method start is not a method descriptor");
    }
    start_handle = (uint16_t)method_start_value.integer;
    if (start_handle == 0U || (uint32_t)start_handle + method_count - 1U > heap_size) {
        machine_panic("GC class method range is out of range");
    }
    for (method_offset = 0U; method_offset < method_count; ++method_offset) {
        gc_mark_handle_if_live((uint16_t)(start_handle + method_offset));
    }
}

static void gc_mark_handle_if_live(uint16_t handle) {
    const struct recorz_mvp_heap_object *object;
    uint8_t field_index;

    if (!heap_handle_is_live(handle) || gc_mark_bit_is_set(handle)) {
        return;
    }
    gc_set_mark_bit(handle);
    object = &heap[handle - 1U];
    if (object->class_handle != 0U) {
        gc_mark_handle_if_live(object->class_handle);
    }
    gc_mark_class_method_table_if_live(object);
    for (field_index = 0U; field_index < object->field_count; ++field_index) {
        gc_mark_value_if_live(object->fields[field_index]);
    }
}

static void gc_mark_conservative_stack_roots(void) {
    uintptr_t marker = 0U;
    uintptr_t start = (uintptr_t)&marker;
    uintptr_t end = gc_stack_base_address;
    uintptr_t scan;
    uintptr_t heap_start = (uintptr_t)heap;
    uintptr_t heap_end = (uintptr_t)(heap + heap_size);
    uintptr_t object_size = (uintptr_t)sizeof(struct recorz_mvp_heap_object);

    if (gc_stack_base_address == 0U) {
        return;
    }
    if (start > end) {
        uintptr_t swap = start;

        start = end;
        end = swap;
    }
    start &= ~((uintptr_t)sizeof(uintptr_t) - 1U);
    end &= ~((uintptr_t)sizeof(uintptr_t) - 1U);
    for (scan = start; scan <= end; scan += sizeof(uintptr_t)) {
        uintptr_t raw = *(const uintptr_t *)scan;

        if (raw >= 1U && raw <= (uintptr_t)heap_size) {
            gc_mark_handle_if_live((uint16_t)raw);
        }
        if (raw >= heap_start && raw < heap_end && object_size != 0U) {
            uintptr_t offset = raw - heap_start;

            if ((offset % object_size) == 0U) {
                gc_mark_handle_if_live((uint16_t)(offset / object_size) + 1U);
            }
        }
    }
}

static void gc_mark_roots(void) {
    uint32_t stack_index;
    uint16_t handle;
    uint16_t index;

    for (stack_index = 0U; stack_index < stack_size; ++stack_index) {
        gc_mark_value_if_live(stack[stack_index]);
    }
    gc_mark_value_if_live(panic_send_receiver);
    for (stack_index = 0U; stack_index < MAX_SEND_ARGS; ++stack_index) {
        gc_mark_value_if_live(panic_send_arguments[stack_index]);
    }
    for (handle = RECORZ_MVP_GLOBAL_TRANSCRIPT; handle <= MAX_GLOBAL_ID; ++handle) {
        gc_mark_handle_if_live(global_handles[handle]);
    }
    for (handle = 0U; handle <= MAX_OBJECT_KIND; ++handle) {
        gc_mark_handle_if_live(class_handles_by_kind[handle]);
        gc_mark_handle_if_live(class_descriptor_handles_by_kind[handle]);
    }
    for (handle = RECORZ_MVP_SELECTOR_SHOW; handle <= MAX_SELECTOR_ID; ++handle) {
        gc_mark_handle_if_live(selector_handles_by_id[handle]);
    }
    gc_mark_handle_if_live(default_form_handle);
    gc_mark_handle_if_live(active_display_form_handle);
    gc_mark_handle_if_live(active_cursor_handle);
    gc_mark_handle_if_live(framebuffer_bitmap_handle);
    gc_mark_handle_if_live(transcript_layout_handle);
    gc_mark_handle_if_live(transcript_style_handle);
    gc_mark_handle_if_live(transcript_metrics_handle);
    gc_mark_handle_if_live(transcript_behavior_handle);
    gc_mark_handle_if_live(transcript_font_handle);
    gc_mark_handle_if_live(startup_hook_receiver_handle);
    for (index = 0U; index < 128U; ++index) {
        gc_mark_handle_if_live(glyph_bitmap_handles[index]);
    }
    for (index = 0U; index < named_object_count; ++index) {
        gc_mark_handle_if_live(named_objects[index].object_handle);
    }
    for (index = 0U; index < dynamic_class_count; ++index) {
        gc_mark_handle_if_live(dynamic_classes[index].class_handle);
        gc_mark_handle_if_live(dynamic_classes[index].superclass_handle);
    }
    for (index = 0U; index < live_method_source_count; ++index) {
        gc_mark_handle_if_live(live_method_sources[index].class_handle);
    }
    for (index = 0U; index < LIVE_STRING_LITERAL_LIMIT; ++index) {
        gc_mark_handle_if_live(live_string_literals[index].class_handle);
    }
    for (index = 0U; index < SOURCE_EVAL_ENV_LIMIT; ++index) {
        if (source_eval_environments[index].in_use) {
            uint16_t binding_index;

            for (binding_index = 0U; binding_index < source_eval_environments[index].binding_count; ++binding_index) {
                gc_mark_value_if_live(source_eval_environments[index].bindings[binding_index].value);
            }
        }
    }
    for (index = 0U; index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++index) {
        if (source_eval_home_contexts[index].in_use) {
            gc_mark_value_if_live(source_eval_home_contexts[index].receiver);
            gc_mark_handle_if_live(source_eval_home_contexts[index].context_handle);
            if (source_eval_home_contexts[index].defining_class != 0) {
                gc_mark_handle_if_live(heap_handle_for_object(source_eval_home_contexts[index].defining_class));
            }
        }
    }
    for (index = 0U; index < gc_temp_root_count; ++index) {
        gc_mark_handle_if_live(gc_temp_roots[index]);
    }
    gc_mark_conservative_stack_roots();
}

static uint16_t gc_sweep_unmarked_slots(void) {
    uint16_t handle;
    uint16_t reclaimed = 0U;

    for (handle = 1U; handle <= heap_size; ++handle) {
        if (!heap_handle_is_live(handle) || gc_mark_bit_is_set(handle)) {
            continue;
        }
        heap_reset_slot(handle);
        ++reclaimed;
    }
    while (heap_size != 0U && !heap_handle_is_live(heap_size)) {
        --heap_size;
    }
    if (reclaimed > heap_live_count) {
        machine_panic("garbage collector reclaimed more objects than are live");
    }
    heap_live_count = (uint16_t)(heap_live_count - reclaimed);
    return reclaimed;
}

static uint16_t gc_release_unused_source_state(void) {
    uint16_t home_index;
    uint16_t released = 0U;

    for (home_index = 0U; home_index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++home_index) {
        if (source_eval_home_contexts[home_index].in_use) {
            int16_t lexical_environment_index = source_eval_home_contexts[home_index].lexical_environment_index;

            source_release_home_context_if_unused((int16_t)home_index);
            if (!source_eval_home_contexts[home_index].in_use) {
                ++released;
                source_release_lexical_environment_chain_if_unused(lexical_environment_index);
            }
        }
    }
    return released;
}

static uint16_t gc_collect_now(void) {
    uint16_t reclaimed;

    gc_clear_mark_bits();
    gc_mark_roots();
    reclaimed = gc_sweep_unmarked_slots();
    if (gc_release_unused_source_state() != 0U) {
        gc_clear_mark_bits();
        gc_mark_roots();
        reclaimed = (uint16_t)(reclaimed + gc_sweep_unmarked_slots());
    }
    runtime_string_compact_live_references();
    initialize_runtime_caches();
    gc_last_reclaimed_count = reclaimed;
    gc_total_reclaimed_count += reclaimed;
    ++gc_collection_count;
    return reclaimed;
}

static void gc_collect_preserving_value(struct recorz_mvp_value value) {
    if (value.kind == RECORZ_MVP_VALUE_OBJECT) {
        gc_push_temp_root((uint16_t)value.integer);
        (void)gc_collect_now();
        gc_pop_temp_root();
        return;
    }
    (void)gc_collect_now();
}

static uint8_t gc_collection_allowed_for_current_phase(void) {
    return (uint8_t)(gc_bootstrap_file_in_active == 0U);
}

static uint16_t heap_find_free_run(uint16_t count) {
    uint16_t handle = 1U;

    if (count == 0U) {
        machine_panic("heap allocation run count is zero");
    }
    while (handle <= heap_size) {
        uint16_t start_handle = handle;
        uint16_t run_length = 0U;

        while (handle <= heap_size && !heap_handle_is_live(handle) && run_length < count) {
            ++handle;
            ++run_length;
        }
        if (run_length >= count) {
            return start_handle;
        }
        if (run_length == 0U) {
            ++handle;
        }
    }
    if ((uint32_t)heap_size + count <= HEAP_LIMIT) {
        return (uint16_t)(heap_size + 1U);
    }
    return 0U;
}

static void heap_initialize_allocated_slot(uint16_t handle, uint8_t kind) {
    if (handle == 0U || handle > HEAP_LIMIT) {
        machine_panic("heap allocation handle is out of range");
    }
    if (handle > heap_size) {
        heap_size = handle;
        if (heap_size > heap_high_water_mark) {
            heap_high_water_mark = heap_size;
        }
    }
    heap_reset_slot(handle);
    heap[handle - 1U].kind = kind;
    ++heap_live_count;
}

static uint16_t heap_allocate_run(uint8_t kind, uint16_t count) {
    uint16_t start_handle = heap_find_free_run(count);
    uint16_t offset;

    if (start_handle == 0U) {
        machine_panic("object heap overflow");
    }
    for (offset = 0U; offset < count; ++offset) {
        heap_initialize_allocated_slot((uint16_t)(start_handle + offset), kind);
    }
    return start_handle;
}

static uint16_t heap_allocate(uint8_t kind) {
    return heap_allocate_run(kind, 1U);
}

static uint16_t heap_allocate_seeded_class_run(uint8_t kind, uint16_t count) {
    uint16_t start_handle = heap_allocate_run(kind, count);
    uint16_t offset;

    if (kind <= MAX_OBJECT_KIND && class_handles_by_kind[kind] != 0U) {
        for (offset = 0U; offset < count; ++offset) {
            heap_set_class((uint16_t)(start_handle + offset), class_handles_by_kind[kind]);
        }
    }
    return start_handle;
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
        string_value(runtime_string_intern_copy(source_text))
    );
    heap_set_field(handle, BLOCK_CLOSURE_FIELD_HOME_RECEIVER, home_receiver);
    heap_set_field(handle, BLOCK_CLOSURE_FIELD_LEXICAL0, small_integer_value(-1));
    heap_set_field(handle, BLOCK_CLOSURE_FIELD_LEXICAL1, small_integer_value(-1));
    return handle;
}

static void source_register_block_state(
    uint16_t block_handle,
    const struct recorz_mvp_heap_object *defining_class,
    int16_t lexical_environment_index,
    int16_t home_context_index
) {
    (void)defining_class;
    heap_set_field(block_handle, BLOCK_CLOSURE_FIELD_LEXICAL0, small_integer_value((int32_t)lexical_environment_index));
    heap_set_field(block_handle, BLOCK_CLOSURE_FIELD_LEXICAL1, small_integer_value((int32_t)home_context_index));
}

static uint8_t source_block_state_for_handle(
    uint16_t block_handle,
    struct recorz_mvp_runtime_block_state *state_out
) {
    const struct recorz_mvp_heap_object *block_object;
    struct recorz_mvp_value home_receiver;
    int16_t lexical_environment_index;
    int16_t home_context_index;
    struct recorz_mvp_source_home_context *home_context;

    if (block_handle == 0U || block_handle > heap_size || state_out == 0) {
        return 0U;
    }
    block_object = &heap[block_handle - 1U];
    if (block_object->kind != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
        return 0U;
    }
    lexical_environment_index = block_object->fields[BLOCK_CLOSURE_FIELD_LEXICAL0].kind == RECORZ_MVP_VALUE_SMALL_INTEGER
        ? (int16_t)small_integer_i32(
              block_object->fields[BLOCK_CLOSURE_FIELD_LEXICAL0],
              "block closure lexical environment state is not a small integer"
          )
        : -1;
    home_context_index = block_object->fields[BLOCK_CLOSURE_FIELD_LEXICAL1].kind == RECORZ_MVP_VALUE_SMALL_INTEGER
        ? (int16_t)small_integer_i32(
              block_object->fields[BLOCK_CLOSURE_FIELD_LEXICAL1],
              "block closure home context state is not a small integer"
          )
        : -1;
    if (lexical_environment_index >= 0 &&
        (lexical_environment_index >= (int16_t)SOURCE_EVAL_ENV_LIMIT ||
         !source_eval_environments[lexical_environment_index].in_use)) {
        lexical_environment_index = -1;
    }
    if (home_context_index >= 0 &&
        (home_context_index >= (int16_t)SOURCE_EVAL_HOME_CONTEXT_LIMIT ||
         !source_eval_home_contexts[home_context_index].in_use)) {
        home_context_index = -1;
    }
    if (lexical_environment_index < 0 && home_context_index < 0) {
        return 0U;
    }
    state_out->lexical_environment_index = lexical_environment_index;
    state_out->home_context_index = home_context_index;
    state_out->defining_class = 0;
    home_context = home_context_index >= 0 ? source_home_context_at(home_context_index) : 0;
    if (home_context != 0) {
        state_out->defining_class = home_context->defining_class;
    }
    if (state_out->defining_class == 0) {
        home_receiver = heap_get_field(block_object, BLOCK_CLOSURE_FIELD_HOME_RECEIVER);
        if (home_receiver.kind == RECORZ_MVP_VALUE_OBJECT) {
            state_out->defining_class = class_object_for_heap_object(heap_object_for_value(home_receiver));
        } else {
            state_out->defining_class = class_object_for_kind(RECORZ_MVP_OBJECT_OBJECT);
        }
    }
    return 1U;
}

static struct recorz_mvp_heap_object *heap_object(uint16_t handle) {
    if (handle == 0U || handle > heap_size) {
        machine_panic("invalid heap object handle");
    }
    return &heap[handle - 1U];
}

static const struct recorz_mvp_heap_object *heap_object_for_value(struct recorz_mvp_value value) {
    const struct recorz_mvp_heap_object *object;

    if (value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("receiver is not a heap object");
    }
    object = (const struct recorz_mvp_heap_object *)heap_object((uint16_t)value.integer);
    if (object->kind == 0U) {
        machine_panic("receiver is not a live heap object");
    }
    return object;
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

static uint16_t selector_object_selector_id(const struct recorz_mvp_heap_object *selector_object) {
    if (selector_object->kind != RECORZ_MVP_OBJECT_SELECTOR) {
        machine_panic("method descriptor selector does not point at a selector object");
    }
    return (uint16_t)small_integer_u32(
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

static uint16_t method_descriptor_selector(const struct recorz_mvp_heap_object *method_object) {
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

static uint16_t selector_object_handle(uint16_t selector) {
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

static uint16_t compiled_method_instruction_operand_a(uint32_t instruction) {
    return (uint16_t)((instruction >> 8) & 0xFFU);
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
    uint16_t selector,
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
    const struct recorz_mvp_seed_class_source_record *seed_source;

    if (dynamic_definition != 0) {
        return dynamic_definition->class_name;
    }
    seed_source = &recorz_mvp_generated_seed_class_sources[class_instance_kind(class_object)];
    if (seed_source->class_name != 0) {
        return seed_source->class_name;
    }
    return object_kind_name((uint8_t)class_instance_kind(class_object));
}

static uint8_t live_instance_field_count_for_class_with_depth(
    const struct recorz_mvp_heap_object *class_object,
    uint32_t depth
) {
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition =
        dynamic_class_definition_for_handle(heap_handle_for_object(class_object));
    const struct recorz_mvp_seed_class_source_record *seed_source =
        &recorz_mvp_generated_seed_class_sources[class_instance_kind(class_object)];
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
    } else if (seed_source->class_name != 0) {
        total_count += seed_source->instance_variable_count;
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
    const struct recorz_mvp_seed_class_source_record *seed_source =
        &recorz_mvp_generated_seed_class_sources[class_instance_kind(class_object)];
    const struct recorz_mvp_heap_object *superclass_object = class_superclass_object_or_null(class_object);
    uint8_t inherited_field_count = 0U;
    uint8_t field_index;

    if (depth > DYNAMIC_CLASS_LIMIT) {
        machine_panic("class field lookup chain is invalid");
    }
    if (superclass_object != 0) {
        inherited_field_count = live_instance_field_count_for_class_with_depth(superclass_object, depth + 1U);
    }
    if (dynamic_definition != 0) {
        for (field_index = 0U; field_index < dynamic_definition->instance_variable_count; ++field_index) {
            if (source_names_equal(field_name, dynamic_definition->instance_variable_names[field_index])) {
                *field_index_out = (uint8_t)(inherited_field_count + field_index);
                return 1U;
            }
        }
    } else if (seed_source->class_name != 0) {
        for (field_index = 0U; field_index < seed_source->instance_variable_count; ++field_index) {
            if (seed_source->instance_variable_names[field_index] != 0 &&
                source_names_equal(field_name, seed_source->instance_variable_names[field_index])) {
                *field_index_out = (uint8_t)(inherited_field_count + field_index);
                return 1U;
            }
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

static const struct recorz_mvp_seed_class_source_record *seed_class_source_record_for_name(const char *class_name) {
    uint16_t object_kind;

    for (object_kind = 1U; object_kind <= MAX_OBJECT_KIND; ++object_kind) {
        const struct recorz_mvp_seed_class_source_record *seed_source = &recorz_mvp_generated_seed_class_sources[object_kind];

        if (seed_source->class_name != 0 && source_names_equal(class_name, seed_source->class_name)) {
            return seed_source;
        }
    }
    return 0;
}

static uint8_t seed_class_method_chunk_for_selector(
    const struct recorz_mvp_seed_class_source_record *seed_source,
    const char *selector_name_text,
    uint8_t class_side,
    char chunk_out[],
    uint32_t chunk_out_size,
    char protocol_name_out[],
    uint32_t protocol_name_out_size
) {
    const char *cursor;
    uint8_t scanning_class_side = 0U;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    char current_protocol[METHOD_SOURCE_NAME_LIMIT];
    char parsed_selector_name[METHOD_SOURCE_NAME_LIMIT];
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    const char *body_cursor = 0;
    uint16_t argument_count = 0U;

    if (seed_source == 0 ||
        (seed_source->builder_source == 0 && seed_source->canonical_source == 0)) {
        return 0U;
    }
    current_protocol[0] = '\0';
    if (protocol_name_out != 0 && protocol_name_out_size != 0U) {
        protocol_name_out[0] = '\0';
    }
    cursor = seed_source->builder_source != 0 ? seed_source->builder_source : seed_source->canonical_source;
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            scanning_class_side = 0U;
            current_protocol[0] = '\0';
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
            scanning_class_side = 1U;
            current_protocol[0] = '\0';
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelProtocol:")) {
            source_parse_protocol_name_from_chunk(chunk, current_protocol, sizeof(current_protocol));
            continue;
        }
        if (scanning_class_side != class_side) {
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernel")) {
            continue;
        }
        if (source_parse_method_header(
                chunk,
                parsed_selector_name,
                argument_names,
                &argument_count,
                &body_cursor) == 0) {
            continue;
        }
        if (!source_names_equal(parsed_selector_name, selector_name_text)) {
            continue;
        }
        source_copy_identifier(chunk_out, chunk_out_size, chunk);
        if (protocol_name_out != 0 && protocol_name_out_size != 0U) {
            source_copy_identifier(
                protocol_name_out,
                protocol_name_out_size,
                current_protocol[0] == '\0' ? "UNFILED" : current_protocol
            );
        }
        return 1U;
    }
    return 0U;
}

static const char *workspace_method_source_text_for_browser_target(
    const struct recorz_mvp_heap_object *method_owner_class_object,
    const char *class_name,
    const char *selector_name_text,
    uint8_t class_side
) {
    const struct recorz_mvp_live_method_source *source_record;
    const struct recorz_mvp_seed_class_source_record *seed_source;
    uint16_t selector_id;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];

    selector_id = source_selector_id_for_name(selector_name_text);
    if (selector_id == 0U) {
        machine_panic("Workspace method source target uses an unknown selector");
    }
    source_record = live_method_source_for(heap_handle_for_object(method_owner_class_object), selector_id);
    if (source_record != 0) {
        return live_method_source_text(source_record);
    }
    seed_source = seed_class_source_record_for_name(class_name);
    if (!seed_class_method_chunk_for_selector(
            seed_source,
            selector_name_text,
            class_side,
            chunk,
            sizeof(chunk),
            0,
            0U)) {
        return 0;
    }
    return runtime_string_allocate_copy(chunk);
}

static const struct recorz_mvp_heap_object *default_form_object(void) {
    if (active_display_form_handle == 0U) {
        machine_panic("default form is not initialized");
    }
    return (const struct recorz_mvp_heap_object *)heap_object(active_display_form_handle);
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

static void validate_test_runner_receiver(const struct recorz_mvp_heap_object *test_runner_object) {
    const struct recorz_mvp_heap_object *test_runner_class = class_object_for_heap_object(test_runner_object);

    if (class_instance_kind(test_runner_class) != RECORZ_MVP_OBJECT_TEST_RUNNER) {
        machine_panic("TestRunner receiver class is invalid");
    }
}

static void test_runner_append_text(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *text
) {
    uint32_t index = 0U;

    while (text[index] != '\0') {
        if (*offset + 1U >= buffer_size) {
            machine_panic("TestRunner text exceeds buffer capacity");
        }
        buffer[(*offset)++] = text[index++];
    }
    buffer[*offset] = '\0';
}

static void test_runner_append_small_integer(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    int32_t value
) {
    render_small_integer(value);
    test_runner_append_text(buffer, buffer_size, offset, print_buffer);
}

static void test_runner_write_line(const char *text) {
    const struct recorz_mvp_heap_object *form = default_form_object();

    form_write_string(form, text);
    form_newline(form);
}

static struct recorz_mvp_value perform_send_and_pop_result(
    struct recorz_mvp_value receiver,
    uint16_t selector,
    uint16_t send_argument_count,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t baseline_stack_size = stack_size;

    perform_send(receiver, selector, send_argument_count, arguments, text);
    if (stack_size != (uint16_t)(baseline_stack_size + 1U)) {
        machine_panic("send did not leave exactly one result on the stack");
    }
    return stack[--stack_size];
}

static void test_runner_reset_state(const struct recorz_mvp_heap_object *test_runner_object) {
    uint16_t test_runner_handle = heap_handle_for_object(test_runner_object);

    validate_test_runner_receiver(test_runner_object);
    heap_set_field(test_runner_handle, TEST_RUNNER_FIELD_PASSED, small_integer_value(0));
    heap_set_field(test_runner_handle, TEST_RUNNER_FIELD_FAILED, small_integer_value(0));
    heap_set_field(test_runner_handle, TEST_RUNNER_FIELD_TOTAL, small_integer_value(0));
    heap_set_field(test_runner_handle, TEST_RUNNER_FIELD_LAST_LABEL, string_value(""));
}

static uint32_t test_runner_counter_value(
    const struct recorz_mvp_heap_object *test_runner_object,
    uint8_t field_index,
    const char *error_text
) {
    validate_test_runner_receiver(test_runner_object);
    return small_integer_u32(heap_get_field(test_runner_object, field_index), error_text);
}

static void test_runner_set_counter(
    const struct recorz_mvp_heap_object *test_runner_object,
    uint8_t field_index,
    uint32_t value
) {
    validate_test_runner_receiver(test_runner_object);
    heap_set_field(
        heap_handle_for_object(test_runner_object),
        field_index,
        small_integer_value((int32_t)value)
    );
}

static void test_runner_set_last_label(
    const struct recorz_mvp_heap_object *test_runner_object,
    const char *label
) {
    validate_test_runner_receiver(test_runner_object);
    heap_set_field(
        heap_handle_for_object(test_runner_object),
        TEST_RUNNER_FIELD_LAST_LABEL,
        string_value(label == 0 ? "" : runtime_string_allocate_copy(label))
    );
}

static uint8_t selector_name_has_test_prefix(const char *name) {
    return name != 0 &&
           name[0] == 't' &&
           name[1] == 'e' &&
           name[2] == 's' &&
           name[3] == 't';
}

static uint8_t test_runner_result_is_pass(struct recorz_mvp_value value) {
    return value.kind == RECORZ_MVP_VALUE_OBJECT &&
           (uint16_t)value.integer == global_handles[RECORZ_MVP_GLOBAL_TRUE];
}

static void test_runner_write_result_line(
    const char *status,
    const char *class_name,
    const char *selector_text
) {
    char line[METHOD_SOURCE_CHUNK_LIMIT];
    uint32_t offset = 0U;

    line[0] = '\0';
    test_runner_append_text(line, sizeof(line), &offset, status);
    test_runner_append_text(line, sizeof(line), &offset, " ");
    test_runner_append_text(line, sizeof(line), &offset, class_name);
    test_runner_append_text(line, sizeof(line), &offset, ">>");
    test_runner_append_text(line, sizeof(line), &offset, selector_text);
    test_runner_write_line(line);
}

static void test_runner_write_summary(
    const char *scope_label,
    uint32_t passed_count,
    uint32_t failed_count,
    uint32_t total_count
) {
    char line[METHOD_SOURCE_CHUNK_LIMIT];
    uint32_t offset = 0U;

    line[0] = '\0';
    test_runner_append_text(line, sizeof(line), &offset, "SUMMARY ");
    test_runner_append_text(line, sizeof(line), &offset, scope_label);
    test_runner_append_text(line, sizeof(line), &offset, " P=");
    test_runner_append_small_integer(line, sizeof(line), &offset, (int32_t)passed_count);
    test_runner_append_text(line, sizeof(line), &offset, " F=");
    test_runner_append_small_integer(line, sizeof(line), &offset, (int32_t)failed_count);
    test_runner_append_text(line, sizeof(line), &offset, " T=");
    test_runner_append_small_integer(line, sizeof(line), &offset, (int32_t)total_count);
    test_runner_write_line(line);
}

static void test_runner_build_label(
    char buffer[],
    uint32_t buffer_size,
    const char *class_name,
    const char *selector_text
) {
    uint32_t offset = 0U;

    buffer[0] = '\0';
    test_runner_append_text(buffer, buffer_size, &offset, class_name);
    test_runner_append_text(buffer, buffer_size, &offset, ">>");
    test_runner_append_text(buffer, buffer_size, &offset, selector_text);
}

static void test_runner_run_class(
    const struct recorz_mvp_heap_object *test_runner_object,
    const struct recorz_mvp_heap_object *class_object,
    const char *class_name
) {
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t method_index;
    uint32_t passed_count;
    uint32_t failed_count;
    uint32_t total_count;

    test_runner_reset_state(test_runner_object);
    passed_count = 0U;
    failed_count = 0U;
    total_count = 0U;
    if (method_start_object != 0) {
        for (method_index = 0U; method_index < method_count; ++method_index) {
            const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
                (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
            );
            const char *selector_text = selector_name(method_descriptor_selector(method_object));
            struct recorz_mvp_value test_instance;
            struct recorz_mvp_value test_result;
            char label[METHOD_SOURCE_CHUNK_LIMIT];

            if (method_descriptor_argument_count(method_object) != 0U ||
                !selector_name_has_test_prefix(selector_text)) {
                continue;
            }
            ++total_count;
            test_runner_build_label(label, sizeof(label), class_name, selector_text);
            test_runner_set_last_label(test_runner_object, label);
            test_instance = perform_send_and_pop_result(
                object_value(heap_handle_for_object(class_object)),
                RECORZ_MVP_SELECTOR_NEW,
                0U,
                0,
                "new"
            );
            test_result = perform_send_and_pop_result(
                test_instance,
                method_descriptor_selector(method_object),
                0U,
                0,
                selector_text
            );
            if (test_runner_result_is_pass(test_result)) {
                ++passed_count;
                test_runner_write_result_line("PASS", class_name, selector_text);
            } else {
                ++failed_count;
                test_runner_write_result_line("FAIL", class_name, selector_text);
            }
        }
    }
    test_runner_set_counter(test_runner_object, TEST_RUNNER_FIELD_PASSED, passed_count);
    test_runner_set_counter(test_runner_object, TEST_RUNNER_FIELD_FAILED, failed_count);
    test_runner_set_counter(test_runner_object, TEST_RUNNER_FIELD_TOTAL, total_count);
    test_runner_write_summary(class_name, passed_count, failed_count, total_count);
}

static void test_runner_accumulate_class(
    const struct recorz_mvp_heap_object *test_runner_object,
    const struct recorz_mvp_heap_object *class_object,
    const char *class_name
) {
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    uint32_t method_index;
    uint32_t passed_count = test_runner_counter_value(
        test_runner_object,
        TEST_RUNNER_FIELD_PASSED,
        "TestRunner passed counter is invalid"
    );
    uint32_t failed_count = test_runner_counter_value(
        test_runner_object,
        TEST_RUNNER_FIELD_FAILED,
        "TestRunner failed counter is invalid"
    );
    uint32_t total_count = test_runner_counter_value(
        test_runner_object,
        TEST_RUNNER_FIELD_TOTAL,
        "TestRunner total counter is invalid"
    );

    if (method_start_object == 0) {
        return;
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );
        const char *selector_text = selector_name(method_descriptor_selector(method_object));
        struct recorz_mvp_value test_instance;
        struct recorz_mvp_value test_result;
        char label[METHOD_SOURCE_CHUNK_LIMIT];

        if (method_descriptor_argument_count(method_object) != 0U ||
            !selector_name_has_test_prefix(selector_text)) {
            continue;
        }
        ++total_count;
        test_runner_build_label(label, sizeof(label), class_name, selector_text);
        test_runner_set_last_label(test_runner_object, label);
        test_instance = perform_send_and_pop_result(
            object_value(heap_handle_for_object(class_object)),
            RECORZ_MVP_SELECTOR_NEW,
            0U,
            0,
            "new"
        );
        test_result = perform_send_and_pop_result(
            test_instance,
            method_descriptor_selector(method_object),
            0U,
            0,
            selector_text
        );
        if (test_runner_result_is_pass(test_result)) {
            ++passed_count;
            test_runner_write_result_line("PASS", class_name, selector_text);
        } else {
            ++failed_count;
            test_runner_write_result_line("FAIL", class_name, selector_text);
        }
    }
    test_runner_set_counter(test_runner_object, TEST_RUNNER_FIELD_PASSED, passed_count);
    test_runner_set_counter(test_runner_object, TEST_RUNNER_FIELD_FAILED, failed_count);
    test_runner_set_counter(test_runner_object, TEST_RUNNER_FIELD_TOTAL, total_count);
}

static void test_runner_run_package(
    const struct recorz_mvp_heap_object *test_runner_object,
    const char *package_name
) {
    const struct recorz_mvp_dynamic_class_definition *sorted_definitions[DYNAMIC_CLASS_LIMIT];
    uint16_t sorted_count = 0U;
    uint16_t dynamic_index;

    validate_test_runner_receiver(test_runner_object);
    if (package_definition_for_name(package_name) == 0) {
        machine_panic("TestRunner runPackageNamed: could not resolve package");
    }
    test_runner_reset_state(test_runner_object);
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
                   sorted_definitions[insert_index - 1U]->class_name
               ) < 0) {
            sorted_definitions[insert_index] = sorted_definitions[insert_index - 1U];
            --insert_index;
        }
        sorted_definitions[insert_index] = definition;
        ++sorted_count;
    }
    for (dynamic_index = 0U; dynamic_index < sorted_count; ++dynamic_index) {
        const struct recorz_mvp_heap_object *class_object =
            lookup_class_by_name(sorted_definitions[dynamic_index]->class_name);

        if (class_object == 0) {
            machine_panic("TestRunner package contains an unresolved class");
        }
        test_runner_accumulate_class(test_runner_object, class_object, sorted_definitions[dynamic_index]->class_name);
    }
    test_runner_write_summary(
        package_name,
        test_runner_counter_value(test_runner_object, TEST_RUNNER_FIELD_PASSED, "TestRunner passed counter is invalid"),
        test_runner_counter_value(test_runner_object, TEST_RUNNER_FIELD_FAILED, "TestRunner failed counter is invalid"),
        test_runner_counter_value(test_runner_object, TEST_RUNNER_FIELD_TOTAL, "TestRunner total counter is invalid")
    );
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

static void workspace_capture_plain_return_state_if_needed(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t tool_handle;
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    struct recorz_mvp_value current_source_value;
    struct recorz_mvp_value arguments[1];

    if (workspace_object == 0) {
        return;
    }
    view_kind_value = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    if (view_kind_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
        (uint32_t)view_kind_value.integer != WORKSPACE_VIEW_INPUT_MONITOR) {
        return;
    }
    if (view_kind_value.kind != RECORZ_MVP_VALUE_NIL &&
        view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        return;
    }
    target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    if (target_name_value.kind != RECORZ_MVP_VALUE_NIL) {
        return;
    }
    tool_handle = named_object_handle_for_name("BootWorkspaceTool");
    if (tool_handle == 0U) {
        return;
    }
    current_source_value = workspace_current_source_value(workspace_object);
    if (current_source_value.kind == RECORZ_MVP_VALUE_STRING &&
        current_source_value.string != 0) {
        arguments[0] = current_source_value;
    } else {
        arguments[0] = string_value("");
    }
    (void)perform_send_and_pop_result(
        object_value(tool_handle),
        RECORZ_MVP_SELECTOR_REMEMBER_PLAIN_WORKSPACE_STATE_CONTENTS,
        1U,
        arguments,
        0
    );
}

static void workspace_remember_input_monitor_view(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    struct recorz_mvp_value prior_view_kind_value = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    struct recorz_mvp_value prior_target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t cursor_index =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? text_length(source_value.string)
            : 0U;
    uint32_t top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;
    char saved_target_name[METHOD_SOURCE_CHUNK_LIMIT];
    char saved_status[WORKSPACE_INPUT_MONITOR_STATUS_LIMIT];
    char saved_feedback[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];
    uint32_t saved_target_offset = 0U;

    saved_target_name[0] = '\0';
    saved_status[0] = '\0';
    saved_feedback[0] = '\0';
    if (prior_view_kind_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
        (uint32_t)prior_view_kind_value.integer == WORKSPACE_VIEW_INPUT_MONITOR) {
        workspace_parse_input_monitor_state(
            prior_target_name_value.kind == RECORZ_MVP_VALUE_STRING ? prior_target_name_value.string : 0,
            &cursor_index,
            &top_line,
            &saved_view_kind,
            saved_target_name,
            sizeof(saved_target_name),
            saved_status,
            sizeof(saved_status),
            saved_feedback,
            sizeof(saved_feedback)
        );
        workspace_input_monitor_set_status(saved_status);
        workspace_input_monitor_set_feedback_text(saved_feedback);
    } else {
        if (prior_view_kind_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
            prior_view_kind_value.integer >= 0) {
            saved_view_kind = (uint32_t)prior_view_kind_value.integer;
        }
        if (prior_target_name_value.kind == RECORZ_MVP_VALUE_STRING &&
            prior_target_name_value.string != 0) {
            append_text_checked(
                saved_target_name,
                sizeof(saved_target_name),
                &saved_target_offset,
                prior_target_name_value.string
            );
        }
        if (saved_view_kind == WORKSPACE_VIEW_METHOD ||
            saved_view_kind == WORKSPACE_VIEW_CLASS_METHOD ||
            saved_view_kind == WORKSPACE_VIEW_CLASS_SOURCE ||
            saved_view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE) {
            cursor_index = 0U;
            top_line = 0U;
        }
        workspace_input_monitor_set_status("");
        workspace_input_monitor_set_feedback_text("");
    }
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)WORKSPACE_VIEW_INPUT_MONITOR)
    );
    workspace_store_input_monitor_state_with_context(
        workspace_object,
        cursor_index,
        top_line,
        saved_view_kind,
        saved_target_name[0] == '\0' ? 0 : saved_target_name
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
    uint8_t last_source_index = workspace_last_source_field_index(workspace_object);

    if (workspace_object->field_count <= last_source_index) {
        heap_set_field(
            workspace_handle,
            last_source_index,
            nil_value()
        );
    }

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
    struct recorz_mvp_value source_value;
    struct recorz_mvp_value view_kind_value;

    if (workspace_object->field_count <= field_index) {
        return nil_value();
    }
    source_value = heap_get_field(workspace_object, field_index);
    if (source_value.kind == RECORZ_MVP_VALUE_STRING &&
        source_value.string != 0 &&
        source_value.string[0] != '\0') {
        return source_value;
    }
    if (workspace_object->field_count <= workspace_current_view_kind_field_index(workspace_object)) {
        return source_value;
    }
    view_kind_value = heap_get_field(workspace_object, workspace_current_view_kind_field_index(workspace_object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        return source_value;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_REGENERATED_BOOT_SOURCE) {
        return string_value(regenerated_boot_source_text(workspace_object));
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_REGENERATED_KERNEL_SOURCE) {
        return string_value(regenerated_kernel_source_text());
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_REGENERATED_FILE_IN_SOURCE) {
        return string_value(regenerated_file_in_source_text());
    }
    return source_value;
}

static uint8_t workspace_current_source_is_editor_target(
    const struct recorz_mvp_heap_object *workspace_object
);

static const char *workspace_copy_source_into_editor_buffer(const char *source) {
    uint32_t length = 0U;

    if (source == 0) {
        workspace_editor_source_buffer[0] = '\0';
        return workspace_editor_source_buffer;
    }
    while (source[length] != '\0') {
        if (length + 1U >= sizeof(workspace_editor_source_buffer)) {
            machine_panic("Workspace editor source exceeds buffer capacity");
        }
        workspace_editor_source_buffer[length] = source[length];
        ++length;
    }
    workspace_editor_source_buffer[length] = '\0';
    return workspace_editor_source_buffer;
}

static void workspace_remember_editor_current_source(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *source
) {
    workspace_remember_current_source(
        workspace_object,
        source == 0 ? 0 : workspace_copy_source_into_editor_buffer(source)
    );
}

static void workspace_prepare_editor_current_source_for_session(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value;

    if (!workspace_current_source_is_editor_target(workspace_object)) {
        return;
    }
    source_value = workspace_current_source_value(workspace_object);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string == workspace_editor_source_buffer) {
        return;
    }
    workspace_remember_editor_current_source(workspace_object, source_value.string);
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
    uint32_t right_margin = text_right_margin();
    uint32_t width = bitmap_width(bitmap_for_form(form));
    uint32_t char_columns;
    uint32_t wrap_limit_x = text_wrap_limit_x_for_form_width(width);

    if (width <= left_margin + right_margin || char_width() == 0U) {
        return 1U;
    }
    char_columns = (width - left_margin - right_margin) / char_width();
    if (wrap_limit_x != 0U && wrap_limit_x > left_margin) {
        uint32_t wrapped_columns = (wrap_limit_x - left_margin) / char_width();

        if (wrapped_columns != 0U && wrapped_columns < char_columns) {
            char_columns = wrapped_columns;
        }
    }
    return char_columns == 0U ? 1U : char_columns;
}

static uint8_t workspace_has_line_capacity(const struct recorz_mvp_heap_object *form) {
    uint32_t form_height = bitmap_height(bitmap_for_form(form));
    uint32_t bottom_margin = text_bottom_margin();

    if (form_height <= bottom_margin) {
        return 0U;
    }
    return cursor_y + char_height() < form_height - bottom_margin;
}

static void workspace_newline(void) {
    cursor_x = text_left_margin();
    cursor_y += text_line_height();
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

static void workspace_copy_raw_text(
    char buffer[],
    uint32_t buffer_size,
    const char *text,
    uint32_t max_columns
) {
    uint32_t length = 0U;

    while (text[length] != '\0' && length + 1U < buffer_size && length < max_columns) {
        buffer[length] = text[length];
        ++length;
    }
    buffer[length] = '\0';
}

static void workspace_input_monitor_compact_text_line(
    char buffer[],
    uint32_t buffer_size,
    const char *text,
    uint32_t max_columns
) {
    uint32_t length = 0U;
    uint8_t truncated = 0U;

    if (buffer_size == 0U) {
        return;
    }
    while (text != 0 &&
           text[length] != '\0' &&
           text[length] != '\n' &&
           text[length] != '\r' &&
           length + 1U < buffer_size &&
           length < max_columns) {
        buffer[length] = text[length];
        ++length;
    }
    if (text != 0 &&
        text[length] != '\0' &&
        text[length] != '\n' &&
        text[length] != '\r') {
        truncated = 1U;
    }
    if (truncated && length >= 3U) {
        length -= 3U;
        buffer[length++] = '.';
        buffer[length++] = '.';
        buffer[length++] = '.';
    }
    buffer[length] = '\0';
}

static void workspace_input_monitor_compact_feedback_line(
    char buffer[],
    uint32_t buffer_size,
    uint32_t max_columns
) {
    const char *cursor = workspace_input_monitor_feedback;
    char line[METHOD_SOURCE_CHUNK_LIMIT];
    char compact_line[METHOD_SOURCE_NAME_LIMIT];
    uint32_t offset = 0U;

    if (buffer_size == 0U) {
        return;
    }
    buffer[0] = '\0';
    compact_line[0] = '\0';
    if (workspace_input_monitor_feedback[0] == '\0') {
        return;
    }
    line[0] = '\0';
    while (source_copy_raw_line(&cursor, line, sizeof(line))) {
        if (line[0] == '\0') {
            continue;
        }
        workspace_input_monitor_compact_text_line(
            compact_line,
            sizeof(compact_line),
            line,
            max_columns
        );
    }
    if (compact_line[0] == '\0') {
        return;
    }
    append_text_checked(buffer, buffer_size, &offset, "OUT> ");
    append_text_checked(buffer, buffer_size, &offset, compact_line);
}

static void workspace_draw_input_monitor_cursor(
    const struct recorz_mvp_heap_object *form,
    uint32_t column
) {
    if (active_cursor_handle != 0U) {
        active_cursor_move_to(text_left_margin() + (column * char_width()), cursor_y);
        active_cursor_set_visible(1U);
        draw_active_cursor_on_form(form);
        return;
    }
    form_fill_rect_color(
        form,
        text_left_margin() + (column * char_width()),
        cursor_y,
        text_pixel_scale() + 1U,
        char_height(),
        text_foreground_color()
    );
}

static void workspace_write_raw_input_monitor_line(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t cursor_column,
    uint8_t draw_cursor
) {
    char clipped_text[METHOD_SOURCE_CHUNK_LIMIT];

    if (!workspace_has_line_capacity(form)) {
        return;
    }
    workspace_copy_raw_text(clipped_text, sizeof(clipped_text), text, workspace_visible_columns(form));
    form_write_string(form, clipped_text);
    if (draw_cursor) {
        workspace_draw_input_monitor_cursor(form, cursor_column);
    }
    workspace_newline();
}

static uint32_t workspace_input_monitor_visible_line_capacity(
    const struct recorz_mvp_heap_object *form
) {
    uint32_t form_height = bitmap_height(bitmap_for_form(form));
    uint32_t bottom_margin = text_bottom_margin();
    uint32_t line_y = cursor_y;
    uint32_t line_step = text_line_height();
    uint32_t count = 0U;

    if (form_height <= bottom_margin) {
        return 0U;
    }
    while (line_y + char_height() < form_height - bottom_margin) {
        ++count;
        line_y += line_step;
    }
    return count;
}

static void workspace_tool_set_string_field(uint8_t field_index, const char *text) {
    uint16_t tool_handle = named_object_handle_for_name("BootWorkspaceTool");

    if (tool_handle == 0U) {
        return;
    }
    heap_set_field(
        tool_handle,
        field_index,
        string_value(text == 0 ? "" : runtime_string_allocate_copy(text))
    );
}

static void workspace_tool_sync_status_field(void) {
    if (!workspace_tool_status_dirty) {
        return;
    }
    workspace_tool_set_string_field(WORKSPACE_TOOL_FIELD_STATUS_TEXT, workspace_input_monitor_status);
    workspace_tool_status_dirty = 0U;
}

static void workspace_tool_sync_feedback_field(void) {
    if (!workspace_tool_feedback_dirty) {
        return;
    }
    workspace_tool_set_string_field(
        WORKSPACE_TOOL_FIELD_FEEDBACK_TEXT,
        workspace_input_monitor_feedback
    );
    workspace_tool_feedback_dirty = 0U;
}

static void workspace_input_monitor_set_status(const char *text) {
    uint32_t offset = 0U;

    workspace_input_monitor_status[0] = '\0';
    if (text == 0 || text[0] == '\0') {
        workspace_tool_status_dirty = 1U;
        workspace_tool_sync_status_field();
        return;
    }
    append_text_checked(
        workspace_input_monitor_status,
        sizeof(workspace_input_monitor_status),
        &offset,
        text
    );
    workspace_tool_status_dirty = 1U;
    workspace_tool_sync_status_field();
}

static void workspace_input_monitor_set_feedback_text(const char *text) {
    uint32_t offset = 0U;

    workspace_input_monitor_feedback[0] = '\0';
    if (text == 0 || text[0] == '\0') {
        workspace_tool_feedback_dirty = 1U;
        workspace_tool_sync_feedback_field();
        return;
    }
    append_text_checked(
        workspace_input_monitor_feedback,
        sizeof(workspace_input_monitor_feedback),
        &offset,
        text
    );
    workspace_tool_feedback_dirty = 1U;
    workspace_tool_sync_feedback_field();
}

static void workspace_input_monitor_clear_feedback(void) {
    workspace_input_monitor_feedback[0] = '\0';
    workspace_tool_feedback_dirty = 1U;
    workspace_tool_sync_feedback_field();
}

static void workspace_input_monitor_feedback_append_char(char ch) {
    uint32_t length = text_length(workspace_input_monitor_feedback);
    uint32_t index;

    if (ch == '\r') {
        return;
    }
    workspace_tool_feedback_dirty = 1U;
    if (length + 1U < sizeof(workspace_input_monitor_feedback)) {
        workspace_input_monitor_feedback[length] = ch;
        workspace_input_monitor_feedback[length + 1U] = '\0';
        return;
    }
    for (index = 1U; index < length; ++index) {
        workspace_input_monitor_feedback[index - 1U] = workspace_input_monitor_feedback[index];
    }
    if (length != 0U) {
        workspace_input_monitor_feedback[length - 1U] = ch;
        workspace_input_monitor_feedback[length] = '\0';
    }
}

static void workspace_input_monitor_feedback_append_text(const char *text) {
    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        workspace_input_monitor_feedback_append_char(*text);
        ++text;
    }
}

static void workspace_input_monitor_feedback_append_line(const char *text) {
    uint32_t length = text_length(workspace_input_monitor_feedback);

    if (length != 0U && workspace_input_monitor_feedback[length - 1U] != '\n') {
        workspace_input_monitor_feedback_append_char('\n');
    }
    workspace_input_monitor_feedback_append_text(text);
}

static uint8_t workspace_input_monitor_state_hex_value(char ch, uint8_t *value_out) {
    if (ch >= '0' && ch <= '9') {
        *value_out = (uint8_t)(ch - '0');
        return 1U;
    }
    if (ch >= 'a' && ch <= 'f') {
        *value_out = (uint8_t)(10 + (ch - 'a'));
        return 1U;
    }
    if (ch >= 'A' && ch <= 'F') {
        *value_out = (uint8_t)(10 + (ch - 'A'));
        return 1U;
    }
    return 0U;
}

static void workspace_append_input_monitor_state_text(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *text
) {
    static const char hex_digits[] = "0123456789ABCDEF";

    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        uint8_t ch = (uint8_t)*text++;

        if (ch < 32U || ch > 126U || ch == '%' || ch == ';') {
            if (*offset + 3U >= buffer_size) {
                buffer[*offset] = '\0';
                return;
            }
            buffer[(*offset)++] = '%';
            buffer[(*offset)++] = hex_digits[(ch >> 4U) & 0x0FU];
            buffer[(*offset)++] = hex_digits[ch & 0x0FU];
            buffer[*offset] = '\0';
            continue;
        }
        if (*offset + 1U >= buffer_size) {
            buffer[*offset] = '\0';
            return;
        }
        buffer[(*offset)++] = (char)ch;
        buffer[*offset] = '\0';
    }
}

static uint8_t workspace_copy_input_monitor_state_text(
    const char **cursor_ref,
    char output[],
    uint32_t output_size
) {
    const char *cursor = *cursor_ref;
    uint32_t offset = 0U;

    if (output != 0 && output_size != 0U) {
        output[0] = '\0';
    }
    while (*cursor != '\0' && *cursor != ';') {
        uint8_t ch = (uint8_t)*cursor++;

        if (ch == '%') {
            uint8_t high_nibble;
            uint8_t low_nibble;

            if (!workspace_input_monitor_state_hex_value(cursor[0], &high_nibble) ||
                !workspace_input_monitor_state_hex_value(cursor[1], &low_nibble)) {
                return 0U;
            }
            ch = (uint8_t)((high_nibble << 4U) | low_nibble);
            cursor += 2;
        }
        if (output != 0 && output_size != 0U) {
            if (offset + 1U >= output_size) {
                return 0U;
            }
            output[offset] = (char)ch;
        }
        ++offset;
    }
    if (output != 0 && output_size != 0U) {
        output[offset] = '\0';
    }
    *cursor_ref = cursor;
    return 1U;
}

static void workspace_input_monitor_feedback_snapshot_text(
    char buffer[],
    uint32_t buffer_size
) {
    const char *feedback_start = workspace_input_monitor_feedback_tail_start(
        workspace_input_monitor_feedback,
        3U
    );
    uint32_t offset = 0U;

    buffer[0] = '\0';
    append_text_checked(buffer, buffer_size, &offset, feedback_start);
}

static uint32_t workspace_input_monitor_reserved_output_lines(uint32_t total_visible_lines) {
    if (total_visible_lines <= 6U) {
        return 0U;
    }
    return 4U;
}

static const char *workspace_input_monitor_feedback_tail_start(
    const char *text,
    uint32_t line_capacity
) {
    const char *line_starts[4];
    const char *cursor = text;
    uint32_t line_count = 0U;

    if (text == 0 || text[0] == '\0' || line_capacity == 0U) {
        return text;
    }
    while (*cursor != '\0') {
        if (line_capacity <= 4U) {
            line_starts[line_count % line_capacity] = cursor;
        }
        ++line_count;
        while (*cursor != '\0' && *cursor != '\n') {
            ++cursor;
        }
        if (*cursor == '\n') {
            ++cursor;
        }
    }
    if (line_count <= line_capacity || line_capacity > 4U) {
        return text;
    }
    return line_starts[line_count % line_capacity];
}

static void workspace_render_input_monitor_feedback(
    const struct recorz_mvp_heap_object *form,
    uint32_t output_line_capacity
) {
    const char *cursor;
    char line[METHOD_SOURCE_CHUNK_LIMIT];
    char prefixed_line[METHOD_SOURCE_CHUNK_LIMIT];
    uint32_t line_offset;
    uint32_t rendered_lines = 0U;

    if (output_line_capacity == 0U) {
        return;
    }
    workspace_write_label_and_text(
        form,
        "STATUS",
        workspace_input_monitor_status[0] == '\0' ? "READY" : workspace_input_monitor_status
    );
    if (output_line_capacity == 1U) {
        return;
    }
    if (workspace_input_monitor_feedback[0] == '\0') {
        workspace_write_raw_input_monitor_line(form, "OUT> (no output)", 0U, 0U);
        rendered_lines = 1U;
    } else {
        cursor = workspace_input_monitor_feedback_tail_start(
            workspace_input_monitor_feedback,
            output_line_capacity - 1U
        );
        while (rendered_lines + 1U <= output_line_capacity - 1U &&
               source_copy_raw_line(&cursor, line, sizeof(line))) {
            line_offset = 0U;
            prefixed_line[0] = '\0';
            append_text_checked(prefixed_line, sizeof(prefixed_line), &line_offset, "OUT> ");
            append_text_checked(prefixed_line, sizeof(prefixed_line), &line_offset, line);
            workspace_write_raw_input_monitor_line(form, prefixed_line, 0U, 0U);
            ++rendered_lines;
        }
    }
    while (rendered_lines + 1U < output_line_capacity) {
        workspace_write_raw_input_monitor_line(form, "OUT>", 0U, 0U);
        ++rendered_lines;
    }
}

static uint8_t workspace_input_monitor_draw_output_from_image(
    const struct recorz_mvp_heap_object *form,
    const char *status,
    const char *feedback,
    uint32_t output_line_capacity
) {
    uint16_t object_handle = named_object_handle_for_name("BootInputMonitorRenderer");
    struct recorz_mvp_value arguments[4];

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = string_value(status == 0 ? "" : status);
    arguments[1] = string_value(feedback == 0 ? "" : feedback);
    arguments[2] = object_value(heap_handle_for_object(form));
    arguments[3] = small_integer_value((int32_t)output_line_capacity);
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_DRAW_STATUS_FEEDBACK_ON_FORM_LINES,
        4U,
        arguments,
        0
    );
    return 1U;
}

static uint8_t workspace_input_monitor_sync_state_from_image(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *text,
    uint32_t cursor_index,
    uint32_t top_line,
    uint32_t visible_line_capacity
) {
    uint16_t object_handle = named_object_handle_for_name("BootInputMonitorRenderer");
    struct recorz_mvp_value arguments[4];
    struct recorz_mvp_value result;
    uint32_t adjusted_top_line;
    struct recorz_mvp_value target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t parsed_cursor_index = 0U;
    uint32_t parsed_top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;
    char saved_target_name[METHOD_SOURCE_CHUNK_LIMIT];
    char saved_status[WORKSPACE_INPUT_MONITOR_STATUS_LIMIT];
    char saved_feedback[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = string_value(text == 0 ? "" : text);
    arguments[1] = small_integer_value((int32_t)cursor_index);
    arguments[2] = small_integer_value((int32_t)top_line);
    arguments[3] = small_integer_value((int32_t)visible_line_capacity);
    result = perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_SYNC_CURSOR_TEXT_INDEX_TOP_VISIBLE,
        4U,
        arguments,
        0
    );
    adjusted_top_line = small_integer_u32(
        result,
        "BootInputMonitorRenderer syncCursorText:index:top:visible: did not return a small integer"
    );
    saved_target_name[0] = '\0';
    saved_status[0] = '\0';
    saved_feedback[0] = '\0';
    workspace_parse_input_monitor_state(
        target_name_value.kind == RECORZ_MVP_VALUE_STRING ? target_name_value.string : 0,
        &parsed_cursor_index,
        &parsed_top_line,
        &saved_view_kind,
        saved_target_name,
        sizeof(saved_target_name),
        saved_status,
        sizeof(saved_status),
        saved_feedback,
        sizeof(saved_feedback)
    );
    workspace_store_input_monitor_state_with_context_text_only(
        workspace_object,
        cursor_index,
        adjusted_top_line,
        saved_view_kind,
        saved_target_name[0] == '\0' ? 0 : saved_target_name
    );
    return 1U;
}

static uint8_t workspace_input_monitor_draw_source_from_image(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t visible_line_capacity
) {
    uint16_t object_handle = named_object_handle_for_name("BootInputMonitorRenderer");
    struct recorz_mvp_value arguments[3];

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = string_value(text == 0 ? "" : text);
    arguments[1] = object_value(heap_handle_for_object(form));
    arguments[2] = small_integer_value((int32_t)visible_line_capacity);
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_DRAW_SOURCE_ON_FORM_VISIBLE,
        3U,
        arguments,
        0
    );
    return 1U;
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

static const char *workspace_text_for_value(
    struct recorz_mvp_value value,
    char buffer[],
    uint32_t buffer_size
) {
    const char *named_object_name;
    struct recorz_mvp_value printed_value;
    uint32_t offset = 0U;

    if (buffer != 0 && buffer_size != 0U) {
        buffer[0] = '\0';
    }
    if (value.kind == RECORZ_MVP_VALUE_NIL) {
        return "nil";
    }
    if (value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER ||
        value.kind == RECORZ_MVP_VALUE_STRING) {
        printed_value = perform_send_and_pop_result(
            value,
            RECORZ_MVP_SELECTOR_PRINT_STRING,
            0U,
            0,
            0
        );
        if (printed_value.kind != RECORZ_MVP_VALUE_STRING || printed_value.string == 0) {
            machine_panic("Workspace printString did not return a string");
        }
        if (buffer != 0 && buffer_size != 0U) {
            append_text_checked(buffer, buffer_size, &offset, printed_value.string);
            return buffer;
        }
        return printed_value.string;
    }
    if (value.kind == RECORZ_MVP_VALUE_OBJECT) {
        named_object_name = workspace_named_object_name_for_handle((uint16_t)value.integer);
        if (named_object_name != 0) {
            return named_object_name;
        }
        return class_name_for_object(class_object_for_heap_object(heap_object_for_value(value)));
    }
    machine_panic("Workspace encountered an unsupported value");
    return "";
}

static void workspace_write_label_and_value(
    const struct recorz_mvp_heap_object *form,
    const char *label,
    struct recorz_mvp_value value
) {
    char text[METHOD_SOURCE_CHUNK_LIMIT];

    workspace_write_label_and_text(form, label, workspace_text_for_value(value, text, sizeof(text)));
}

static void workspace_surface_reset_buffer(char buffer[], uint32_t buffer_size) {
    if (buffer != 0 && buffer_size != 0U) {
        buffer[0] = '\0';
    }
}

static void workspace_surface_append_line(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *text
) {
    if (buffer == 0 || offset == 0 || buffer_size == 0U) {
        return;
    }
    if (*offset != 0U && buffer[*offset - 1U] != '\n') {
        append_text_checked(buffer, buffer_size, offset, "\n");
    }
    append_text_checked(buffer, buffer_size, offset, text == 0 ? "" : text);
}

static void workspace_surface_append_label_text(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *label,
    const char *text
) {
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint32_t line_offset = 0U;

    line[0] = '\0';
    append_text_checked(line, sizeof(line), &line_offset, label == 0 ? "" : label);
    append_text_checked(line, sizeof(line), &line_offset, ": ");
    append_text_checked(line, sizeof(line), &line_offset, text == 0 ? "" : text);
    workspace_surface_append_line(buffer, buffer_size, offset, line);
}

static void workspace_surface_append_label_integer(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const char *label,
    uint32_t value
) {
    char line[METHOD_SOURCE_LINE_LIMIT];
    uint32_t line_offset = 0U;

    line[0] = '\0';
    append_text_checked(line, sizeof(line), &line_offset, label == 0 ? "" : label);
    append_text_checked(line, sizeof(line), &line_offset, ": ");
    render_small_integer((int32_t)value);
    append_text_checked(line, sizeof(line), &line_offset, print_buffer);
    workspace_surface_append_line(buffer, buffer_size, offset, line);
}

static void workspace_invalidate_named_object(const char *name) {
    uint16_t object_handle = named_object_handle_for_name(name);

    if (object_handle == 0U) {
        return;
    }
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_INVALIDATE,
        0U,
        0,
        0
    );
}

static void workspace_invalidate_editor_surface_views(void) {
    workspace_invalidate_named_object("BootWorkspaceHeaderView");
    workspace_invalidate_named_object("BootWorkspaceSourceView");
    workspace_invalidate_named_object("BootWorkspaceStatusView");
}

static void workspace_invalidate_browser_surface_views(void) {
    workspace_invalidate_named_object("BootBrowserHeaderView");
    workspace_invalidate_named_object("BootBrowserListView");
    workspace_invalidate_named_object("BootBrowserSourceView");
    workspace_invalidate_named_object("BootBrowserStatusView");
}

static uint32_t workspace_surface_visible_line_capacity_for_view_height(uint32_t view_height) {
    uint32_t line_height = text_line_height();
    uint32_t cell_height = char_height();

    if (line_height == 0U ||
        cell_height == 0U ||
        view_height <= VIEW_CONTENT_VERTICAL_PADDING + cell_height + 1U) {
        return 0U;
    }
    return (view_height - VIEW_CONTENT_VERTICAL_PADDING - cell_height - 1U) / line_height;
}

static uint32_t workspace_surface_visible_column_capacity_for_view_width(uint32_t view_width) {
    uint32_t cell_width = char_width();
    uint32_t visible_columns;

    if (cell_width == 0U || view_width <= VIEW_CONTENT_HORIZONTAL_PADDING) {
        return 0U;
    }
    visible_columns = (view_width - VIEW_CONTENT_HORIZONTAL_PADDING) / cell_width;
    if (visible_columns == 0U) {
        return 0U;
    }
    return visible_columns - 1U;
}

static void workspace_surface_copy_source_viewport(
    char buffer[],
    uint32_t buffer_size,
    const char *source,
    uint32_t top_line,
    uint32_t left_column,
    uint32_t visible_line_capacity,
    uint32_t visible_column_capacity
) {
    const char *cursor = source == 0 ? "" : source;
    char line[METHOD_SOURCE_LINE_LIMIT];
    char visible_line[METHOD_SOURCE_LINE_LIMIT];
    uint32_t current_line = 0U;
    uint32_t offset = 0U;

    if (buffer_size == 0U) {
        return;
    }
    buffer[0] = '\0';
    if (visible_line_capacity == 0U || visible_column_capacity == 0U) {
        return;
    }
    while (source_copy_raw_line(&cursor, line, sizeof(line))) {
        uint32_t source_index = 0U;
        uint32_t visible_index = 0U;

        if (current_line < top_line) {
            ++current_line;
            continue;
        }
        if (current_line >= top_line + visible_line_capacity) {
            break;
        }
        while (line[source_index] != '\0' && source_index < left_column) {
            ++source_index;
        }
        while (line[source_index] != '\0' &&
               visible_index + 1U < sizeof(visible_line) &&
               visible_index < visible_column_capacity) {
            visible_line[visible_index++] = line[source_index++];
        }
        visible_line[visible_index] = '\0';
        if (offset != 0U) {
            append_text_checked(buffer, buffer_size, &offset, "\n");
        }
        append_text_checked(buffer, buffer_size, &offset, visible_line);
        ++current_line;
    }
}

static void workspace_draw_editor_cursor_overlay(
    const struct recorz_mvp_heap_object *form,
    uint32_t cursor_line,
    uint32_t cursor_column
) {
    uint32_t line_height = text_line_height();
    uint32_t column_width = char_width();
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t cursor_bar_width;
    uint32_t cursor_underline_height;
    uint32_t cursor_height;

    if (visible_lines == 0U || visible_columns == 0U) {
        return;
    }
    if (cursor_line >= visible_lines || cursor_column >= visible_columns) {
        return;
    }
    cursor_x = WORKSPACE_SOURCE_VIEW_LEFT + VIEW_CONTENT_INSET + (cursor_column * column_width);
    cursor_y = WORKSPACE_SOURCE_VIEW_TOP + VIEW_CONTENT_INSET + text_line_height() + (cursor_line * line_height);
    cursor_bar_width = text_pixel_scale() + 2U;
    cursor_underline_height = text_pixel_scale() > 1U ? text_pixel_scale() : 2U;
    cursor_height = line_height > 2U ? line_height - 2U : 1U;
    if (cursor_bar_width > column_width) {
        cursor_bar_width = column_width;
    }
    if (cursor_underline_height > line_height) {
        cursor_underline_height = line_height;
    }
    form_fill_rect_color(
        form,
        cursor_x,
        cursor_y,
        cursor_bar_width,
        cursor_height,
        text_foreground_color()
    );
    form_fill_rect_color(
        form,
        cursor_x,
        cursor_y + line_height - cursor_underline_height,
        column_width,
        cursor_underline_height,
        11393254U
    );
}

static uint8_t workspace_source_viewport_code_point_at(
    const char *text,
    uint32_t target_line,
    uint32_t target_column,
    uint8_t *code_point_out
) {
    const char *cursor = text == 0 ? "" : text;
    uint32_t line = 0U;
    uint32_t column = 0U;

    while (*cursor != '\0') {
        uint8_t code_point = (uint8_t)*cursor++;

        if (code_point == (uint8_t)'\r') {
            continue;
        }
        if (code_point == (uint8_t)'\n') {
            ++line;
            column = 0U;
            continue;
        }
        if (line == target_line && column == target_column) {
            if (code_point_out != 0) {
                *code_point_out = code_point;
            }
            return 1U;
        }
        ++column;
    }
    return 0U;
}

static void workspace_redraw_editor_source_cell(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t line,
    uint32_t column
) {
    uint32_t line_height = text_line_height();
    uint32_t column_width = char_width();
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t cell_x;
    uint32_t cell_y;
    uint8_t code_point = 0U;

    if (line_height == 0U || column_width == 0U) {
        return;
    }
    if (line >= visible_lines || column >= visible_columns) {
        return;
    }
    cell_x = WORKSPACE_SOURCE_VIEW_LEFT + VIEW_CONTENT_INSET + (column * column_width);
    cell_y = WORKSPACE_SOURCE_VIEW_TOP + VIEW_CONTENT_INSET + text_line_height() + (line * line_height);
    form_fill_rect_color(
        form,
        cell_x,
        cell_y,
        column_width,
        line_height,
        text_background_color()
    );
    if (workspace_source_viewport_code_point_at(text, line, column, &code_point)) {
        form_draw_code_point_at_with_colors(
            form,
            code_point,
            cell_x,
            cell_y,
            text_foreground_color(),
            text_background_color()
        );
    }
}

static void workspace_redraw_editor_source_absolute_cell(
    const struct recorz_mvp_heap_object *form,
    const char *source,
    uint32_t top_line,
    uint32_t left_column,
    uint32_t absolute_line,
    uint32_t absolute_column
) {
    uint32_t line_height = text_line_height();
    uint32_t column_width = char_width();
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t relative_line;
    uint32_t relative_column;
    uint32_t cell_x;
    uint32_t cell_y;
    uint8_t code_point = 0U;

    if (absolute_line < top_line || absolute_column < left_column) {
        return;
    }
    relative_line = absolute_line - top_line;
    relative_column = absolute_column - left_column;
    if (line_height == 0U ||
        column_width == 0U ||
        relative_line >= visible_lines ||
        relative_column >= visible_columns) {
        return;
    }
    cell_x = WORKSPACE_SOURCE_VIEW_LEFT + VIEW_CONTENT_INSET + (relative_column * column_width);
    cell_y = WORKSPACE_SOURCE_VIEW_TOP + VIEW_CONTENT_INSET + text_line_height() + (relative_line * line_height);
    form_fill_rect_color(form, cell_x, cell_y, column_width, line_height, text_background_color());
    if (workspace_source_viewport_code_point_at(source, absolute_line, absolute_column, &code_point)) {
        form_draw_code_point_at_with_colors(
            form,
            code_point,
            cell_x,
            cell_y,
            text_foreground_color(),
            text_background_color()
        );
    }
}

static void workspace_redraw_editor_source_viewport_lines(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t first_line,
    uint32_t line_count
) {
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t line_end = first_line + line_count;
    uint32_t line;

    if (first_line >= visible_lines) {
        return;
    }
    if (line_end > visible_lines) {
        line_end = visible_lines;
    }
    for (line = first_line; line < line_end; ++line) {
        uint32_t column;

        for (column = 0U; column < visible_columns; ++column) {
            workspace_redraw_editor_source_cell(form, text, line, column);
        }
    }
}

static void workspace_redraw_editor_source_viewport_columns(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t first_column,
    uint32_t column_count
) {
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t column_end = first_column + column_count;
    uint32_t line;

    if (first_column >= visible_columns) {
        return;
    }
    if (column_end > visible_columns) {
        column_end = visible_columns;
    }
    for (line = 0U; line < visible_lines; ++line) {
        uint32_t column;

        for (column = first_column; column < column_end; ++column) {
            workspace_redraw_editor_source_cell(form, text, line, column);
        }
    }
}

static void workspace_draw_editor_source_viewport_overlay(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t cursor_line,
    uint32_t cursor_column
) {
    const char *cursor = text == 0 ? "" : text;
    uint32_t origin_x = WORKSPACE_SOURCE_VIEW_LEFT + VIEW_CONTENT_INSET;
    uint32_t origin_y = WORKSPACE_SOURCE_VIEW_TOP + VIEW_CONTENT_INSET + text_line_height();
    uint32_t line = 0U;
    uint32_t column = 0U;
    uint32_t line_height = text_line_height();
    uint32_t column_width = char_width();

    if (line_height == 0U || column_width == 0U) {
        return;
    }
    while (*cursor != '\0') {
        uint8_t code_point = (uint8_t)*cursor++;

        if (code_point == (uint8_t)'\r') {
            continue;
        }
        if (code_point == (uint8_t)'\n') {
            ++line;
            column = 0U;
            continue;
        }
        form_draw_code_point_at_with_colors(
            form,
            code_point,
            origin_x + (column * column_width),
            origin_y + (line * line_height),
            text_foreground_color(),
            text_background_color()
        );
        ++column;
    }
    workspace_draw_editor_cursor_overlay(form, cursor_line, cursor_column);
}

static void workspace_draw_text_line_at(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t x,
    uint32_t y,
    uint32_t max_columns
) {
    char line_buffer[METHOD_SOURCE_LINE_LIMIT];
    uint32_t column_width = char_width();
    uint32_t index = 0U;

    if (column_width == 0U || max_columns == 0U) {
        return;
    }
    workspace_copy_raw_text(line_buffer, sizeof(line_buffer), text == 0 ? "" : text, max_columns);
    while (line_buffer[index] != '\0') {
        form_draw_code_point_at_with_colors(
            form,
            (uint8_t)line_buffer[index],
            x + (index * column_width),
            y,
            text_foreground_color(),
            text_background_color()
        );
        ++index;
    }
}

static uint8_t workspace_text_line_at(
    const char *text,
    uint32_t target_line,
    char buffer[],
    uint32_t buffer_size
) {
    const char *cursor = text == 0 ? "" : text;
    uint32_t line = 0U;

    if (buffer_size == 0U) {
        return 0U;
    }
    buffer[0] = '\0';
    while (source_copy_raw_line(&cursor, buffer, buffer_size)) {
        if (line == target_line) {
            return 1U;
        }
        ++line;
    }
    buffer[0] = '\0';
    return 0U;
}

static uint32_t workspace_browser_list_content_top(void) {
    return BROWSER_LIST_VIEW_TOP + VIEW_CONTENT_INSET + text_line_height();
}

static uint32_t workspace_browser_list_row_y(uint32_t row) {
    return workspace_browser_list_content_top() + (row * text_line_height());
}

static uint32_t workspace_browser_list_selection_line_y(uint32_t row) {
    uint32_t line_height = text_line_height();

    if (line_height < 2U) {
        return workspace_browser_list_row_y(row);
    }
    return BROWSER_LIST_VIEW_TOP + VIEW_CONTENT_INSET + (((row + 2U) * line_height) - 2U);
}

static void workspace_clear_browser_list_selection_overlay(
    const struct recorz_mvp_heap_object *form,
    uint32_t row
) {
    uint32_t width = BROWSER_LIST_VIEW_WIDTH - VIEW_CONTENT_HORIZONTAL_PADDING;
    uint32_t x = BROWSER_LIST_VIEW_LEFT + VIEW_CONTENT_INSET;
    uint32_t y = workspace_browser_list_selection_line_y(row);

    if (width == 0U) {
        return;
    }
    form_fill_rect_color(form, x, y, width + 1U, 1U, text_background_color());
}

static void workspace_draw_browser_list_selection_overlay(
    const struct recorz_mvp_heap_object *form,
    uint32_t row
) {
    uint32_t width = BROWSER_LIST_VIEW_WIDTH - VIEW_CONTENT_HORIZONTAL_PADDING;
    uint32_t x = BROWSER_LIST_VIEW_LEFT + VIEW_CONTENT_INSET;
    uint32_t y = workspace_browser_list_selection_line_y(row);

    form_draw_line_color(form, (int32_t)x, (int32_t)y, (int32_t)(x + width), (int32_t)y, 11393254U);
}

static void workspace_redraw_browser_list_row(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t row
) {
    char line_buffer[METHOD_SOURCE_LINE_LIMIT];
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(BROWSER_LIST_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(BROWSER_LIST_VIEW_WIDTH);
    uint32_t line_height = text_line_height();
    uint32_t y;

    if (row >= visible_lines || line_height == 0U) {
        return;
    }
    y = workspace_browser_list_row_y(row);
    form_fill_rect_color(
        form,
        BROWSER_LIST_VIEW_LEFT + 1U,
        y,
        BROWSER_LIST_VIEW_WIDTH - 2U,
        line_height,
        text_background_color()
    );
    if (workspace_text_line_at(text, row, line_buffer, sizeof(line_buffer))) {
        workspace_draw_text_line_at(
            form,
            line_buffer,
            BROWSER_LIST_VIEW_LEFT + VIEW_CONTENT_INSET,
            y,
            visible_columns
        );
    }
}

static void workspace_redraw_browser_list_rows(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t first_row,
    uint32_t row_count
) {
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(BROWSER_LIST_VIEW_HEIGHT);
    uint32_t row_end = first_row + row_count;
    uint32_t row;

    if (first_row >= visible_lines) {
        return;
    }
    if (row_end > visible_lines) {
        row_end = visible_lines;
    }
    for (row = first_row; row < row_end; ++row) {
        workspace_redraw_browser_list_row(form, text, row);
    }
}

static uint8_t workspace_draw_editor_surface_from_image(
    const struct recorz_mvp_heap_object *form,
    const char *source,
    const char *status,
    const char *feedback
) {
    uint16_t object_handle = named_object_handle_for_name("BootWorkspaceEditorSurface");
    struct recorz_mvp_value arguments[4];

    if (object_handle == 0U) {
        return 0U;
    }
    workspace_invalidate_editor_surface_views();
    arguments[0] = string_value(source == 0 ? "" : source);
    arguments[1] = string_value(status == 0 ? "" : status);
    arguments[2] = string_value(feedback == 0 ? "" : feedback);
    arguments[3] = object_value(heap_handle_for_object(form));
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_DRAW_SOURCE_STATUS_FEEDBACK_ON_FORM,
        4U,
        arguments,
        0
    );
    return 1U;
}

static void workspace_require_editor_surface(
    const struct recorz_mvp_heap_object *form,
    const char *source,
    const char *status,
    const char *feedback
) {
    if (!workspace_draw_editor_surface_from_image(form, source, status, feedback)) {
        machine_panic("Workspace editor surface is missing");
    }
}

static uint8_t workspace_draw_browser_surface_from_image(
    const struct recorz_mvp_heap_object *form,
    const char *list_text,
    const char *source_text,
    const char *title_text,
    const char *status_text
) {
    uint16_t object_handle = named_object_handle_for_name("BootBrowserSurface");
    struct recorz_mvp_value arguments[5];

    if (object_handle == 0U) {
        return 0U;
    }
    workspace_invalidate_browser_surface_views();
    arguments[0] = string_value(list_text == 0 ? "" : list_text);
    arguments[1] = string_value(source_text == 0 ? "" : source_text);
    arguments[2] = string_value(title_text == 0 ? "" : title_text);
    arguments[3] = string_value(status_text == 0 ? "" : status_text);
    arguments[4] = object_value(heap_handle_for_object(form));
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_DRAW_LIST_SOURCE_TITLE_STATUS_ON_FORM,
        5U,
        arguments,
        0
    );
    return 1U;
}

static void workspace_require_browser_surface(
    const struct recorz_mvp_heap_object *form,
    const char *list_text,
    const char *source_text,
    const char *title_text,
    const char *status_text
) {
    if (!workspace_draw_browser_surface_from_image(form, list_text, source_text, title_text, status_text)) {
        machine_panic("Workspace browser surface is missing");
    }
}

static void workspace_clear_view_content_area(
    const struct recorz_mvp_heap_object *form,
    uint32_t left,
    uint32_t top,
    uint32_t width,
    uint32_t height
) {
    uint32_t content_top = top + VIEW_CONTENT_INSET + text_line_height();
    uint32_t content_width;
    uint32_t content_height;

    if (width <= 2U || height <= VIEW_CONTENT_INSET + text_line_height() + 1U) {
        return;
    }
    content_width = width - 2U;
    content_height = height - (VIEW_CONTENT_INSET + text_line_height()) - 1U;
    if (content_width == 0U || content_height == 0U) {
        return;
    }
    form_fill_rect_color(
        form,
        left + 1U,
        content_top,
        content_width,
        content_height,
        text_background_color()
    );
}

static uint8_t workspace_redraw_named_status_widget(
    const struct recorz_mvp_heap_object *form,
    const char *widget_name,
    const char *view_name,
    const char *status,
    const char *feedback
) {
    uint16_t widget_handle = named_object_handle_for_name(widget_name);
    uint16_t view_handle = named_object_handle_for_name(view_name);
    struct recorz_mvp_value arguments[3];

    if (widget_handle == 0U || view_handle == 0U) {
        return 0U;
    }
    arguments[0] = object_value(view_handle);
    arguments[1] = string_value(status == 0 ? "" : status);
    arguments[2] = string_value(feedback == 0 ? "" : feedback);
    (void)perform_send_and_pop_result(
        object_value(widget_handle),
        RECORZ_MVP_SELECTOR_SET_VIEW_STATUS_FEEDBACK,
        3U,
        arguments,
        0
    );
    arguments[0] = object_value(heap_handle_for_object(form));
    (void)perform_send_and_pop_result(
        object_value(widget_handle),
        RECORZ_MVP_SELECTOR_REDRAW_ON_FORM,
        1U,
        arguments,
        0
    );
    return 1U;
}

static const char *workspace_session_current_text_for_selector(uint16_t selector) {
    uint16_t session_handle = named_object_handle_for_name("BootWorkspaceSession");
    struct recorz_mvp_value result;

    if (session_handle == 0U) {
        return 0;
    }
    result = perform_send_and_pop_result(
        object_value(session_handle),
        selector,
        0U,
        0,
        0
    );
    if (result.kind != RECORZ_MVP_VALUE_STRING || result.string == 0) {
        return 0;
    }
    return result.string;
}

static uint32_t workspace_session_small_integer_for_selector(uint16_t selector, const char *message) {
    uint16_t session_handle = named_object_handle_for_name("BootWorkspaceSession");
    struct recorz_mvp_value result;

    if (session_handle == 0U) {
        return 0U;
    }
    result = perform_send_and_pop_result(
        object_value(session_handle),
        selector,
        0U,
        0,
        0
    );
    return small_integer_u32(result, message);
}

static uint32_t workspace_session_selected_index(void) {
    return workspace_session_small_integer_for_selector(
        RECORZ_MVP_SELECTOR_SELECTED_INDEX,
        "BootWorkspaceSession selectedIndex did not return a non-negative small integer"
    );
}

static uint32_t workspace_session_list_top_line(void) {
    return workspace_session_small_integer_for_selector(
        RECORZ_MVP_SELECTOR_LIST_TOP_LINE,
        "BootWorkspaceSession listTopLine did not return a non-negative small integer"
    );
}

static uint32_t workspace_session_visible_list_lines(void) {
    return workspace_session_small_integer_for_selector(
        RECORZ_MVP_SELECTOR_VISIBLE_LIST_LINES,
        "BootWorkspaceSession visibleListLines did not return a non-negative small integer"
    );
}

static uint8_t workspace_redraw_image_session_status_only(void) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const char *status = workspace_session_current_text_for_selector(
        RECORZ_MVP_SELECTOR_CURRENT_STATUS_TEXT
    );
    const char *feedback = workspace_session_current_text_for_selector(
        RECORZ_MVP_SELECTOR_CURRENT_FEEDBACK_TEXT
    );

    workspace_clear_view_content_area(
        form,
        STATUS_VIEW_LEFT,
        STATUS_VIEW_TOP,
        STATUS_VIEW_WIDTH,
        STATUS_VIEW_HEIGHT
    );
    if (!workspace_redraw_named_status_widget(
            form,
            "BootWorkspaceStatusWidget",
            "BootWorkspaceStatusView",
            status,
            feedback)) {
        return 0U;
    }
    ++render_counter_editor_status_redraws;
    return 1U;
}

static uint8_t workspace_scroll_copy_image_session_editor_viewport(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t old_cursor_line,
    uint32_t old_cursor_column,
    uint32_t old_top_line,
    uint32_t old_left_column
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *source =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t top_line = workspace_visible_origin_top_line_value();
    uint32_t left_column = workspace_visible_origin_left_column_value();
    uint32_t cursor_line = workspace_cursor_line_value();
    uint32_t cursor_column = workspace_cursor_column_value();
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t line_height = text_line_height();
    uint32_t column_width = char_width();
    int32_t top_delta = (int32_t)top_line - (int32_t)old_top_line;
    int32_t left_delta = (int32_t)left_column - (int32_t)old_left_column;
    uint32_t content_left;
    uint32_t content_top;
    uint32_t content_width;
    uint32_t content_height;
    uint32_t vertical_pixels;
    uint32_t horizontal_pixels;
    const char *status;
    const char *feedback;

    if ((top_delta == 0 && left_delta == 0) ||
        visible_lines == 0U ||
        visible_columns == 0U ||
        line_height == 0U ||
        column_width == 0U) {
        return 0U;
    }
    if ((top_delta > 0 && (uint32_t)top_delta >= visible_lines) ||
        (top_delta < 0 && (uint32_t)(-top_delta) >= visible_lines) ||
        (left_delta > 0 && (uint32_t)left_delta >= visible_columns) ||
        (left_delta < 0 && (uint32_t)(-left_delta) >= visible_columns)) {
        return 0U;
    }
    if (old_cursor_line >= old_top_line &&
        old_cursor_line < old_top_line + visible_lines &&
        old_cursor_column >= old_left_column &&
        old_cursor_column < old_left_column + visible_columns) {
        workspace_redraw_editor_source_absolute_cell(
            form,
            source,
            old_top_line,
            old_left_column,
            old_cursor_line,
            old_cursor_column
        );
    }
    content_left = WORKSPACE_SOURCE_VIEW_LEFT + VIEW_CONTENT_INSET;
    content_top = WORKSPACE_SOURCE_VIEW_TOP + VIEW_CONTENT_INSET + text_line_height();
    content_width = visible_columns * column_width;
    content_height = visible_lines * line_height;
    vertical_pixels = (uint32_t)(top_delta < 0 ? -top_delta : top_delta) * line_height;
    horizontal_pixels = (uint32_t)(left_delta < 0 ? -left_delta : left_delta) * column_width;
    if (horizontal_pixels < content_width && vertical_pixels < content_height) {
        form_copy_rect(
            form,
            content_left + (left_delta > 0 ? horizontal_pixels : 0U),
            content_top + (top_delta > 0 ? vertical_pixels : 0U),
            content_width - horizontal_pixels,
            content_height - vertical_pixels,
            content_left + (left_delta < 0 ? horizontal_pixels : 0U),
            content_top + (top_delta < 0 ? vertical_pixels : 0U)
        );
    }
    workspace_surface_copy_source_viewport(
        workspace_surface_editor_buffer,
        sizeof(workspace_surface_editor_buffer),
        source,
        top_line,
        left_column,
        visible_lines,
        visible_columns
    );
    if (top_delta > 0) {
        workspace_redraw_editor_source_viewport_lines(
            form,
            workspace_surface_editor_buffer,
            visible_lines - (uint32_t)top_delta,
            (uint32_t)top_delta
        );
    } else if (top_delta < 0) {
        workspace_redraw_editor_source_viewport_lines(
            form,
            workspace_surface_editor_buffer,
            0U,
            (uint32_t)(-top_delta)
        );
    }
    if (left_delta > 0) {
        workspace_redraw_editor_source_viewport_columns(
            form,
            workspace_surface_editor_buffer,
            visible_columns - (uint32_t)left_delta,
            (uint32_t)left_delta
        );
    } else if (left_delta < 0) {
        workspace_redraw_editor_source_viewport_columns(
            form,
            workspace_surface_editor_buffer,
            0U,
            (uint32_t)(-left_delta)
        );
    }
    status = workspace_session_current_text_for_selector(RECORZ_MVP_SELECTOR_CURRENT_STATUS_TEXT);
    feedback = workspace_session_current_text_for_selector(RECORZ_MVP_SELECTOR_CURRENT_FEEDBACK_TEXT);
    workspace_clear_view_content_area(
        form,
        STATUS_VIEW_LEFT,
        STATUS_VIEW_TOP,
        STATUS_VIEW_WIDTH,
        STATUS_VIEW_HEIGHT
    );
    if (!workspace_redraw_named_status_widget(
            form,
            "BootWorkspaceStatusWidget",
            "BootWorkspaceStatusView",
            status,
            feedback)) {
        return 0U;
    }
    if (cursor_line < top_line ||
        cursor_line >= top_line + visible_lines ||
        cursor_column < left_column ||
        cursor_column >= left_column + visible_columns) {
        return 0U;
    }
    workspace_draw_editor_cursor_overlay(
        form,
        cursor_line - top_line,
        cursor_column - left_column
    );
    ++render_counter_editor_scroll_redraws;
    return 1U;
}

static uint8_t workspace_redraw_named_label_widget(
    const struct recorz_mvp_heap_object *form,
    const char *widget_name,
    const char *view_name,
    const char *text
) {
    uint16_t widget_handle = named_object_handle_for_name(widget_name);
    uint16_t view_handle = named_object_handle_for_name(view_name);
    struct recorz_mvp_value arguments[2];

    if (widget_handle == 0U || view_handle == 0U) {
        return 0U;
    }
    arguments[0] = object_value(view_handle);
    arguments[1] = string_value(text == 0 ? "" : text);
    (void)perform_send_and_pop_result(
        object_value(widget_handle),
        RECORZ_MVP_SELECTOR_SET_VIEW_TEXT,
        2U,
        arguments,
        0
    );
    arguments[0] = object_value(heap_handle_for_object(form));
    (void)perform_send_and_pop_result(
        object_value(widget_handle),
        RECORZ_MVP_SELECTOR_REDRAW_ON_FORM,
        1U,
        arguments,
        0
    );
    return 1U;
}

static uint8_t workspace_redraw_image_session_package_source_editor(
    const struct recorz_mvp_heap_object *workspace_object
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *source =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    const char *header = workspace_session_current_text_for_selector(
        RECORZ_MVP_SELECTOR_CURRENT_HEADER_TEXT
    );
    const char *status = workspace_session_current_text_for_selector(
        RECORZ_MVP_SELECTOR_CURRENT_STATUS_TEXT
    );
    const char *feedback = workspace_session_current_text_for_selector(
        RECORZ_MVP_SELECTOR_CURRENT_FEEDBACK_TEXT
    );
    uint32_t top_line = workspace_visible_origin_top_line_value();
    uint32_t left_column = workspace_visible_origin_left_column_value();
    uint32_t cursor_line = workspace_cursor_line_value();
    uint32_t cursor_column = workspace_cursor_column_value();

    workspace_surface_copy_source_viewport(
        workspace_surface_editor_buffer,
        sizeof(workspace_surface_editor_buffer),
        source,
        top_line,
        left_column,
        workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT),
        workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH)
    );
    workspace_require_editor_surface(form, "", status, feedback);
    if (!workspace_redraw_named_label_widget(
            form,
            "BootWorkspaceHeaderWidget",
            "BootWorkspaceHeaderView",
            header)) {
        return 0U;
    }
    workspace_draw_editor_source_viewport_overlay(
        form,
        workspace_surface_editor_buffer,
        cursor_line >= top_line ? cursor_line - top_line : 0U,
        cursor_column >= left_column ? cursor_column - left_column : 0U
    );
    return 1U;
}

static uint8_t workspace_input_byte_is_status_only_command(uint8_t ch) {
    return (uint8_t)(
        ch == 0x04 ||
        ch == 0x10 ||
        ch == 0x12 ||
        ch == 0x14
    );
}

static uint8_t workspace_redraw_named_list_widget(
    const struct recorz_mvp_heap_object *form,
    const char *widget_name,
    const char *view_name,
    const char *items
) {
    uint16_t widget_handle = named_object_handle_for_name(widget_name);
    uint16_t view_handle = named_object_handle_for_name(view_name);
    struct recorz_mvp_value arguments[4];

    if (widget_handle == 0U || view_handle == 0U) {
        return 0U;
    }
    arguments[0] = object_value(view_handle);
    arguments[1] = string_value(items == 0 ? "" : items);
    arguments[2] = small_integer_value(0);
    arguments[3] = small_integer_value(0);
    (void)perform_send_and_pop_result(
        object_value(widget_handle),
        RECORZ_MVP_SELECTOR_SET_VIEW_ITEMS_SELECTED_INDEX_TOP_LINE,
        4U,
        arguments,
        0
    );
    arguments[0] = object_value(heap_handle_for_object(form));
    (void)perform_send_and_pop_result(
        object_value(widget_handle),
        RECORZ_MVP_SELECTOR_REDRAW_ON_FORM,
        1U,
        arguments,
        0
    );
    return 1U;
}

static const char *workspace_package_names_visible_text(
    uint32_t first_index,
    uint32_t count,
    char buffer[],
    uint32_t buffer_size
);

static uint8_t workspace_scroll_copy_image_session_browser_list(
    uint32_t old_selected_index,
    uint32_t old_list_top_line
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    uint32_t selected_index = workspace_session_selected_index();
    uint32_t list_top_line = workspace_session_list_top_line();
    uint32_t visible_lines = workspace_session_visible_list_lines();
    uint32_t line_height = text_line_height();
    int32_t top_delta = (int32_t)list_top_line - (int32_t)old_list_top_line;
    uint32_t content_top = workspace_browser_list_content_top();
    uint32_t content_height;
    uint32_t content_width;
    uint32_t vertical_pixels;
    const char *visible_text;

    if (visible_lines == 0U || line_height == 0U) {
        return 0U;
    }
    if (selected_index == old_selected_index && list_top_line == old_list_top_line) {
        return 0U;
    }
    if (old_selected_index >= old_list_top_line &&
        old_selected_index < old_list_top_line + visible_lines) {
        workspace_clear_browser_list_selection_overlay(form, old_selected_index - old_list_top_line);
    }
    visible_text = workspace_package_names_visible_text(
        list_top_line + 1U,
        visible_lines,
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer)
    );
    if (top_delta == 0) {
        if (selected_index >= list_top_line && selected_index < list_top_line + visible_lines) {
            workspace_draw_browser_list_selection_overlay(form, selected_index - list_top_line);
        }
        ++render_counter_browser_list_redraws;
        return 1U;
    }
    if ((top_delta > 0 && (uint32_t)top_delta >= visible_lines) ||
        (top_delta < 0 && (uint32_t)(-top_delta) >= visible_lines) ||
        BROWSER_LIST_VIEW_HEIGHT <= VIEW_CONTENT_INSET + line_height + 1U) {
        return 0U;
    }
    content_width = BROWSER_LIST_VIEW_WIDTH - 2U;
    content_height = BROWSER_LIST_VIEW_HEIGHT - (VIEW_CONTENT_INSET + line_height) - 1U;
    vertical_pixels = (uint32_t)(top_delta < 0 ? -top_delta : top_delta) * line_height;
    if (content_width == 0U || content_height == 0U || vertical_pixels >= content_height) {
        return 0U;
    }
    form_copy_rect(
        form,
        BROWSER_LIST_VIEW_LEFT + 1U,
        content_top + (top_delta > 0 ? vertical_pixels : 0U),
        content_width,
        content_height - vertical_pixels,
        BROWSER_LIST_VIEW_LEFT + 1U,
        content_top + (top_delta < 0 ? vertical_pixels : 0U)
    );
    if (top_delta > 0) {
        workspace_redraw_browser_list_rows(
            form,
            visible_text,
            visible_lines - (uint32_t)top_delta,
            (uint32_t)top_delta
        );
    } else {
        workspace_redraw_browser_list_rows(
            form,
            visible_text,
            0U,
            (uint32_t)(-top_delta)
        );
    }
    if (selected_index >= list_top_line && selected_index < list_top_line + visible_lines) {
        workspace_draw_browser_list_selection_overlay(form, selected_index - list_top_line);
    }
    ++render_counter_browser_list_redraws;
    return 1U;
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
    uint32_t list_offset = 0U;
    uint32_t source_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLASS",
        class_name
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "SUPER",
        superclass_name
    );
    if (definition != 0 && definition->package_name[0] != '\0') {
        workspace_surface_append_label_text(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "PACKAGE",
            definition->package_name
        );
    }
    if (definition != 0 && definition->class_comment[0] != '\0') {
        workspace_surface_append_label_text(
            workspace_surface_source_buffer,
            sizeof(workspace_surface_source_buffer),
            &source_offset,
            "COMMENT",
            definition->class_comment
        );
    }
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "SLOTS",
        live_instance_field_count_for_class(class_object)
    );
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "METHODS",
        class_method_count(class_object)
    );
    workspace_surface_append_line(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "Open methods, protocols, or canonical source from this class."
    );
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        class_name,
        "CLASS BROWSER"
    );
}

static uint32_t workspace_package_count(void) {
    return package_count;
}

static uint32_t workspace_class_count(void) {
    uint8_t kind;
    uint32_t count = 0U;

    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] != 0U) {
            ++count;
        }
    }
    return count + (uint32_t)dynamic_class_count;
}

static const char *workspace_class_name_at_index(uint32_t one_based_index) {
    uint8_t kind;
    uint32_t remaining;

    if (one_based_index == 0U) {
        return 0;
    }
    remaining = one_based_index;
    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] == 0U) {
            continue;
        }
        if (remaining == 1U) {
            return object_kind_name(kind);
        }
        --remaining;
    }
    if (remaining == 0U || remaining > (uint32_t)dynamic_class_count) {
        return 0;
    }
    return dynamic_classes[remaining - 1U].class_name;
}

static const char *workspace_class_names_visible_text(
    uint32_t first_index,
    uint32_t count,
    char buffer[],
    uint32_t buffer_size
) {
    uint32_t offset = 0U;
    uint32_t index;

    if (buffer_size == 0U) {
        return "";
    }
    buffer[0] = '\0';
    if (count == 0U) {
        return buffer;
    }
    if (first_index == 0U) {
        first_index = 1U;
    }
    if (first_index > workspace_class_count()) {
        return buffer;
    }
    for (index = first_index; index < first_index + count; ++index) {
        const char *class_name = workspace_class_name_at_index(index);

        if (class_name == 0) {
            break;
        }
        if (offset != 0U) {
            append_text_checked(buffer, buffer_size, &offset, "\n");
        }
        append_text_checked(buffer, buffer_size, &offset, class_name);
    }
    return buffer;
}

static const char *workspace_package_name_at_index(uint32_t one_based_index) {
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT];
    uint16_t sorted_count;

    if (one_based_index == 0U) {
        return 0;
    }
    sorted_count = workspace_collect_sorted_packages(sorted_packages);
    if (one_based_index > sorted_count) {
        return 0;
    }
    return sorted_packages[one_based_index - 1U]->package_name;
}

static const char *workspace_package_names_visible_text(
    uint32_t first_index,
    uint32_t count,
    char buffer[],
    uint32_t buffer_size
) {
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT];
    uint16_t sorted_count;
    uint32_t offset = 0U;
    uint32_t index;

    if (buffer_size == 0U) {
        return "";
    }
    buffer[0] = '\0';
    if (count == 0U) {
        return buffer;
    }
    if (first_index == 0U) {
        first_index = 1U;
    }
    sorted_count = workspace_collect_sorted_packages(sorted_packages);
    if (first_index > sorted_count) {
        return buffer;
    }
    for (index = first_index - 1U; index < sorted_count && index < (first_index - 1U) + count; ++index) {
        if (offset != 0U) {
            append_text_checked(buffer, buffer_size, &offset, "\n");
        }
        append_text_checked(buffer, buffer_size, &offset, sorted_packages[index]->package_name);
    }
    return buffer;
}

static const char *workspace_visible_contents_text(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t top_line,
    uint32_t left_column,
    uint32_t line_count,
    uint32_t column_count,
    char buffer[],
    uint32_t buffer_size
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *source = "";
    uint32_t offset = 0U;
    uint32_t current_line = 0U;
    uint32_t current_column = 0U;
    uint32_t lines_emitted = 0U;

    if (buffer_size == 0U) {
        return "";
    }
    buffer[0] = '\0';
    if (line_count == 0U || column_count == 0U) {
        return buffer;
    }
    if (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0) {
        source = source_value.string;
    }
    while (*source != '\0' && lines_emitted < line_count) {
        char ch = *source++;

        if (current_line < top_line) {
            if (ch == '\n') {
                ++current_line;
                current_column = 0U;
            } else {
                ++current_column;
            }
            continue;
        }
        if (ch == '\n') {
            if (lines_emitted + 1U < line_count) {
                append_text_checked(buffer, buffer_size, &offset, "\n");
            }
            ++lines_emitted;
            ++current_line;
            current_column = 0U;
            continue;
        }
        if (current_column >= left_column &&
            (current_column - left_column) < column_count) {
            if (offset + 1U >= buffer_size) {
                machine_panic("Workspace visible contents exceed buffer capacity");
            }
            buffer[offset++] = ch;
            buffer[offset] = '\0';
        }
        ++current_column;
    }
    return buffer;
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

static uint16_t workspace_collect_sorted_packages(
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT]
) {
    uint16_t package_index;
    uint16_t sorted_count = 0U;

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
    return sorted_count;
}

static void workspace_render_package_list_browser(
    const struct recorz_mvp_heap_object *workspace_object
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT];
    uint16_t package_index;
    uint16_t sorted_count;
    uint32_t list_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "VIEW",
        "PACKAGES"
    );
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "PACKAGES",
        workspace_package_count()
    );
    if (workspace_package_count() == 0U) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "NO PACKAGES"
        );
    } else {
        sorted_count = workspace_collect_sorted_packages(sorted_packages);
        for (package_index = 0U; package_index < sorted_count; ++package_index) {
            char line[METHOD_SOURCE_LINE_LIMIT];
            uint32_t line_offset = 0U;

            append_text_checked(line, sizeof(line), &line_offset, sorted_packages[package_index]->package_name);
            append_text_checked(line, sizeof(line), &line_offset, " :: ");
            render_small_integer((int32_t)workspace_package_class_count(sorted_packages[package_index]->package_name));
            append_text_checked(line, sizeof(line), &line_offset, print_buffer);
            workspace_surface_append_line(
                workspace_surface_list_buffer,
                sizeof(workspace_surface_list_buffer),
                &list_offset,
                line
            );
        }
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        "Browse a package, then edit its source in the same tool flow.",
        "PACKAGES",
        "BROWSE PACKAGES"
    );
}

static void workspace_prepare_interactive_package_list_buffer(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT],
    uint16_t sorted_count,
    uint16_t selected_index
) {
    uint16_t package_index;
    uint32_t list_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "VIEW",
        "PACKAGES"
    );
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "PACKAGES",
        sorted_count
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "MOVE",
        "ARROWS CTRL-N/P"
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "EDIT",
        "ENTER/CTRL-X"
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLOSE",
        "CTRL-O/D"
    );
    if (sorted_count == 0U) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "NO PACKAGES"
        );
    } else {
        if (selected_index >= sorted_count) {
            selected_index = (uint16_t)(sorted_count - 1U);
        }
        for (package_index = 0U; package_index < sorted_count; ++package_index) {
            char line[METHOD_SOURCE_LINE_LIMIT];
            uint32_t line_offset = 0U;

            append_text_checked(
                line,
                sizeof(line),
                &line_offset,
                package_index == selected_index ? "> " : "  "
            );
            append_text_checked(
                line,
                sizeof(line),
                &line_offset,
                sorted_packages[package_index]->package_name
            );
            append_text_checked(line, sizeof(line), &line_offset, " :: ");
            render_small_integer((int32_t)workspace_package_class_count(sorted_packages[package_index]->package_name));
            append_text_checked(line, sizeof(line), &line_offset, print_buffer);
            workspace_surface_append_line(
                workspace_surface_list_buffer,
                sizeof(workspace_surface_list_buffer),
                &list_offset,
                line
            );
        }
    }
}

static void workspace_render_interactive_package_list_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT],
    uint16_t sorted_count,
    uint16_t selected_index
) {
    const struct recorz_mvp_heap_object *form = default_form_object();

    ++render_counter_browser_full_redraws;
    workspace_prepare_interactive_package_list_buffer(
        workspace_object,
        sorted_packages,
        sorted_count,
        selected_index
    );
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        "Arrow keys move. Enter or Ctrl-X opens the selected package source.",
        "PACKAGES",
        "INTERACTIVE PACKAGE LIST"
    );
}

static void workspace_render_interactive_package_list_browser_incremental(
    const struct recorz_mvp_heap_object *workspace_object,
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT],
    uint16_t sorted_count,
    uint16_t selected_index
) {
    const struct recorz_mvp_heap_object *form = default_form_object();

    ++render_counter_browser_list_redraws;
    workspace_prepare_interactive_package_list_buffer(
        workspace_object,
        sorted_packages,
        sorted_count,
        selected_index
    );
    workspace_clear_view_content_area(
        form,
        BROWSER_LIST_VIEW_LEFT,
        BROWSER_LIST_VIEW_TOP,
        BROWSER_LIST_VIEW_WIDTH,
        BROWSER_LIST_VIEW_HEIGHT
    );
    if (!workspace_redraw_named_list_widget(
            form,
            "BootBrowserListWidget",
            "BootBrowserListView",
            workspace_surface_list_buffer)) {
        workspace_require_browser_surface(
            form,
            workspace_surface_list_buffer,
            "Arrow keys move. Enter or Ctrl-X opens the selected package source.",
            "PACKAGES",
            "INTERACTIVE PACKAGE LIST"
        );
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
    uint32_t list_offset = 0U;
    uint32_t source_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLASSES",
        class_count
    );
    if (class_count == 0U) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "EMPTY PACKAGE"
        );
    } else {
        for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
            if (!source_names_equal(dynamic_classes[dynamic_index].package_name, package_name)) {
                continue;
            }
            workspace_surface_append_line(
                workspace_surface_list_buffer,
                sizeof(workspace_surface_list_buffer),
                &list_offset,
                dynamic_classes[dynamic_index].class_name
            );
        }
    }
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "PACKAGE",
        package_name
    );
    if (package_definition != 0 && package_definition->package_comment[0] != '\0') {
        workspace_surface_append_label_text(
            workspace_surface_source_buffer,
            sizeof(workspace_surface_source_buffer),
            &source_offset,
            "COMMENT",
            package_definition->package_comment
        );
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        package_name,
        "PACKAGE BROWSER"
    );
}

static void workspace_render_class_list_browser(
    const struct recorz_mvp_heap_object *workspace_object
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    uint8_t kind;
    uint16_t dynamic_index;
    uint32_t seeded_count = 0U;
    uint32_t list_offset = 0U;

    (void)workspace_object;
    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] != 0U) {
            ++seeded_count;
        }
    }
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "SEEDED",
        seeded_count
    );
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "DYNAMIC",
        dynamic_class_count
    );
    for (kind = 1U; kind <= MAX_OBJECT_KIND; ++kind) {
        if (class_descriptor_handles_by_kind[kind] == 0U) {
            continue;
        }
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            object_kind_name(kind)
        );
    }
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            dynamic_classes[dynamic_index].class_name
        );
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        "Browse a class, then open methods, protocols, or canonical source.",
        "CLASSES",
        "CLASS BROWSER"
    );
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
    uint32_t list_offset = 0U;
    uint32_t source_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "METHODS",
        method_count
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "CLASS",
        class_name
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "SIDE",
        side_label
    );
    if (method_count == 0U) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "NO METHODS"
        );
    } else {
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
                method_descriptor_selector(method_object),
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
                selector_name(method_descriptor_selector(method_object))
            );
            workspace_surface_append_line(
                workspace_surface_list_buffer,
                sizeof(workspace_surface_list_buffer),
                &list_offset,
                line
            );
        }
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        class_name,
        side_label
    );
}

static const char *workspace_protocol_name_for_method(
    const struct recorz_mvp_heap_object *class_object,
    const struct recorz_mvp_heap_object *method_object
) {
    const struct recorz_mvp_live_method_source *source_record = live_method_source_for_selector_and_arity(
        heap_handle_for_object(class_object),
        method_descriptor_selector(method_object),
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
    uint32_t list_offset = 0U;
    uint32_t source_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "PROTOCOLS",
        protocol_count
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "CLASS",
        class_name
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "SIDE",
        side_label
    );
    if (method_count == 0U) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "NO METHODS"
        );
    } else {
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
            workspace_surface_append_line(
                workspace_surface_list_buffer,
                sizeof(workspace_surface_list_buffer),
                &list_offset,
                line
            );
        }
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        class_name,
        side_label
    );
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
    uint32_t list_offset = 0U;
    uint32_t source_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "METHODS",
        protocol_method_count
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "CLASS",
        class_name
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "SIDE",
        side_label
    );
    workspace_surface_append_label_text(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        "PROTO",
        protocol_name
    );
    if (method_count == 0U || protocol_method_count == 0U) {
        workspace_surface_append_line(
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer),
            &list_offset,
            "NO METHODS"
        );
    } else {
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
            workspace_surface_append_line(
                workspace_surface_list_buffer,
                sizeof(workspace_surface_list_buffer),
                &list_offset,
                selector_name(method_descriptor_selector(method_object))
            );
        }
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        protocol_name,
        side_label
    );
}

static void workspace_render_method_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *class_name,
    const char *selector_name_text,
    const char *side_label
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t list_offset = 0U;

    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLASS",
        class_name
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "SIDE",
        side_label
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "METHOD",
        selector_name_text
    );
    workspace_surface_copy_source_viewport(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "NO SOURCE BUFFER",
        0U,
        0U,
        workspace_surface_visible_line_capacity_for_view_height(BROWSER_SOURCE_VIEW_HEIGHT),
        workspace_surface_visible_column_capacity_for_view_width(BROWSER_SOURCE_VIEW_WIDTH)
    );
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        selector_name_text,
        "METHOD SOURCE"
    );
}

static void workspace_render_class_source_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *class_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t list_offset = 0U;

    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLASS",
        class_name
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "VIEW",
        "SOURCE"
    );
    workspace_surface_copy_source_viewport(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "NO SOURCE BUFFER",
        0U,
        0U,
        workspace_surface_visible_line_capacity_for_view_height(BROWSER_SOURCE_VIEW_HEIGHT),
        workspace_surface_visible_column_capacity_for_view_width(BROWSER_SOURCE_VIEW_WIDTH)
    );
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        class_name,
        "CLASS SOURCE"
    );
}

static void workspace_render_package_source_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *package_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t list_offset = 0U;

    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "PACKAGE",
        package_name
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "VIEW",
        "SOURCE"
    );
    workspace_surface_copy_source_viewport(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "NO SOURCE BUFFER",
        0U,
        0U,
        workspace_surface_visible_line_capacity_for_view_height(BROWSER_SOURCE_VIEW_HEIGHT),
        workspace_surface_visible_column_capacity_for_view_width(BROWSER_SOURCE_VIEW_WIDTH)
    );
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        package_name,
        "PACKAGE SOURCE"
    );
}

static void workspace_render_regenerated_source_browser(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *source_name
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t list_offset = 0U;

    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "VIEW",
        "REGEN"
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "SOURCE",
        source_name
    );
    workspace_surface_copy_source_viewport(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "NO SOURCE BUFFER",
        0U,
        0U,
        workspace_surface_visible_line_capacity_for_view_height(BROWSER_SOURCE_VIEW_HEIGHT),
        workspace_surface_visible_column_capacity_for_view_width(BROWSER_SOURCE_VIEW_WIDTH)
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLOSE",
        "CTRL-O/D"
    );
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        source_name,
        "REGENERATED SOURCE"
    );
}

static void workspace_render_input_monitor_browser_mode(
    const struct recorz_mvp_heap_object *workspace_object,
    uint8_t full_redraw,
    uint8_t cursor_only_redraw,
    uint32_t old_cursor_line,
    uint32_t old_cursor_column,
    uint32_t old_top_line
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    char mode_text[METHOD_SOURCE_NAME_LIMIT];
    char target_text[METHOD_SOURCE_CHUNK_LIMIT];
    uint32_t cursor_index;
    uint32_t cursor_line = 0U;
    uint32_t cursor_column = 0U;
    uint32_t top_line;
    uint32_t visible_line_capacity;
    uint32_t saved_cursor_index = 0U;
    uint32_t saved_top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;
    uint8_t source_editor_mode = 0U;
    char saved_target_name[METHOD_SOURCE_CHUNK_LIMIT];
    char compact_status[METHOD_SOURCE_NAME_LIMIT];

    saved_target_name[0] = '\0';
    compact_status[0] = '\0';
    if (workspace_parse_input_monitor_state(
            heap_get_field(
                workspace_object,
                workspace_current_target_name_field_index(workspace_object)
            ).kind == RECORZ_MVP_VALUE_STRING
                ? heap_get_field(
                      workspace_object,
                      workspace_current_target_name_field_index(workspace_object)
                  ).string
                : 0,
            &saved_cursor_index,
            &saved_top_line,
            &saved_view_kind,
            saved_target_name,
            sizeof(saved_target_name),
            0,
            0U,
            0,
            0U) &&
        (saved_view_kind == WORKSPACE_VIEW_METHOD ||
         saved_view_kind == WORKSPACE_VIEW_CLASS_METHOD ||
         saved_view_kind == WORKSPACE_VIEW_CLASS_SOURCE ||
         saved_view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE)) {
        source_editor_mode = 1U;
    }
    if (source_editor_mode) {
        if (saved_view_kind == WORKSPACE_VIEW_METHOD) {
            append_text_checked(mode_text, sizeof(mode_text), &(uint32_t){0U}, "METHOD SOURCE");
        } else if (saved_view_kind == WORKSPACE_VIEW_CLASS_METHOD) {
            append_text_checked(mode_text, sizeof(mode_text), &(uint32_t){0U}, "CLASS METHOD");
        } else if (saved_view_kind == WORKSPACE_VIEW_CLASS_SOURCE) {
            append_text_checked(mode_text, sizeof(mode_text), &(uint32_t){0U}, "CLASS SOURCE");
        } else {
            append_text_checked(mode_text, sizeof(mode_text), &(uint32_t){0U}, "PACKAGE SOURCE");
        }
        append_text_checked(
            target_text,
            sizeof(target_text),
            &(uint32_t){0U},
            saved_target_name[0] == '\0' ? "CURRENT SOURCE" : saved_target_name
        );
    } else {
        append_text_checked(mode_text, sizeof(mode_text), &(uint32_t){0U}, "WORKSPACE");
        append_text_checked(target_text, sizeof(target_text), &(uint32_t){0U}, "DOIT BUFFER");
    }
    cursor_index = workspace_input_monitor_cursor_index(workspace_object);
    visible_line_capacity = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    top_line = workspace_input_monitor_top_line(workspace_object);
    workspace_input_monitor_cursor_line_and_column(
        source_value.kind == RECORZ_MVP_VALUE_STRING ? source_value.string : 0,
        cursor_index,
        &cursor_line,
        &cursor_column
    );
    if (visible_line_capacity != 0U) {
        if (top_line > cursor_line) {
            top_line = cursor_line;
        }
        if (cursor_line >= top_line + visible_line_capacity) {
            top_line = cursor_line - visible_line_capacity + 1U;
        }
    } else {
        top_line = cursor_line;
    }
    workspace_store_input_monitor_state(workspace_object, cursor_index, top_line);
    {
        uint32_t surface_status_offset = 0U;
        uint32_t surface_feedback_offset = 0U;
        char compact_feedback[METHOD_SOURCE_NAME_LIMIT];
        uint32_t status_columns = workspace_surface_visible_column_capacity_for_view_width(STATUS_VIEW_WIDTH);
        uint32_t status_limit = status_columns > 8U ? status_columns - 8U : status_columns;
        uint32_t feedback_limit = status_columns > 5U ? status_columns - 5U : status_columns;
        uint8_t show_verbose_status = (uint8_t)(
            workspace_input_monitor_status[0] != '\0' &&
            !source_names_equal(
                workspace_input_monitor_status,
                source_editor_mode ? "SOURCE EDITOR READY" : "WORKSPACE READY"
            )
        );

        workspace_surface_reset_buffer(workspace_surface_status_buffer, sizeof(workspace_surface_status_buffer));
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            mode_text
        );
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            " "
        );
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            target_text
        );
        if (show_verbose_status) {
            append_text_checked(
                workspace_surface_status_buffer,
                sizeof(workspace_surface_status_buffer),
                &surface_status_offset,
                " :: "
            );
            append_text_checked(
                workspace_surface_status_buffer,
                sizeof(workspace_surface_status_buffer),
                &surface_status_offset,
                workspace_input_monitor_status
            );
        }
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            "  L"
        );
        render_small_integer((int32_t)(cursor_line + 1U));
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            print_buffer
        );
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            " C"
        );
        render_small_integer((int32_t)(cursor_column + 1U));
        append_text_checked(
            workspace_surface_status_buffer,
            sizeof(workspace_surface_status_buffer),
            &surface_status_offset,
            print_buffer
        );
        workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
        workspace_input_monitor_compact_text_line(
            compact_status,
            sizeof(compact_status),
            workspace_surface_status_buffer,
            status_limit
        );
        compact_feedback[0] = '\0';
        workspace_input_monitor_compact_feedback_line(
            compact_feedback,
            sizeof(compact_feedback),
            feedback_limit
        );
        if (compact_feedback[0] != '\0') {
            append_text_checked(
                workspace_surface_source_buffer,
                sizeof(workspace_surface_source_buffer),
                &surface_feedback_offset,
                compact_feedback
            );
        } else if (source_editor_mode) {
            append_text_checked(
                workspace_surface_source_buffer,
                sizeof(workspace_surface_source_buffer),
                &surface_feedback_offset,
                "X ACCEPT  Y REVERT  O RETURN"
            );
        } else {
            append_text_checked(
                workspace_surface_source_buffer,
                sizeof(workspace_surface_source_buffer),
                &surface_feedback_offset,
                "D DOIT  P PRINT  O BROWSE"
            );
        }
    }
    workspace_surface_copy_source_viewport(
        workspace_surface_editor_buffer,
        sizeof(workspace_surface_editor_buffer),
        source_value.kind == RECORZ_MVP_VALUE_STRING ? source_value.string : "",
        top_line,
        0U,
        visible_line_capacity,
        workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH)
    );
    if (full_redraw) {
        ++render_counter_editor_full_redraws;
        workspace_require_editor_surface(
            form,
            "",
            compact_status,
            workspace_surface_source_buffer
        );
        workspace_draw_editor_source_viewport_overlay(
            form,
            workspace_surface_editor_buffer,
            cursor_line >= top_line ? cursor_line - top_line : 0U,
            cursor_column
        );
        return;
    }
    if (cursor_only_redraw &&
        old_top_line == top_line &&
        cursor_line >= top_line &&
        old_cursor_line >= top_line) {
        ++render_counter_editor_cursor_redraws;
        workspace_clear_view_content_area(
            form,
            STATUS_VIEW_LEFT,
            STATUS_VIEW_TOP,
            STATUS_VIEW_WIDTH,
            STATUS_VIEW_HEIGHT
        );
        if (!workspace_redraw_named_status_widget(
                form,
                "BootWorkspaceStatusWidget",
                "BootWorkspaceStatusView",
                compact_status,
                workspace_surface_source_buffer)) {
            workspace_require_editor_surface(
                form,
                "",
                compact_status,
                workspace_surface_source_buffer
            );
            workspace_draw_editor_source_viewport_overlay(
                form,
                workspace_surface_editor_buffer,
                cursor_line >= top_line ? cursor_line - top_line : 0U,
                cursor_column
            );
            return;
        }
        workspace_redraw_editor_source_cell(
            form,
            workspace_surface_editor_buffer,
            old_cursor_line - top_line,
            old_cursor_column
        );
        workspace_draw_editor_cursor_overlay(
            form,
            cursor_line - top_line,
            cursor_column
        );
        return;
    }
    ++render_counter_editor_pane_redraws;
    workspace_clear_view_content_area(
        form,
        WORKSPACE_SOURCE_VIEW_LEFT,
        WORKSPACE_SOURCE_VIEW_TOP,
        WORKSPACE_SOURCE_VIEW_WIDTH,
        WORKSPACE_SOURCE_VIEW_HEIGHT
    );
    workspace_clear_view_content_area(
        form,
        STATUS_VIEW_LEFT,
        STATUS_VIEW_TOP,
        STATUS_VIEW_WIDTH,
        STATUS_VIEW_HEIGHT
    );
    if (!workspace_redraw_named_status_widget(
            form,
            "BootWorkspaceStatusWidget",
            "BootWorkspaceStatusView",
            compact_status,
            workspace_surface_source_buffer)) {
        workspace_require_editor_surface(
            form,
            "",
            compact_status,
            workspace_surface_source_buffer
        );
            workspace_draw_editor_source_viewport_overlay(
                form,
                workspace_surface_editor_buffer,
                cursor_line >= top_line ? cursor_line - top_line : 0U,
                cursor_column
            );
            return;
    }
    workspace_draw_editor_source_viewport_overlay(
        form,
        workspace_surface_editor_buffer,
        cursor_line >= top_line ? cursor_line - top_line : 0U,
        cursor_column
    );
}

static void workspace_render_input_monitor_browser(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_render_input_monitor_browser_mode(workspace_object, 1U, 0U, 0U, 0U, 0U);
}

static void workspace_render_input_monitor_browser_incremental(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_render_input_monitor_browser_mode(workspace_object, 0U, 0U, 0U, 0U, 0U);
}

static void workspace_render_input_monitor_browser_cursor_move(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t old_cursor_line,
    uint32_t old_cursor_column,
    uint32_t old_top_line
) {
    workspace_render_input_monitor_browser_mode(
        workspace_object,
        0U,
        1U,
        old_cursor_line,
        old_cursor_column,
        old_top_line
    );
}

static void workspace_bind_input_monitor_buffer(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t length = 0U;
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);

    if (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0) {
        while (source_value.string[length] != '\0' && length + 1U < sizeof(workspace_input_monitor_buffer)) {
            workspace_input_monitor_buffer[length] = source_value.string[length];
            ++length;
        }
    }
    workspace_input_monitor_buffer[length] = '\0';
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        string_value(workspace_input_monitor_buffer)
    );
}

static uint8_t workspace_parse_input_monitor_state(
    const char *text,
    uint32_t *cursor_index_out,
    uint32_t *top_line_out,
    uint32_t *saved_view_kind_out,
    char *saved_target_out,
    uint32_t saved_target_out_size,
    char *status_out,
    uint32_t status_out_size,
    char *feedback_out,
    uint32_t feedback_out_size
) {
    const char *cursor = text;
    uint32_t cursor_index;
    uint32_t top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;

    if (!source_starts_with(cursor, "CURSOR:")) {
        return 0U;
    }
    if (saved_target_out != 0 && saved_target_out_size != 0U) {
        saved_target_out[0] = '\0';
    }
    if (status_out != 0 && status_out_size != 0U) {
        status_out[0] = '\0';
    }
    if (feedback_out != 0 && feedback_out_size != 0U) {
        feedback_out[0] = '\0';
    }
    cursor += 7U;
    if (!parse_decimal_u32_prefix(cursor, &cursor, &cursor_index)) {
        return 0U;
    }
    if (*cursor != '\0') {
        if (!source_starts_with(cursor, ";TOP:")) {
            return 0U;
        }
        cursor += 5U;
        if (!parse_decimal_u32_prefix(cursor, &cursor, &top_line) || *cursor != '\0') {
            if (!source_starts_with(cursor, ";VIEW:")) {
                return 0U;
            }
        }
    }
    if (*cursor != '\0') {
        if (!source_starts_with(cursor, ";VIEW:")) {
            return 0U;
        }
        cursor += 6U;
        if (!parse_decimal_u32_prefix(cursor, &cursor, &saved_view_kind)) {
            return 0U;
        }
    }
    if (source_starts_with(cursor, ";TARGET:")) {
        cursor += 8U;
        if (!workspace_copy_input_monitor_state_text(&cursor, saved_target_out, saved_target_out_size)) {
            return 0U;
        }
    }
    if (source_starts_with(cursor, ";STATUS:")) {
        cursor += 8U;
        if (!workspace_copy_input_monitor_state_text(&cursor, status_out, status_out_size)) {
            return 0U;
        }
    }
    if (source_starts_with(cursor, ";FEEDBACK:")) {
        cursor += 10U;
        if (!workspace_copy_input_monitor_state_text(&cursor, feedback_out, feedback_out_size)) {
            return 0U;
        }
    }
    if (*cursor != '\0') {
        return 0U;
    }
    *cursor_index_out = cursor_index;
    *top_line_out = top_line;
    if (saved_view_kind_out != 0) {
        *saved_view_kind_out = saved_view_kind;
    }
    return 1U;
}

static void workspace_store_input_monitor_state_with_context_text_only(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line,
    uint32_t saved_view_kind,
    const char *saved_target_name
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    uint32_t offset = 0U;
    uint32_t length = text_length(workspace_input_monitor_buffer);
    char feedback_snapshot[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];

    if (cursor_index > length) {
        cursor_index = length;
    }
    workspace_input_monitor_cursor_state[0] = '\0';
    append_text_checked(
        workspace_input_monitor_cursor_state,
        sizeof(workspace_input_monitor_cursor_state),
        &offset,
        "CURSOR:"
    );
    render_small_integer((int32_t)cursor_index);
    append_text_checked(
        workspace_input_monitor_cursor_state,
        sizeof(workspace_input_monitor_cursor_state),
        &offset,
        print_buffer
    );
    append_text_checked(
        workspace_input_monitor_cursor_state,
        sizeof(workspace_input_monitor_cursor_state),
        &offset,
        ";TOP:"
    );
    render_small_integer((int32_t)top_line);
    append_text_checked(
        workspace_input_monitor_cursor_state,
        sizeof(workspace_input_monitor_cursor_state),
        &offset,
        print_buffer
    );
    append_text_checked(
        workspace_input_monitor_cursor_state,
        sizeof(workspace_input_monitor_cursor_state),
        &offset,
        ";VIEW:"
    );
    render_small_integer((int32_t)saved_view_kind);
    append_text_checked(
        workspace_input_monitor_cursor_state,
        sizeof(workspace_input_monitor_cursor_state),
        &offset,
        print_buffer
    );
    if (saved_target_name != 0 && saved_target_name[0] != '\0') {
        append_text_checked(
            workspace_input_monitor_cursor_state,
            sizeof(workspace_input_monitor_cursor_state),
            &offset,
            ";TARGET:"
        );
        workspace_append_input_monitor_state_text(
            workspace_input_monitor_cursor_state,
            sizeof(workspace_input_monitor_cursor_state),
            &offset,
            saved_target_name
        );
    }
    if (workspace_input_monitor_status[0] != '\0') {
        append_text_checked(
            workspace_input_monitor_cursor_state,
            sizeof(workspace_input_monitor_cursor_state),
            &offset,
            ";STATUS:"
        );
        workspace_append_input_monitor_state_text(
            workspace_input_monitor_cursor_state,
            sizeof(workspace_input_monitor_cursor_state),
            &offset,
            workspace_input_monitor_status
        );
    }
    if (workspace_input_monitor_feedback[0] != '\0') {
        workspace_input_monitor_feedback_snapshot_text(
            feedback_snapshot,
            sizeof(feedback_snapshot)
        );
        if (feedback_snapshot[0] != '\0') {
            append_text_checked(
                workspace_input_monitor_cursor_state,
                sizeof(workspace_input_monitor_cursor_state),
                &offset,
                ";FEEDBACK:"
            );
            workspace_append_input_monitor_state_text(
                workspace_input_monitor_cursor_state,
                sizeof(workspace_input_monitor_cursor_state),
                &offset,
                feedback_snapshot
            );
        }
    }
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        string_value(workspace_input_monitor_cursor_state)
    );
}

static void workspace_store_input_monitor_state_with_context(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line,
    uint32_t saved_view_kind,
    const char *saved_target_name
) {
    workspace_store_input_monitor_state_with_context_text_only(
        workspace_object,
        cursor_index,
        top_line,
        saved_view_kind,
        saved_target_name
    );
    workspace_sync_input_monitor_text_state(workspace_object, cursor_index, top_line);
}

static void workspace_store_input_monitor_state(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line
) {
    struct recorz_mvp_value target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t parsed_cursor_index = 0U;
    uint32_t parsed_top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;
    char saved_target_name[METHOD_SOURCE_CHUNK_LIMIT];
    char saved_status[WORKSPACE_INPUT_MONITOR_STATUS_LIMIT];
    char saved_feedback[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];

    saved_target_name[0] = '\0';
    saved_status[0] = '\0';
    saved_feedback[0] = '\0';
    workspace_parse_input_monitor_state(
        target_name_value.kind == RECORZ_MVP_VALUE_STRING ? target_name_value.string : 0,
        &parsed_cursor_index,
        &parsed_top_line,
        &saved_view_kind,
        saved_target_name,
        sizeof(saved_target_name),
        saved_status,
        sizeof(saved_status),
        saved_feedback,
        sizeof(saved_feedback)
    );
    workspace_store_input_monitor_state_with_context(
        workspace_object,
        cursor_index,
        top_line,
        saved_view_kind,
        saved_target_name[0] == '\0' ? 0 : saved_target_name
    );
}

static uint32_t workspace_input_monitor_cursor_index(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    struct recorz_mvp_value target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t source_length =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? text_length(source_value.string)
            : 0U;
    uint32_t cursor_index = source_length;
    uint32_t top_line = 0U;

    if (workspace_parse_input_monitor_state(
            target_name_value.kind == RECORZ_MVP_VALUE_STRING ? target_name_value.string : 0,
            &cursor_index,
            &top_line,
            0,
            0,
            0U,
            0,
            0U,
            0,
            0U) &&
        cursor_index <= source_length) {
        return cursor_index;
    }
    return source_length;
}

static uint32_t workspace_input_monitor_top_line(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t cursor_index = 0U;
    uint32_t top_line = 0U;

    if (workspace_parse_input_monitor_state(
            target_name_value.kind == RECORZ_MVP_VALUE_STRING ? target_name_value.string : 0,
            &cursor_index,
            &top_line,
            0,
            0,
            0U,
            0,
            0U,
            0,
            0U)) {
        return top_line;
    }
    return 0U;
}

static void workspace_input_monitor_cursor_line_and_column(
    const char *text,
    uint32_t cursor_index,
    uint32_t *line_out,
    uint32_t *column_out
) {
    uint32_t line = 0U;
    uint32_t column = 0U;
    uint32_t index = 0U;

    if (text == 0) {
        *line_out = 0U;
        *column_out = 0U;
        return;
    }
    while (text[index] != '\0' && index < cursor_index) {
        if (text[index] == '\n') {
            ++line;
            column = 0U;
        } else {
            ++column;
        }
        ++index;
    }
    *line_out = line;
    *column_out = column;
}

static struct recorz_mvp_heap_object *workspace_cursor_object(void) {
    uint16_t handle = global_handles[RECORZ_MVP_GLOBAL_WORKSPACE_CURSOR];

    if (handle == 0U || handle > heap_size) {
        machine_panic("workspace cursor object is unavailable");
    }
    return (struct recorz_mvp_heap_object *)heap_object(handle);
}

static struct recorz_mvp_heap_object *workspace_selection_object(void) {
    uint16_t handle = global_handles[RECORZ_MVP_GLOBAL_WORKSPACE_SELECTION];

    if (handle == 0U || handle > heap_size) {
        machine_panic("workspace selection object is unavailable");
    }
    return (struct recorz_mvp_heap_object *)heap_object(handle);
}

static uint32_t workspace_cursor_index_value(void) {
    struct recorz_mvp_heap_object *cursor_object = workspace_cursor_object();
    struct recorz_mvp_value value = heap_get_field(cursor_object, TEXT_CURSOR_FIELD_INDEX);

    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        return 0U;
    }
    return (uint32_t)value.integer;
}

static uint32_t workspace_cursor_line_value(void) {
    struct recorz_mvp_heap_object *cursor_object = workspace_cursor_object();
    struct recorz_mvp_value value = heap_get_field(cursor_object, TEXT_CURSOR_FIELD_LINE);

    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        return 0U;
    }
    return (uint32_t)value.integer;
}

static uint32_t workspace_cursor_column_value(void) {
    struct recorz_mvp_heap_object *cursor_object = workspace_cursor_object();
    struct recorz_mvp_value value = heap_get_field(cursor_object, TEXT_CURSOR_FIELD_COLUMN);

    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        return 0U;
    }
    return (uint32_t)value.integer;
}

static uint32_t workspace_cursor_cached_top_line_value(void) {
    struct recorz_mvp_heap_object *cursor_object = workspace_cursor_object();
    struct recorz_mvp_value value = heap_get_field(cursor_object, TEXT_CURSOR_FIELD_TOP_LINE);

    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        return 0U;
    }
    return (uint32_t)value.integer;
}

static uint32_t workspace_visible_origin_top_line_value(void) {
    struct recorz_mvp_heap_object *origin_object = workspace_visible_origin_object_or_null();
    struct recorz_mvp_value value;

    if (origin_object == 0) {
        return workspace_cursor_cached_top_line_value();
    }
    value = heap_get_field(origin_object, WORKSPACE_VISIBLE_ORIGIN_FIELD_TOP_LINE);
    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        return workspace_cursor_cached_top_line_value();
    }
    return (uint32_t)value.integer;
}

static uint32_t workspace_visible_origin_left_column_value(void) {
    struct recorz_mvp_heap_object *origin_object = workspace_visible_origin_object_or_null();
    struct recorz_mvp_value value;

    if (origin_object == 0) {
        return 0U;
    }
    value = heap_get_field(origin_object, WORKSPACE_VISIBLE_ORIGIN_FIELD_LEFT_COLUMN);
    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        return 0U;
    }
    return (uint32_t)value.integer;
}

static uint32_t workspace_cursor_top_line_value(void) {
    return workspace_visible_origin_top_line_value();
}

static void workspace_sync_workspace_cursor_index(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index
) {
    workspace_sync_input_monitor_text_state(
        workspace_object,
        cursor_index,
        workspace_visible_origin_top_line_value()
    );
}

static void workspace_capture_input_monitor_cursor_visual_state(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t *cursor_line_out,
    uint32_t *cursor_column_out,
    uint32_t *top_line_out
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);

    workspace_input_monitor_cursor_line_and_column(
        source_value.kind == RECORZ_MVP_VALUE_STRING ? source_value.string : 0,
        cursor_index,
        cursor_line_out,
        cursor_column_out
    );
    if (top_line_out != 0) {
        *top_line_out = workspace_input_monitor_top_line(workspace_object);
    }
}

static void workspace_sync_input_monitor_text_state(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index,
    uint32_t top_line
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t text_length_value = text_length(text);
    uint32_t line = 0U;
    uint32_t column = 0U;
    struct recorz_mvp_heap_object *cursor_object;

    if (cursor_index > text_length_value) {
        cursor_index = text_length_value;
    }
    workspace_input_monitor_cursor_line_and_column(text, cursor_index, &line, &column);
    cursor_object = workspace_cursor_object();
    heap_set_field(heap_handle_for_object(cursor_object), TEXT_CURSOR_FIELD_INDEX, small_integer_value((int32_t)cursor_index));
    heap_set_field(heap_handle_for_object(cursor_object), TEXT_CURSOR_FIELD_LINE, small_integer_value((int32_t)line));
    heap_set_field(heap_handle_for_object(cursor_object), TEXT_CURSOR_FIELD_COLUMN, small_integer_value((int32_t)column));
    heap_set_field(heap_handle_for_object(cursor_object), TEXT_CURSOR_FIELD_TOP_LINE, small_integer_value((int32_t)top_line));
}

static uint32_t workspace_input_monitor_line_start(const char *text, uint32_t cursor_index) {
    while (cursor_index != 0U && text[cursor_index - 1U] != '\n') {
        --cursor_index;
    }
    return cursor_index;
}

static uint32_t workspace_input_monitor_line_end(const char *text, uint32_t cursor_index) {
    while (text[cursor_index] != '\0' && text[cursor_index] != '\n') {
        ++cursor_index;
    }
    return cursor_index;
}

static void workspace_bind_input_monitor_cursor_state(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t cursor_index = 0U;
    uint32_t top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;
    char saved_target_name[METHOD_SOURCE_CHUNK_LIMIT];
    char status_text[WORKSPACE_INPUT_MONITOR_STATUS_LIMIT];
    char feedback_text[WORKSPACE_INPUT_MONITOR_FEEDBACK_LIMIT];

    saved_target_name[0] = '\0';
    status_text[0] = '\0';
    feedback_text[0] = '\0';
    if (!workspace_parse_input_monitor_state(
            target_name_value.kind == RECORZ_MVP_VALUE_STRING ? target_name_value.string : 0,
            &cursor_index,
            &top_line,
            &saved_view_kind,
            saved_target_name,
            sizeof(saved_target_name),
            status_text,
            sizeof(status_text),
            feedback_text,
            sizeof(feedback_text))) {
        if (heap_get_field(
                workspace_object,
                workspace_current_view_kind_field_index(workspace_object)
            ).kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
            (uint32_t)heap_get_field(
                workspace_object,
                workspace_current_view_kind_field_index(workspace_object)
            ).integer == WORKSPACE_VIEW_INPUT_MONITOR &&
            (saved_view_kind == WORKSPACE_VIEW_METHOD ||
             saved_view_kind == WORKSPACE_VIEW_CLASS_METHOD ||
             saved_view_kind == WORKSPACE_VIEW_CLASS_SOURCE ||
             saved_view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE)) {
            cursor_index = 0U;
        } else {
            cursor_index = text_length(workspace_input_monitor_buffer);
        }
        top_line = 0U;
        status_text[0] = '\0';
        feedback_text[0] = '\0';
    }
    if (status_text[0] == '\0') {
        if (saved_view_kind == WORKSPACE_VIEW_METHOD ||
            saved_view_kind == WORKSPACE_VIEW_CLASS_METHOD ||
            saved_view_kind == WORKSPACE_VIEW_CLASS_SOURCE ||
            saved_view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE) {
            workspace_input_monitor_set_status("SOURCE EDITOR READY");
        } else {
            workspace_input_monitor_set_status("WORKSPACE READY");
        }
    } else {
        workspace_input_monitor_set_status(status_text);
    }
    workspace_input_monitor_set_feedback_text(feedback_text);
    workspace_store_input_monitor_state(workspace_object, cursor_index, top_line);
}

static uint8_t workspace_input_monitor_accept_context(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t *view_kind_out,
    char target_name_out[],
    uint32_t target_name_out_size
) {
    struct recorz_mvp_value target_name_value = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t cursor_index = 0U;
    uint32_t top_line = 0U;
    uint32_t saved_view_kind = WORKSPACE_VIEW_NONE;

    if (target_name_out != 0 && target_name_out_size != 0U) {
        target_name_out[0] = '\0';
    }
    if (!workspace_parse_input_monitor_state(
            target_name_value.kind == RECORZ_MVP_VALUE_STRING ? target_name_value.string : 0,
            &cursor_index,
            &top_line,
            &saved_view_kind,
            target_name_out,
            target_name_out_size,
            0,
            0U,
            0,
            0U)) {
        return 0U;
    }
    if (saved_view_kind != WORKSPACE_VIEW_METHOD &&
        saved_view_kind != WORKSPACE_VIEW_CLASS_SOURCE &&
        saved_view_kind != WORKSPACE_VIEW_CLASS_METHOD &&
        saved_view_kind != WORKSPACE_VIEW_PACKAGE_SOURCE) {
        return 0U;
    }
    if (view_kind_out != 0) {
        *view_kind_out = saved_view_kind;
    }
    return 1U;
}

static void workspace_store_input_monitor_cursor_index(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t cursor_index
) {
    workspace_store_input_monitor_state(
        workspace_object,
        cursor_index,
        workspace_input_monitor_top_line(workspace_object)
    );
}

static void workspace_move_input_monitor_cursor_left(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);

    if (cursor_index != 0U) {
        workspace_store_input_monitor_cursor_index(workspace_object, cursor_index - 1U);
    }
}

static void workspace_move_input_monitor_cursor_right(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t length =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? text_length(source_value.string)
            : 0U;

    if (cursor_index < length) {
        workspace_store_input_monitor_cursor_index(workspace_object, cursor_index + 1U);
    }
}

static void workspace_move_input_monitor_cursor_up(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0) ? source_value.string : "";
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);
    uint32_t line_start = workspace_input_monitor_line_start(text, cursor_index);
    uint32_t column = cursor_index - line_start;
    uint32_t previous_line_end;
    uint32_t previous_line_start;
    uint32_t previous_line_length;

    if (line_start == 0U) {
        return;
    }
    previous_line_end = line_start - 1U;
    previous_line_start = workspace_input_monitor_line_start(text, previous_line_end);
    previous_line_length = previous_line_end - previous_line_start;
    workspace_store_input_monitor_cursor_index(
        workspace_object,
        previous_line_start + (column < previous_line_length ? column : previous_line_length)
    );
}

static void workspace_move_input_monitor_cursor_down(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0) ? source_value.string : "";
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);
    uint32_t line_start = workspace_input_monitor_line_start(text, cursor_index);
    uint32_t line_end = workspace_input_monitor_line_end(text, cursor_index);
    uint32_t column = cursor_index - line_start;
    uint32_t next_line_start;
    uint32_t next_line_end;
    uint32_t next_line_length;

    if (text[line_end] == '\0') {
        return;
    }
    next_line_start = line_end + 1U;
    next_line_end = workspace_input_monitor_line_end(text, next_line_start);
    next_line_length = next_line_end - next_line_start;
    workspace_store_input_monitor_cursor_index(
        workspace_object,
        next_line_start + (column < next_line_length ? column : next_line_length)
    );
}

static void workspace_move_input_monitor_cursor_to_line_start(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_store_input_monitor_cursor_index(
        workspace_object,
        workspace_input_monitor_line_start(
            workspace_input_monitor_buffer,
            workspace_input_monitor_cursor_index(workspace_object)
        )
    );
}

static void workspace_move_input_monitor_cursor_to_line_end(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_store_input_monitor_cursor_index(
        workspace_object,
        workspace_input_monitor_line_end(
            workspace_input_monitor_buffer,
            workspace_input_monitor_cursor_index(workspace_object)
        )
    );
}

static void workspace_move_cursor_left_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t cursor_index = workspace_cursor_index_value();

    if (cursor_index == 0U) {
        return;
    }
    workspace_sync_workspace_cursor_index(workspace_object, cursor_index - 1U);
}

static void workspace_move_cursor_right_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    uint32_t cursor_index = workspace_cursor_index_value();
    uint32_t length =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? text_length(source_value.string)
            : 0U;

    if (cursor_index >= length) {
        return;
    }
    workspace_sync_workspace_cursor_index(workspace_object, cursor_index + 1U);
}

static void workspace_move_cursor_to_line_start_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";

    workspace_sync_workspace_cursor_index(
        workspace_object,
        workspace_input_monitor_line_start(text, workspace_cursor_index_value())
    );
}

static void workspace_move_cursor_to_line_end_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";

    workspace_sync_workspace_cursor_index(
        workspace_object,
        workspace_input_monitor_line_end(text, workspace_cursor_index_value())
    );
}

static void workspace_move_cursor_up_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t cursor_index = workspace_cursor_index_value();
    uint32_t line_start = workspace_input_monitor_line_start(text, cursor_index);
    uint32_t column = cursor_index - line_start;
    uint32_t previous_line_end;
    uint32_t previous_line_start;
    uint32_t previous_line_length;

    if (line_start == 0U) {
        return;
    }
    previous_line_end = line_start - 1U;
    previous_line_start = workspace_input_monitor_line_start(text, previous_line_end);
    previous_line_length = previous_line_end - previous_line_start;
    workspace_sync_workspace_cursor_index(
        workspace_object,
        previous_line_start + (column < previous_line_length ? column : previous_line_length)
    );
}

static void workspace_move_cursor_down_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *text =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t cursor_index = workspace_cursor_index_value();
    uint32_t line_start = workspace_input_monitor_line_start(text, cursor_index);
    uint32_t line_end = workspace_input_monitor_line_end(text, cursor_index);
    uint32_t column = cursor_index - line_start;
    uint32_t next_line_start;
    uint32_t next_line_end;
    uint32_t next_line_length;

    if (text[line_end] == '\0') {
        return;
    }
    next_line_start = line_end + 1U;
    next_line_end = workspace_input_monitor_line_end(text, next_line_start);
    next_line_length = next_line_end - next_line_start;
    workspace_sync_workspace_cursor_index(
        workspace_object,
        next_line_start + (column < next_line_length ? column : next_line_length)
    );
}

static void workspace_insert_code_point_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object,
    uint8_t code_point
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *source =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t length = text_length(source);
    uint32_t cursor_index = workspace_cursor_index_value();
    uint32_t source_index;
    uint32_t write_index = 0U;

    if (length + 1U >= sizeof(workspace_edit_buffer)) {
        machine_panic("Workspace insertCodePoint exceeds editor buffer capacity");
    }
    if (cursor_index > length) {
        cursor_index = length;
    }
    if (source == workspace_editor_source_buffer) {
        uint32_t shift_index = length + 1U;

        if (length + 1U >= sizeof(workspace_editor_source_buffer)) {
            machine_panic("Workspace insertCodePoint exceeds editor source capacity");
        }
        while (shift_index > cursor_index) {
            workspace_editor_source_buffer[shift_index] =
                workspace_editor_source_buffer[shift_index - 1U];
            --shift_index;
        }
        workspace_editor_source_buffer[cursor_index] = (char)code_point;
        workspace_sync_workspace_cursor_index(workspace_object, cursor_index + 1U);
        return;
    }
    for (source_index = 0U; source_index < cursor_index; ++source_index) {
        workspace_edit_buffer[write_index++] = source[source_index];
    }
    workspace_edit_buffer[write_index++] = (char)code_point;
    for (source_index = cursor_index; source_index < length; ++source_index) {
        workspace_edit_buffer[write_index++] = source[source_index];
    }
    workspace_edit_buffer[write_index] = '\0';
    workspace_remember_current_source(workspace_object, runtime_string_allocate_copy(workspace_edit_buffer));
    workspace_sync_workspace_cursor_index(workspace_object, cursor_index + 1U);
}

static void workspace_delete_backward_in_current_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *source =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t length = text_length(source);
    uint32_t cursor_index = workspace_cursor_index_value();
    uint32_t source_index;
    uint32_t write_index = 0U;

    if (length == 0U || cursor_index == 0U) {
        return;
    }
    if (cursor_index > length) {
        cursor_index = length;
    }
    if (source == workspace_editor_source_buffer) {
        for (source_index = cursor_index - 1U; source_index < length; ++source_index) {
            workspace_editor_source_buffer[source_index] =
                workspace_editor_source_buffer[source_index + 1U];
        }
        workspace_sync_workspace_cursor_index(workspace_object, cursor_index - 1U);
        return;
    }
    for (source_index = 0U; source_index < length; ++source_index) {
        if (source_index + 1U == cursor_index) {
            continue;
        }
        workspace_edit_buffer[write_index++] = source[source_index];
    }
    workspace_edit_buffer[write_index] = '\0';
    workspace_remember_current_source(workspace_object, runtime_string_allocate_copy(workspace_edit_buffer));
    workspace_sync_workspace_cursor_index(workspace_object, cursor_index - 1U);
}

static void workspace_insert_input_monitor_character(
    const struct recorz_mvp_heap_object *workspace_object,
    char ch
) {
    uint32_t length = text_length(workspace_input_monitor_buffer);
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);
    uint32_t index;

    if (length + 1U >= sizeof(workspace_input_monitor_buffer)) {
        return;
    }
    if (cursor_index > length) {
        cursor_index = length;
    }
    for (index = length + 1U; index > cursor_index; --index) {
        workspace_input_monitor_buffer[index] = workspace_input_monitor_buffer[index - 1U];
    }
    workspace_input_monitor_buffer[cursor_index] = ch;
    workspace_store_input_monitor_cursor_index(workspace_object, cursor_index + 1U);
}

static void workspace_backspace_input_monitor_character(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t length = text_length(workspace_input_monitor_buffer);
    uint32_t cursor_index = workspace_input_monitor_cursor_index(workspace_object);
    uint32_t index;

    if (length == 0U || cursor_index == 0U) {
        return;
    }
    if (cursor_index > length) {
        cursor_index = length;
    }
    for (index = cursor_index - 1U; index < length; ++index) {
        workspace_input_monitor_buffer[index] = workspace_input_monitor_buffer[index + 1U];
    }
    workspace_store_input_monitor_cursor_index(workspace_object, cursor_index - 1U);
}

static void workspace_evaluate_input_monitor_buffer(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    struct recorz_mvp_value saved_view_kind = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_target_name = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    const char *chunk_source;
    uint32_t browser_view_kind = WORKSPACE_VIEW_NONE;
    char browser_target_name[METHOD_SOURCE_CHUNK_LIMIT];

    if (workspace_input_monitor_buffer[0] == '\0') {
        workspace_input_monitor_set_status("BUFFER EMPTY");
        return;
    }
    if (workspace_input_monitor_accept_context(
            workspace_object,
            &browser_view_kind,
            browser_target_name,
            sizeof(browser_target_name))) {
        workspace_input_monitor_set_status("CTRL-X INSTALLS SOURCE");
        return;
    }
    workspace_input_monitor_clear_feedback();
    workspace_input_monitor_set_status("DOIT RUNNING");
    workspace_input_monitor_capture_enabled = 1U;
    chunk_source = workspace_normalize_do_it_source(workspace_input_monitor_buffer);
    workspace_remember_source(workspace_object, chunk_source);
    workspace_evaluate_source(workspace_source_for_evaluation(chunk_source));
    workspace_input_monitor_capture_enabled = 0U;
    workspace_input_monitor_set_status("DOIT COMPLETE");
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        saved_view_kind
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        saved_target_name
    );
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        string_value(workspace_input_monitor_buffer)
    );
    workspace_remember_source(workspace_object, chunk_source);
}

static void workspace_print_input_monitor_buffer(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    struct recorz_mvp_value saved_view_kind = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_target_name = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    const char *chunk_source;
    struct recorz_mvp_value result;
    char rendered_value[METHOD_SOURCE_CHUNK_LIMIT];
    uint32_t browser_view_kind = WORKSPACE_VIEW_NONE;
    char browser_target_name[METHOD_SOURCE_CHUNK_LIMIT];

    if (workspace_input_monitor_buffer[0] == '\0') {
        workspace_input_monitor_set_status("BUFFER EMPTY");
        return;
    }
    if (workspace_input_monitor_accept_context(
            workspace_object,
            &browser_view_kind,
            browser_target_name,
            sizeof(browser_target_name))) {
        workspace_input_monitor_set_status("CTRL-X INSTALLS SOURCE");
        return;
    }
    workspace_input_monitor_clear_feedback();
    workspace_input_monitor_set_status("PRINT RUNNING");
    workspace_input_monitor_capture_enabled = 1U;
    chunk_source = workspace_normalize_do_it_source(workspace_input_monitor_buffer);
    workspace_remember_source(workspace_object, chunk_source);
    result = workspace_evaluate_source(workspace_source_for_evaluation(chunk_source));
    workspace_input_monitor_capture_enabled = 0U;
    workspace_input_monitor_feedback_append_line(
        workspace_text_for_value(result, rendered_value, sizeof(rendered_value))
    );
    workspace_input_monitor_set_status("PRINT COMPLETE");
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        saved_view_kind
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        saved_target_name
    );
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        string_value(workspace_input_monitor_buffer)
    );
    workspace_remember_source(workspace_object, chunk_source);
}

static uint32_t workspace_current_view_kind_value(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value view_kind_value = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );

    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || view_kind_value.integer < 0) {
        return WORKSPACE_VIEW_NONE;
    }
    return (uint32_t)view_kind_value.integer;
}

static uint8_t workspace_current_source_is_editor_target(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t view_kind = workspace_current_view_kind_value(workspace_object);

    return (uint8_t)(view_kind == WORKSPACE_VIEW_METHOD ||
                     view_kind == WORKSPACE_VIEW_CLASS_METHOD ||
                     view_kind == WORKSPACE_VIEW_CLASS_SOURCE ||
                     view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE);
}

static void workspace_tool_print_current_in_place(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *chunk_source;
    struct recorz_mvp_value result;
    char rendered_value[METHOD_SOURCE_CHUNK_LIMIT];

    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        workspace_input_monitor_set_status("BUFFER EMPTY");
        return;
    }
    if (workspace_current_source_is_editor_target(workspace_object)) {
        workspace_input_monitor_set_status("CTRL-X INSTALLS SOURCE");
        return;
    }
    workspace_input_monitor_clear_feedback();
    workspace_input_monitor_set_status("PRINT RUNNING");
    workspace_input_monitor_capture_enabled = 1U;
    chunk_source = workspace_normalize_do_it_source(source_value.string);
    workspace_remember_source(workspace_object, chunk_source);
    result = workspace_evaluate_source(workspace_source_for_evaluation(chunk_source));
    workspace_input_monitor_capture_enabled = 0U;
    workspace_input_monitor_feedback_append_line(
        workspace_text_for_value(result, rendered_value, sizeof(rendered_value))
    );
    workspace_input_monitor_set_status("PRINT COMPLETE");
}

static void workspace_accept_current_in_place(
    const struct recorz_mvp_heap_object *object
) {
    struct recorz_mvp_value source_value;
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const struct recorz_mvp_heap_object *class_object;
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_text[METHOD_SOURCE_NAME_LIMIT];

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
        machine_panic("Workspace acceptCurrent requires a browser target");
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        if (object->field_count <= workspace_current_target_name_field_index(object)) {
            machine_panic("Workspace acceptCurrent is missing the current package target");
        }
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace acceptCurrent is missing the current package target");
        }
        file_in_chunk_stream_source(source_value.string);
        workspace_remember_editor_current_source(
            object,
            file_out_package_source_by_name(target_name_value.string)
        );
        workspace_render_package_source_browser(object, target_name_value.string);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_SOURCE) {
        if (object->field_count <= workspace_current_target_name_field_index(object)) {
            machine_panic("Workspace acceptCurrent is missing the current class target");
        }
        target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace acceptCurrent is missing the current class target");
        }
        class_object = lookup_class_by_name(target_name_value.string);
        if (class_object == 0) {
            machine_panic("Workspace acceptCurrent could not resolve the current class");
        }
        file_in_class_source_on_existing_class(source_value.string, class_object);
        workspace_remember_editor_current_source(
            object,
            file_out_class_source_by_name(target_name_value.string)
        );
        workspace_render_class_source_browser(object, target_name_value.string);
        return;
    }
    if ((uint32_t)view_kind_value.integer != WORKSPACE_VIEW_METHOD &&
        (uint32_t)view_kind_value.integer != WORKSPACE_VIEW_CLASS_METHOD) {
        machine_panic("Workspace acceptCurrent requires a method, class, or package source browser target");
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
}

static uint8_t workspace_resolve_source_browser_target(
    uint32_t view_kind,
    const char *target_name,
    uint32_t *normalized_view_kind_out,
    char normalized_target_name_out[],
    uint32_t normalized_target_name_out_size,
    const char **source_out
) {
    const struct recorz_mvp_heap_object *class_object;
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char protocol_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_text[METHOD_SOURCE_NAME_LIMIT];

    if (normalized_view_kind_out != 0) {
        *normalized_view_kind_out = WORKSPACE_VIEW_NONE;
    }
    if (normalized_target_name_out != 0 && normalized_target_name_out_size != 0U) {
        normalized_target_name_out[0] = '\0';
    }
    if (source_out != 0) {
        *source_out = 0;
    }
    if (target_name == 0 || target_name[0] == '\0') {
        return 0U;
    }
    if (view_kind == WORKSPACE_VIEW_PACKAGE || view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        if (normalized_view_kind_out != 0) {
            *normalized_view_kind_out = WORKSPACE_VIEW_PACKAGE_SOURCE;
        }
        if (normalized_target_name_out != 0 && normalized_target_name_out_size != 0U) {
            source_copy_identifier(normalized_target_name_out, normalized_target_name_out_size, target_name);
        }
        if (source_out != 0) {
            *source_out = file_out_package_source_by_name(target_name);
        }
        return 1U;
    }
    if (view_kind == WORKSPACE_VIEW_CLASS ||
        view_kind == WORKSPACE_VIEW_CLASS_SOURCE ||
        view_kind == WORKSPACE_VIEW_METHODS ||
        view_kind == WORKSPACE_VIEW_CLASS_METHODS ||
        view_kind == WORKSPACE_VIEW_PROTOCOLS ||
        view_kind == WORKSPACE_VIEW_CLASS_PROTOCOLS) {
        if (normalized_view_kind_out != 0) {
            *normalized_view_kind_out = WORKSPACE_VIEW_CLASS_SOURCE;
        }
        if (normalized_target_name_out != 0 && normalized_target_name_out_size != 0U) {
            source_copy_identifier(normalized_target_name_out, normalized_target_name_out_size, target_name);
        }
        if (source_out != 0) {
            *source_out = file_out_class_source_by_name(target_name);
        }
        return 1U;
    }
    if (view_kind == WORKSPACE_VIEW_PROTOCOL || view_kind == WORKSPACE_VIEW_CLASS_PROTOCOL) {
        if (!workspace_parse_protocol_target_name(
                target_name,
                class_name,
                sizeof(class_name),
                protocol_name,
                sizeof(protocol_name))) {
            return 0U;
        }
        if (normalized_view_kind_out != 0) {
            *normalized_view_kind_out = WORKSPACE_VIEW_CLASS_SOURCE;
        }
        if (normalized_target_name_out != 0 && normalized_target_name_out_size != 0U) {
            source_copy_identifier(normalized_target_name_out, normalized_target_name_out_size, class_name);
        }
        if (source_out != 0) {
            *source_out = file_out_class_source_by_name(class_name);
        }
        return 1U;
    }
    if (view_kind != WORKSPACE_VIEW_METHOD && view_kind != WORKSPACE_VIEW_CLASS_METHOD) {
        return 0U;
    }
    if (!workspace_parse_method_target_name(
            target_name,
            class_name,
            sizeof(class_name),
            selector_name_text,
            sizeof(selector_name_text))) {
        return 0U;
    }
    class_object = lookup_class_by_name(class_name);
    if (class_object == 0) {
        return 0U;
    }
    if (normalized_view_kind_out != 0) {
        *normalized_view_kind_out = view_kind;
    }
    if (normalized_target_name_out != 0 && normalized_target_name_out_size != 0U) {
        source_copy_identifier(normalized_target_name_out, normalized_target_name_out_size, target_name);
    }
    if (source_out != 0) {
        *source_out = workspace_method_source_text_for_browser_target(
            view_kind == WORKSPACE_VIEW_METHOD ? class_object : class_side_lookup_target(class_object),
            class_name,
            selector_name_text,
            view_kind == WORKSPACE_VIEW_CLASS_METHOD
        );
    }
    return 1U;
}

static const char *workspace_source_text_for_browser_target(
    uint32_t view_kind,
    const char *target_name
) {
    const char *source = 0;

    if (!workspace_resolve_source_browser_target(
            view_kind,
            target_name,
            0,
            0,
            0U,
            &source)) {
        return 0;
    }
    return source;
}

static void workspace_render_source_browser_target(
    const struct recorz_mvp_heap_object *object,
    uint32_t view_kind,
    const char *target_name
) {
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_text[METHOD_SOURCE_NAME_LIMIT];

    if (view_kind == WORKSPACE_VIEW_CLASS_SOURCE) {
        workspace_render_class_source_browser(object, target_name);
        return;
    }
    if (view_kind == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        workspace_render_package_source_browser(object, target_name);
        return;
    }
    if (!workspace_parse_method_target_name(
            target_name,
            class_name,
            sizeof(class_name),
            selector_name_text,
            sizeof(selector_name_text))) {
        machine_panic("Workspace source browser target is invalid");
    }
    workspace_render_method_browser(
        object,
        class_name,
        selector_name_text,
        view_kind == WORKSPACE_VIEW_METHOD ? "INST" : "CLASS"
    );
}

static void workspace_revert_current_in_place(
    const struct recorz_mvp_heap_object *object
) {
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const char *source;
    uint32_t normalized_view_kind = WORKSPACE_VIEW_NONE;
    char normalized_target_name[METHOD_SOURCE_NAME_LIMIT];

    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Workspace revertCurrent requires a source browser target");
    }
    if (object->field_count <= workspace_current_target_name_field_index(object)) {
        machine_panic("Workspace revertCurrent is missing the current browser target");
    }
    target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
    if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        machine_panic("Workspace revertCurrent is missing the current browser target");
    }
    if (!workspace_resolve_source_browser_target(
            (uint32_t)view_kind_value.integer,
            target_name_value.string,
            &normalized_view_kind,
            normalized_target_name,
            sizeof(normalized_target_name),
            &source)) {
        machine_panic("Workspace revertCurrent requires a source browser target");
    }
    workspace_remember_editor_current_source(object, source);
    workspace_render_source_browser_target(object, normalized_view_kind, normalized_target_name);
}

static void workspace_run_current_tests_in_place(
    const struct recorz_mvp_heap_object *object
) {
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const struct recorz_mvp_heap_object *test_runner_object;
    const struct recorz_mvp_heap_object *class_object;
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_text[METHOD_SOURCE_NAME_LIMIT];
    char protocol_name[METHOD_SOURCE_NAME_LIMIT];

    if (global_handles[RECORZ_MVP_GLOBAL_TEST_RUNNER] == 0U) {
        machine_panic("Workspace runCurrentTests requires TestRunner");
    }
    test_runner_object = (const struct recorz_mvp_heap_object *)heap_object(
        global_handles[RECORZ_MVP_GLOBAL_TEST_RUNNER]
    );
    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Workspace runCurrentTests requires a browser target");
    }
    if (object->field_count <= workspace_current_target_name_field_index(object)) {
        machine_panic("Workspace runCurrentTests is missing the current browser target");
    }
    target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace runCurrentTests is missing the current package target");
        }
        test_runner_run_package(test_runner_object, target_name_value.string);
        return;
    }
    if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        machine_panic("Workspace runCurrentTests is missing the current class target");
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_SOURCE ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHODS ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHODS ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PROTOCOLS ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOLS) {
        class_object = lookup_class_by_name(target_name_value.string);
        if (class_object == 0) {
            machine_panic("Workspace runCurrentTests could not resolve the target class");
        }
        test_runner_run_class(test_runner_object, class_object, target_name_value.string);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_METHOD ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHOD) {
        if (!workspace_parse_method_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                selector_name_text,
                sizeof(selector_name_text))) {
            machine_panic("Workspace runCurrentTests current method target is invalid");
        }
        class_object = lookup_class_by_name(class_name);
        if (class_object == 0) {
            machine_panic("Workspace runCurrentTests could not resolve the target class");
        }
        test_runner_run_class(test_runner_object, class_object, class_name);
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PROTOCOL ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOL) {
        if (!workspace_parse_protocol_target_name(
                target_name_value.string,
                class_name,
                sizeof(class_name),
                protocol_name,
                sizeof(protocol_name))) {
            machine_panic("Workspace runCurrentTests current protocol target is invalid");
        }
        class_object = lookup_class_by_name(class_name);
        if (class_object == 0) {
            machine_panic("Workspace runCurrentTests could not resolve the target class");
        }
        test_runner_run_class(test_runner_object, class_object, class_name);
        return;
    }
    machine_panic("Workspace runCurrentTests requires a class or package browser target");
}

static void workspace_revert_input_monitor_buffer(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    uint32_t revert_view_kind = WORKSPACE_VIEW_NONE;
    char revert_target_name[METHOD_SOURCE_CHUNK_LIMIT];

    if (!workspace_input_monitor_accept_context(
            workspace_object,
            &revert_view_kind,
            revert_target_name,
            sizeof(revert_target_name))) {
        workspace_input_monitor_set_status("REVERT NEEDS SOURCE TARGET");
        return;
    }
    workspace_input_monitor_clear_feedback();
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)revert_view_kind)
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        revert_target_name[0] == '\0' ? nil_value() : string_value(revert_target_name)
    );
    workspace_revert_current_in_place(workspace_object);
    workspace_remember_input_monitor_view(workspace_object);
    workspace_bind_input_monitor_buffer(workspace_object);
    workspace_store_input_monitor_state_with_context(
        workspace_object,
        text_length(workspace_input_monitor_buffer),
        0U,
        revert_view_kind,
        revert_target_name[0] == '\0' ? 0 : revert_target_name
    );
    workspace_input_monitor_set_status("SOURCE RESTORED");
}

static void workspace_accept_input_monitor_buffer(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    struct recorz_mvp_value saved_view_kind = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_target_name = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t accept_view_kind = WORKSPACE_VIEW_NONE;
    char accept_target_name[METHOD_SOURCE_CHUNK_LIMIT];
    const char *prior_source;

    if (workspace_input_monitor_buffer[0] == '\0') {
        workspace_input_monitor_set_status("BUFFER EMPTY");
        return;
    }
    if (!workspace_input_monitor_accept_context(
            workspace_object,
            &accept_view_kind,
            accept_target_name,
            sizeof(accept_target_name))) {
        workspace_input_monitor_set_status("ACCEPT NEEDS SOURCE TARGET");
        return;
    }
    prior_source = workspace_source_text_for_browser_target(accept_view_kind, accept_target_name);
    if (prior_source != 0) {
        workspace_remember_source(workspace_object, prior_source);
    }
    workspace_input_monitor_clear_feedback();
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)accept_view_kind)
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        accept_target_name[0] == '\0' ? nil_value() : string_value(accept_target_name)
    );
    workspace_accept_current_in_place(workspace_object);
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        saved_view_kind
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        saved_target_name
    );
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        string_value(workspace_input_monitor_buffer)
    );
    workspace_input_monitor_set_status("INSTALL COMPLETE");
}

static void workspace_save_and_reopen_in_place(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t view_kind = workspace_current_view_kind_value(workspace_object);

    startup_hook_receiver_handle = heap_handle_for_object(workspace_object);
    startup_hook_selector_id =
        view_kind == WORKSPACE_VIEW_PACKAGES
            ? RECORZ_MVP_SELECTOR_BROWSE_PACKAGES_INTERACTIVE
            : RECORZ_MVP_SELECTOR_INTERACTIVE_INPUT_MONITOR;
    emit_live_snapshot();
}

static void workspace_save_recovery_snapshot_in_place(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint32_t view_kind = workspace_current_view_kind_value(workspace_object);

    startup_hook_receiver_handle = heap_handle_for_object(workspace_object);
    startup_hook_selector_id =
        view_kind == WORKSPACE_VIEW_PACKAGES
            ? RECORZ_MVP_SELECTOR_BROWSE_PACKAGES_INTERACTIVE
            : RECORZ_MVP_SELECTOR_INTERACTIVE_INPUT_MONITOR;
    emit_live_snapshot();
}

static void workspace_run_input_monitor_tests(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    struct recorz_mvp_value saved_view_kind = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_target_name = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    uint32_t test_view_kind = WORKSPACE_VIEW_NONE;
    char test_target_name[METHOD_SOURCE_CHUNK_LIMIT];

    if (!workspace_input_monitor_accept_context(
            workspace_object,
            &test_view_kind,
            test_target_name,
            sizeof(test_target_name))) {
        workspace_input_monitor_set_status("TESTS NEED TARGET");
        return;
    }
    workspace_input_monitor_clear_feedback();
    workspace_input_monitor_set_status("TESTS RUNNING");
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)test_view_kind)
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        test_target_name[0] == '\0' ? nil_value() : string_value(test_target_name)
    );
    workspace_input_monitor_capture_enabled = 1U;
    workspace_run_current_tests_in_place(workspace_object);
    workspace_input_monitor_capture_enabled = 0U;
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        saved_view_kind
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        saved_target_name
    );
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        string_value(workspace_input_monitor_buffer)
    );
    workspace_input_monitor_set_status("TESTS COMPLETE");
}

static void workspace_emit_regenerated_source_from_input_monitor(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_input_monitor_set_status("REGEN RUNNING");
    emit_regenerated_kernel_source();
    emit_regenerated_boot_source(workspace_object);
    workspace_input_monitor_set_status("REGEN COMPLETE");
}

static uint8_t workspace_browse_input_monitor_context(
    const struct recorz_mvp_heap_object *workspace_object
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    uint32_t browse_view_kind = WORKSPACE_VIEW_NONE;
    char browse_target_name[METHOD_SOURCE_CHUNK_LIMIT];

    browse_target_name[0] = '\0';
    if (!workspace_input_monitor_accept_context(
            workspace_object,
            &browse_view_kind,
            browse_target_name,
            sizeof(browse_target_name))) {
        return 0U;
    }
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)browse_view_kind)
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        browse_target_name[0] == '\0' ? nil_value() : string_value(browse_target_name)
    );
    workspace_reopen_in_place(workspace_object);
    return 1U;
}

static void workspace_view_regenerated_source_from_input_monitor(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t regenerated_view_kind,
    const char *source_name
) {
    uint16_t workspace_handle = heap_handle_for_object(workspace_object);
    struct recorz_mvp_heap_object *selection_object = workspace_selection_object();
    uint16_t selection_handle = heap_handle_for_object(selection_object);
    struct recorz_mvp_value saved_view_kind = heap_get_field(
        workspace_object,
        workspace_current_view_kind_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_target_name = heap_get_field(
        workspace_object,
        workspace_current_target_name_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_source = heap_get_field(
        workspace_object,
        workspace_current_source_field_index(workspace_object)
    );
    struct recorz_mvp_value saved_selection_start_line = heap_get_field(
        selection_object,
        TEXT_SELECTION_FIELD_START_LINE
    );
    struct recorz_mvp_value saved_selection_start_column = heap_get_field(
        selection_object,
        TEXT_SELECTION_FIELD_START_COLUMN
    );
    struct recorz_mvp_value saved_selection_end_line = heap_get_field(
        selection_object,
        TEXT_SELECTION_FIELD_END_LINE
    );
    struct recorz_mvp_value saved_selection_end_column = heap_get_field(
        selection_object,
        TEXT_SELECTION_FIELD_END_COLUMN
    );

    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        small_integer_value((int32_t)regenerated_view_kind)
    );
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        nil_value()
    );
    workspace_render_regenerated_source_browser(workspace_object, source_name);
    while (1) {
        char ch = machine_wait_getc();

        if (ch == 0x0f || ch == 0x04) {
            break;
        }
    }
    heap_set_field(
        workspace_handle,
        workspace_current_view_kind_field_index(workspace_object),
        saved_view_kind
    );
    heap_set_field(
        workspace_handle,
        workspace_current_target_name_field_index(workspace_object),
        saved_target_name
    );
    heap_set_field(
        workspace_handle,
        workspace_current_source_field_index(workspace_object),
        saved_source
    );
    workspace_bind_input_monitor_buffer(workspace_object);
    workspace_bind_input_monitor_cursor_state(workspace_object);
    heap_set_field(
        selection_handle,
        TEXT_SELECTION_FIELD_START_LINE,
        saved_selection_start_line
    );
    heap_set_field(
        selection_handle,
        TEXT_SELECTION_FIELD_START_COLUMN,
        saved_selection_start_column
    );
    heap_set_field(
        selection_handle,
        TEXT_SELECTION_FIELD_END_LINE,
        saved_selection_end_line
    );
    heap_set_field(
        selection_handle,
        TEXT_SELECTION_FIELD_END_COLUMN,
        saved_selection_end_column
    );
    workspace_render_input_monitor_browser(workspace_object);
}

static uint8_t workspace_view_router_redraw_from_image(void) {
    uint16_t object_handle = named_object_handle_for_name("BootViewRouter");
    struct recorz_mvp_value arguments[1];

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = object_value(heap_handle_for_object(default_form_object()));
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_REDRAW_ON_FORM,
        1U,
        arguments,
        0
    );
    return 1U;
}

static uint8_t workspace_view_router_handle_byte_from_image(char ch) {
    uint16_t object_handle = named_object_handle_for_name("BootViewRouter");
    struct recorz_mvp_value arguments[2];
    struct recorz_mvp_value result;

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = small_integer_value((int32_t)(uint8_t)ch);
    arguments[1] = object_value(heap_handle_for_object(default_form_object()));
    result = perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_HANDLE_BYTE_ON_FORM,
        2U,
        arguments,
        0
    );
    return (uint8_t)(small_integer_u32(
        result,
        "BootViewRouter handleByte:onForm: did not return a small integer"
    ) != 0U);
}

static uint8_t workspace_session_open_from_image(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t mode
) {
    uint16_t object_handle = named_object_handle_for_name("BootWorkspaceSession");
    struct recorz_mvp_value arguments[2];
    struct recorz_mvp_value result;

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = object_value(heap_handle_for_object(workspace_object));
    arguments[1] = small_integer_value((int32_t)mode);
    result = perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_OPEN_ON_MODE,
        2U,
        arguments,
        0
    );
    return (uint8_t)small_integer_u32(
        result,
        "BootWorkspaceSession openOn:mode: did not return a small integer"
    );
}

static uint8_t workspace_session_redraw_from_image(void) {
    uint16_t object_handle = named_object_handle_for_name("BootWorkspaceSession");
    struct recorz_mvp_value arguments[1];
    const struct recorz_mvp_heap_object *workspace_object = workspace_global_object();

    if (object_handle == 0U) {
        return 0U;
    }
    if (workspace_object != 0 &&
        workspace_current_view_kind_value(workspace_object) == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        return workspace_redraw_image_session_package_source_editor(workspace_object);
    }
    arguments[0] = object_value(heap_handle_for_object(default_form_object()));
    (void)perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_REDRAW_ON_FORM,
        1U,
        arguments,
        0
    );
    return 1U;
}

static uint8_t workspace_overlay_image_session_editor_cursor_move(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t old_cursor_line,
    uint32_t old_cursor_column,
    uint32_t old_top_line,
    uint32_t old_left_column
) {
    const struct recorz_mvp_heap_object *form = default_form_object();
    struct recorz_mvp_value source_value = workspace_current_source_value(workspace_object);
    const char *source =
        (source_value.kind == RECORZ_MVP_VALUE_STRING && source_value.string != 0)
            ? source_value.string
            : "";
    uint32_t top_line = workspace_visible_origin_top_line_value();
    uint32_t left_column = workspace_visible_origin_left_column_value();
    uint32_t cursor_line = workspace_cursor_line_value();
    uint32_t cursor_column = workspace_cursor_column_value();
    uint32_t visible_lines = workspace_surface_visible_line_capacity_for_view_height(WORKSPACE_SOURCE_VIEW_HEIGHT);
    uint32_t visible_columns = workspace_surface_visible_column_capacity_for_view_width(WORKSPACE_SOURCE_VIEW_WIDTH);
    uint32_t old_relative_column;
    uint32_t relative_column;

    if (top_line != old_top_line || left_column != old_left_column) {
        return workspace_scroll_copy_image_session_editor_viewport(
            workspace_object,
            old_cursor_line,
            old_cursor_column,
            old_top_line,
            old_left_column
        );
    }
    if (visible_lines == 0U ||
        visible_columns == 0U) {
        return 0U;
    }
    if (old_cursor_column < old_left_column || cursor_column < left_column) {
        return 0U;
    }
    old_relative_column = old_cursor_column - old_left_column;
    relative_column = cursor_column - left_column;
    if (old_cursor_line < top_line ||
        old_cursor_line >= top_line + visible_lines ||
        cursor_line < top_line ||
        cursor_line >= top_line + visible_lines ||
        old_relative_column >= visible_columns ||
        relative_column >= visible_columns) {
        return 0U;
    }
    workspace_surface_copy_source_viewport(
        workspace_surface_editor_buffer,
        sizeof(workspace_surface_editor_buffer),
        source,
        top_line,
        left_column,
        visible_lines,
        visible_columns
    );
    workspace_redraw_editor_source_cell(
        form,
        workspace_surface_editor_buffer,
        old_cursor_line - top_line,
        old_relative_column
    );
    workspace_draw_editor_cursor_overlay(
        form,
        cursor_line - top_line,
        relative_column
    );
    ++render_counter_editor_cursor_redraws;
    return 1U;
}

static uint8_t workspace_session_handle_byte_from_image(char ch) {
    uint16_t object_handle = named_object_handle_for_name("BootWorkspaceSession");
    struct recorz_mvp_value arguments[2];
    struct recorz_mvp_value result;

    if (object_handle == 0U) {
        return 0U;
    }
    arguments[0] = small_integer_value((int32_t)(uint8_t)ch);
    arguments[1] = object_value(heap_handle_for_object(default_form_object()));
    result = perform_send_and_pop_result(
        object_value(object_handle),
        RECORZ_MVP_SELECTOR_HANDLE_BYTE_ON_FORM,
        2U,
        arguments,
        0
    );
    return (uint8_t)small_integer_u32(
        result,
        "BootWorkspaceSession handleByte:onForm: did not return a small integer"
    );
}

static void workspace_count_session_render_code(uint8_t render_code) {
    if (render_code == 1U) {
        ++render_counter_editor_full_redraws;
        return;
    }
    if (render_code == 2U) {
        ++render_counter_editor_pane_redraws;
        return;
    }
    if (render_code == 4U) {
        ++render_counter_browser_full_redraws;
        return;
    }
    if (render_code == 5U) {
        ++render_counter_browser_list_redraws;
    }
}

static void workspace_present_image_session(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t mode
) {
    uint8_t render_code;

    render_counters_reset();
    workspace_prepare_editor_current_source_for_session(workspace_object);
    render_code = workspace_session_open_from_image(workspace_object, mode);
    if (!workspace_session_redraw_from_image()) {
        machine_panic("Workspace image session requires BootWorkspaceSession");
    }
    workspace_count_session_render_code(render_code);
}

static void workspace_run_interactive_image_session(
    const struct recorz_mvp_heap_object *workspace_object,
    uint32_t mode
) {
    uint8_t saw_carriage_return = 0U;
    uint8_t render_code;

    render_counters_reset();
    workspace_prepare_editor_current_source_for_session(workspace_object);
    render_code = workspace_session_open_from_image(workspace_object, mode);
    if (!workspace_session_redraw_from_image()) {
        machine_panic("Workspace interactive session requires BootWorkspaceSession");
    }
    workspace_count_session_render_code(render_code);
    while (1) {
        char ch = machine_wait_getc();
        uint32_t old_cursor_line = 0U;
        uint32_t old_cursor_column = 0U;
        uint32_t old_top_line = 0U;
        uint32_t old_left_column = 0U;
        uint32_t old_browser_selected_index = 0U;
        uint32_t old_browser_list_top_line = 0U;

        if (ch == '\r') {
            saw_carriage_return = 1U;
        } else if (ch == '\n' && saw_carriage_return) {
            saw_carriage_return = 0U;
            continue;
        } else {
            saw_carriage_return = 0U;
        }
        if ((uint8_t)ch == DEBUG_DUMP_RENDER_COUNTERS_BYTE) {
            render_counters_dump();
            continue;
        }
        if (workspace_current_view_kind_value(workspace_object) != WORKSPACE_VIEW_PACKAGES) {
            old_cursor_line = workspace_cursor_line_value();
            old_cursor_column = workspace_cursor_column_value();
            old_top_line = workspace_cursor_top_line_value();
            old_left_column = workspace_visible_origin_left_column_value();
        } else {
            old_browser_selected_index = workspace_session_selected_index();
            old_browser_list_top_line = workspace_session_list_top_line();
        }
        render_code = workspace_session_handle_byte_from_image(ch);
        if (render_code == 9U) {
            break;
        }
        if (render_code == 0U) {
            continue;
        }
        if (render_code == 6U &&
            workspace_current_view_kind_value(workspace_object) != WORKSPACE_VIEW_PACKAGES &&
            workspace_overlay_image_session_editor_cursor_move(
                workspace_object,
                old_cursor_line,
                old_cursor_column,
                old_top_line,
                old_left_column)) {
            continue;
        }
        if (render_code == 5U &&
            workspace_current_view_kind_value(workspace_object) == WORKSPACE_VIEW_PACKAGES &&
            workspace_scroll_copy_image_session_browser_list(
                old_browser_selected_index,
                old_browser_list_top_line)) {
            continue;
        }
        if (render_code == 6U) {
            render_code = 2U;
        }
        if (render_code == 1U &&
            workspace_current_view_kind_value(workspace_object) != WORKSPACE_VIEW_PACKAGES &&
            workspace_input_byte_is_status_only_command((uint8_t)ch)) {
            render_code = 7U;
        }
        if (render_code == 7U) {
            if (workspace_redraw_image_session_status_only()) {
                continue;
            }
            render_code = 1U;
        }
        if (render_code == 1U || render_code == 4U) {
            machine_discard_pending_input();
        }
        workspace_count_session_render_code(render_code);
        workspace_prepare_editor_current_source_for_session(workspace_object);
        if (!workspace_session_redraw_from_image()) {
            machine_panic("Workspace interactive session requires BootWorkspaceSession");
        }
    }
}

static void workspace_run_interactive_input_monitor(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_run_interactive_image_session(workspace_object, 1U);
}

static void workspace_run_interactive_views(
    const struct recorz_mvp_heap_object *workspace_object
) {
    (void)workspace_object;
    if (!workspace_view_router_redraw_from_image()) {
        machine_panic("Workspace interactive views requires BootViewRouter");
    }
    while (1) {
        char ch = machine_wait_getc();

        if (workspace_view_router_handle_byte_from_image(ch)) {
            continue;
        }
        if (ch == 0x04 || ch == 0x0f) {
            break;
        }
    }
}

static void workspace_edit_package_in_place(
    const struct recorz_mvp_heap_object *workspace_object,
    const char *package_name
) {
    const char *source;

    if (package_name == 0 || package_name[0] == '\0') {
        machine_panic("Workspace package editor is missing the package name");
    }
    if (package_definition_for_name(package_name) == 0) {
        machine_panic("Workspace package editor could not resolve the package");
    }
    workspace_capture_plain_return_state_if_needed(workspace_object);
    source = file_out_package_source_by_name(package_name);
    workspace_remember_editor_current_source(workspace_object, source);
    workspace_remember_source(workspace_object, source);
    workspace_remember_view(workspace_object, WORKSPACE_VIEW_PACKAGE_SOURCE, package_name);
    workspace_run_interactive_input_monitor(workspace_object);
}

static void workspace_run_interactive_package_list_browser(
    const struct recorz_mvp_heap_object *workspace_object
) {
    workspace_run_interactive_image_session(workspace_object, 0U);
}

static void workspace_edit_current_in_place(
    const struct recorz_mvp_heap_object *object
) {
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const char *source;
    uint32_t normalized_view_kind = WORKSPACE_VIEW_NONE;
    char normalized_target_name[METHOD_SOURCE_NAME_LIMIT];

    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Workspace editCurrent requires a browser target");
    }
    if (object->field_count <= workspace_current_target_name_field_index(object)) {
        machine_panic("Workspace editCurrent is missing the current browser target");
    }
    target_name_value = heap_get_field(object, workspace_current_target_name_field_index(object));
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE ||
        (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
            target_name_value.string == 0 ||
            target_name_value.string[0] == '\0') {
            machine_panic("Workspace editCurrent is missing the current package target");
        }
        source = file_out_package_source_by_name(target_name_value.string);
        workspace_remember_editor_current_source(object, source);
        workspace_remember_source(object, source);
        workspace_remember_view(object, WORKSPACE_VIEW_PACKAGE_SOURCE, target_name_value.string);
        workspace_run_interactive_input_monitor(object);
        return;
    }
    if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        machine_panic("Workspace editCurrent is missing the current browser target");
    }
    if (!workspace_resolve_source_browser_target(
            (uint32_t)view_kind_value.integer,
            target_name_value.string,
            &normalized_view_kind,
            normalized_target_name,
            sizeof(normalized_target_name),
            &source)) {
        machine_panic("Workspace editCurrent requires a class, method, protocol, or package browser target");
    }
    workspace_remember_editor_current_source(object, source);
    workspace_remember_source(object, source);
    workspace_remember_view(object, normalized_view_kind, normalized_target_name);
    workspace_run_interactive_input_monitor(object);
}

static void workspace_render_dynamic_object_fields(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
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
        workspace_render_dynamic_object_fields(buffer, buffer_size, offset, superclass_object, object, depth + 1U);
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
        workspace_surface_append_label_text(
            buffer,
            buffer_size,
            offset,
            dynamic_definition->instance_variable_names[field_index],
            workspace_text_for_value(
                heap_get_field(object, resolved_field_index),
                workspace_target_buffer,
                sizeof(workspace_target_buffer)
            )
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
    uint32_t list_offset = 0U;
    uint32_t source_offset = 0U;

    (void)workspace_object;
    workspace_surface_reset_buffer(workspace_surface_list_buffer, sizeof(workspace_surface_list_buffer));
    workspace_surface_reset_buffer(workspace_surface_source_buffer, sizeof(workspace_surface_source_buffer));
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "OBJECT",
        object_name
    );
    workspace_surface_append_label_text(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "CLASS",
        class_name_for_object(class_object)
    );
    workspace_surface_append_label_integer(
        workspace_surface_list_buffer,
        sizeof(workspace_surface_list_buffer),
        &list_offset,
        "SLOTS",
        object->field_count
    );
    workspace_render_dynamic_object_fields(
        workspace_surface_source_buffer,
        sizeof(workspace_surface_source_buffer),
        &source_offset,
        class_object,
        object,
        0U
    );
    if (source_offset == 0U) {
        workspace_surface_append_line(
            workspace_surface_source_buffer,
            sizeof(workspace_surface_source_buffer),
            &source_offset,
            "NO DYNAMIC FIELDS"
        );
    }
    workspace_require_browser_surface(
        form,
        workspace_surface_list_buffer,
        workspace_surface_source_buffer,
        object_name,
        "OBJECT BROWSER"
    );
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
        uint16_t operand_a;
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
                    operand_a > RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT) {
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
        const struct recorz_mvp_heap_object *class_object;
        const struct recorz_mvp_heap_object *metaclass_object;

        if (object->kind == 0U) {
            continue;
        }
        class_object = class_object_for_heap_object(object);
        metaclass_object = class_object_for_heap_object(class_object);

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

        if (object->kind == 0U || object->kind > MAX_OBJECT_KIND || object->class_handle == 0U) {
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
    uint16_t handle_index;

    heap_size = 0U;
    heap_live_count = 0U;
    heap_high_water_mark = 0U;
    mono_bitmap_count = 0U;
    next_dynamic_method_entry_execution_id = RECORZ_MVP_METHOD_ENTRY_COUNT;
    dynamic_class_count = 0U;
    package_count = 0U;
    named_object_count = 0U;
    live_method_source_count = 0U;
    live_package_do_it_source_count = 0U;
    default_form_handle = 0U;
    active_display_form_handle = 0U;
    active_cursor_handle = 0U;
    active_cursor_visible = 0U;
    active_cursor_screen_x = 0U;
    active_cursor_screen_y = 0U;
    framebuffer_bitmap_handle = 0U;
    glyph_fallback_handle = 0U;
    transcript_layout_handle = 0U;
    transcript_style_handle = 0U;
    transcript_metrics_handle = 0U;
    transcript_behavior_handle = 0U;
    transcript_font_handle = 0U;
    startup_hook_receiver_handle = 0U;
    startup_hook_selector_id = 0U;
    cursor_x = 0U;
    cursor_y = 0U;
    live_method_source_pool_used = 0U;
    live_method_source_pool[0] = '\0';
    live_package_do_it_source_pool_used = 0U;
    live_package_do_it_source_pool[0] = '\0';
    runtime_string_pool_offset = 0U;
    runtime_string_pool[0] = '\0';
    snapshot_string_pool[0] = '\0';
    booted_from_snapshot = 0U;
    gc_collection_count = 0U;
    gc_last_reclaimed_count = 0U;
    gc_total_reclaimed_count = 0U;
    gc_stack_base_address = 0U;
    gc_temp_root_count = 0U;
    gc_bootstrap_file_in_active = 0U;
    for (global_index = 0U; global_index <= MAX_GLOBAL_ID; ++global_index) {
        global_handles[global_index] = 0U;
    }
    for (handle_index = 0U; handle_index < HEAP_LIMIT; ++handle_index) {
        heap[handle_index].kind = 0U;
        heap[handle_index].field_count = 0U;
        heap[handle_index].class_handle = 0U;
    }
    for (handle_index = 0U; handle_index < sizeof(gc_mark_bits); ++handle_index) {
        gc_mark_bits[handle_index] = 0U;
    }
    for (handle_index = 0U; handle_index < GC_TEMP_ROOT_LIMIT; ++handle_index) {
        gc_temp_roots[handle_index] = 0U;
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
    for (named_index = 0U; named_index < LIVE_PACKAGE_DO_IT_SOURCE_LIMIT; ++named_index) {
        live_package_do_it_sources[named_index].package_name[0] = '\0';
        live_package_do_it_sources[named_index].source_offset = 0U;
        live_package_do_it_sources[named_index].source_length = 0U;
    }
    for (code_index = 0U; code_index < 128U; ++code_index) {
        glyph_bitmap_handles[code_index] = 0U;
    }
}

static uint32_t small_integer_u32(struct recorz_mvp_value value, const char *message) {
    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER || value.integer < 0) {
        machine_panic(message);
    }
    return (uint32_t)value.integer;
}

static int32_t small_integer_i32(struct recorz_mvp_value value, const char *message) {
    if (value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic(message);
    }
    return value.integer;
}

static const struct recorz_mvp_heap_object *transcript_layout_object(void) {
    const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(transcript_layout_handle);

    if (object->kind != RECORZ_MVP_OBJECT_TEXT_LAYOUT) {
        machine_panic("transcript layout root is not a text layout");
    }
    return object;
}

static const struct recorz_mvp_heap_object *transcript_layout_reference_object(
    uint8_t field_index,
    uint8_t expected_kind,
    const char *message
) {
    struct recorz_mvp_value reference_value = heap_get_field(transcript_layout_object(), field_index);
    const struct recorz_mvp_heap_object *reference_object;

    if (reference_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic(message);
    }
    reference_object = heap_object_for_value(reference_value);
    if (reference_object->kind != expected_kind) {
        machine_panic(message);
    }
    return reference_object;
}

static uint32_t transcript_layout_u32(uint8_t field_index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_layout_object(), field_index), message);
}

static const struct recorz_mvp_heap_object *transcript_margins_object(void) {
    return transcript_layout_reference_object(
        TEXT_LAYOUT_FIELD_MARGINS,
        RECORZ_MVP_OBJECT_TEXT_MARGINS,
        "text layout margins is not a text margins object"
    );
}

static uint32_t transcript_margins_u32(uint8_t field_index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_margins_object(), field_index), message);
}

static const struct recorz_mvp_heap_object *transcript_flow_object(void) {
    return transcript_layout_reference_object(
        TEXT_LAYOUT_FIELD_FLOW,
        RECORZ_MVP_OBJECT_TEXT_FLOW,
        "text layout flow is not a text flow object"
    );
}

static uint32_t transcript_flow_u32(uint8_t field_index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_flow_object(), field_index), message);
}

static uint32_t image_text_policy_u32_or_fallback(
    const char *object_name,
    uint16_t selector_id,
    uint16_t argument_count,
    const struct recorz_mvp_value arguments[],
    uint32_t fallback,
    const char *message
) {
    uint16_t object_handle = named_object_handle_for_name(object_name);
    struct recorz_mvp_value result;

    if (object_handle == 0U) {
        return fallback;
    }
    result = perform_send_and_pop_result(
        object_value(object_handle),
        selector_id,
        argument_count,
        arguments,
        0
    );
    return small_integer_u32(result, message);
}

static uint32_t fallback_text_left_margin(void) {
    return transcript_margins_u32(TEXT_MARGINS_FIELD_LEFT, "text margins left is not a small integer");
}

static uint32_t text_left_margin(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextLeftMargin",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_left_margin(),
        "BootTextLeftMargin did not return a small integer"
    );
}

static uint32_t fallback_text_top_margin(void) {
    return transcript_margins_u32(TEXT_MARGINS_FIELD_TOP, "text margins top is not a small integer");
}

static uint32_t text_top_margin(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextTopMargin",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_top_margin(),
        "BootTextTopMargin did not return a small integer"
    );
}

static uint32_t fallback_text_right_margin(void) {
    return transcript_margins_u32(TEXT_MARGINS_FIELD_RIGHT, "text margins right is not a small integer");
}

static uint32_t text_right_margin(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextRightMargin",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_right_margin(),
        "BootTextRightMargin did not return a small integer"
    );
}

static uint32_t fallback_text_bottom_margin(void) {
    return transcript_margins_u32(TEXT_MARGINS_FIELD_BOTTOM, "text margins bottom is not a small integer");
}

static uint32_t text_bottom_margin(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextBottomMargin",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_bottom_margin(),
        "BootTextBottomMargin did not return a small integer"
    );
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

static uint32_t fallback_text_wrap_limit_x_for_form_width(uint32_t form_width) {
    uint32_t left_margin = fallback_text_left_margin();
    uint32_t right_margin = fallback_text_right_margin();
    uint32_t wrap_limit_x = 0U;
    uint32_t wrap_width = transcript_flow_u32(TEXT_FLOW_FIELD_WRAP_WIDTH, "text flow wrapWidth is not a small integer");
    uint32_t line_break_mode = transcript_flow_u32(
        TEXT_FLOW_FIELD_LINE_BREAK_MODE,
        "text flow lineBreakMode is not a small integer"
    );

    if (line_break_mode == 0U) {
        return 0U;
    }
    if (form_width > right_margin) {
        wrap_limit_x = form_width - right_margin;
    }
    if (wrap_width != 0U) {
        uint32_t configured_limit_x = left_margin + (wrap_width * char_width());

        if (wrap_limit_x == 0U || configured_limit_x < wrap_limit_x) {
            wrap_limit_x = configured_limit_x;
        }
    }
    return wrap_limit_x;
}

static uint32_t text_wrap_limit_x_for_form_width(uint32_t form_width) {
    struct recorz_mvp_value arguments[1];

    arguments[0] = small_integer_value((int32_t)form_width);
    return image_text_policy_u32_or_fallback(
        "BootTextWrapLimit",
        RECORZ_MVP_SELECTOR_VALUE_ARG,
        1U,
        arguments,
        fallback_text_wrap_limit_x_for_form_width(form_width),
        "BootTextWrapLimit did not return a small integer"
    );
}

static uint32_t fallback_text_next_tab_x(uint32_t current_x) {
    uint32_t tab_width = transcript_flow_u32(TEXT_FLOW_FIELD_TAB_WIDTH, "text flow tabWidth is not a small integer");
    uint32_t left_margin = fallback_text_left_margin();
    uint32_t column = 0U;
    uint32_t spaces_to_next_tab;

    if (tab_width == 0U) {
        machine_panic("text flow tabWidth must be non-zero");
    }
    if (current_x > left_margin && char_width() != 0U) {
        column = (current_x - left_margin) / char_width();
    }
    spaces_to_next_tab = tab_width - (column % tab_width);
    if (spaces_to_next_tab == 0U) {
        spaces_to_next_tab = tab_width;
    }
    return current_x + (spaces_to_next_tab * char_width());
}

static uint32_t text_next_tab_x(uint32_t current_x) {
    struct recorz_mvp_value arguments[1];
    uint32_t next_x;

    arguments[0] = small_integer_value((int32_t)current_x);
    next_x = image_text_policy_u32_or_fallback(
        "BootTextNextTabX",
        RECORZ_MVP_SELECTOR_VALUE_ARG,
        1U,
        arguments,
        fallback_text_next_tab_x(current_x),
        "BootTextNextTabX did not return a small integer"
    );
    if (next_x <= current_x) {
        machine_panic("text layout nextTabXFrom: must advance the cursor");
    }
    return next_x;
}

static const struct recorz_mvp_heap_object *transcript_font_object(void) {
    const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(transcript_font_handle);

    if (object->kind != RECORZ_MVP_OBJECT_FONT) {
        machine_panic("transcript font root is not a font");
    }
    return object;
}

static const struct recorz_mvp_heap_object *transcript_font_reference_object(
    uint8_t field_index,
    uint8_t expected_kind,
    const char *message
) {
    struct recorz_mvp_value reference_value = heap_get_field(transcript_font_object(), field_index);
    const struct recorz_mvp_heap_object *reference_object;

    if (reference_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic(message);
    }
    reference_object = heap_object_for_value(reference_value);
    if (reference_object->kind != expected_kind) {
        machine_panic(message);
    }
    return reference_object;
}

static uint32_t transcript_font_u32(uint8_t index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_font_object(), index), message);
}

static const struct recorz_mvp_heap_object *transcript_style_object(void) {
    const struct recorz_mvp_heap_object *object = (const struct recorz_mvp_heap_object *)heap_object(transcript_style_handle);

    if (object->kind != RECORZ_MVP_OBJECT_TEXT_STYLE) {
        machine_panic("transcript style root is not a text style");
    }
    return object;
}

static const struct recorz_mvp_heap_object *text_style_object_for_value(
    struct recorz_mvp_value value,
    const char *message
) {
    const struct recorz_mvp_heap_object *style_object;

    if (value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic(message);
    }
    style_object = heap_object_for_value(value);
    if (style_object->kind != RECORZ_MVP_OBJECT_TEXT_STYLE) {
        machine_panic(message);
    }
    return style_object;
}

static uint32_t text_style_u32(
    const struct recorz_mvp_heap_object *style_object,
    uint8_t index,
    const char *message
) {
    if (style_object->kind != RECORZ_MVP_OBJECT_TEXT_STYLE) {
        machine_panic(message);
    }
    return small_integer_u32(heap_get_field(style_object, index), message);
}

static uint32_t text_background_color(void) {
    return text_style_u32(
        transcript_style_object(),
        TEXT_STYLE_FIELD_BACKGROUND_COLOR,
        "text style background color is not a small integer"
    );
}

static uint32_t text_foreground_color(void) {
    return text_style_u32(
        transcript_style_object(),
        TEXT_STYLE_FIELD_FOREGROUND_COLOR,
        "text style foreground color is not a small integer"
    );
}

static uint32_t styled_text_background_color(const struct recorz_mvp_heap_object *style_object) {
    return text_style_u32(
        style_object,
        TEXT_STYLE_FIELD_BACKGROUND_COLOR,
        "styled text background color is not a small integer"
    );
}

static uint32_t styled_text_foreground_color(const struct recorz_mvp_heap_object *style_object) {
    return text_style_u32(
        style_object,
        TEXT_STYLE_FIELD_FOREGROUND_COLOR,
        "styled text foreground color is not a small integer"
    );
}

static const struct recorz_mvp_heap_object *transcript_metrics_object(void) {
    return transcript_font_reference_object(
        FONT_FIELD_METRICS,
        RECORZ_MVP_OBJECT_TEXT_METRICS,
        "transcript font metrics is not a text metrics object"
    );
}

static uint32_t transcript_metrics_u32(uint8_t index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_metrics_object(), index), message);
}

static const struct recorz_mvp_heap_object *transcript_vertical_metrics_object(void) {
    struct recorz_mvp_value vertical_metrics_value = heap_get_field(
        transcript_metrics_object(),
        TEXT_METRICS_FIELD_VERTICAL_METRICS
    );
    const struct recorz_mvp_heap_object *vertical_metrics_object;

    if (vertical_metrics_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("text metrics verticalMetrics is not a text vertical metrics object");
    }
    vertical_metrics_object = heap_object_for_value(vertical_metrics_value);
    if (vertical_metrics_object->kind != RECORZ_MVP_OBJECT_TEXT_VERTICAL_METRICS) {
        machine_panic("text metrics verticalMetrics is not a text vertical metrics object");
    }
    return vertical_metrics_object;
}

static uint32_t transcript_vertical_metrics_u32(uint8_t index, const char *message) {
    return small_integer_u32(heap_get_field(transcript_vertical_metrics_object(), index), message);
}

static const struct recorz_mvp_heap_object *transcript_behavior_object(void) {
    return transcript_font_reference_object(
        FONT_FIELD_BEHAVIOR,
        RECORZ_MVP_OBJECT_TEXT_BEHAVIOR,
        "transcript font behavior is not a text behavior object"
    );
}

static void validate_transcript_font_glyphs_reference(void) {
    struct recorz_mvp_value glyphs_value = heap_get_field(transcript_font_object(), FONT_FIELD_GLYPHS);

    if (glyphs_value.kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)glyphs_value.integer != global_handles[RECORZ_MVP_GLOBAL_GLYPHS]) {
        machine_panic("transcript font glyphs does not point at Glyphs");
    }
}

static uint32_t transcript_clear_on_overflow(void) {
    return small_integer_u32(
        heap_get_field(transcript_behavior_object(), TEXT_BEHAVIOR_FIELD_CLEAR_ON_OVERFLOW),
        "text behavior clear-on-overflow is not a small integer"
    );
}

static uint32_t fallback_char_width(void) {
    return transcript_metrics_u32(
        TEXT_METRICS_FIELD_CELL_WIDTH,
        "text metrics cell width is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t char_width(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextCharacterWidth",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_char_width(),
        "BootTextCharacterWidth did not return a small integer"
    );
}

static uint32_t fallback_char_height(void) {
    return transcript_metrics_u32(
        TEXT_METRICS_FIELD_CELL_HEIGHT,
        "text metrics cell height is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t char_height(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextCharacterHeight",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_char_height(),
        "BootTextCharacterHeight did not return a small integer"
    );
}

static uint32_t fallback_text_baseline(void) {
    return transcript_metrics_u32(
        TEXT_METRICS_FIELD_BASELINE,
        "text metrics baseline is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t text_baseline(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextBaseline",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_baseline(),
        "BootTextBaseline did not return a small integer"
    );
}

static uint32_t fallback_text_ascent(void) {
    return transcript_vertical_metrics_u32(
        TEXT_VERTICAL_METRICS_FIELD_ASCENT,
        "text vertical metrics ascent is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t text_ascent(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextAscent",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_ascent(),
        "BootTextAscent did not return a small integer"
    );
}

static uint32_t fallback_text_descent(void) {
    return transcript_vertical_metrics_u32(
        TEXT_VERTICAL_METRICS_FIELD_DESCENT,
        "text vertical metrics descent is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t text_descent(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextDescent",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_descent(),
        "BootTextDescent did not return a small integer"
    );
}

static uint32_t fallback_text_line_height(void) {
    return transcript_vertical_metrics_u32(
        TEXT_VERTICAL_METRICS_FIELD_LINE_HEIGHT,
        "text vertical metrics line height is not a small integer"
    ) * text_pixel_scale();
}

static uint32_t text_line_height(void) {
    return image_text_policy_u32_or_fallback(
        "BootTextLineAdvance",
        RECORZ_MVP_SELECTOR_VALUE,
        0U,
        0,
        fallback_text_line_height(),
        "BootTextLineAdvance did not return a small integer"
    );
}

static void validate_transcript_text_metrics(const char *prefix) {
    uint32_t baseline = text_baseline();
    uint32_t ascent = text_ascent();
    uint32_t descent = text_descent();
    uint32_t line_height = text_line_height();
    uint32_t cell_height = char_height();

    if (baseline == 0U) {
        machine_panic(prefix);
    }
    if (line_height == 0U) {
        machine_panic(prefix);
    }
    if (ascent == 0U) {
        machine_panic(prefix);
    }
    if (line_height < cell_height) {
        machine_panic(prefix);
    }
    if (line_height < cell_height + text_line_spacing()) {
        machine_panic(prefix);
    }
    if (baseline < ascent) {
        machine_panic(prefix);
    }
    if (baseline + descent > line_height) {
        machine_panic(prefix);
    }
    if (ascent + descent != cell_height) {
        machine_panic(prefix);
    }
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

struct recorz_mvp_form_surface {
    uint32_t storage_kind;
    uint32_t width;
    uint32_t height;
    struct recorz_mvp_heap_object *mutable_bitmap;
};

static struct recorz_mvp_form_surface form_surface_for_form(const struct recorz_mvp_heap_object *form) {
    const struct recorz_mvp_heap_object *bitmap = bitmap_for_form(form);
    struct recorz_mvp_form_surface surface;

    surface.storage_kind = bitmap_storage_kind(bitmap);
    surface.width = bitmap_width(bitmap);
    surface.height = bitmap_height(bitmap);
    surface.mutable_bitmap =
        surface.storage_kind == BITMAP_STORAGE_HEAP_MONO ? mutable_bitmap_for_form(form) : 0;
    return surface;
}

static uint8_t form_surface_normalize_rect(
    const struct recorz_mvp_form_surface *surface,
    uint32_t *x,
    uint32_t *y,
    uint32_t *width,
    uint32_t *height
) {
    if (*width == 0U || *height == 0U || *x >= surface->width || *y >= surface->height) {
        return 0U;
    }
    if (*width > surface->width - *x) {
        *width = surface->width - *x;
    }
    if (*height > surface->height - *y) {
        *height = surface->height - *y;
    }
    return (uint8_t)(*width != 0U && *height != 0U);
}

static uint8_t form_surface_normalize_copy_rect(
    const struct recorz_mvp_form_surface *surface,
    uint32_t *source_x,
    uint32_t *source_y,
    uint32_t *width,
    uint32_t *height,
    uint32_t *dest_x,
    uint32_t *dest_y
) {
    if (*width == 0U ||
        *height == 0U ||
        *source_x >= surface->width ||
        *source_y >= surface->height ||
        *dest_x >= surface->width ||
        *dest_y >= surface->height) {
        return 0U;
    }
    if (*width > surface->width - *source_x) {
        *width = surface->width - *source_x;
    }
    if (*height > surface->height - *source_y) {
        *height = surface->height - *source_y;
    }
    if (*width > surface->width - *dest_x) {
        *width = surface->width - *dest_x;
    }
    if (*height > surface->height - *dest_y) {
        *height = surface->height - *dest_y;
    }
    return (uint8_t)(*width != 0U && *height != 0U);
}

static uint8_t form_surface_normalize_scaled_mono_blit(
    const struct recorz_mvp_form_surface *surface,
    uint32_t *dest_x,
    uint32_t *dest_y,
    uint32_t copy_width,
    uint32_t copy_height,
    uint32_t scale,
    uint32_t *draw_width_pixels,
    uint32_t *draw_height_pixels
) {
    uint32_t width_pixels;
    uint32_t height_pixels;

    if (*dest_x >= surface->width || *dest_y >= surface->height) {
        return 0U;
    }
    width_pixels = copy_width * scale;
    height_pixels = copy_height * scale;
    if (width_pixels == 0U || height_pixels == 0U) {
        return 0U;
    }
    if (width_pixels > surface->width - *dest_x) {
        width_pixels = surface->width - *dest_x;
    }
    if (height_pixels > surface->height - *dest_y) {
        height_pixels = surface->height - *dest_y;
    }
    if (width_pixels == 0U || height_pixels == 0U) {
        return 0U;
    }
    *draw_width_pixels = width_pixels;
    *draw_height_pixels = height_pixels;
    return 1U;
}

static void reset_text_cursor(void) {
    cursor_x = text_left_margin();
    cursor_y = text_top_margin();
}

static const struct recorz_mvp_heap_object *glyph_bitmap_for_char(char ch) {
    uint32_t index = (uint8_t)ch;

    validate_transcript_font_glyphs_reference();
    if (index >= 128U || glyph_bitmap_handles[index] == 0U) {
        return (const struct recorz_mvp_heap_object *)heap_object(glyph_fallback_handle);
    }
    return (const struct recorz_mvp_heap_object *)heap_object(glyph_bitmap_handles[index]);
}

static struct recorz_mvp_value glyph_bitmap_value_for_code(uint32_t code) {
    validate_transcript_font_glyphs_reference();
    if (code >= 128U || glyph_bitmap_handles[code] == 0U) {
        return object_value(glyph_fallback_handle);
    }
    return object_value(glyph_bitmap_handles[code]);
}

static void fill_mono_bitmap_rect(
    struct recorz_mvp_heap_object *bitmap,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint8_t bit_value
) {
    uint32_t *rows = mutable_mono_bitmap_rows(bitmap);
    uint32_t bitmap_width_value = bitmap_width(bitmap);
    uint32_t bitmap_height_value = bitmap_height(bitmap);
    uint32_t row;

    if (width == 0U || height == 0U || x >= bitmap_width_value || y >= bitmap_height_value) {
        return;
    }
    if (width > bitmap_width_value - x) {
        width = bitmap_width_value - x;
    }
    if (height > bitmap_height_value - y) {
        height = bitmap_height_value - y;
    }

    for (row = 0U; row < height; ++row) {
        uint32_t target_y = y + row;
        uint32_t col;
        for (col = 0U; col < width; ++col) {
            uint32_t target_x = x + col;
            uint32_t mask = 1U << (bitmap_width_value - target_x - 1U);
            if (bit_value) {
                rows[target_y] |= mask;
            } else {
                rows[target_y] &= ~mask;
            }
        }
    }
}

static void copy_mono_bitmap_rect_in_place(
    struct recorz_mvp_heap_object *bitmap,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
) {
    uint32_t *rows = mutable_mono_bitmap_rows(bitmap);
    uint32_t bitmap_width_value = bitmap_width(bitmap);
    uint32_t bitmap_height_value = bitmap_height(bitmap);
    int32_t row = 0;
    int32_t row_end = (int32_t)height;
    int32_t row_step = 1;

    if (width == 0U ||
        height == 0U ||
        source_x >= bitmap_width_value ||
        source_y >= bitmap_height_value ||
        dest_x >= bitmap_width_value ||
        dest_y >= bitmap_height_value) {
        return;
    }
    if (width > bitmap_width_value - source_x) {
        width = bitmap_width_value - source_x;
    }
    if (height > bitmap_height_value - source_y) {
        height = bitmap_height_value - source_y;
    }
    if (width > bitmap_width_value - dest_x) {
        width = bitmap_width_value - dest_x;
    }
    if (height > bitmap_height_value - dest_y) {
        height = bitmap_height_value - dest_y;
    }
    if (width == 0U || height == 0U) {
        return;
    }
    if (source_y < dest_y && source_y + height > dest_y) {
        row = (int32_t)height - 1;
        row_end = -1;
        row_step = -1;
    }
    for (; row != row_end; row += row_step) {
        uint32_t source_row = source_y + (uint32_t)row;
        uint32_t dest_row = dest_y + (uint32_t)row;
        int32_t column = 0;
        int32_t column_end = (int32_t)width;
        int32_t column_step = 1;

        if (source_x < dest_x && source_x + width > dest_x) {
            column = (int32_t)width - 1;
            column_end = -1;
            column_step = -1;
        }
        for (; column != column_end; column += column_step) {
            uint32_t source_column = source_x + (uint32_t)column;
            uint32_t dest_column = dest_x + (uint32_t)column;
            uint32_t source_mask = 1U << (bitmap_width_value - source_column - 1U);
            uint32_t dest_mask = 1U << (bitmap_width_value - dest_column - 1U);

            if ((rows[source_row] & source_mask) != 0U) {
                rows[dest_row] |= dest_mask;
            } else {
                rows[dest_row] &= ~dest_mask;
            }
        }
    }
}

static void form_surface_fill_rect_color(
    const struct recorz_mvp_form_surface *surface,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
) {
    if (!form_surface_normalize_rect(surface, &x, &y, &width, &height)) {
        return;
    }
    if (surface->storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        display_form_fill_rect(x, y, width, height, color);
        return;
    }
    if (surface->storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        fill_mono_bitmap_rect(surface->mutable_bitmap, x, y, width, height, (uint8_t)(color != 0U));
        return;
    }
    machine_panic("fill expects a framebuffer or heap monochrome form");
}

static void form_surface_copy_rect(
    const struct recorz_mvp_form_surface *surface,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
) {
    if (!form_surface_normalize_copy_rect(
            surface,
            &source_x,
            &source_y,
            &width,
            &height,
            &dest_x,
            &dest_y)) {
        return;
    }
    if (surface->storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        display_form_copy_rect(source_x, source_y, width, height, dest_x, dest_y);
        return;
    }
    if (surface->storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        copy_mono_bitmap_rect_in_place(
            surface->mutable_bitmap,
            source_x,
            source_y,
            width,
            height,
            dest_x,
            dest_y
        );
        return;
    }
    machine_panic("copy expects a framebuffer or heap monochrome form");
}

static void form_surface_draw_line_color(
    const struct recorz_mvp_form_surface *surface,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint32_t color
) {
    if (surface->storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        display_form_draw_line(x0, y0, x1, y1, color);
        return;
    }
    if (surface->storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        draw_mono_bitmap_line(surface->mutable_bitmap, x0, y0, x1, y1, (uint8_t)(color != 0U));
        return;
    }
    machine_panic("line draw expects a framebuffer or heap monochrome form");
}

static void bitblt_copy_mono_bitmap_to_surface(
    const struct recorz_mvp_heap_object *source_bitmap,
    const struct recorz_mvp_form_surface *dest_surface,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t copy_width,
    uint32_t copy_height,
    uint32_t x,
    uint32_t y,
    uint32_t scale,
    uint32_t one_color,
    uint32_t zero_color,
    uint8_t transfer_rule
) {
    uint32_t draw_width_pixels = 0U;
    uint32_t draw_height_pixels = 0U;
    uint8_t transparent_zero = bitblt_transfer_rule_uses_transparent_zero(transfer_rule);

    if (scale == 0U) {
        machine_panic("BitBlt copy scale must be non-zero");
    }
    if (!normalize_bitblt_source_region(source_bitmap, &source_x, &source_y, &copy_width, &copy_height)) {
        return;
    }
    if (dest_surface->storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        if (!form_surface_normalize_scaled_mono_blit(
                dest_surface,
                &x,
                &y,
                copy_width,
                copy_height,
                scale,
                &draw_width_pixels,
                &draw_height_pixels)) {
            return;
        }
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
            draw_width_pixels,
            draw_height_pixels,
            one_color,
            zero_color,
            transparent_zero
        );
        return;
    }
    if (dest_surface->storage_kind == BITMAP_STORAGE_HEAP_MONO) {
        bitblt_copy_mono_bitmap_to_mono_bitmap(
            source_bitmap,
            dest_surface->mutable_bitmap,
            source_x,
            source_y,
            copy_width,
            copy_height,
            x,
            y,
            scale,
            transfer_rule
        );
        return;
    }
    machine_panic("BitBlt destination bitmap storage is unsupported");
}

static void set_mono_bitmap_pixel(
    struct recorz_mvp_heap_object *bitmap,
    uint32_t x,
    uint32_t y,
    uint8_t bit_value
) {
    uint32_t *rows = mutable_mono_bitmap_rows(bitmap);
    uint32_t bitmap_width_value = bitmap_width(bitmap);
    uint32_t bitmap_height_value = bitmap_height(bitmap);
    uint32_t mask;

    if (x >= bitmap_width_value || y >= bitmap_height_value) {
        return;
    }
    mask = 1U << (bitmap_width_value - x - 1U);
    if (bit_value) {
        rows[y] |= mask;
    } else {
        rows[y] &= ~mask;
    }
}

static void draw_mono_bitmap_line(
    struct recorz_mvp_heap_object *bitmap,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint8_t bit_value
) {
    int32_t dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y0 < y1) ? (y0 - y1) : (y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    for (;;) {
        if (x0 >= 0 && y0 >= 0) {
            set_mono_bitmap_pixel(bitmap, (uint32_t)x0, (uint32_t)y0, bit_value);
        }
        if (x0 == x1 && y0 == y1) {
            return;
        }
        if ((err * 2) >= dy) {
            err += dy;
            x0 += sx;
        }
        if ((err * 2) <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static uint8_t bitblt_transfer_rule_uses_transparent_zero(uint8_t transfer_rule) {
    switch (transfer_rule) {
        case RECORZ_MVP_TRANSFER_RULE_COPY:
            return 0U;
        case RECORZ_MVP_TRANSFER_RULE_OVER:
            return 1U;
        default:
            machine_panic("BitBlt transfer rule is unsupported");
            return 0U;
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
    uint8_t transfer_rule
) {
    const uint32_t *source_rows = mono_bitmap_rows(source_bitmap);
    uint32_t *dest_rows = mutable_mono_bitmap_rows(dest_bitmap);
    uint32_t source_width = bitmap_width(source_bitmap);
    uint32_t dest_width = bitmap_width(dest_bitmap);
    uint32_t dest_height = bitmap_height(dest_bitmap);
    uint32_t source_row_offset;
    uint8_t transparent_zero = bitblt_transfer_rule_uses_transparent_zero(transfer_rule);

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
    uint8_t transfer_rule
) {
    struct recorz_mvp_form_surface dest_surface = form_surface_for_form(dest_form);

    bitblt_copy_mono_bitmap_to_surface(
        source_bitmap,
        &dest_surface,
        source_x,
        source_y,
        copy_width,
        copy_height,
        x,
        y,
        scale,
        one_color,
        zero_color,
        transfer_rule
    );
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
    struct recorz_mvp_form_surface surface = form_surface_for_form(form);

    form_fill_rect_color(
        form,
        0U,
        0U,
        surface.width,
        surface.height,
        color
    );
    if (surface.storage_kind == BITMAP_STORAGE_FRAMEBUFFER) {
        reset_text_cursor();
    }
}

static void form_fill_rect_color(
    const struct recorz_mvp_heap_object *form,
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
) {
    struct recorz_mvp_form_surface surface = form_surface_for_form(form);

    form_surface_fill_rect_color(&surface, x, y, width, height, color);
}

static void form_copy_rect(
    const struct recorz_mvp_heap_object *form,
    uint32_t source_x,
    uint32_t source_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
) {
    struct recorz_mvp_form_surface surface = form_surface_for_form(form);

    form_surface_copy_rect(&surface, source_x, source_y, width, height, dest_x, dest_y);
}

static void form_draw_line_color(
    const struct recorz_mvp_heap_object *form,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint32_t color
) {
    struct recorz_mvp_form_surface surface = form_surface_for_form(form);

    form_surface_draw_line_color(&surface, x0, y0, x1, y1, color);
}

static void form_clear(const struct recorz_mvp_heap_object *form) {
    fill_form_color(form, text_background_color());
}

static void form_newline(const struct recorz_mvp_heap_object *form) {
    uint32_t form_height = bitmap_height(bitmap_for_form(form));
    uint32_t bottom_margin = text_bottom_margin();
    uint8_t echo_serial = (uint8_t)(bitmap_storage_kind(bitmap_for_form(form)) == BITMAP_STORAGE_FRAMEBUFFER);
    uint8_t capture_feedback =
        (uint8_t)(workspace_input_monitor_capture_enabled &&
                  heap_handle_for_object(form) == active_display_form_handle);

    cursor_x = text_left_margin();
    cursor_y += text_line_height();
    if (transcript_clear_on_overflow() &&
        form_height > bottom_margin &&
        cursor_y + char_height() >= form_height - bottom_margin) {
        form_clear(form);
    }
    if (echo_serial) {
        machine_putc('\n');
    }
    if (capture_feedback) {
        workspace_input_monitor_feedback_append_char('\n');
    }
}

static uint8_t form_has_image_side_styled_text_override(const struct recorz_mvp_heap_object *form) {
    const struct recorz_mvp_live_method_source *source_record;
    const struct recorz_mvp_heap_object *owner_class = 0;

    if (named_object_handle_for_name("BootTextRendererWriteStringOverride") == 0U) {
        return 0U;
    }
    source_record = live_method_source_for_class_chain(
        class_object_for_heap_object(form),
        RECORZ_MVP_SELECTOR_WRITE_STYLED_TEXT,
        1U,
        &owner_class
    );
    return (uint8_t)(
        source_record != 0 &&
        source_text_requires_live_evaluator(live_method_source_text(source_record))
    );
}

static struct recorz_mvp_value image_text_renderer_styled_text_bridge_value(
    const char *text,
    uint32_t foreground_color,
    uint32_t background_color
) {
    uint16_t style_handle = named_object_handle_for_name("RecorzTextStyleBridge");
    uint16_t styled_text_handle = named_object_handle_for_name("RecorzStyledTextBridge");

    if (style_handle == 0U) {
        style_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_TEXT_STYLE);
        remember_named_object_handle(style_handle, "RecorzTextStyleBridge");
    }
    if (styled_text_handle == 0U) {
        styled_text_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_STYLED_TEXT);
        remember_named_object_handle(styled_text_handle, "RecorzStyledTextBridge");
    }
    heap_set_field(style_handle, TEXT_STYLE_FIELD_FOREGROUND_COLOR, small_integer_value((int32_t)foreground_color));
    heap_set_field(style_handle, TEXT_STYLE_FIELD_BACKGROUND_COLOR, small_integer_value((int32_t)background_color));
    heap_set_field(styled_text_handle, STYLED_TEXT_FIELD_TEXT, string_value(text == 0 ? "" : text));
    heap_set_field(styled_text_handle, STYLED_TEXT_FIELD_STYLE, object_value(style_handle));
    return object_value(styled_text_handle);
}

static void form_write_code_point_with_colors(
    const struct recorz_mvp_heap_object *form,
    uint8_t code_point,
    uint32_t foreground_color,
    uint32_t background_color
) {
    uint32_t wrap_limit_x = text_wrap_limit_x_for_form_width(bitmap_width(bitmap_for_form(form)));
    uint8_t echo_serial = (uint8_t)(bitmap_storage_kind(bitmap_for_form(form)) == BITMAP_STORAGE_FRAMEBUFFER);
    uint8_t capture_feedback =
        (uint8_t)(workspace_input_monitor_capture_enabled &&
                  heap_handle_for_object(form) == active_display_form_handle);

    if (code_point == (uint8_t)'\n') {
        form_newline(form);
        return;
    }
    if (code_point == (uint8_t)'\t') {
        uint32_t next_tab_x = text_next_tab_x(cursor_x);

        while (cursor_x < next_tab_x) {
            form_write_code_point_with_colors(form, (uint8_t)' ', foreground_color, background_color);
        }
        if (echo_serial) {
            machine_putc('\t');
        }
        if (capture_feedback) {
            workspace_input_monitor_feedback_append_char('\t');
        }
        return;
    }
    if (wrap_limit_x != 0U &&
        cursor_x + char_width() > wrap_limit_x) {
        form_newline(form);
    }
    {
        const struct recorz_mvp_heap_object *glyph_bitmap = glyph_bitmap_for_char((char)code_point);

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
            foreground_color,
            background_color,
            RECORZ_MVP_TRANSFER_RULE_COPY
        );
    }
    cursor_x += char_width();
    if (echo_serial) {
        machine_putc((char)code_point);
    }
    if (capture_feedback) {
        workspace_input_monitor_feedback_append_char((char)code_point);
    }
}

static void form_write_string_with_colors(
    const struct recorz_mvp_heap_object *form,
    const char *text,
    uint32_t foreground_color,
    uint32_t background_color
) {
    if (form_has_image_side_styled_text_override(form)) {
        struct recorz_mvp_value arguments[1];

        arguments[0] = image_text_renderer_styled_text_bridge_value(text, foreground_color, background_color);
        (void)perform_send_and_pop_result(
            object_value(heap_handle_for_object(form)),
            RECORZ_MVP_SELECTOR_WRITE_STYLED_TEXT,
            1U,
            arguments,
            0
        );
        return;
    }

    while (*text != '\0') {
        form_write_code_point_with_colors(form, (uint8_t)*text, foreground_color, background_color);
        ++text;
    }
}

static void form_write_string(const struct recorz_mvp_heap_object *form, const char *text) {
    form_write_string_with_colors(
        form,
        text,
        text_foreground_color(),
        text_background_color()
    );
}

static void character_scanner_set_state(
    const struct recorz_mvp_heap_object *scanner,
    uint32_t stop_reason,
    uint32_t text_index,
    uint32_t x,
    uint32_t y
) {
    uint16_t scanner_handle = heap_handle_for_object(scanner);

    heap_set_field(scanner_handle, CHARACTER_SCANNER_FIELD_STOP, small_integer_value((int32_t)stop_reason));
    heap_set_field(scanner_handle, CHARACTER_SCANNER_FIELD_INDEX, small_integer_value((int32_t)text_index));
    heap_set_field(scanner_handle, CHARACTER_SCANNER_FIELD_X, small_integer_value((int32_t)x));
    heap_set_field(scanner_handle, CHARACTER_SCANNER_FIELD_Y, small_integer_value((int32_t)y));
}

static void form_draw_code_point_at_with_colors(
    const struct recorz_mvp_heap_object *form,
    uint8_t code_point,
    uint32_t x,
    uint32_t y,
    uint32_t foreground_color,
    uint32_t background_color
) {
    uint8_t echo_serial = (uint8_t)(bitmap_storage_kind(bitmap_for_form(form)) == BITMAP_STORAGE_FRAMEBUFFER);
    uint8_t capture_feedback =
        (uint8_t)(workspace_input_monitor_capture_enabled &&
                  heap_handle_for_object(form) == active_display_form_handle);
    const struct recorz_mvp_heap_object *glyph_bitmap = glyph_bitmap_for_char((char)code_point);

    bitblt_copy_mono_bitmap_to_form(
        glyph_bitmap,
        form,
        0U,
        0U,
        bitmap_width(glyph_bitmap),
        bitmap_height(glyph_bitmap),
        x,
        y,
        text_pixel_scale(),
        foreground_color,
        background_color,
        RECORZ_MVP_TRANSFER_RULE_COPY
    );
    if (echo_serial) {
        machine_putc((char)code_point);
    }
    if (capture_feedback) {
        workspace_input_monitor_feedback_append_char((char)code_point);
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

static uint8_t form_can_be_display(const struct recorz_mvp_heap_object *form) {
    struct recorz_mvp_value bits_value = heap_get_field(form, FORM_FIELD_BITS);

    if (bits_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        return 0U;
    }
    return (uint8_t)((uint16_t)bits_value.integer == framebuffer_bitmap_handle);
}

static const struct recorz_mvp_heap_object *cursor_bitmap_object(const struct recorz_mvp_heap_object *cursor) {
    struct recorz_mvp_value bits_value = heap_get_field(cursor, CURSOR_FIELD_BITS);
    const struct recorz_mvp_heap_object *bitmap;
    uint32_t storage_kind;

    if (bits_value.kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("Cursor bits must be a bitmap object");
    }
    bitmap = heap_object_for_value(bits_value);
    if (bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
        machine_panic("Cursor bits must be a bitmap object");
    }
    storage_kind = bitmap_storage_kind(bitmap);
    if (storage_kind != BITMAP_STORAGE_GLYPH_MONO && storage_kind != BITMAP_STORAGE_HEAP_MONO) {
        machine_panic("Cursor bits must be a monochrome bitmap");
    }
    return bitmap;
}

static uint32_t cursor_hotspot_u32(
    const struct recorz_mvp_heap_object *cursor,
    uint8_t field_index,
    const char *message
) {
    struct recorz_mvp_value hotspot_value = heap_get_field(cursor, field_index);

    return small_integer_u32(hotspot_value, message);
}

static void active_cursor_move_to(uint32_t x, uint32_t y) {
    active_cursor_screen_x = x;
    active_cursor_screen_y = y;
}

static void active_cursor_set_visible(uint8_t visible) {
    active_cursor_visible = visible != 0U ? 1U : 0U;
}

static uint8_t active_cursor_is_visible(void) {
    return (uint8_t)(active_cursor_handle != 0U && active_cursor_visible != 0U);
}

static void draw_active_cursor_on_form(const struct recorz_mvp_heap_object *form) {
    const struct recorz_mvp_heap_object *cursor_object;
    const struct recorz_mvp_heap_object *cursor_bitmap;
    uint32_t scale;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t cursor_left;
    uint32_t cursor_top;

    if (!active_cursor_is_visible()) {
        return;
    }
    cursor_object = (const struct recorz_mvp_heap_object *)heap_object(active_cursor_handle);
    cursor_bitmap = cursor_bitmap_object(cursor_object);
    scale = text_pixel_scale();
    hotspot_x = cursor_hotspot_u32(cursor_object, CURSOR_FIELD_HOTSPOT_X, "Cursor hotspotX must be a non-negative small integer");
    hotspot_y = cursor_hotspot_u32(cursor_object, CURSOR_FIELD_HOTSPOT_Y, "Cursor hotspotY must be a non-negative small integer");
    cursor_left = active_cursor_screen_x;
    cursor_top = active_cursor_screen_y;

    if (cursor_left >= hotspot_x * scale) {
        cursor_left -= hotspot_x * scale;
    } else {
        cursor_left = 0U;
    }
    if (cursor_top >= hotspot_y * scale) {
        cursor_top -= hotspot_y * scale;
    } else {
        cursor_top = 0U;
    }
    bitblt_copy_mono_bitmap_to_form(
        cursor_bitmap,
        form,
        0U,
        0U,
        bitmap_width(cursor_bitmap),
        bitmap_height(cursor_bitmap),
        cursor_left,
        cursor_top,
        scale,
        text_foreground_color(),
        0U,
        RECORZ_MVP_TRANSFER_RULE_OVER
    );
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
    active_display_form_handle = default_form_handle;
    active_cursor_handle = 0U;
    active_cursor_visible = 0U;
    active_cursor_screen_x = 0U;
    active_cursor_screen_y = 0U;
    framebuffer_bitmap_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_FRAMEBUFFER_BITMAP]);
    transcript_behavior_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_BEHAVIOR]);
    transcript_layout_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_LAYOUT]);
    transcript_style_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_STYLE]);
    transcript_metrics_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_METRICS]);
    transcript_font_handle = seed_handle_at(seed, seed->root_object_indices[RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT]);
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
    if (heap_get_field(transcript_font_object(), FONT_FIELD_METRICS).kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)heap_get_field(transcript_font_object(), FONT_FIELD_METRICS).integer != transcript_metrics_handle) {
        machine_panic("transcript font does not point at transcript metrics");
    }
    if (heap_get_field(transcript_font_object(), FONT_FIELD_BEHAVIOR).kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)heap_get_field(transcript_font_object(), FONT_FIELD_BEHAVIOR).integer != transcript_behavior_handle) {
        machine_panic("transcript font does not point at transcript behavior");
    }
    if (transcript_font_u32(FONT_FIELD_POINT_SIZE, "transcript font point size is not a small integer") == 0U) {
        machine_panic("transcript font point size must be non-zero");
    }
    validate_transcript_font_glyphs_reference();
    validate_transcript_text_metrics("transcript text metrics are inconsistent");
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
    uint16_t env_index;
    uint16_t home_index;

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
    for (env_index = 0U; env_index < SOURCE_EVAL_ENV_LIMIT; ++env_index) {
        if (source_eval_environments[env_index].in_use) {
            uint16_t binding_index;

            for (binding_index = 0U; binding_index < source_eval_environments[env_index].binding_count; ++binding_index) {
                runtime_string_rewrite_live_value(
                    &source_eval_environments[env_index].bindings[binding_index].value,
                    old_text,
                    new_text
                );
            }
        }
    }
    for (home_index = 0U; home_index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++home_index) {
        if (source_eval_home_contexts[home_index].in_use) {
            runtime_string_rewrite_live_value(
                &source_eval_home_contexts[home_index].receiver,
                old_text,
                new_text
            );
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
    uint16_t env_index;
    uint16_t home_index;

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
    for (env_index = 0U; env_index < SOURCE_EVAL_ENV_LIMIT; ++env_index) {
        if (source_eval_environments[env_index].in_use) {
            uint16_t binding_index;

            for (binding_index = 0U; binding_index < source_eval_environments[env_index].binding_count; ++binding_index) {
                runtime_string_mark_live_value(
                    source_eval_environments[env_index].bindings[binding_index].value,
                    live_starts,
                    sizeof(live_starts)
                );
            }
        }
    }
    for (home_index = 0U; home_index < SOURCE_EVAL_HOME_CONTEXT_LIMIT; ++home_index) {
        if (source_eval_home_contexts[home_index].in_use) {
            runtime_string_mark_live_value(
                source_eval_home_contexts[home_index].receiver,
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

static const char *runtime_string_intern_copy(const char *text) {
    uint32_t length;
    uint32_t offset = 0U;

    if (text == 0) {
        machine_panic("runtime string source is null");
    }
    length = text_length(text);
    while (offset < runtime_string_pool_offset) {
        const char *candidate = runtime_string_pool + offset;
        uint32_t candidate_length = text_length(candidate);

        if (candidate_length == length) {
            uint32_t index;
            uint8_t matches = 1U;

            for (index = 0U; index < length; ++index) {
                if (candidate[index] != text[index]) {
                    matches = 0U;
                    break;
                }
            }
            if (matches) {
                return candidate;
            }
        }
        offset += candidate_length + 1U;
    }
    return runtime_string_allocate_copy(text);
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

static const char *live_package_do_it_source_text(
    const struct recorz_mvp_live_package_do_it_source *source_record
) {
    if (source_record->source_length == 0U) {
        return "";
    }
    if (source_record->source_offset + source_record->source_length >= LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT) {
        machine_panic("live package do-it source offset is out of range");
    }
    if (live_package_do_it_source_pool[source_record->source_offset + source_record->source_length] != '\0') {
        machine_panic("live package do-it source is not terminated");
    }
    return live_package_do_it_source_pool + source_record->source_offset;
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

static void repack_live_package_do_it_source_pool(void) {
    uint16_t source_index;
    uint16_t write_offset = 0U;

    for (source_index = 0U; source_index < live_package_do_it_source_count; ++source_index) {
        struct recorz_mvp_live_package_do_it_source *source_record = &live_package_do_it_sources[source_index];
        uint16_t source_offset = source_record->source_offset;
        uint16_t source_length = source_record->source_length;
        uint16_t char_index;

        if (source_length == 0U) {
            source_record->source_offset = 0U;
            continue;
        }
        if (source_offset + source_length >= LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT) {
            machine_panic("live package do-it source offset is out of range");
        }
        if (write_offset + source_length + 1U > LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT) {
            machine_panic("live package do-it source pool overflow");
        }
        for (char_index = 0U; char_index <= source_length; ++char_index) {
            live_package_do_it_source_pool[write_offset + char_index] =
                live_package_do_it_source_pool[source_offset + char_index];
        }
        source_record->source_offset = write_offset;
        write_offset = (uint16_t)(write_offset + source_length + 1U);
    }
    live_package_do_it_source_pool_used = write_offset;
    if (live_package_do_it_source_pool_used < LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT) {
        live_package_do_it_source_pool[live_package_do_it_source_pool_used] = '\0';
    }
}

static const struct recorz_mvp_live_package_do_it_source *live_package_do_it_source_for(
    const char *package_name,
    const char *source
) {
    uint16_t source_index;

    for (source_index = 0U; source_index < live_package_do_it_source_count; ++source_index) {
        const struct recorz_mvp_live_package_do_it_source *source_record = &live_package_do_it_sources[source_index];

        if (!source_names_equal(source_record->package_name, package_name)) {
            continue;
        }
        if (source_names_equal(live_package_do_it_source_text(source_record), source)) {
            return source_record;
        }
    }
    return 0;
}

static void clear_live_package_do_it_sources_for_package(const char *package_name) {
    uint16_t source_index = 0U;

    while (source_index < live_package_do_it_source_count) {
        uint16_t move_index;

        if (!source_names_equal(live_package_do_it_sources[source_index].package_name, package_name)) {
            ++source_index;
            continue;
        }
        for (move_index = (uint16_t)(source_index + 1U); move_index < live_package_do_it_source_count; ++move_index) {
            source_copy_identifier(
                live_package_do_it_sources[move_index - 1U].package_name,
                sizeof(live_package_do_it_sources[move_index - 1U].package_name),
                live_package_do_it_sources[move_index].package_name
            );
            live_package_do_it_sources[move_index - 1U].source_offset =
                live_package_do_it_sources[move_index].source_offset;
            live_package_do_it_sources[move_index - 1U].source_length =
                live_package_do_it_sources[move_index].source_length;
        }
        --live_package_do_it_source_count;
        repack_live_package_do_it_source_pool();
    }
}

static struct recorz_mvp_live_method_source *mutable_live_method_source_for(
    uint16_t class_handle,
    uint16_t selector_id,
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
    uint16_t selector_id
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
    uint16_t selector_id,
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
    uint16_t selector_id,
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

static void remember_live_package_do_it_source(const char *package_name, const char *source) {
    struct recorz_mvp_live_package_do_it_source *source_record;
    uint32_t length;
    uint32_t start;

    if (package_name == 0 || package_name[0] == '\0' || source == 0 || source[0] == '\0') {
        return;
    }
    if (live_package_do_it_source_for(package_name, source) != 0) {
        return;
    }
    length = text_length(source);
    if (length + 1U > METHOD_SOURCE_CHUNK_LIMIT) {
        machine_panic("live package do-it source exceeds chunk capacity");
    }
    if (live_package_do_it_source_count >= LIVE_PACKAGE_DO_IT_SOURCE_LIMIT) {
        machine_panic("live package do-it source registry overflow");
    }
    if (live_package_do_it_source_pool_used + length + 1U > LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT) {
        repack_live_package_do_it_source_pool();
    }
    if (live_package_do_it_source_pool_used + length + 1U > LIVE_PACKAGE_DO_IT_SOURCE_POOL_LIMIT) {
        machine_panic("live package do-it source pool overflow");
    }
    source_record = &live_package_do_it_sources[live_package_do_it_source_count++];
    source_copy_identifier(source_record->package_name, sizeof(source_record->package_name), package_name);
    source_record->source_offset = live_package_do_it_source_pool_used;
    source_record->source_length = (uint16_t)length;
    start = live_package_do_it_source_pool_used;
    live_package_do_it_source_pool_used = (uint16_t)(live_package_do_it_source_pool_used + length + 1U);
    while (*source != '\0') {
        live_package_do_it_source_pool[start++] = *source++;
    }
    live_package_do_it_source_pool[start] = '\0';
}

static void forget_live_string_literals(
    uint16_t class_handle,
    uint16_t selector_id,
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
    uint16_t selector_id,
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
           (RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT * 2U) +
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
    append_memory_report_line(buffer, &offset, "HEAP", heap_live_count, HEAP_LIMIT);
    append_memory_report_line(buffer, &offset, "HWM", heap_high_water_mark, HEAP_LIMIT);
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
    append_memory_report_stat(buffer, &offset, "GCC", gc_collection_count);
    append_memory_report_stat(buffer, &offset, "GCR", gc_last_reclaimed_count);
    append_memory_report_stat(buffer, &offset, "GCT", gc_total_reclaimed_count);
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

    (void)gc_collect_now();
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
    write_u16_le(snapshot_buffer + offset, active_display_form_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, active_cursor_handle);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, active_cursor_visible != 0U ? 1U : 0U);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, (uint16_t)active_cursor_screen_x);
    offset += 2U;
    write_u16_le(snapshot_buffer + offset, (uint16_t)active_cursor_screen_y);
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
    write_u16_le(snapshot_buffer + offset, transcript_font_handle);
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
        write_u16_le(snapshot_buffer + offset, source_record->selector_id);
        offset += 2U;
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
        write_u16_le(snapshot_buffer + offset, literal_record->selector_id);
        offset += 2U;
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
    machine_puts("recorz qemu-riscv32 mvp: snapshot saved, shutting down\n");
    machine_shutdown();
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
    uint16_t saved_active_display_form_handle;
    uint16_t saved_active_cursor_handle;
    uint16_t saved_active_cursor_visible;
    uint16_t saved_active_cursor_x;
    uint16_t saved_active_cursor_y;
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
    saved_active_display_form_handle = read_u16_le(blob + 38U);
    saved_active_cursor_handle = read_u16_le(blob + 40U);
    saved_active_cursor_visible = read_u16_le(blob + 42U);
    saved_active_cursor_x = read_u16_le(blob + 44U);
    saved_active_cursor_y = read_u16_le(blob + 46U);
    expected_size = read_u32_le(blob + 48U);
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
                            (RECORZ_MVP_SEED_ROOT_TRANSCRIPT_FONT * 2U) +
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

        if (heap_size >= HEAP_LIMIT) {
            machine_panic("snapshot object count exceeds heap capacity");
        }
        ++heap_size;
        if (heap_size > heap_high_water_mark) {
            heap_high_water_mark = heap_size;
        }
        heap_reset_slot(heap_size);
        heap[heap_size - 1U].kind = kind;
        if (kind != 0U) {
            ++heap_live_count;
        }
    }
    offset = SNAPSHOT_HEADER_SIZE;
    for (handle = 1U; handle <= object_count; ++handle) {
        struct recorz_mvp_heap_object *object = heap_object(handle);
        uint8_t field_index;

        object->kind = blob[offset++];
        object->field_count = blob[offset++];
        object->class_handle = read_u16_le(blob + offset);
        offset += 2U;
        if (object->kind == 0U) {
            if (object->field_count != 0U || object->class_handle != 0U) {
                machine_panic("snapshot free slot has unexpected metadata");
            }
        } else {
            if (object->field_count > OBJECT_FIELD_LIMIT) {
                machine_panic("snapshot field count exceeds object field capacity");
            }
            if (object->class_handle == 0U || object->class_handle > object_count) {
                machine_panic("snapshot class handle is out of range");
            }
        }
        for (field_index = 0U; field_index < OBJECT_FIELD_LIMIT; ++field_index) {
            object->fields[field_index] = snapshot_decode_value(blob + offset, string_byte_count);
            offset += SNAPSHOT_VALUE_SIZE;
        }
    }
    for (handle = RECORZ_MVP_GLOBAL_TRANSCRIPT; handle <= MAX_GLOBAL_ID; ++handle) {
        global_handles[handle] = read_u16_le(blob + offset);
        if (!heap_handle_is_live(global_handles[handle])) {
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
    transcript_font_handle = read_u16_le(blob + offset);
    offset += 2U;
    if (!heap_handle_is_live(default_form_handle) ||
        !heap_handle_is_live(framebuffer_bitmap_handle) ||
        !heap_handle_is_live(transcript_behavior_handle) ||
        !heap_handle_is_live(transcript_layout_handle) ||
        !heap_handle_is_live(transcript_style_handle) ||
        !heap_handle_is_live(transcript_metrics_handle) ||
        !heap_handle_is_live(transcript_font_handle)) {
        machine_panic("snapshot root handle is out of range");
    }
    if (!heap_handle_is_live(saved_active_display_form_handle)) {
        machine_panic("snapshot active display form handle is out of range");
    }
    if (saved_active_cursor_handle != 0U && !heap_handle_is_live(saved_active_cursor_handle)) {
        machine_panic("snapshot active cursor handle is out of range");
    }
    if (saved_active_cursor_visible > 1U) {
        machine_panic("snapshot active cursor visibility is invalid");
    }
    active_display_form_handle = saved_active_display_form_handle;
    active_cursor_handle = saved_active_cursor_handle;
    active_cursor_visible = saved_active_cursor_handle == 0U ? 0U : (uint8_t)saved_active_cursor_visible;
    active_cursor_screen_x = saved_active_cursor_x;
    active_cursor_screen_y = saved_active_cursor_y;
    for (handle = 0U; handle < 128U; ++handle) {
        glyph_bitmap_handles[handle] = read_u16_le(blob + offset);
        if (glyph_bitmap_handles[handle] != 0U && !heap_handle_is_live(glyph_bitmap_handles[handle])) {
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
        if (!heap_handle_is_live(definition->class_handle) ||
            !heap_handle_is_live(definition->superclass_handle)) {
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
        if (!heap_handle_is_live(named_objects[named_index].object_handle)) {
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
        live_method_sources[named_index].selector_id = read_u16_le(blob + offset);
        offset += 2U;
        live_method_sources[named_index].argument_count = blob[offset++];
        for (name_index = 0U; name_index < METHOD_SOURCE_NAME_LIMIT; ++name_index) {
            live_method_sources[named_index].protocol_name[name_index] = (char)blob[offset++];
        }
        live_method_sources[named_index].source_offset = read_u16_le(blob + offset);
        offset += 2U;
        live_method_sources[named_index].source_length = read_u16_le(blob + offset);
        offset += 2U;
        if (!heap_handle_is_live(live_method_sources[named_index].class_handle)) {
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
        uint16_t selector_id = read_u16_le(blob + offset + 4U);
        uint8_t argument_count = blob[offset + 6U];
        uint16_t text_length_value = read_u16_le(blob + offset + 7U);
        char literal_text[METHOD_SOURCE_CHUNK_LIMIT];
        uint32_t text_index;

        offset += SNAPSHOT_LIVE_STRING_LITERAL_RECORD_SIZE;
        if (slot_id == 0U || slot_id > LIVE_STRING_LITERAL_LIMIT) {
            machine_panic("snapshot live string literal slot is out of range");
        }
        if (!heap_handle_is_live(class_handle)) {
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
        if (!heap_handle_is_live(startup_hook_receiver_handle)) {
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
    if (heap_get_field(transcript_font_object(), FONT_FIELD_METRICS).kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)heap_get_field(transcript_font_object(), FONT_FIELD_METRICS).integer != transcript_metrics_handle) {
        machine_panic("snapshot transcript font does not point at transcript metrics");
    }
    if (heap_get_field(transcript_font_object(), FONT_FIELD_BEHAVIOR).kind != RECORZ_MVP_VALUE_OBJECT ||
        (uint16_t)heap_get_field(transcript_font_object(), FONT_FIELD_BEHAVIOR).integer != transcript_behavior_handle) {
        machine_panic("snapshot transcript font does not point at transcript behavior");
    }
    if (transcript_font_u32(FONT_FIELD_POINT_SIZE, "snapshot transcript font point size is not a small integer") == 0U) {
        machine_panic("snapshot transcript font point size must be non-zero");
    }
    validate_transcript_font_glyphs_reference();
    validate_transcript_text_metrics("snapshot transcript text metrics are inconsistent");
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
        RECORZ_MVP_TRANSFER_RULE_OVER
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
        RECORZ_MVP_TRANSFER_RULE_OVER
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
        RECORZ_MVP_TRANSFER_RULE_OVER
    );
    push(arguments[5]);
}

static void execute_entry_bitblt_draw_line_on_form_from_x_from_y_to_x_to_y_color(
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
        machine_panic("BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a form");
    }
    form = heap_object_for_value(arguments[0]);
    if (primitive_kind_for_heap_object(form) != RECORZ_MVP_OBJECT_FORM) {
        machine_panic("BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a form");
    }
    form_draw_line_color(
        form,
        small_integer_i32(arguments[1], "BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a small integer fromX"),
        small_integer_i32(arguments[2], "BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a small integer fromY"),
        small_integer_i32(arguments[3], "BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a small integer toX"),
        small_integer_i32(arguments[4], "BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a small integer toY"),
        small_integer_u32(arguments[5], "BitBlt drawLineOnForm:fromX:fromY:toX:toY:color: expects a non-negative small integer color")
    );
    push(arguments[0]);
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

static void execute_entry_form_write_styled_text(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *styled_text_object;
    const struct recorz_mvp_heap_object *style_object;
    struct recorz_mvp_value text_value;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("Form writeStyledText: expects a styled text object");
    }
    styled_text_object = heap_object_for_value(arguments[0]);
    if (styled_text_object->kind != RECORZ_MVP_OBJECT_STYLED_TEXT) {
        machine_panic("Form writeStyledText: expects a styled text object");
    }
    text_value = heap_get_field(styled_text_object, STYLED_TEXT_FIELD_TEXT);
    if (text_value.kind != RECORZ_MVP_VALUE_STRING || text_value.string == 0) {
        machine_panic("StyledText text is not a string");
    }
    style_object = text_style_object_for_value(
        heap_get_field(styled_text_object, STYLED_TEXT_FIELD_STYLE),
        "StyledText style is not a text style object"
    );
    {
        const char *cursor = text_value.string;
        uint32_t foreground_color = styled_text_foreground_color(style_object);
        uint32_t background_color = styled_text_background_color(style_object);

        while (*cursor != '\0') {
            form_write_code_point_with_colors(object, (uint8_t)*cursor, foreground_color, background_color);
            ++cursor;
        }
    }
    push(receiver);
}

static void execute_entry_form_write_code_point_color(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Form writeCodePoint:color: expects a small integer code point");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Form writeCodePoint:color: expects a small integer color");
    }
    form_write_code_point_with_colors(
        object,
        (uint8_t)arguments[0].integer,
        (uint32_t)arguments[1].integer,
        text_background_color()
    );
    push(receiver);
}

static void execute_entry_text_style_with_text(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t styled_text_handle;

    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("TextStyle withText: expects a string");
    }
    (void)text_style_object_for_value(receiver, "TextStyle withText: expects a text style receiver");
    styled_text_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_STYLED_TEXT);
    heap_set_field(styled_text_handle, STYLED_TEXT_FIELD_TEXT, arguments[0]);
    heap_set_field(styled_text_handle, STYLED_TEXT_FIELD_STYLE, receiver);
    push(object_value(styled_text_handle));
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

static void execute_entry_form_be_display(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    if (!form_can_be_display(object)) {
        machine_panic("Form beDisplay expects a framebuffer-backed form");
    }
    active_display_form_handle = heap_handle_for_object(object);
    push(receiver);
}

static void execute_entry_display_text_cursor_x(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(small_integer_value((int32_t)cursor_x));
}

static void execute_entry_display_text_cursor_y(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(small_integer_value((int32_t)cursor_y));
}

static void execute_entry_display_move_text_cursor_to_x_y(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)text;
    cursor_x = small_integer_u32(arguments[0], "Display moveTextCursorToX:y: expects a non-negative small integer x");
    cursor_y = small_integer_u32(arguments[1], "Display moveTextCursorToX:y: expects a non-negative small integer y");
    push(receiver);
}

static void execute_entry_cursor_factory_from_bits_hotspot_x_hotspot_y(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t cursor_handle;

    (void)object;
    (void)receiver;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("Cursor fromBits:hotspotX:hotspotY: expects a bitmap");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_SMALL_INTEGER ||
        arguments[2].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Cursor fromBits:hotspotX:hotspotY: expects small integer hotspot coordinates");
    }
    {
        const struct recorz_mvp_heap_object *bitmap = heap_object_for_value(arguments[0]);
        uint32_t storage_kind;

        if (bitmap->kind != RECORZ_MVP_OBJECT_BITMAP) {
            machine_panic("Cursor fromBits:hotspotX:hotspotY: expects a bitmap");
        }
        storage_kind = bitmap_storage_kind(bitmap);
        if (storage_kind != BITMAP_STORAGE_GLYPH_MONO && storage_kind != BITMAP_STORAGE_HEAP_MONO) {
            machine_panic("Cursor fromBits:hotspotX:hotspotY: expects a monochrome bitmap");
        }
    }
    cursor_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_CURSOR);
    heap_set_field(cursor_handle, CURSOR_FIELD_BITS, arguments[0]);
    heap_set_field(cursor_handle, CURSOR_FIELD_HOTSPOT_X, arguments[1]);
    heap_set_field(cursor_handle, CURSOR_FIELD_HOTSPOT_Y, arguments[2]);
    push(object_value(cursor_handle));
}

static void execute_entry_cursor_factory_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    if (active_cursor_handle == 0U) {
        push(nil_value());
        return;
    }
    push(object_value(active_cursor_handle));
}

static void execute_entry_cursor_factory_show(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)arguments;
    (void)text;
    active_cursor_set_visible(1U);
    push(receiver);
}

static void execute_entry_cursor_factory_hide(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)arguments;
    (void)text;
    active_cursor_set_visible(0U);
    push(receiver);
}

static void execute_entry_cursor_factory_visible(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(boolean_value(active_cursor_is_visible()));
}

static void execute_entry_cursor_factory_x(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(small_integer_value((int32_t)active_cursor_screen_x));
}

static void execute_entry_cursor_factory_y(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(small_integer_value((int32_t)active_cursor_screen_y));
}

static void execute_entry_cursor_factory_move_to_x_y(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)text;
    active_cursor_move_to(
        small_integer_u32(arguments[0], "Cursor moveToX:y: expects a non-negative x small integer"),
        small_integer_u32(arguments[1], "Cursor moveToX:y: expects a non-negative y small integer")
    );
    push(receiver);
}

static void execute_entry_cursor_be_cursor(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    (void)cursor_bitmap_object(object);
    (void)cursor_hotspot_u32(object, CURSOR_FIELD_HOTSPOT_X, "Cursor hotspotX must be a non-negative small integer");
    (void)cursor_hotspot_u32(object, CURSOR_FIELD_HOTSPOT_Y, "Cursor hotspotY must be a non-negative small integer");
    active_cursor_handle = heap_handle_for_object(object);
    push(receiver);
}

static void execute_entry_character_scanner_scan(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *form;
    const struct recorz_mvp_heap_object *style_object;
    const char *source_text;
    uint32_t source_length;
    uint32_t text_index;
    uint32_t x;
    uint32_t y;
    uint32_t right_margin;
    uint32_t selection_boundary;
    uint32_t cursor_boundary;
    uint32_t foreground_color;
    uint32_t background_color;

    (void)receiver;
    (void)text;
    if (object->kind != RECORZ_MVP_OBJECT_CHARACTER_SCANNER) {
        machine_panic("CharacterScanner scan expects a character scanner receiver");
    }
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("CharacterScanner scan expects a string");
    }
    if (arguments[5].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("CharacterScanner scan expects a form");
    }
    source_text = arguments[0].string;
    source_length = text_length(source_text);
    text_index = small_integer_u32(arguments[1], "CharacterScanner scan index must be a positive small integer");
    if (text_index == 0U) {
        machine_panic("CharacterScanner scan index must be one-based");
    }
    x = small_integer_u32(arguments[2], "CharacterScanner scan x must be a non-negative small integer");
    y = small_integer_u32(arguments[3], "CharacterScanner scan y must be a non-negative small integer");
    style_object = text_style_object_for_value(arguments[4], "CharacterScanner scan expects a text style");
    form = heap_object_for_value(arguments[5]);
    if (form->kind != RECORZ_MVP_OBJECT_FORM) {
        machine_panic("CharacterScanner scan expects a form");
    }
    right_margin = small_integer_u32(arguments[6], "CharacterScanner scan right margin must be a non-negative small integer");
    selection_boundary = small_integer_u32(
        arguments[7],
        "CharacterScanner scan selectionBoundary must be a non-negative small integer"
    );
    cursor_boundary = small_integer_u32(
        arguments[8],
        "CharacterScanner scan cursorBoundary must be a non-negative small integer"
    );
    foreground_color = styled_text_foreground_color(style_object);
    background_color = styled_text_background_color(style_object);

    while (text_index <= source_length) {
        uint8_t code_point = (uint8_t)source_text[text_index - 1U];

        if (selection_boundary != 0U && text_index == selection_boundary) {
            character_scanner_set_state(object, CHARACTER_SCANNER_STOP_SELECTION, text_index, x, y);
            push(receiver);
            return;
        }
        if (cursor_boundary != 0U && text_index == cursor_boundary) {
            character_scanner_set_state(object, CHARACTER_SCANNER_STOP_CURSOR, text_index, x, y);
            push(receiver);
            return;
        }
        if (code_point == (uint8_t)'\t') {
            character_scanner_set_state(object, CHARACTER_SCANNER_STOP_TAB, text_index, x, y);
            push(receiver);
            return;
        }
        if (code_point == (uint8_t)'\n') {
            character_scanner_set_state(object, CHARACTER_SCANNER_STOP_NEWLINE, text_index, x, y);
            push(receiver);
            return;
        }
        if (code_point < 32U) {
            character_scanner_set_state(object, CHARACTER_SCANNER_STOP_CONTROL, text_index, x, y);
            push(receiver);
            return;
        }
        if (right_margin != 0U && x + char_width() > right_margin) {
            character_scanner_set_state(object, CHARACTER_SCANNER_STOP_RIGHT_MARGIN, text_index, x, y);
            push(receiver);
            return;
        }
        form_draw_code_point_at_with_colors(form, code_point, x, y, foreground_color, background_color);
        x += char_width();
        text_index += 1U;
    }

    character_scanner_set_state(object, CHARACTER_SCANNER_STOP_END_OF_RUN, text_index, x, y);
    push(receiver);
}

static void execute_entry_class_new(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint16_t instance_handle;
    uint8_t instance_kind;
    uint8_t field_count;
    uint8_t field_index;

    (void)receiver;
    (void)arguments;
    (void)text;
    if (object->kind != RECORZ_MVP_OBJECT_CLASS) {
        machine_panic("Class new expects a class receiver");
    }
    instance_kind = (uint8_t)class_instance_kind(object);
    instance_handle = heap_allocate(instance_kind);
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

static uint8_t compile_source_identifier_is_global_binding(
    const struct recorz_mvp_heap_object *class_object,
    const char *name,
    const char argument_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t argument_count,
    const char temporary_names[][METHOD_SOURCE_NAME_LIMIT],
    uint16_t temporary_count,
    uint8_t expected_global_id
) {
    uint16_t argument_index;
    uint16_t temporary_index;
    uint8_t field_index;

    for (argument_index = 0U; argument_index < argument_count; ++argument_index) {
        if (argument_names[argument_index][0] != '\0' &&
            source_names_equal(name, argument_names[argument_index])) {
            return 0U;
        }
    }
    for (temporary_index = 0U; temporary_index < temporary_count; ++temporary_index) {
        if (temporary_names[temporary_index][0] != '\0' &&
            source_names_equal(name, temporary_names[temporary_index])) {
            return 0U;
        }
    }
    if (class_field_index_for_name(class_object, name, &field_index)) {
        return 0U;
    }
    return (uint8_t)(source_global_id_for_name(name) == expected_global_id);
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
    uint16_t selector_id;

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
    char special_selector[METHOD_SOURCE_NAME_LIMIT];
    const char *token_cursor;
    uint16_t selector_id;
    uint32_t root_id;

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
            if (compile_source_identifier_is_global_binding(
                    class_object,
                    token,
                    argument_names,
                    argument_count,
                    temporary_names,
                    temporary_count,
                    RECORZ_MVP_GLOBAL_DISPLAY)) {
                const char *selector_cursor = source_parse_identifier(
                    source_skip_horizontal_space(token_cursor),
                    special_selector,
                    sizeof(special_selector)
                );

                if (selector_cursor != 0) {
                    const char *after_selector = source_skip_horizontal_space(selector_cursor);

                    if (*after_selector != ':' &&
                        display_source_root_id_for_selector(special_selector, &root_id)) {
                        compile_source_append_instruction(
                            instruction_words,
                            instruction_count,
                            COMPILED_METHOD_OP_PUSH_ROOT,
                            (uint8_t)root_id,
                            0U
                        );
                        cursor = after_selector;
                        goto compile_source_primary_unary_tail;
                    }
                }
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
compile_source_primary_unary_tail:
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
    uint16_t selector_id;

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
    uint16_t *selector_id_out,
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
    uint16_t selector_id;
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
    uint16_t selector_id;

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
    if (argument_count == 1U &&
        receiver.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
        arguments[0].kind == RECORZ_MVP_VALUE_SMALL_INTEGER) {
        if (source_names_equal(selector_name, "/")) {
            if (arguments[0].integer == 0) {
                machine_panic("/ expects a non-zero small integer argument");
            }
            return source_eval_value_result(small_integer_value(receiver.integer / arguments[0].integer));
        }
        if (source_names_equal(selector_name, "%")) {
            if (arguments[0].integer == 0) {
                machine_panic("% expects a non-zero small integer argument");
            }
            return source_eval_value_result(small_integer_value(receiver.integer % arguments[0].integer));
        }
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
    char special_selector[METHOD_SOURCE_NAME_LIMIT];
    char block_source[METHOD_SOURCE_CHUNK_LIMIT];
    int32_t small_integer;
    uint32_t root_id;

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
        if (!context->is_block &&
            context->defining_class != 0 &&
            context->selector_id != 0U) {
            uint16_t literal_slot = remember_live_string_literal(
                heap_handle_for_object(context->defining_class),
                context->selector_id,
                context->argument_count,
                quoted_text
            );

            result = source_eval_value_result(string_value(live_string_literals[literal_slot - 1U].text));
        } else {
            result = source_eval_value_result(string_value(runtime_string_intern_copy(quoted_text)));
        }
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
    if (result.value.kind == RECORZ_MVP_VALUE_OBJECT &&
        (uint16_t)result.value.integer == global_handles[RECORZ_MVP_GLOBAL_DISPLAY]) {
        const char *selector_cursor = source_parse_identifier(cursor, special_selector, sizeof(special_selector));

        if (selector_cursor != 0) {
            const char *after_selector = source_skip_horizontal_space(selector_cursor);

            if (*after_selector != ':' &&
                display_source_root_id_for_selector(special_selector, &root_id)) {
                result = source_eval_value_result(seed_root_value(root_id));
                cursor = after_selector;
            }
        }
    }
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
    while (source_char_is_live_binary_selector(*cursor)) {
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
    if ((source_names_equal(selector_part, "ifTrue") || source_names_equal(selector_part, "ifFalse")) &&
        *source_skip_statement_space(part_cursor + 1) == '[') {
        return source_evaluate_conditional_expression(context, receiver_result, cursor, cursor_out);
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
    struct recorz_mvp_runtime_block_state block_state_storage;
    const struct recorz_mvp_runtime_block_state *block_state = 0;
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
    uint8_t allocate_context_object;

    if (primitive_kind_for_heap_object(object) != RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
        machine_panic("source block execution expects a block closure");
    }
    source_value = heap_get_field(object, BLOCK_CLOSURE_FIELD_SOURCE);
    if (source_value.kind != RECORZ_MVP_VALUE_STRING || source_value.string == 0) {
        machine_panic("block closure source field is invalid");
    }
    home_receiver = heap_get_field(object, BLOCK_CLOSURE_FIELD_HOME_RECEIVER);
    if (source_block_state_for_handle(heap_handle_for_object(object), &block_state_storage)) {
        block_state = &block_state_storage;
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
    allocate_context_object = source_text_contains_identifier(source_value.string, "thisContext");
    context.defining_class = defining_class;
    context.receiver = home_receiver;
    context.lexical_environment_index = lexical_environment_index;
    context.home_context_index = block_state == 0 ? -1 : block_state->home_context_index;
    context.current_context_handle = allocate_context_object != 0U ?
        allocate_source_context_object(
            sender_context_handle,
            home_receiver,
            "<block>"
        ) :
        0U;
    context.selector_id = 0U;
    context.argument_count = 0U;
    context.is_block = 1U;
    result = source_evaluate_statement_sequence(&context, body_cursor, 1U);
    if (context.current_context_handle != 0U) {
        heap_set_field(context.current_context_handle, CONTEXT_FIELD_ALIVE, boolean_value(0U));
    }
    if (result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
        if (context.home_context_index < 0) {
            machine_panic("block attempted a non-local return without a live home context");
        }
        if (!source_home_context_at(context.home_context_index)->alive) {
            machine_panic("block attempted a non-local return to a dead home context");
        }
    }
    source_release_lexical_environment_chain_if_unused(lexical_environment_index);
    return result;
}

static struct recorz_mvp_source_eval_result source_execute_inline_block_source(
    struct recorz_mvp_source_method_context *context,
    const char *source
) {
    struct recorz_mvp_source_method_context block_context;
    struct recorz_mvp_source_eval_result result;
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    uint16_t parsed_argument_count = 0U;
    const char *body_cursor;
    int16_t lexical_environment_index;
    uint8_t allocate_context_object;

    lexical_environment_index = source_allocate_lexical_environment(context->lexical_environment_index);
    body_cursor = source_parse_block_header(source, argument_names, &parsed_argument_count);
    if (parsed_argument_count != 0U) {
        machine_panic("inline conditional blocks do not support block arguments yet");
    }
    allocate_context_object = source_text_contains_identifier(source, "thisContext");
    block_context.defining_class = context->defining_class;
    block_context.receiver = context->receiver;
    block_context.lexical_environment_index = lexical_environment_index;
    block_context.home_context_index = context->home_context_index;
    block_context.current_context_handle = allocate_context_object != 0U ?
        allocate_source_context_object(
            context->current_context_handle,
            context->receiver,
            "<block>"
        ) :
        0U;
    block_context.selector_id = 0U;
    block_context.argument_count = 0U;
    block_context.is_block = 1U;
    result = source_evaluate_statement_sequence(&block_context, body_cursor, 1U);
    if (block_context.current_context_handle != 0U) {
        heap_set_field(block_context.current_context_handle, CONTEXT_FIELD_ALIVE, boolean_value(0U));
    }
    if (result.kind == RECORZ_MVP_SOURCE_EVAL_RETURN) {
        if (block_context.home_context_index < 0) {
            machine_panic("block attempted a non-local return without a live home context");
        }
        if (!source_home_context_at(block_context.home_context_index)->alive) {
            machine_panic("block attempted a non-local return to a dead home context");
        }
    }
    source_release_single_lexical_environment_if_unused(lexical_environment_index);
    return result;
}

static struct recorz_mvp_source_eval_result source_evaluate_conditional_expression(
    struct recorz_mvp_source_method_context *context,
    struct recorz_mvp_source_eval_result receiver_result,
    const char *cursor,
    const char **cursor_out
) {
    char first_selector[METHOD_SOURCE_NAME_LIMIT];
    char second_selector[METHOD_SOURCE_NAME_LIMIT];
    char first_block[METHOD_SOURCE_CHUNK_LIMIT];
    char second_block[METHOD_SOURCE_CHUNK_LIMIT];
    const char *part_cursor;
    const char *after_selector;
    const char *after_first_block;
    const char *after_second_block;
    const char *chosen_block = 0;
    uint8_t first_is_true;
    uint8_t has_second_clause = 0U;

    part_cursor = source_parse_identifier(cursor, first_selector, sizeof(first_selector));
    if (part_cursor == 0) {
        machine_panic("live source conditional expression is missing a selector");
    }
    after_selector = source_skip_statement_space(part_cursor);
    if (*after_selector != ':') {
        machine_panic("live source conditional expression is missing ':'");
    }
    after_selector = source_skip_statement_space(after_selector + 1);
    if (*after_selector != '[') {
        machine_panic("live source inline conditional expects a block argument");
    }
    after_first_block = source_copy_bracket_body(after_selector, first_block, sizeof(first_block));
    if (after_first_block == 0) {
        machine_panic("live source conditional expression is missing a block argument");
    }
    cursor = source_skip_statement_space(after_first_block);
    part_cursor = source_parse_identifier(cursor, second_selector, sizeof(second_selector));
    if (part_cursor != 0) {
        after_selector = source_skip_statement_space(part_cursor);
        if (*after_selector == ':' &&
            ((source_names_equal(first_selector, "ifTrue") && source_names_equal(second_selector, "ifFalse")) ||
             (source_names_equal(first_selector, "ifFalse") && source_names_equal(second_selector, "ifTrue")))) {
            after_selector = source_skip_statement_space(after_selector + 1);
            if (*after_selector != '[') {
                machine_panic("live source inline conditional matching clause expects a block argument");
            }
            after_second_block = source_copy_bracket_body(after_selector, second_block, sizeof(second_block));
            if (after_second_block == 0) {
                machine_panic("live source conditional expression is missing a matching block argument");
            }
            cursor = after_second_block;
            has_second_clause = 1U;
        }
    }

    first_is_true = (uint8_t)source_names_equal(first_selector, "ifTrue");
    if (condition_value_is_true(receiver_result.value) == first_is_true) {
        chosen_block = first_block;
    } else if (has_second_clause) {
        chosen_block = second_block;
    }
    *cursor_out = cursor;
    if (chosen_block == 0) {
        return source_eval_value_result(nil_value());
    }
    return source_execute_inline_block_source(context, chosen_block);
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
    uint8_t allocate_context_object;

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
    context.selector_id = source_selector_id_for_name(selector_name);
    if (context.selector_id == 0U) {
        machine_panic("live source method uses an unknown selector");
    }
    lexical_environment_index = source_allocate_lexical_environment(-1);
    for (argument_index = 0U; argument_index < argument_count; ++argument_index) {
        source_append_binding(lexical_environment_index, argument_names[argument_index], arguments[argument_index]);
    }
    allocate_context_object = source_text_contains_identifier(source, "thisContext");
    home_context_index = source_allocate_home_context(
        class_object,
        receiver,
        lexical_environment_index,
        sender_context_handle,
        selector_name,
        allocate_context_object
    );
    context.defining_class = class_object;
    context.receiver = receiver;
    context.lexical_environment_index = lexical_environment_index;
    context.home_context_index = home_context_index;
    context.current_context_handle = source_home_context_at(home_context_index)->context_handle;
    context.argument_count = (uint8_t)argument_count;
    context.is_block = 0U;
    result = source_evaluate_statement_sequence(&context, body_cursor, 0U);
    source_home_context_at(home_context_index)->alive = 0U;
    if (context.current_context_handle != 0U) {
        heap_set_field(context.current_context_handle, CONTEXT_FIELD_ALIVE, boolean_value(0U));
    }
    source_release_home_context_if_unused(home_context_index);
    source_release_lexical_environment_chain_if_unused(lexical_environment_index);
    push(result.value);
}

static uint8_t remember_seeded_primitive_method_source(
    const struct recorz_mvp_heap_object *class_object,
    const char *protocol_name,
    const char *chunk
) {
    char selector_name[METHOD_SOURCE_NAME_LIMIT];
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    const struct recorz_mvp_heap_object *method_object;
    const struct recorz_mvp_heap_object *entry_object;
    struct recorz_mvp_value implementation_value;
    const char *body_cursor;
    uint16_t argument_count;
    uint16_t selector_id;

    if (source_parse_method_header(
            chunk,
            selector_name,
            argument_names,
            &argument_count,
            &body_cursor) == 0) {
        machine_panic("KernelInstaller source method header is invalid");
    }
    body_cursor = source_skip_statement_space(body_cursor);
    if (!source_starts_with(body_cursor, "<primitive:")) {
        return 0U;
    }
    selector_id = source_selector_id_for_name(selector_name);
    if (selector_id == 0U) {
        machine_panic("KernelInstaller source method uses an unknown selector");
    }
    method_object = lookup_builtin_method_descriptor(class_object, selector_id, argument_count);
    if (method_object == 0) {
        machine_panic("KernelInstaller primitive method chunk does not match an installed method");
    }
    entry_object = method_descriptor_entry_object(method_object);
    implementation_value = method_entry_implementation_value(entry_object);
    if (implementation_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("KernelInstaller primitive method chunk does not match a primitive method");
    }
    remember_live_method_source(
        heap_handle_for_object(class_object),
        selector_id,
        (uint8_t)argument_count,
        protocol_name,
        chunk
    );
    return 1U;
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
        uint16_t selector_id;
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
        if (remember_seeded_primitive_method_source(class_object, current_protocol, chunk)) {
            ++installed_method_count;
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

static void file_in_class_source_on_existing_class(
    const char *source,
    const struct recorz_mvp_heap_object *class_object
) {
    const char *cursor = source;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    char current_protocol[METHOD_SOURCE_NAME_LIMIT];
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    struct recorz_mvp_live_class_definition definition;
    const struct recorz_mvp_heap_object *install_class_object = class_object;
    uint8_t saw_class_chunk = 0U;
    uint8_t installed_method_count = 0U;

    if (source == 0 || *source == '\0') {
        machine_panic("KernelInstaller class source is empty");
    }
    source_copy_identifier(class_name, sizeof(class_name), class_name_for_object(class_object));
    current_protocol[0] = '\0';
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        if (source_starts_with(chunk, "RecorzKernelPackage:")) {
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            source_parse_class_definition_from_chunk(chunk, &definition);
            if (!source_names_equal(definition.class_name, class_name)) {
                machine_panic("KernelInstaller class source target does not match the current class");
            }
            saw_class_chunk = 1U;
            install_class_object = class_object;
            current_protocol[0] = '\0';
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
            char class_side_name[METHOD_SOURCE_NAME_LIMIT];

            source_parse_class_side_name_from_chunk(chunk, class_side_name, sizeof(class_side_name));
            if (!source_names_equal(class_side_name, class_name)) {
                machine_panic("KernelInstaller class-side source target does not match the current class");
            }
            install_class_object = class_side_lookup_target(class_object);
            current_protocol[0] = '\0';
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelProtocol:")) {
            source_parse_protocol_name_from_chunk(chunk, current_protocol, sizeof(current_protocol));
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelBootObject:") ||
            source_starts_with(chunk, "RecorzKernelRoot:") ||
            source_starts_with(chunk, "RecorzKernelSelector:") ||
            source_starts_with(chunk, "RecorzKernelGlyphBitmapFamily:")) {
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelDoIt:")) {
            machine_panic("KernelInstaller class source browser does not accept do-it chunks");
        }
        if (install_class_object == 0) {
            machine_panic("KernelInstaller class source is missing an initial class chunk");
        }
        install_method_chunk_on_class(install_class_object, current_protocol, chunk);
        ++installed_method_count;
    }
    if (!saw_class_chunk || installed_method_count == 0U) {
        machine_panic("KernelInstaller class source did not install any methods");
    }
}

static void install_method_chunk_on_class(
    const struct recorz_mvp_heap_object *class_object,
    const char *protocol_name,
    const char *chunk
) {
    uint16_t compiled_method_handle;
    uint16_t selector_id;
    uint16_t argument_count;

    if (remember_seeded_primitive_method_source(class_object, protocol_name, chunk)) {
        return;
    }
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
    if (gc_collection_allowed_for_current_phase()) {
        (void)gc_collect_now();
    }
}

static void install_method_source_on_class(
    const struct recorz_mvp_heap_object *class_object,
    const char *source
) {
    uint16_t compiled_method_handle;
    uint16_t selector_id;
    uint16_t argument_count;

    compiled_method_handle = compile_source_method_and_allocate(class_object, source, &selector_id, &argument_count);
    validate_compiled_method(heap_object(compiled_method_handle), argument_count);
    install_compiled_method_update(class_object, selector_id, argument_count, compiled_method_handle);
    remember_live_method_source(heap_handle_for_object(class_object), selector_id, (uint8_t)argument_count, "", source);
    if (gc_collection_allowed_for_current_phase()) {
        (void)gc_collect_now();
    }
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
    const struct recorz_mvp_seed_class_source_record *seed_source;

    if (definition->class_name[0] == '\0') {
        machine_panic("dynamic class definition is missing a class name");
    }
    if (class_object != 0) {
        dynamic_definition = mutable_dynamic_class_definition_for_handle(heap_handle_for_object(class_object));
        if (dynamic_definition == 0) {
            seed_source = seed_class_source_record_for_name(definition->class_name);
            if (seed_source == 0) {
                machine_panic("KernelInstaller seeded class metadata is missing");
            }
            if (definition->instance_variable_count != seed_source->instance_variable_count) {
                machine_panic("KernelInstaller seeded class instance variables do not match class chunk");
            }
            for (class_handle = 0U; class_handle < definition->instance_variable_count; ++class_handle) {
                if (seed_source->instance_variable_names[class_handle] == 0 ||
                    !source_names_equal(
                        definition->instance_variable_names[class_handle],
                        seed_source->instance_variable_names[class_handle])) {
                    machine_panic("KernelInstaller seeded class instance variables do not match class chunk");
                }
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

static void append_seeded_class_header_source(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const struct recorz_mvp_seed_class_source_record *seed_source
) {
    uint8_t ivar_index;

    append_text_checked(buffer, buffer_size, offset, "RecorzKernelClass: #");
    append_text_checked(buffer, buffer_size, offset, seed_source->class_name);
    append_text_checked(buffer, buffer_size, offset, " descriptorOrder: ");
    render_small_integer((int32_t)seed_source->descriptor_order);
    append_text_checked(buffer, buffer_size, offset, print_buffer);
    append_text_checked(buffer, buffer_size, offset, " objectKindOrder: ");
    render_small_integer((int32_t)seed_source->object_kind_order);
    append_text_checked(buffer, buffer_size, offset, print_buffer);
    if (seed_source->source_boot_order >= 0) {
        append_text_checked(buffer, buffer_size, offset, " sourceBootOrder: ");
        render_small_integer((int32_t)seed_source->source_boot_order);
        append_text_checked(buffer, buffer_size, offset, print_buffer);
    }
    append_text_checked(buffer, buffer_size, offset, " instanceVariableNames: '");
    for (ivar_index = 0U; ivar_index < seed_source->instance_variable_count; ++ivar_index) {
        if (ivar_index != 0U) {
            append_char_checked(buffer, buffer_size, offset, ' ');
        }
        append_text_checked(buffer, buffer_size, offset, seed_source->instance_variable_names[ivar_index]);
    }
    append_char_checked(buffer, buffer_size, offset, '\'');
}

static uint8_t seed_class_side_has_source_chunks(
    const struct recorz_mvp_seed_class_source_record *seed_source,
    uint16_t class_handle,
    uint8_t class_side
) {
    const char *cursor;
    uint8_t scanning_class_side = 0U;
    char chunk[METHOD_SOURCE_CHUNK_LIMIT];
    char parsed_selector_name[METHOD_SOURCE_NAME_LIMIT];
    char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
    const char *body_cursor = 0;
    uint16_t argument_count = 0U;

    if (live_method_source_count_for_class_handle(class_handle) != 0U) {
        return 1U;
    }
    if (seed_source == 0 ||
        (seed_source->builder_source == 0 && seed_source->canonical_source == 0)) {
        return 0U;
    }
    cursor = seed_source->builder_source != 0 ? seed_source->builder_source : seed_source->canonical_source;
    while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            scanning_class_side = 0U;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
            scanning_class_side = 1U;
            continue;
        }
        if (scanning_class_side != class_side) {
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernel")) {
            continue;
        }
        if (source_parse_method_header(
                chunk,
                parsed_selector_name,
                argument_names,
                &argument_count,
                &body_cursor) != 0) {
            return 1U;
        }
    }
    return 0U;
}

static void append_class_method_source_chunks(
    char buffer[],
    uint32_t buffer_size,
    uint32_t *offset,
    const struct recorz_mvp_heap_object *class_object,
    const struct recorz_mvp_seed_class_source_record *seed_source,
    const char *class_name,
    uint8_t class_side,
    uint8_t *wrote_any_chunk
) {
    const struct recorz_mvp_heap_object *method_start_object = class_method_start_object(class_object);
    uint32_t method_count = class_method_count(class_object);
    char current_protocol[METHOD_SOURCE_NAME_LIMIT];
    uint32_t method_index;

    (void)class_name;
    current_protocol[0] = '\0';
    if (seed_source != 0) {
        const char *cursor = seed_source->builder_source != 0 ? seed_source->builder_source : seed_source->canonical_source;
        uint8_t scanning_class_side = 0U;
        char chunk[METHOD_SOURCE_CHUNK_LIMIT];
        char parsed_selector_name[METHOD_SOURCE_NAME_LIMIT];
        char argument_names[MAX_SEND_ARGS][METHOD_SOURCE_NAME_LIMIT];
        const char *body_cursor = 0;
        uint16_t argument_count = 0U;
        uint8_t emitted_live_sources[LIVE_METHOD_SOURCE_LIMIT];

        for (method_index = 0U; method_index < LIVE_METHOD_SOURCE_LIMIT; ++method_index) {
            emitted_live_sources[method_index] = 0U;
        }
        while (source_copy_next_chunk(&cursor, chunk, sizeof(chunk)) != 0U) {
            const struct recorz_mvp_live_method_source *source_record = 0;
            const char *method_source = chunk;
            const char *protocol_name;
            uint16_t selector_id = 0U;

            if (source_starts_with(chunk, "RecorzKernelClass:")) {
                scanning_class_side = 0U;
                current_protocol[0] = '\0';
                continue;
            }
            if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
                scanning_class_side = 1U;
                current_protocol[0] = '\0';
                continue;
            }
            if (scanning_class_side != class_side) {
                continue;
            }
            if (source_starts_with(chunk, "RecorzKernelProtocol:")) {
                source_parse_protocol_name_from_chunk(chunk, current_protocol, sizeof(current_protocol));
                continue;
            }
            if (source_starts_with(chunk, "RecorzKernel")) {
                append_chunk_text(buffer, buffer_size, offset, chunk, wrote_any_chunk);
                continue;
            }
            if (source_parse_method_header(
                    chunk,
                    parsed_selector_name,
                    argument_names,
                    &argument_count,
                    &body_cursor) == 0) {
                append_chunk_text(buffer, buffer_size, offset, chunk, wrote_any_chunk);
                continue;
            }
            selector_id = source_selector_id_for_name(parsed_selector_name);
            if (selector_id == 0U) {
                machine_panic("KernelInstaller fileOutClassNamed: canonical source uses an unknown selector");
            }
            source_record = live_method_source_for_selector_and_arity(
                heap_handle_for_object(class_object),
                selector_id,
                (uint8_t)argument_count
            );
            if (source_record != 0) {
                uint16_t source_index;

                method_source = live_method_source_text(source_record);
                for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
                    if (&live_method_sources[source_index] == source_record) {
                        emitted_live_sources[source_index] = 1U;
                        break;
                    }
                }
                if (source_record->protocol_name[0] != '\0') {
                    protocol_name = source_record->protocol_name;
                } else {
                    protocol_name = current_protocol[0] == '\0' ? "UNFILED" : current_protocol;
                }
            } else {
                protocol_name = current_protocol[0] == '\0' ? "UNFILED" : current_protocol;
            }
            if (!source_names_equal(current_protocol, protocol_name)) {
                source_copy_identifier(current_protocol, sizeof(current_protocol), protocol_name);
                if (current_protocol[0] != '\0' && !source_names_equal(current_protocol, "UNFILED")) {
                    append_protocol_chunk(buffer, buffer_size, offset, current_protocol, wrote_any_chunk);
                }
            }
            append_chunk_text(buffer, buffer_size, offset, method_source, wrote_any_chunk);
        }
        {
            const struct recorz_mvp_live_method_source *sorted_sources[LIVE_METHOD_SOURCE_LIMIT];
            uint16_t sorted_count = 0U;
            uint16_t source_index;

            for (source_index = 0U; source_index < live_method_source_count; ++source_index) {
                const struct recorz_mvp_live_method_source *source_record = &live_method_sources[source_index];
                uint16_t insert_index;

                if (source_record->class_handle != heap_handle_for_object(class_object) ||
                    emitted_live_sources[source_index]) {
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
                const char *protocol_name = sorted_sources[source_index]->protocol_name[0] == '\0'
                    ? "UNFILED"
                    : sorted_sources[source_index]->protocol_name;

                if (!source_names_equal(current_protocol, protocol_name)) {
                    source_copy_identifier(current_protocol, sizeof(current_protocol), protocol_name);
                    if (current_protocol[0] != '\0' && !source_names_equal(current_protocol, "UNFILED")) {
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
        return;
    }
    if (method_count == 0U) {
        return;
    }
    if (method_start_object == 0) {
        machine_panic("KernelInstaller fileOutClassNamed: method start is missing");
    }
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *method_object = (const struct recorz_mvp_heap_object *)heap_object(
            (uint16_t)(heap_handle_for_object(method_start_object) + method_index)
        );
        const struct recorz_mvp_live_method_source *source_record = live_method_source_for_selector_and_arity(
            heap_handle_for_object(class_object),
            method_descriptor_selector(method_object),
            (uint8_t)method_descriptor_argument_count(method_object)
        );
        const char *protocol_name;

        if (source_record == 0) {
            machine_panic("KernelInstaller fileOutClassNamed: method is missing source");
        }
        protocol_name = source_record->protocol_name[0] == '\0' ? "UNFILED" : source_record->protocol_name;
        if (!source_names_equal(current_protocol, protocol_name)) {
            source_copy_identifier(current_protocol, sizeof(current_protocol), protocol_name);
            if (current_protocol[0] != '\0' && !source_names_equal(current_protocol, "UNFILED")) {
                append_protocol_chunk(buffer, buffer_size, offset, current_protocol, wrote_any_chunk);
            }
        }
        append_chunk_text(
            buffer,
            buffer_size,
            offset,
            live_method_source_text(source_record),
            wrote_any_chunk
        );
    }
}

static const struct recorz_mvp_live_method_source *live_method_source_for_selector_and_arity(
    uint16_t class_handle,
    uint16_t selector_id,
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

static const char *file_out_class_source_text(
    const char *class_name,
    char buffer[],
    uint32_t buffer_size
) {
    const struct recorz_mvp_heap_object *class_object;
    const struct recorz_mvp_dynamic_class_definition *dynamic_definition;
    const struct recorz_mvp_seed_class_source_record *seed_source = 0;
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
        seed_source = &recorz_mvp_generated_seed_class_sources[class_instance_kind(class_object)];
        if (seed_source->class_name == 0) {
            machine_panic("KernelInstaller fileOutClassNamed: seeded class metadata is missing");
        }
    }
    metaclass_object = class_side_lookup_target(class_object);
    metaclass_handle = heap_handle_for_object(metaclass_object);
    if (dynamic_definition != 0 &&
        live_method_source_count_for_class_handle(class_handle) != class_method_count(class_object)) {
        machine_panic("KernelInstaller fileOutClassNamed: class contains methods without live source");
    }
    buffer[0] = '\0';
    if (dynamic_definition != 0) {
        append_dynamic_class_header_source(
            buffer,
            buffer_size,
            &offset,
            dynamic_definition
        );
    } else {
        append_seeded_class_header_source(
            buffer,
            buffer_size,
            &offset,
            seed_source
        );
    }
    append_text_checked(buffer, buffer_size, &offset, "\n!\n");
    append_class_method_source_chunks(
        buffer,
        buffer_size,
        &offset,
        class_object,
        seed_source,
        dynamic_definition != 0 ? dynamic_definition->class_name : seed_source->class_name,
        0U,
        &wrote_any_chunk
    );

    if (dynamic_definition != 0 &&
        live_method_source_count_for_class_handle(metaclass_handle) != class_method_count(metaclass_object)) {
        machine_panic("KernelInstaller fileOutClassNamed: metaclass contains methods without live source");
    }
    if ((dynamic_definition != 0 && class_method_count(metaclass_object) != 0U) ||
        (dynamic_definition == 0 && seed_class_side_has_source_chunks(seed_source, metaclass_handle, 1U))) {
        append_text_checked(
            buffer,
            buffer_size,
            &offset,
            "RecorzKernelClassSide: #"
        );
        append_text_checked(
            buffer,
            buffer_size,
            &offset,
            dynamic_definition != 0 ? dynamic_definition->class_name : seed_source->class_name
        );
        append_text_checked(buffer, buffer_size, &offset, "\n!\n");
        append_class_method_source_chunks(
            buffer,
            buffer_size,
            &offset,
            metaclass_object,
            seed_source,
            dynamic_definition != 0 ? dynamic_definition->class_name : seed_source->class_name,
            1U,
            &wrote_any_chunk
        );
    }
    return buffer;
}

static const char *file_out_class_source_by_name(const char *class_name) {
    return runtime_string_intern_copy(
        file_out_class_source_text(class_name, kernel_source_io_buffer, sizeof(kernel_source_io_buffer))
    );
}

static const char *file_out_package_source_text(
    const char *package_name,
    char buffer[],
    uint32_t buffer_size
) {
    const struct recorz_mvp_live_package_definition *package_definition;
    const struct recorz_mvp_dynamic_class_definition *sorted_definitions[DYNAMIC_CLASS_LIMIT];
    uint16_t sorted_count = 0U;
    uint16_t dynamic_index;
    uint16_t do_it_index;
    uint32_t offset = 0U;

    if (package_name == 0 || package_name[0] == '\0') {
        machine_panic("KernelInstaller fileOutPackageNamed: package name is empty");
    }
    package_definition = package_definition_for_name(package_name);
    if (package_definition == 0) {
        machine_panic("KernelInstaller fileOutPackageNamed: could not resolve package");
    }
    buffer[0] = '\0';
    append_text_checked(buffer, buffer_size, &offset, "RecorzKernelPackage: '");
    append_text_checked(buffer, buffer_size, &offset, package_name);
    append_char_checked(buffer, buffer_size, &offset, '\'');
    if (package_definition->package_comment[0] != '\0') {
        append_text_checked(buffer, buffer_size, &offset, " comment: '");
        append_text_checked(buffer, buffer_size, &offset, package_definition->package_comment);
        append_char_checked(buffer, buffer_size, &offset, '\'');
    }
    append_text_checked(buffer, buffer_size, &offset, "\n!\n");
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
        const char *class_source = file_out_class_source_text(
            definition->class_name,
            kernel_source_io_buffer,
            sizeof(kernel_source_io_buffer)
        );

        append_text_checked(buffer, buffer_size, &offset, class_source);
        if (dynamic_index + 1U < sorted_count &&
            buffer[offset - 1U] != '\n') {
            append_char_checked(buffer, buffer_size, &offset, '\n');
        }
    }
    for (do_it_index = 0U; do_it_index < live_package_do_it_source_count; ++do_it_index) {
        const struct recorz_mvp_live_package_do_it_source *source_record = &live_package_do_it_sources[do_it_index];

        if (!source_names_equal(source_record->package_name, package_name)) {
            continue;
        }
        if (offset != 0U && buffer[offset - 1U] != '\n') {
            append_char_checked(buffer, buffer_size, &offset, '\n');
        }
        append_text_checked(
            buffer,
            buffer_size,
            &offset,
            live_package_do_it_source_text(source_record)
        );
        if (buffer[offset - 1U] != '\n') {
            append_char_checked(buffer, buffer_size, &offset, '\n');
        }
        append_text_checked(buffer, buffer_size, &offset, "!\n");
    }
    return buffer;
}

static const char *file_out_package_source_by_name(const char *package_name) {
    return runtime_string_intern_copy(
        file_out_package_source_text(package_name, package_source_io_buffer, sizeof(package_source_io_buffer))
    );
}

struct recorz_mvp_regenerated_boot_source_emitter {
    uint32_t emitted_size;
    uint8_t serial_mode;
    uint8_t line_open;
    uint8_t line_bytes;
    const char *data_label;
    char *buffer;
    uint32_t buffer_capacity;
};

static void emit_regenerated_boot_source_hex_byte(uint8_t value) {
    static const char digits[] = "0123456789abcdef";

    machine_putc(digits[(value >> 4U) & 0x0FU]);
    machine_putc(digits[value & 0x0FU]);
}

static void regenerated_boot_source_emit_byte(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter,
    char ch
) {
    if (emitter->buffer != 0) {
        if (emitter->emitted_size + 1U >= emitter->buffer_capacity) {
            machine_panic("regenerated source exceeds buffer capacity");
        }
        emitter->buffer[emitter->emitted_size] = ch;
        emitter->buffer[emitter->emitted_size + 1U] = '\0';
    }
    if (emitter->serial_mode) {
        if (!emitter->line_open) {
            machine_puts(emitter->data_label);
            emitter->line_open = 1U;
            emitter->line_bytes = 0U;
        }
        emit_regenerated_boot_source_hex_byte((uint8_t)ch);
        ++emitter->line_bytes;
        if (emitter->line_bytes == 32U) {
            machine_putc('\n');
            emitter->line_open = 0U;
            emitter->line_bytes = 0U;
        }
    }
    ++emitter->emitted_size;
}

static void regenerated_boot_source_emit_text(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter,
    const char *text
) {
    uint32_t index = 0U;

    while (text[index] != '\0') {
        regenerated_boot_source_emit_byte(emitter, text[index++]);
    }
}

static void regenerated_boot_source_emit_quoted_string(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter,
    const char *text
) {
    uint32_t index = 0U;

    while (text[index] != '\0') {
        if (text[index] == '\'') {
            regenerated_boot_source_emit_byte(emitter, '\'');
        }
        regenerated_boot_source_emit_byte(emitter, text[index]);
        ++index;
    }
}

static void regenerated_boot_source_emit_view_restore(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter,
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value view_kind_value =
        heap_get_field(workspace_object, workspace_current_view_kind_field_index(workspace_object));
    struct recorz_mvp_value target_name_value =
        heap_get_field(workspace_object, workspace_current_target_name_field_index(workspace_object));
    char class_name[METHOD_SOURCE_NAME_LIMIT];
    char selector_name_text[METHOD_SOURCE_NAME_LIMIT];
    char protocol_name[METHOD_SOURCE_NAME_LIMIT];

    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        regenerated_boot_source_emit_text(emitter, "Workspace reopen.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_OPENING_MENU) {
        regenerated_boot_source_emit_text(emitter, "Workspace developmentHome.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASSES) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClasses.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_INTERACTIVE_CLASSES) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClasses.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_PACKAGES) {
        regenerated_boot_source_emit_text(emitter, "Workspace browsePackages.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_REGENERATED_BOOT_SOURCE) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseRegeneratedBootSource.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_REGENERATED_KERNEL_SOURCE) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseRegeneratedKernelSource.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_REGENERATED_FILE_IN_SOURCE) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseRegeneratedFileInSource.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_INPUT_MONITOR) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseInteractiveInput.\n");
        return;
    }
    if (target_name_value.kind != RECORZ_MVP_VALUE_STRING ||
        target_name_value.string == 0 ||
        target_name_value.string[0] == '\0') {
        regenerated_boot_source_emit_text(emitter, "Workspace reopen.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_METHODS) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseMethodsForClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_PACKAGE) {
        regenerated_boot_source_emit_text(emitter, "Workspace browsePackageNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_PROTOCOLS) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseProtocolsForClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASS) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_OBJECT) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseObjectNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHODS) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClassMethodsForClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOLS) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClassProtocolsForClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASS_SOURCE) {
        regenerated_boot_source_emit_text(emitter, "Workspace fileOutClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_PACKAGE_SOURCE) {
        regenerated_boot_source_emit_text(emitter, "Workspace fileOutPackageNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, target_name_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_METHOD &&
        workspace_parse_method_target_name(
            target_name_value.string,
            class_name,
            sizeof(class_name),
            selector_name_text,
            sizeof(selector_name_text))) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseMethod: '");
        regenerated_boot_source_emit_quoted_string(emitter, selector_name_text);
        regenerated_boot_source_emit_text(emitter, "' ofClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, class_name);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASS_METHOD &&
        workspace_parse_method_target_name(
            target_name_value.string,
            class_name,
            sizeof(class_name),
            selector_name_text,
            sizeof(selector_name_text))) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClassMethod: '");
        regenerated_boot_source_emit_quoted_string(emitter, selector_name_text);
        regenerated_boot_source_emit_text(emitter, "' ofClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, class_name);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_PROTOCOL &&
        workspace_parse_protocol_target_name(
            target_name_value.string,
            class_name,
            sizeof(class_name),
            protocol_name,
            sizeof(protocol_name))) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseProtocol: '");
        regenerated_boot_source_emit_quoted_string(emitter, protocol_name);
        regenerated_boot_source_emit_text(emitter, "' ofClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, class_name);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    if (view_kind_value.integer == WORKSPACE_VIEW_CLASS_PROTOCOL &&
        workspace_parse_protocol_target_name(
            target_name_value.string,
            class_name,
            sizeof(class_name),
            protocol_name,
            sizeof(protocol_name))) {
        regenerated_boot_source_emit_text(emitter, "Workspace browseClassProtocol: '");
        regenerated_boot_source_emit_quoted_string(emitter, protocol_name);
        regenerated_boot_source_emit_text(emitter, "' ofClassNamed: '");
        regenerated_boot_source_emit_quoted_string(emitter, class_name);
        regenerated_boot_source_emit_text(emitter, "'.\n");
        return;
    }
    regenerated_boot_source_emit_text(emitter, "Workspace reopen.\n");
}

static void regenerated_boot_source_emit_kernel_source(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter,
    uint8_t quote_units
) {
    uint8_t emitted_any_unit = 0U;
    uint16_t descriptor_order;

    for (descriptor_order = 0U; descriptor_order <= MAX_OBJECT_KIND; ++descriptor_order) {
        uint16_t seed_kind;

        for (seed_kind = 1U; seed_kind <= MAX_OBJECT_KIND; ++seed_kind) {
            const struct recorz_mvp_seed_class_source_record *seed_source =
                &recorz_mvp_generated_seed_class_sources[seed_kind];

            if (seed_source->class_name == 0 ||
                seed_source->descriptor_order != descriptor_order) {
                continue;
            }
            if (emitted_any_unit) {
                regenerated_boot_source_emit_text(emitter, "\n");
            }
            if (quote_units) {
                regenerated_boot_source_emit_quoted_string(
                    emitter,
                    file_out_class_source_text(
                        seed_source->class_name,
                        kernel_source_io_buffer,
                        sizeof(kernel_source_io_buffer))
                );
            } else {
                regenerated_boot_source_emit_text(
                    emitter,
                    file_out_class_source_text(
                        seed_source->class_name,
                        kernel_source_io_buffer,
                        sizeof(kernel_source_io_buffer))
                );
            }
            emitted_any_unit = 1U;
        }
    }
}

static void regenerated_boot_source_emit_boot_seed_source(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter
) {
    uint8_t emitted_any_unit = 0U;
    int16_t max_boot_order = -1;
    uint16_t object_kind;

    for (object_kind = 1U; object_kind <= MAX_OBJECT_KIND; ++object_kind) {
        const struct recorz_mvp_seed_class_source_record *seed_source = &recorz_mvp_generated_seed_class_sources[object_kind];

        if (seed_source->class_name != 0 &&
            seed_source->source_boot_order > max_boot_order) {
            max_boot_order = seed_source->source_boot_order;
        }
    }
    if (max_boot_order >= 0) {
        for (object_kind = 0U; object_kind <= (uint16_t)max_boot_order; ++object_kind) {
            uint16_t seed_kind;

            for (seed_kind = 1U; seed_kind <= MAX_OBJECT_KIND; ++seed_kind) {
                const struct recorz_mvp_seed_class_source_record *seed_source =
                    &recorz_mvp_generated_seed_class_sources[seed_kind];

                if (seed_source->class_name == 0 ||
                    seed_source->source_boot_order != (int16_t)object_kind) {
                    continue;
                }
                if (emitted_any_unit) {
                    regenerated_boot_source_emit_text(emitter, "\n");
                }
                regenerated_boot_source_emit_quoted_string(
                    emitter,
                    file_out_class_source_text(
                        seed_source->class_name,
                        kernel_source_io_buffer,
                        sizeof(kernel_source_io_buffer))
                );
                emitted_any_unit = 1U;
            }
        }
    }
}

static void regenerated_boot_source_emit_system_source(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter
) {
    const struct recorz_mvp_dynamic_class_definition *sorted_unpackaged[DYNAMIC_CLASS_LIMIT];
    const struct recorz_mvp_live_package_definition *sorted_packages[PACKAGE_LIMIT];
    uint16_t unpackaged_count = 0U;
    uint16_t sorted_package_count = 0U;
    uint16_t dynamic_index;
    uint16_t package_index;
    uint8_t emitted_any_unit = 0U;

    regenerated_boot_source_emit_boot_seed_source(emitter);
    if (emitter->emitted_size != 0U) {
        emitted_any_unit = 1U;
    }
    for (dynamic_index = 0U; dynamic_index < dynamic_class_count; ++dynamic_index) {
        const struct recorz_mvp_dynamic_class_definition *definition = &dynamic_classes[dynamic_index];
        uint16_t insert_index;

        if (definition->package_name[0] != '\0') {
            continue;
        }
        insert_index = unpackaged_count;
        while (insert_index != 0U &&
               compare_source_names(
                   definition->class_name,
                   sorted_unpackaged[insert_index - 1U]->class_name) < 0) {
            sorted_unpackaged[insert_index] = sorted_unpackaged[insert_index - 1U];
            --insert_index;
        }
        sorted_unpackaged[insert_index] = definition;
        ++unpackaged_count;
    }
    for (dynamic_index = 0U; dynamic_index < unpackaged_count; ++dynamic_index) {
        if (emitted_any_unit) {
            regenerated_boot_source_emit_text(emitter, "\n");
        }
        regenerated_boot_source_emit_quoted_string(
            emitter,
            file_out_class_source_text(
                sorted_unpackaged[dynamic_index]->class_name,
                kernel_source_io_buffer,
                sizeof(kernel_source_io_buffer))
        );
        emitted_any_unit = 1U;
    }
    for (package_index = 0U; package_index < package_count; ++package_index) {
        const struct recorz_mvp_live_package_definition *definition = &live_packages[package_index];
        uint16_t insert_index;

        if (definition->package_name[0] == '\0') {
            continue;
        }
        insert_index = sorted_package_count;
        while (insert_index != 0U &&
               compare_source_names(
                   definition->package_name,
                   sorted_packages[insert_index - 1U]->package_name) < 0) {
            sorted_packages[insert_index] = sorted_packages[insert_index - 1U];
            --insert_index;
        }
        sorted_packages[insert_index] = definition;
        ++sorted_package_count;
    }
    for (package_index = 0U; package_index < sorted_package_count; ++package_index) {
        if (emitted_any_unit) {
            regenerated_boot_source_emit_text(emitter, "\n");
        }
        regenerated_boot_source_emit_quoted_string(
            emitter,
            file_out_package_source_text(
                sorted_packages[package_index]->package_name,
                package_source_io_buffer,
                sizeof(package_source_io_buffer))
        );
        emitted_any_unit = 1U;
    }
}

static void regenerated_boot_source_emit_program(
    struct recorz_mvp_regenerated_boot_source_emitter *emitter,
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_value current_source_value = nil_value();

    if (workspace_object->field_count > workspace_current_source_field_index(workspace_object)) {
        current_source_value = heap_get_field(workspace_object, workspace_current_source_field_index(workspace_object));
    }

    regenerated_boot_source_emit_text(emitter, "Workspace fileIn: '");
    regenerated_boot_source_emit_system_source(emitter);
    regenerated_boot_source_emit_text(emitter, "'.\n");
    regenerated_boot_source_emit_view_restore(emitter, workspace_object);
    if (current_source_value.kind == RECORZ_MVP_VALUE_STRING &&
        current_source_value.string != 0 &&
        current_source_value.string[0] != '\0') {
        regenerated_boot_source_emit_text(emitter, "Workspace setContents: '");
        regenerated_boot_source_emit_quoted_string(emitter, current_source_value.string);
        regenerated_boot_source_emit_text(emitter, "'.\n");
    }
}

static void emit_regenerated_boot_source(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_regenerated_boot_source_emitter counter = {
        0U, 0U, 0U, 0U, "recorz-regenerated-boot-source-data ", 0, 0U
    };
    struct recorz_mvp_regenerated_boot_source_emitter emitter = {
        0U, 1U, 0U, 0U, "recorz-regenerated-boot-source-data ", 0, 0U
    };

    regenerated_boot_source_emit_program(&counter, workspace_object);
    machine_puts("recorz-regenerated-boot-source-begin ");
    panic_put_u32(counter.emitted_size);
    machine_putc('\n');
    regenerated_boot_source_emit_program(&emitter, workspace_object);
    if (emitter.line_open) {
        machine_putc('\n');
    }
    if (emitter.emitted_size != counter.emitted_size) {
        machine_panic("regenerated boot source size mismatch");
    }
    machine_puts("recorz-regenerated-boot-source-end\n");
}

static void emit_regenerated_kernel_source(void) {
    struct recorz_mvp_regenerated_boot_source_emitter counter = {
        0U, 0U, 0U, 0U, "recorz-regenerated-kernel-source-data ", 0, 0U
    };
    struct recorz_mvp_regenerated_boot_source_emitter emitter = {
        0U, 1U, 0U, 0U, "recorz-regenerated-kernel-source-data ", 0, 0U
    };

    regenerated_boot_source_emit_kernel_source(&counter, 0U);
    machine_puts("recorz-regenerated-kernel-source-begin ");
    panic_put_u32(counter.emitted_size);
    machine_putc('\n');
    regenerated_boot_source_emit_kernel_source(&emitter, 0U);
    if (emitter.line_open) {
        machine_putc('\n');
    }
    if (emitter.emitted_size != counter.emitted_size) {
        machine_panic("regenerated kernel source size mismatch");
    }
    machine_puts("recorz-regenerated-kernel-source-end\n");
}

static const char *regenerated_boot_source_text(
    const struct recorz_mvp_heap_object *workspace_object
) {
    struct recorz_mvp_regenerated_boot_source_emitter emitter = {
        0U,
        0U,
        0U,
        0U,
        0,
        regenerated_source_io_buffer,
        sizeof(regenerated_source_io_buffer)
    };

    regenerated_source_io_buffer[0] = '\0';
    regenerated_boot_source_emit_program(&emitter, workspace_object);
    return regenerated_source_io_buffer;
}

static const char *regenerated_kernel_source_text(void) {
    struct recorz_mvp_regenerated_boot_source_emitter emitter = {
        0U,
        0U,
        0U,
        0U,
        0,
        regenerated_source_io_buffer,
        sizeof(regenerated_source_io_buffer)
    };

    regenerated_source_io_buffer[0] = '\0';
    regenerated_boot_source_emit_kernel_source(&emitter, 0U);
    return regenerated_source_io_buffer;
}

static const char *regenerated_file_in_source_text(void) {
    return "RecorzKernelDoIt:\nWorkspace emitRegeneratedBootSource.\n";
}

static const char *development_home_initial_source_text(void) {
    return "Workspace developmentHome.";
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
    forget_live_method_source(heap_handle_for_object(class_object), selector_id, (uint8_t)argument_count);
    install_compiled_method_update(
        class_object,
        selector_id,
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
    uint16_t install_class_handle = 0U;
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
            clear_live_package_do_it_sources_for_package(package_definition.package_name);
            current_protocol[0] = '\0';
            ++package_chunk_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClass:")) {
            struct recorz_mvp_live_class_definition definition;
            const struct recorz_mvp_heap_object *class_object;

            source_parse_class_definition_from_chunk(chunk, &definition);
            if (definition.package_name[0] != '\0') {
                source_copy_identifier(current_package, sizeof(current_package), definition.package_name);
            }
            class_object = ensure_class_defined(&definition);
            install_class_handle = heap_handle_for_object(class_object);
            current_protocol[0] = '\0';
            ++class_header_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelClassSide:")) {
            char class_name[METHOD_SOURCE_NAME_LIMIT];
            const struct recorz_mvp_heap_object *class_object;

            source_parse_class_side_name_from_chunk(chunk, class_name, sizeof(class_name));
            class_object = lookup_class_by_name(class_name);
            if (class_object == 0) {
                machine_panic("KernelInstaller class-side chunk could not resolve class");
            }
            install_class_handle = heap_handle_for_object(ensure_dedicated_metaclass_for_class(
                class_object,
                class_superclass_object_or_null(class_object)
            ));
            current_protocol[0] = '\0';
            ++class_header_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelProtocol:")) {
            if (install_class_handle == 0U) {
                machine_panic("KernelInstaller protocol chunk has no active class side");
            }
            source_parse_protocol_name_from_chunk(chunk, current_protocol, sizeof(current_protocol));
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelDoIt:")) {
            workspace_evaluate_source(source_parse_do_it_chunk_body(chunk));
            remember_live_package_do_it_source(current_package, chunk);
            current_protocol[0] = '\0';
            ++do_it_chunk_count;
            continue;
        }
        if (source_starts_with(chunk, "RecorzKernelBootObject:") ||
            source_starts_with(chunk, "RecorzKernelRoot:") ||
            source_starts_with(chunk, "RecorzKernelSelector:") ||
            source_starts_with(chunk, "RecorzKernelGlyphBitmapFamily:")) {
            continue;
        }
        if (install_class_handle == 0U) {
            machine_panic("KernelInstaller file-in stream is missing an initial RecorzKernelClass chunk");
        }
        install_method_chunk_on_class(heap_object(install_class_handle), current_protocol, chunk);
    }
    if (class_header_count == 0U && do_it_chunk_count == 0U && package_chunk_count == 0U) {
        machine_panic("KernelInstaller file-in stream contains no package, class, or do-it chunks");
    }
    if (gc_collection_allowed_for_current_phase()) {
        (void)gc_collect_now();
    }
}

static void apply_external_file_in_blob(const uint8_t *blob, uint32_t size) {
    uint32_t index;

    if (blob == 0 || size == 0U) {
        return;
    }
    if (size >= sizeof(file_in_source_io_buffer)) {
        machine_panic("external file-in payload exceeds buffer capacity");
    }
    for (index = 0U; index < size; ++index) {
        if (blob[index] == '\0') {
            machine_panic("external file-in payload contains an unexpected NUL byte");
        }
        file_in_source_io_buffer[index] = (char)blob[index];
    }
    file_in_source_io_buffer[size] = '\0';
    file_in_chunk_stream_source(file_in_source_io_buffer);
}

static void execute_entry_kernel_installer_remember_object_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *remembered_object;

    (void)object;
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_OBJECT) {
        machine_panic("KernelInstaller rememberObject:named: expects an object");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("KernelInstaller rememberObject:named: expects a name string");
    }
    remembered_object = heap_object_for_value(arguments[0]);
    if (remembered_object->kind == RECORZ_MVP_OBJECT_BLOCK_CLOSURE) {
        heap_set_field((uint16_t)arguments[0].integer, BLOCK_CLOSURE_FIELD_LEXICAL0, small_integer_value(-1));
        heap_set_field((uint16_t)arguments[0].integer, BLOCK_CLOSURE_FIELD_LEXICAL1, small_integer_value(-1));
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
    uint16_t selector_id;

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

static void execute_entry_test_runner_run_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;

    (void)text;
    validate_test_runner_receiver(object);
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("TestRunner runClassNamed: expects a class name string");
    }
    class_object = lookup_class_by_name(arguments[0].string);
    if (class_object == 0) {
        machine_panic("TestRunner runClassNamed: could not resolve class");
    }
    test_runner_run_class(object, class_object, arguments[0].string);
    push(receiver);
}

static void execute_entry_test_runner_run_package_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    validate_test_runner_receiver(object);
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("TestRunner runPackageNamed: expects a package name string");
    }
    test_runner_run_package(object, arguments[0].string);
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

static void execute_entry_workspace_edit_package_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace editPackageNamed: expects a package name string");
    }
    workspace_edit_package_in_place(object, arguments[0].string);
    push(receiver);
}

static void execute_entry_workspace_edit_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_edit_current_in_place(object);
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
    if (workspace_current_source_is_editor_target(object)) {
        workspace_remember_editor_current_source(object, arguments[0].string);
    } else {
        workspace_remember_current_source(object, arguments[0].string);
    }
    push(receiver);
}

static void execute_entry_workspace_set_current_view_kind(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        machine_panic("Workspace setCurrentViewKind: expects a small integer");
    }
    heap_set_field(
        heap_handle_for_object(object),
        workspace_current_view_kind_field_index(object),
        arguments[0]
    );
    push(receiver);
}

static void execute_entry_workspace_set_current_target_name(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_NIL &&
        (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0)) {
        machine_panic("Workspace setCurrentTargetName: expects a string or nil");
    }
    heap_set_field(
        heap_handle_for_object(object),
        workspace_current_target_name_field_index(object),
        arguments[0].kind == RECORZ_MVP_VALUE_NIL ? nil_value() : arguments[0]
    );
    push(receiver);
}

static void execute_entry_workspace_package_count(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(small_integer_value((int32_t)workspace_package_count()));
}

static void execute_entry_workspace_class_count(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)receiver;
    (void)arguments;
    (void)text;
    push(small_integer_value((int32_t)workspace_class_count()));
}

static void execute_entry_workspace_class_name_at(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const char *class_name;
    uint32_t index;

    (void)object;
    (void)receiver;
    (void)text;
    index = small_integer_u32(
        arguments[0],
        "Workspace classNameAt: expects a positive small integer"
    );
    class_name = workspace_class_name_at_index(index);
    if (class_name == 0) {
        push(string_value(""));
        return;
    }
    push(string_value(class_name));
}

static void execute_entry_workspace_class_names_visible_from_count(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint32_t first_index;
    uint32_t count;

    (void)object;
    (void)receiver;
    (void)text;
    first_index = small_integer_u32(
        arguments[0],
        "Workspace classNamesVisibleFrom:count: expects a positive first index"
    );
    count = small_integer_u32(
        arguments[1],
        "Workspace classNamesVisibleFrom:count: expects a non-negative line count"
    );
    push(string_value(runtime_string_intern_copy(
        workspace_class_names_visible_text(
            first_index,
            count,
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer)
        ))));
}

static void execute_entry_workspace_package_name_at(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const char *package_name;
    uint32_t index;

    (void)object;
    (void)receiver;
    (void)text;
    index = small_integer_u32(
        arguments[0],
        "Workspace packageNameAt: expects a positive small integer"
    );
    package_name = workspace_package_name_at_index(index);
    if (package_name == 0) {
        push(string_value(""));
        return;
    }
    push(string_value(package_name));
}

static void execute_entry_workspace_package_names_visible_from_count(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint32_t first_index;
    uint32_t count;

    (void)object;
    (void)receiver;
    (void)text;
    first_index = small_integer_u32(
        arguments[0],
        "Workspace packageNamesVisibleFrom:count: expects a positive first index"
    );
    count = small_integer_u32(
        arguments[1],
        "Workspace packageNamesVisibleFrom:count: expects a non-negative line count"
    );
    push(string_value(runtime_string_intern_copy(
        workspace_package_names_visible_text(
            first_index,
            count,
            workspace_surface_list_buffer,
            sizeof(workspace_surface_list_buffer)
        ))));
}

static void execute_entry_workspace_visible_contents_top_lines_columns(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint32_t top_line;
    uint32_t line_count;
    uint32_t column_count;

    (void)receiver;
    (void)text;
    top_line = small_integer_u32(
        arguments[0],
        "Workspace visibleContentsTop:lines:columns: expects a non-negative top line"
    );
    line_count = small_integer_u32(
        arguments[1],
        "Workspace visibleContentsTop:lines:columns: expects a non-negative line count"
    );
    column_count = small_integer_u32(
        arguments[2],
        "Workspace visibleContentsTop:lines:columns: expects a non-negative column count"
    );
    push(string_value(runtime_string_intern_copy(
        workspace_visible_contents_text(
            object,
            top_line,
            0U,
            line_count,
            column_count,
            workspace_surface_editor_buffer,
            sizeof(workspace_surface_editor_buffer)
        ))));
}

static void execute_entry_workspace_visible_contents_top_left_lines_columns(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint32_t top_line;
    uint32_t left_column;
    uint32_t line_count;
    uint32_t column_count;

    (void)receiver;
    (void)text;
    top_line = small_integer_u32(
        arguments[0],
        "Workspace visibleContentsTop:left:lines:columns: expects a non-negative top line"
    );
    left_column = small_integer_u32(
        arguments[1],
        "Workspace visibleContentsTop:left:lines:columns: expects a non-negative left column"
    );
    line_count = small_integer_u32(
        arguments[2],
        "Workspace visibleContentsTop:left:lines:columns: expects a non-negative line count"
    );
    column_count = small_integer_u32(
        arguments[3],
        "Workspace visibleContentsTop:left:lines:columns: expects a non-negative column count"
    );
    push(string_value(runtime_string_intern_copy(
        workspace_visible_contents_text(
            object,
            top_line,
            left_column,
            line_count,
            column_count,
            workspace_surface_editor_buffer,
            sizeof(workspace_surface_editor_buffer)
        ))));
}

static void execute_entry_workspace_move_cursor_left(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_move_cursor_left_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_move_cursor_right(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_move_cursor_right_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_move_cursor_up(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_move_cursor_up_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_move_cursor_down(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_move_cursor_down_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_move_cursor_to_line_start(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_move_cursor_to_line_start_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_move_cursor_to_line_end(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_move_cursor_to_line_end_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_insert_code_point(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    uint32_t code_point;

    (void)text;
    code_point = small_integer_u32(
        arguments[0],
        "Workspace insertCodePoint: expects a non-negative small integer"
    );
    if (code_point > 255U) {
        machine_panic("Workspace insertCodePoint: code point exceeds byte range");
    }
    workspace_insert_code_point_in_current_source(object, (uint8_t)code_point);
    push(receiver);
}

static void execute_entry_workspace_delete_backward(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_delete_backward_in_current_source(object);
    push(receiver);
}

static void execute_entry_workspace_seed_boot_contents(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace seedBootContents: expects a source string");
    }
    if (!booted_from_snapshot) {
        workspace_remember_current_source(object, arguments[0].string);
    }
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
    (void)arguments;
    (void)text;
    workspace_accept_current_in_place(object);
    push(receiver);
}

static void execute_entry_workspace_revert_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_revert_current_in_place(object);
    push(receiver);
}

static void execute_entry_workspace_run_current_tests(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_run_current_tests_in_place(object);
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

static void execute_entry_workspace_browse_packages_interactive(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_run_interactive_image_session(object, 2U);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    const char *method_source;
    uint16_t selector_id;

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
    workspace_capture_plain_return_state_if_needed(object);
    method_source = workspace_method_source_text_for_browser_target(
        class_object,
        arguments[1].string,
        arguments[0].string,
        0U
    );
    workspace_remember_editor_current_source(object, method_source);
    workspace_remember_source(object, method_source);
    workspace_remember_view(
        object,
        WORKSPACE_VIEW_METHOD,
        workspace_compose_method_target_name(arguments[1].string, arguments[0].string)
    );
    workspace_render_method_browser(object, arguments[1].string, arguments[0].string, "INST");
    push(receiver);
}

static void execute_entry_workspace_edit_method_of_class_named(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    const struct recorz_mvp_heap_object *class_object;
    const char *method_source;
    uint16_t selector_id;

    (void)text;
    if (arguments[0].kind != RECORZ_MVP_VALUE_STRING || arguments[0].string == 0) {
        machine_panic("Workspace editMethod:ofClassNamed: expects a selector name string");
    }
    if (arguments[1].kind != RECORZ_MVP_VALUE_STRING || arguments[1].string == 0) {
        machine_panic("Workspace editMethod:ofClassNamed: expects a class name string");
    }
    selector_id = source_selector_id_for_name(arguments[0].string);
    if (selector_id == 0U) {
        machine_panic("Workspace editMethod:ofClassNamed: selector is not declared");
    }
    class_object = lookup_class_by_name(arguments[1].string);
    if (class_object == 0) {
        machine_panic("Workspace editMethod:ofClassNamed: could not resolve class");
    }
    if (lookup_builtin_method_descriptor(class_object, selector_id, 0U) == 0 &&
        lookup_builtin_method_descriptor(class_object, selector_id, 1U) == 0) {
        machine_panic("Workspace editMethod:ofClassNamed: could not resolve method");
    }
    workspace_capture_plain_return_state_if_needed(object);
    method_source = workspace_method_source_text_for_browser_target(
        class_object,
        arguments[1].string,
        arguments[0].string,
        0U
    );
    workspace_remember_editor_current_source(object, method_source);
    workspace_remember_source(object, method_source);
    workspace_remember_view(
        object,
        WORKSPACE_VIEW_METHOD,
        workspace_compose_method_target_name(arguments[1].string, arguments[0].string)
    );
    workspace_render_method_browser(object, arguments[1].string, arguments[0].string, "INST");
    workspace_run_interactive_input_monitor(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    workspace_capture_plain_return_state_if_needed(object);
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
    const char *method_source;
    uint16_t selector_id;

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
    workspace_capture_plain_return_state_if_needed(object);
    method_source = workspace_method_source_text_for_browser_target(
        metaclass_object,
        arguments[1].string,
        arguments[0].string,
        1U
    );
    workspace_remember_editor_current_source(object, method_source);
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

static void execute_entry_workspace_save_and_reopen(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_save_and_reopen_in_place(object);
    push(receiver);
}

static void execute_entry_workspace_save_and_rerun(
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
        machine_panic("Workspace saveAndRerun has no remembered source");
    }
    startup_hook_receiver_handle = heap_handle_for_object(object);
    startup_hook_selector_id = RECORZ_MVP_SELECTOR_RERUN;
    emit_live_snapshot();
    push(receiver);
}

static void execute_entry_workspace_save_recovery_snapshot(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_save_recovery_snapshot_in_place(object);
    push(receiver);
}

static void execute_entry_workspace_recover_last_source(
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
        machine_panic("Workspace recoverLastSource has no remembered source");
    }
    workspace_remember_current_source(object, source_value.string);
    push(receiver);
}

static void execute_entry_workspace_emit_regenerated_boot_source(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    emit_regenerated_kernel_source();
    emit_regenerated_boot_source(object);
    push(receiver);
}

static void execute_entry_workspace_browse_regenerated_boot_source(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_capture_plain_return_state_if_needed(object);
    workspace_remember_current_source(object, 0);
    workspace_remember_view(object, WORKSPACE_VIEW_REGENERATED_BOOT_SOURCE, 0);
    workspace_render_regenerated_source_browser(object, "BOOT");
    push(receiver);
}

static void execute_entry_workspace_browse_regenerated_kernel_source(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_capture_plain_return_state_if_needed(object);
    workspace_remember_current_source(object, 0);
    workspace_remember_view(object, WORKSPACE_VIEW_REGENERATED_KERNEL_SOURCE, 0);
    workspace_render_regenerated_source_browser(object, "KERNEL");
    push(receiver);
}

static void execute_entry_workspace_browse_regenerated_file_in_source(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_capture_plain_return_state_if_needed(object);
    workspace_remember_current_source(object, 0);
    workspace_remember_view(object, WORKSPACE_VIEW_REGENERATED_FILE_IN_SOURCE, 0);
    workspace_render_regenerated_source_browser(object, "FILEIN");
    push(receiver);
}

static void execute_entry_workspace_development_home(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value source_value;
    struct recorz_mvp_value view_kind_value;
    uint8_t seed_initial_source = 0U;

    (void)arguments;
    (void)text;
    source_value = workspace_current_source_value(object);
    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (source_value.kind != RECORZ_MVP_VALUE_STRING ||
        source_value.string == 0 ||
        source_value.string[0] == '\0') {
        seed_initial_source = 1U;
    } else if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER ||
               (uint32_t)view_kind_value.integer != WORKSPACE_VIEW_INPUT_MONITOR) {
        if (source_starts_with(source_value.string, "Display clear.\nWorkspace developmentHome.")) {
            seed_initial_source = 1U;
        }
    }
    if (seed_initial_source) {
        workspace_remember_current_source(object, development_home_initial_source_text());
        heap_set_field(
            heap_handle_for_object(object),
            workspace_current_view_kind_field_index(object),
            small_integer_value((int32_t)WORKSPACE_VIEW_OPENING_MENU)
        );
        heap_set_field(
            heap_handle_for_object(object),
            workspace_current_target_name_field_index(object),
            nil_value()
        );
    } else if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER ||
               (uint32_t)view_kind_value.integer == WORKSPACE_VIEW_NONE) {
        heap_set_field(
            heap_handle_for_object(object),
            workspace_current_view_kind_field_index(object),
            small_integer_value((int32_t)WORKSPACE_VIEW_OPENING_MENU)
        );
        heap_set_field(
            heap_handle_for_object(object),
            workspace_current_target_name_field_index(object),
            nil_value()
        );
    }
    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
        workspace_view_kind_uses_image_session((uint32_t)view_kind_value.integer)) {
        workspace_present_image_session(
            object,
            workspace_image_session_mode_for_view_kind((uint32_t)view_kind_value.integer)
        );
    } else {
        workspace_present_image_session(object, 0U);
    }
    push(receiver);
}

static void execute_entry_workspace_browse_interactive_input(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_present_image_session(object, 1U);
    push(receiver);
}

static void execute_entry_workspace_browse_interactive_views(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_run_interactive_views(object);
    push(receiver);
}

static void execute_entry_workspace_interactive_input_monitor(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    struct recorz_mvp_value view_kind_value;

    (void)arguments;
    (void)text;
    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind == RECORZ_MVP_VALUE_SMALL_INTEGER &&
        workspace_view_kind_uses_image_session((uint32_t)view_kind_value.integer)) {
        workspace_run_interactive_image_session(
            object,
            workspace_image_session_mode_for_view_kind((uint32_t)view_kind_value.integer)
        );
    } else {
        workspace_run_interactive_image_session(object, 1U);
    }
    push(receiver);
}

static void workspace_reopen_in_place(
    const struct recorz_mvp_heap_object *object
) {
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    const struct recorz_mvp_heap_object *form = default_form_object();
    const struct recorz_mvp_heap_object *class_object;

    view_kind_value = heap_get_field(object, workspace_current_view_kind_field_index(object));
    if (view_kind_value.kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
        form_clear(form);
        form_write_string(form, "WORKSPACE");
        form_newline(form);
        return;
    }
    if (workspace_view_kind_uses_image_session((uint32_t)view_kind_value.integer)) {
        workspace_present_image_session(
            object,
            workspace_image_session_mode_for_view_kind((uint32_t)view_kind_value.integer)
        );
        return;
    }
    if ((uint32_t)view_kind_value.integer == WORKSPACE_VIEW_CLASSES) {
        workspace_render_class_list_browser(object);
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
        return;
    }
    class_object = lookup_class_by_name(target_name_value.string);
    if (class_object == 0) {
        machine_panic("Workspace reopen could not resolve the remembered class");
    }
    workspace_render_class_browser(object, class_object, target_name_value.string);
}

static void execute_entry_workspace_reopen(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)arguments;
    (void)text;
    workspace_reopen_in_place(object);
    push(receiver);
}

static void execute_entry_workspace_tool_print_current(
    const struct recorz_mvp_heap_object *object,
    struct recorz_mvp_value receiver,
    const struct recorz_mvp_value arguments[],
    const char *text
) {
    (void)object;
    (void)arguments;
    (void)text;
    workspace_tool_print_current_in_place(workspace_global_object());
    push(receiver);
}

static void execute_entry_workspace_tool_named_object_or_nil(
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
        machine_panic("WorkspaceTool namedObjectOrNil: expects a name string");
    }
    object_handle = named_object_handle_for_name(arguments[0].string);
    if (object_handle == 0U) {
        push(nil_value());
        return;
    }
    push(object_value(object_handle));
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
                struct recorz_mvp_runtime_block_state block_state_storage;
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
                    if (source_block_state_for_handle(
                            heap_handle_for_object(heap_object_for_value(send_receiver)),
                            &block_state_storage)) {
                        block_state = &block_state_storage;
                    }
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
                        source_release_lexical_environment_chain_if_unused(shared_lexical_environment_index);
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
                    if (source_block_state_for_handle(
                            heap_handle_for_object(heap_object_for_value(send_arguments[chosen_index])),
                            &block_state_storage)) {
                        block_state = &block_state_storage;
                    } else {
                        block_state = 0;
                    }
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
                        source_release_lexical_environment_chain_if_unused(shared_lexical_environment_index);
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
                source_release_lexical_environment_chain_if_unused(shared_lexical_environment_index);
                return;
            case RECORZ_MVP_OP_RETURN_RECEIVER:
                push(receiver);
                source_release_lexical_environment_chain_if_unused(shared_lexical_environment_index);
                return;
            default:
                machine_panic("unknown opcode in MVP VM");
        }
    }

    machine_panic("executable did not return");
}

static struct recorz_mvp_value workspace_evaluate_source(const char *source) {
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
    struct recorz_mvp_value result;
    int16_t home_context_index = source_allocate_home_context(
        class_object_for_heap_object(workspace_receiver_object),
        workspace_receiver,
        -1,
        0U,
        "<workspace>",
        1U
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
    source_release_home_context_if_unused(home_context_index);
    if (stack_size != stack_size_before + 1U) {
        machine_panic("Workspace source execution did not return exactly one value");
    }
    result = pop_value();
    if (gc_collection_allowed_for_current_phase()) {
        gc_collect_preserving_value(result);
    }
    return result;
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
    uint16_t selector,
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
    uint16_t selector,
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

static void initialize_method_descriptor(
    uint16_t descriptor_handle,
    uint8_t class_kind,
    uint16_t selector,
    uint16_t argument_count,
    uint16_t entry_handle
) {
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
}

static uint16_t allocate_method_descriptor(
    uint8_t class_kind,
    uint16_t selector,
    uint16_t argument_count,
    uint16_t entry_handle
) {
    uint16_t descriptor_handle = heap_allocate_seeded_class(RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR);

    initialize_method_descriptor(descriptor_handle, class_kind, selector, argument_count, entry_handle);
    return descriptor_handle;
}

static void clone_method_descriptor_into_handle(
    uint16_t cloned_handle,
    const struct recorz_mvp_heap_object *method_object
) {
    uint8_t field_index;

    for (field_index = 0U; field_index < method_object->field_count; ++field_index) {
        heap_set_field(cloned_handle, field_index, heap_get_field(method_object, field_index));
    }
}

static void append_compiled_method_to_class(
    const struct recorz_mvp_heap_object *class_object,
    uint16_t selector,
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
    new_method_start_handle = heap_allocate_seeded_class_run(
        RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR,
        (uint16_t)(method_count + 1U)
    );
    for (method_index = 0U; method_index < method_count; ++method_index) {
        const struct recorz_mvp_heap_object *existing_method_object =
            (const struct recorz_mvp_heap_object *)heap_object((uint16_t)(heap_handle_for_object(method_start_object) + method_index));

        if (existing_method_object->kind != RECORZ_MVP_OBJECT_METHOD_DESCRIPTOR) {
            machine_panic("class method range contains a non-method descriptor");
        }
        clone_method_descriptor_into_handle((uint16_t)(new_method_start_handle + method_index), existing_method_object);
    }
    initialize_method_descriptor(
        (uint16_t)(new_method_start_handle + method_count),
        (uint8_t)class_kind,
        selector,
        argument_count,
        entry_handle
    );
    heap_set_field(class_handle, CLASS_FIELD_METHOD_START, object_value(new_method_start_handle));
    heap_set_field(class_handle, CLASS_FIELD_METHOD_COUNT, small_integer_value((int32_t)(method_count + 1U)));
}

static void install_compiled_method_update(
    const struct recorz_mvp_heap_object *class_object,
    uint16_t selector,
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
    forget_live_string_literals(heap_handle_for_object(class_object), selector, (uint8_t)argument_count);
    forget_live_method_source(heap_handle_for_object(class_object), selector, (uint8_t)argument_count);
    install_compiled_method_update(class_object, selector, argument_count, compiled_method_handle);
    (void)gc_collect_now();
}

static const struct recorz_mvp_live_method_source *live_method_source_for_class_chain(
    const struct recorz_mvp_heap_object *class_object,
    uint16_t selector_id,
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
    uint16_t selector,
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
    if (entry_object->kind != RECORZ_MVP_OBJECT_METHOD_ENTRY) {
        machine_panic("method descriptor entry does not point at a method entry");
    }
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
    uint16_t selector,
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
        if (selector == RECORZ_MVP_SELECTOR_SIZE) {
            push(small_integer_value((int32_t)text_length(receiver.string == 0 ? "" : receiver.string)));
            return;
        }
        if (selector == RECORZ_MVP_SELECTOR_AT) {
            uint32_t index;

            if (arguments[0].kind != RECORZ_MVP_VALUE_SMALL_INTEGER) {
                machine_panic("String at: expects a small integer argument");
            }
            if (receiver.string == 0) {
                machine_panic("String at: receiver is null");
            }
            if (arguments[0].integer <= 0) {
                machine_panic("String at: index must be positive");
            }
            index = (uint32_t)arguments[0].integer - 1U;
            if (index >= text_length(receiver.string)) {
                machine_panic("String at: index is out of range");
            }
            push(small_integer_value((int32_t)(uint8_t)receiver.string[index]));
            return;
        }
        machine_panic("unsupported String selector");
    }

    machine_panic("unsupported receiver in MVP VM");
}

static void perform_send(
    struct recorz_mvp_value receiver,
    uint16_t selector,
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
    if (!heap_handle_is_live(startup_hook_receiver_handle)) {
        machine_panic("startup hook receiver is out of range");
    }
    if (startup_hook_selector_id > MAX_SELECTOR_ID) {
        machine_panic("startup hook selector is out of range");
    }
    receiver = object_value(startup_hook_receiver_handle);
    stack_size = 0U;
    panic_phase = "startup";
    perform_send(receiver, startup_hook_selector_id, 0U, 0, 0);
    if (stack_size == 0U) {
        machine_panic("startup hook did not return a value");
    }
    --stack_size;
    (void)gc_collect_now();
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
    uintptr_t stack_base_marker = 0U;
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
    gc_stack_base_address = (uintptr_t)&stack_base_marker;
    if (snapshot_blob != 0 && snapshot_size != 0U) {
        panic_phase = "snapshot";
        load_snapshot_state(snapshot_blob, snapshot_size);
        booted_from_snapshot = 1U;
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
        gc_bootstrap_file_in_active = 1U;
        apply_external_file_in_blob(file_in_blob, file_in_size);
        gc_bootstrap_file_in_active = 0U;
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
        "<program>",
        1U
    );
    context_handle = source_home_context_at(home_context_index)->context_handle;
    executable.block_defining_class = class_object_for_heap_object(top_level_receiver_object);
    executable.home_context_index = home_context_index;
    execute_executable(&executable, top_level_receiver_object, top_level_receiver, 0U, 0, context_handle);
    source_home_context_at(home_context_index)->alive = 0U;
    mark_context_dead(context_handle);
    source_release_home_context_if_unused(home_context_index);
    panic_phase = "return";
    machine_set_panic_hook(0);
}
