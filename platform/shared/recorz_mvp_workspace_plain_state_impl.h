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
    uint16_t session_handle;
    uint16_t tool_handle;
    struct recorz_mvp_value view_kind_value;
    struct recorz_mvp_value target_name_value;
    struct recorz_mvp_value current_source_value;
    struct recorz_mvp_value arguments[1];

    if (workspace_object == 0) {
        return;
    }
    session_handle = named_object_handle_for_name("BootWorkspaceSession");
    if (session_handle != 0U) {
        (void)perform_send_and_pop_result(
            object_value(session_handle),
            RECORZ_MVP_SELECTOR_REMEMBER_PLAIN_WORKSPACE_STATE_IF_NEEDED,
            0U,
            0,
            0
        );
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
