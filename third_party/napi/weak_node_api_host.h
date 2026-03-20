// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_NAPI_WEAK_NODE_API_HOST_H_
#define THIRD_PARTY_NAPI_WEAK_NODE_API_HOST_H_

namespace lynx {
namespace napi {

/**
 * @struct NodeApiRawPtrHost
 * @brief Raw pointer version of NodeApiHost struct.
 *
 * This struct has the same members as NodeApiHost but all are void* type,
 * initialized to nullptr. This can be useful for avoiding symbol conflicts
 * between weak_node_api and primjs napi in the same source file.
 */
struct NodeApiRawPtrHost {
  void* napi_get_last_error_info_rawptr = nullptr;
  void* napi_get_undefined_rawptr = nullptr;
  void* napi_get_null_rawptr = nullptr;
  void* napi_get_global_rawptr = nullptr;
  void* napi_get_boolean_rawptr = nullptr;
  void* napi_create_object_rawptr = nullptr;
  void* napi_create_array_rawptr = nullptr;
  void* napi_create_array_with_length_rawptr = nullptr;
  void* napi_create_double_rawptr = nullptr;
  void* napi_create_int32_rawptr = nullptr;
  void* napi_create_uint32_rawptr = nullptr;
  void* napi_create_int64_rawptr = nullptr;
  void* napi_create_string_latin1_rawptr = nullptr;
  void* napi_create_string_utf8_rawptr = nullptr;
  void* napi_create_string_utf16_rawptr = nullptr;
  void* napi_create_symbol_rawptr = nullptr;
  void* napi_create_function_rawptr = nullptr;
  void* napi_create_error_rawptr = nullptr;
  void* napi_create_type_error_rawptr = nullptr;
  void* napi_create_range_error_rawptr = nullptr;
  void* napi_typeof_rawptr = nullptr;
  void* napi_get_value_double_rawptr = nullptr;
  void* napi_get_value_int32_rawptr = nullptr;
  void* napi_get_value_uint32_rawptr = nullptr;
  void* napi_get_value_int64_rawptr = nullptr;
  void* napi_get_value_bool_rawptr = nullptr;
  void* napi_get_value_string_latin1_rawptr = nullptr;
  void* napi_get_value_string_utf8_rawptr = nullptr;
  void* napi_get_value_string_utf16_rawptr = nullptr;
  void* napi_coerce_to_bool_rawptr = nullptr;
  void* napi_coerce_to_number_rawptr = nullptr;
  void* napi_coerce_to_object_rawptr = nullptr;
  void* napi_coerce_to_string_rawptr = nullptr;
  void* napi_get_prototype_rawptr = nullptr;
  void* napi_get_property_names_rawptr = nullptr;
  void* napi_set_property_rawptr = nullptr;
  void* napi_has_property_rawptr = nullptr;
  void* napi_get_property_rawptr = nullptr;
  void* napi_delete_property_rawptr = nullptr;
  void* napi_has_own_property_rawptr = nullptr;
  void* napi_set_named_property_rawptr = nullptr;
  void* napi_has_named_property_rawptr = nullptr;
  void* napi_get_named_property_rawptr = nullptr;
  void* napi_set_element_rawptr = nullptr;
  void* napi_has_element_rawptr = nullptr;
  void* napi_get_element_rawptr = nullptr;
  void* napi_delete_element_rawptr = nullptr;
  void* napi_define_properties_rawptr = nullptr;
  void* napi_is_array_rawptr = nullptr;
  void* napi_get_array_length_rawptr = nullptr;
  void* napi_strict_equals_rawptr = nullptr;
  void* napi_call_function_rawptr = nullptr;
  void* napi_new_instance_rawptr = nullptr;
  void* napi_instanceof_rawptr = nullptr;
  void* napi_get_cb_info_rawptr = nullptr;
  void* napi_get_new_target_rawptr = nullptr;
  void* napi_define_class_rawptr = nullptr;
  void* napi_wrap_rawptr = nullptr;
  void* napi_unwrap_rawptr = nullptr;
  void* napi_remove_wrap_rawptr = nullptr;
  void* napi_create_external_rawptr = nullptr;
  void* napi_get_value_external_rawptr = nullptr;
  void* napi_create_reference_rawptr = nullptr;
  void* napi_delete_reference_rawptr = nullptr;
  void* napi_reference_ref_rawptr = nullptr;
  void* napi_reference_unref_rawptr = nullptr;
  void* napi_get_reference_value_rawptr = nullptr;
  void* napi_open_handle_scope_rawptr = nullptr;
  void* napi_close_handle_scope_rawptr = nullptr;
  void* napi_open_escapable_handle_scope_rawptr = nullptr;
  void* napi_close_escapable_handle_scope_rawptr = nullptr;
  void* napi_escape_handle_rawptr = nullptr;
  void* napi_throw_rawptr = nullptr;
  void* napi_throw_error_rawptr = nullptr;
  void* napi_throw_type_error_rawptr = nullptr;
  void* napi_throw_range_error_rawptr = nullptr;
  void* napi_is_error_rawptr = nullptr;
  void* napi_is_exception_pending_rawptr = nullptr;
  void* napi_get_and_clear_last_exception_rawptr = nullptr;
  void* napi_is_arraybuffer_rawptr = nullptr;
  void* napi_create_arraybuffer_rawptr = nullptr;
  void* napi_create_external_arraybuffer_rawptr = nullptr;
  void* napi_get_arraybuffer_info_rawptr = nullptr;
  void* napi_is_typedarray_rawptr = nullptr;
  void* napi_create_typedarray_rawptr = nullptr;
  void* napi_get_typedarray_info_rawptr = nullptr;
  void* napi_create_dataview_rawptr = nullptr;
  void* napi_is_dataview_rawptr = nullptr;
  void* napi_get_dataview_info_rawptr = nullptr;
  void* napi_get_version_rawptr = nullptr;
  void* napi_create_promise_rawptr = nullptr;
  void* napi_resolve_deferred_rawptr = nullptr;
  void* napi_reject_deferred_rawptr = nullptr;
  void* napi_is_promise_rawptr = nullptr;
  void* napi_run_script_rawptr = nullptr;
  void* napi_adjust_external_memory_rawptr = nullptr;
  void* napi_create_date_rawptr = nullptr;
  void* napi_is_date_rawptr = nullptr;
  void* napi_get_date_value_rawptr = nullptr;
  void* napi_add_finalizer_rawptr = nullptr;
  void* napi_create_bigint_int64_rawptr = nullptr;
  void* napi_create_bigint_uint64_rawptr = nullptr;
  void* napi_create_bigint_words_rawptr = nullptr;
  void* napi_get_value_bigint_int64_rawptr = nullptr;
  void* napi_get_value_bigint_uint64_rawptr = nullptr;
  void* napi_get_value_bigint_words_rawptr = nullptr;
  void* napi_get_all_property_names_rawptr = nullptr;
  void* napi_set_instance_data_rawptr = nullptr;
  void* napi_get_instance_data_rawptr = nullptr;
  void* napi_detach_arraybuffer_rawptr = nullptr;
  void* napi_is_detached_arraybuffer_rawptr = nullptr;
  void* napi_type_tag_object_rawptr = nullptr;
  void* napi_check_object_type_tag_rawptr = nullptr;
  void* napi_object_freeze_rawptr = nullptr;
  void* napi_object_seal_rawptr = nullptr;
  void* napi_module_register_rawptr = nullptr;
  void* napi_fatal_error_rawptr = nullptr;
  void* napi_async_init_rawptr = nullptr;
  void* napi_async_destroy_rawptr = nullptr;
  void* napi_make_callback_rawptr = nullptr;
  void* napi_create_buffer_rawptr = nullptr;
  void* napi_create_external_buffer_rawptr = nullptr;
  void* napi_create_buffer_copy_rawptr = nullptr;
  void* napi_is_buffer_rawptr = nullptr;
  void* napi_get_buffer_info_rawptr = nullptr;
  void* napi_create_async_work_rawptr = nullptr;
  void* napi_delete_async_work_rawptr = nullptr;
  void* napi_queue_async_work_rawptr = nullptr;
  void* napi_cancel_async_work_rawptr = nullptr;
  void* napi_get_node_version_rawptr = nullptr;
  void* napi_get_uv_event_loop_rawptr = nullptr;
  void* napi_fatal_exception_rawptr = nullptr;
  void* napi_add_env_cleanup_hook_rawptr = nullptr;
  void* napi_remove_env_cleanup_hook_rawptr = nullptr;
  void* napi_open_callback_scope_rawptr = nullptr;
  void* napi_close_callback_scope_rawptr = nullptr;
  void* napi_create_threadsafe_function_rawptr = nullptr;
  void* napi_get_threadsafe_function_context_rawptr = nullptr;
  void* napi_call_threadsafe_function_rawptr = nullptr;
  void* napi_acquire_threadsafe_function_rawptr = nullptr;
  void* napi_release_threadsafe_function_rawptr = nullptr;
  void* napi_unref_threadsafe_function_rawptr = nullptr;
  void* napi_ref_threadsafe_function_rawptr = nullptr;
  void* napi_add_async_cleanup_hook_rawptr = nullptr;
  void* napi_remove_async_cleanup_hook_rawptr = nullptr;
  void* napi_find_module_rawptr = nullptr;
};

const NodeApiRawPtrHost& GetWeakNapiRawPtrHost();

}  // namespace napi
}  // namespace lynx

#endif  // THIRD_PARTY_NAPI_WEAK_NODE_API_HOST_H_
