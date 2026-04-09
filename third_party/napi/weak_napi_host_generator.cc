// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/napi/include/js_native_api_adapter.h"
#include "third_party/napi/weak_node_api_host.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace napi {

const NodeApiRawPtrHost& GetWeakNapiRawPtrHost() {
  static NodeApiRawPtrHost* sWeakRawPtrHost = []() {
    auto* host = new NodeApiRawPtrHost();
    host->napi_get_undefined_rawptr =
        reinterpret_cast<void*>(napi_get_undefined);
    host->napi_get_null_rawptr = reinterpret_cast<void*>(napi_get_null);
    host->napi_get_global_rawptr = reinterpret_cast<void*>(napi_get_global);
    host->napi_get_boolean_rawptr = reinterpret_cast<void*>(napi_get_boolean);
    host->napi_create_object_rawptr =
        reinterpret_cast<void*>(napi_create_object);
    host->napi_create_array_rawptr = reinterpret_cast<void*>(napi_create_array);
    host->napi_create_array_with_length_rawptr =
        reinterpret_cast<void*>(napi_create_array_with_length);
    host->napi_create_double_rawptr =
        reinterpret_cast<void*>(napi_create_double);
    host->napi_create_int32_rawptr = reinterpret_cast<void*>(napi_create_int32);
    host->napi_create_uint32_rawptr =
        reinterpret_cast<void*>(napi_create_uint32);
    host->napi_create_int64_rawptr = reinterpret_cast<void*>(napi_create_int64);
    host->napi_create_string_latin1_rawptr =
        reinterpret_cast<void*>(napi_create_string_latin1);
    host->napi_create_string_utf8_rawptr =
        reinterpret_cast<void*>(napi_create_string_utf8);
    host->napi_create_string_utf16_rawptr =
        reinterpret_cast<void*>(napi_create_string_utf16);
    host->napi_create_symbol_rawptr =
        reinterpret_cast<void*>(napi_create_symbol);
    host->napi_create_function_rawptr =
        reinterpret_cast<void*>(napi_create_function);
    host->napi_create_error_rawptr = reinterpret_cast<void*>(napi_create_error);
    host->napi_create_type_error_rawptr =
        reinterpret_cast<void*>(napi_create_type_error);
    host->napi_create_range_error_rawptr =
        reinterpret_cast<void*>(napi_create_range_error);
    host->napi_typeof_rawptr = reinterpret_cast<void*>(napi_typeof);
    host->napi_get_value_double_rawptr =
        reinterpret_cast<void*>(napi_get_value_double);
    host->napi_get_value_int32_rawptr =
        reinterpret_cast<void*>(napi_get_value_int32);
    host->napi_get_value_uint32_rawptr =
        reinterpret_cast<void*>(napi_get_value_uint32);
    host->napi_get_value_int64_rawptr =
        reinterpret_cast<void*>(napi_get_value_int64);
    host->napi_get_value_bool_rawptr =
        reinterpret_cast<void*>(napi_get_value_bool);
    host->napi_get_value_string_latin1_rawptr =
        reinterpret_cast<void*>(napi_get_value_string_latin1);
    host->napi_get_value_string_utf8_rawptr =
        reinterpret_cast<void*>(napi_get_value_string_utf8);
    host->napi_get_value_string_utf16_rawptr =
        reinterpret_cast<void*>(napi_get_value_string_utf16);
    host->napi_coerce_to_bool_rawptr =
        reinterpret_cast<void*>(napi_coerce_to_bool);
    host->napi_coerce_to_number_rawptr =
        reinterpret_cast<void*>(napi_coerce_to_number);
    host->napi_coerce_to_object_rawptr =
        reinterpret_cast<void*>(napi_coerce_to_object);
    host->napi_coerce_to_string_rawptr =
        reinterpret_cast<void*>(napi_coerce_to_string);
    host->napi_get_prototype_rawptr =
        reinterpret_cast<void*>(napi_get_prototype);
    host->napi_get_property_names_rawptr =
        reinterpret_cast<void*>(napi_get_property_names);
    host->napi_set_property_rawptr = reinterpret_cast<void*>(napi_set_property);
    host->napi_has_property_rawptr = reinterpret_cast<void*>(napi_has_property);
    host->napi_get_property_rawptr = reinterpret_cast<void*>(napi_get_property);
    host->napi_delete_property_rawptr =
        reinterpret_cast<void*>(napi_delete_property);
    host->napi_has_own_property_rawptr =
        reinterpret_cast<void*>(napi_has_own_property);
    host->napi_set_named_property_rawptr =
        reinterpret_cast<void*>(napi_set_named_property);
    host->napi_has_named_property_rawptr =
        reinterpret_cast<void*>(napi_has_named_property);
    host->napi_get_named_property_rawptr =
        reinterpret_cast<void*>(napi_get_named_property);
    host->napi_set_element_rawptr = reinterpret_cast<void*>(napi_set_element);
    host->napi_has_element_rawptr = reinterpret_cast<void*>(napi_has_element);
    host->napi_get_element_rawptr = reinterpret_cast<void*>(napi_get_element);
    host->napi_delete_element_rawptr =
        reinterpret_cast<void*>(napi_delete_element);
    host->napi_define_properties_rawptr =
        reinterpret_cast<void*>(napi_define_properties);
    host->napi_is_array_rawptr = reinterpret_cast<void*>(napi_is_array);
    host->napi_get_array_length_rawptr =
        reinterpret_cast<void*>(napi_get_array_length);
    host->napi_strict_equals_rawptr =
        reinterpret_cast<void*>(napi_strict_equals);
    host->napi_call_function_rawptr =
        reinterpret_cast<void*>(napi_call_function);
    host->napi_new_instance_rawptr = reinterpret_cast<void*>(napi_new_instance);
    host->napi_instanceof_rawptr = reinterpret_cast<void*>(napi_instanceof);
    host->napi_get_cb_info_rawptr = reinterpret_cast<void*>(napi_get_cb_info);
    host->napi_get_new_target_rawptr =
        reinterpret_cast<void*>(napi_get_new_target);
    host->napi_define_class_rawptr = reinterpret_cast<void*>(napi_define_class);
    host->napi_wrap_rawptr = reinterpret_cast<void*>(napi_wrap);
    host->napi_unwrap_rawptr = reinterpret_cast<void*>(napi_unwrap);
    host->napi_remove_wrap_rawptr = reinterpret_cast<void*>(napi_remove_wrap);
    host->napi_create_external_rawptr =
        reinterpret_cast<void*>(napi_create_external);
    host->napi_get_value_external_rawptr =
        reinterpret_cast<void*>(napi_get_value_external);
    host->napi_create_reference_rawptr =
        reinterpret_cast<void*>(napi_create_reference);
    host->napi_delete_reference_rawptr =
        reinterpret_cast<void*>(napi_delete_reference);
    host->napi_reference_ref_rawptr =
        reinterpret_cast<void*>(napi_reference_ref);
    host->napi_reference_unref_rawptr =
        reinterpret_cast<void*>(napi_reference_unref);
    host->napi_get_reference_value_rawptr =
        reinterpret_cast<void*>(napi_get_reference_value);
    host->napi_open_handle_scope_rawptr =
        reinterpret_cast<void*>(napi_open_handle_scope);
    host->napi_close_handle_scope_rawptr =
        reinterpret_cast<void*>(napi_close_handle_scope);
    host->napi_open_escapable_handle_scope_rawptr =
        reinterpret_cast<void*>(napi_open_escapable_handle_scope);
    host->napi_close_escapable_handle_scope_rawptr =
        reinterpret_cast<void*>(napi_close_escapable_handle_scope);
    host->napi_escape_handle_rawptr =
        reinterpret_cast<void*>(napi_escape_handle);
    host->napi_throw_rawptr = reinterpret_cast<void*>(napi_throw);
    host->napi_throw_error_rawptr = reinterpret_cast<void*>(napi_throw_error);
    host->napi_throw_type_error_rawptr =
        reinterpret_cast<void*>(napi_throw_type_error);
    host->napi_throw_range_error_rawptr =
        reinterpret_cast<void*>(napi_throw_range_error);
    host->napi_is_error_rawptr = reinterpret_cast<void*>(napi_is_error);
    host->napi_is_exception_pending_rawptr =
        reinterpret_cast<void*>(napi_is_exception_pending);
    host->napi_get_and_clear_last_exception_rawptr =
        reinterpret_cast<void*>(napi_get_and_clear_last_exception);
    host->napi_get_last_error_info_rawptr =
        reinterpret_cast<void*>(napi_get_last_error_info);
    host->napi_is_arraybuffer_rawptr =
        reinterpret_cast<void*>(napi_is_arraybuffer);
    host->napi_create_arraybuffer_rawptr =
        reinterpret_cast<void*>(napi_create_arraybuffer);
    host->napi_create_external_arraybuffer_rawptr =
        reinterpret_cast<void*>(napi_create_external_arraybuffer);
    host->napi_get_arraybuffer_info_rawptr =
        reinterpret_cast<void*>(napi_get_arraybuffer_info);
    host->napi_is_typedarray_rawptr =
        reinterpret_cast<void*>(napi_is_typedarray);
    host->napi_create_typedarray_rawptr =
        reinterpret_cast<void*>(napi_create_typedarray);
    host->napi_get_typedarray_info_rawptr =
        reinterpret_cast<void*>(napi_get_typedarray_info);
    host->napi_create_dataview_rawptr =
        reinterpret_cast<void*>(napi_create_dataview);
    host->napi_is_dataview_rawptr = reinterpret_cast<void*>(napi_is_dataview);
    host->napi_get_dataview_info_rawptr =
        reinterpret_cast<void*>(napi_get_dataview_info);
    host->napi_create_promise_rawptr =
        reinterpret_cast<void*>(napi_create_promise);
    host->napi_is_promise_rawptr = reinterpret_cast<void*>(napi_is_promise);
    host->napi_resolve_deferred_rawptr =
        reinterpret_cast<void*>(napi_resolve_deferred);
    host->napi_reject_deferred_rawptr =
        reinterpret_cast<void*>(napi_reject_deferred);
    host->napi_run_script_rawptr = reinterpret_cast<void*>(napi_run_script);
    host->napi_adjust_external_memory_rawptr =
        reinterpret_cast<void*>(napi_adjust_external_memory);
    host->napi_add_finalizer_rawptr =
        reinterpret_cast<void*>(napi_add_finalizer);
    host->napi_set_instance_data_rawptr =
        reinterpret_cast<void*>(napi_set_instance_data);
    host->napi_get_instance_data_rawptr =
        reinterpret_cast<void*>(napi_get_instance_data);
    host->napi_add_env_cleanup_hook_rawptr =
        reinterpret_cast<void*>(napi_add_env_cleanup_hook);
    host->napi_remove_env_cleanup_hook_rawptr =
        reinterpret_cast<void*>(napi_remove_env_cleanup_hook);
    host->napi_create_async_work_rawptr =
        reinterpret_cast<void*>(napi_create_async_work);
    host->napi_delete_async_work_rawptr =
        reinterpret_cast<void*>(napi_delete_async_work);
    host->napi_queue_async_work_rawptr =
        reinterpret_cast<void*>(napi_queue_async_work);
    host->napi_cancel_async_work_rawptr =
        reinterpret_cast<void*>(napi_cancel_async_work);
    host->napi_create_threadsafe_function_rawptr =
        reinterpret_cast<void*>(napi_create_threadsafe_function);
    host->napi_get_threadsafe_function_context_rawptr =
        reinterpret_cast<void*>(napi_get_threadsafe_function_context);
    host->napi_call_threadsafe_function_rawptr =
        reinterpret_cast<void*>(napi_call_threadsafe_function);
    host->napi_acquire_threadsafe_function_rawptr =
        reinterpret_cast<void*>(napi_acquire_threadsafe_function);
    host->napi_release_threadsafe_function_rawptr =
        reinterpret_cast<void*>(napi_release_threadsafe_function);
    host->napi_unref_threadsafe_function_rawptr =
        reinterpret_cast<void*>(napi_unref_threadsafe_function);
    host->napi_ref_threadsafe_function_rawptr =
        reinterpret_cast<void*>(napi_ref_threadsafe_function);
    host->napi_create_date_rawptr = reinterpret_cast<void*>(napi_create_date);
    host->napi_is_date_rawptr = reinterpret_cast<void*>(napi_is_date);
    host->napi_get_date_value_rawptr =
        reinterpret_cast<void*>(napi_get_date_value);
    host->napi_module_register_rawptr =
        reinterpret_cast<void*>(napi_module_register);
    host->napi_fatal_error_rawptr = reinterpret_cast<void*>(napi_fatal_error);
    host->napi_get_all_property_names_rawptr =
        reinterpret_cast<void*>(napi_get_all_property_names);
    host->napi_create_bigint_int64_rawptr =
        reinterpret_cast<void*>(napi_create_bigint_int64);
    host->napi_create_bigint_uint64_rawptr =
        reinterpret_cast<void*>(napi_create_bigint_uint64);
    host->napi_create_bigint_words_rawptr =
        reinterpret_cast<void*>(napi_create_bigint_words);
    host->napi_get_value_bigint_int64_rawptr =
        reinterpret_cast<void*>(napi_get_value_bigint_int64);
    host->napi_get_value_bigint_uint64_rawptr =
        reinterpret_cast<void*>(napi_get_value_bigint_uint64);
    host->napi_get_value_bigint_words_rawptr =
        reinterpret_cast<void*>(napi_get_value_bigint_words);
    host->napi_get_version_rawptr = reinterpret_cast<void*>(napi_get_version);
    host->napi_find_module_rawptr =
        reinterpret_cast<void*>(napi_find_module_primjs);
    return host;
  }();
  return *sWeakRawPtrHost;
}

}  // namespace napi
}  // namespace lynx
