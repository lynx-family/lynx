// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/napi/weak_node_api_host.h"
#include "third_party/weak-node-api/generated/weak_node_api.hpp"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace napi {

void SetupWeakNodeApiEnv(void) __attribute__((constructor));
void SetupWeakNodeApiEnv() {
  const NodeApiRawPtrHost& raw_ptr_host = GetWeakNapiRawPtrHost();
  static NodeApiHost sWeakHost({
      .napi_get_last_error_info = reinterpret_cast<napi_status (*)(
          node_api_basic_env, const napi_extended_error_info**)>(
          raw_ptr_host.napi_get_last_error_info_rawptr),
      .napi_get_undefined =
          reinterpret_cast<napi_status (*)(napi_env, napi_value*)>(
              raw_ptr_host.napi_get_undefined_rawptr),
      .napi_get_null = reinterpret_cast<napi_status (*)(napi_env, napi_value*)>(
          raw_ptr_host.napi_get_null_rawptr),
      .napi_get_global =
          reinterpret_cast<napi_status (*)(napi_env, napi_value*)>(
              raw_ptr_host.napi_get_global_rawptr),
      .napi_get_boolean =
          reinterpret_cast<napi_status (*)(napi_env, bool, napi_value*)>(
              raw_ptr_host.napi_get_boolean_rawptr),
      .napi_create_object =
          reinterpret_cast<napi_status (*)(napi_env, napi_value*)>(
              raw_ptr_host.napi_create_object_rawptr),
      .napi_create_array =
          reinterpret_cast<napi_status (*)(napi_env, napi_value*)>(
              raw_ptr_host.napi_create_array_rawptr),
      .napi_create_array_with_length =
          reinterpret_cast<napi_status (*)(napi_env, size_t, napi_value*)>(
              raw_ptr_host.napi_create_array_with_length_rawptr),
      .napi_create_double =
          reinterpret_cast<napi_status (*)(napi_env, double, napi_value*)>(
              raw_ptr_host.napi_create_double_rawptr),
      .napi_create_int32 =
          reinterpret_cast<napi_status (*)(napi_env, int32_t, napi_value*)>(
              raw_ptr_host.napi_create_int32_rawptr),
      .napi_create_uint32 =
          reinterpret_cast<napi_status (*)(napi_env, uint32_t, napi_value*)>(
              raw_ptr_host.napi_create_uint32_rawptr),
      .napi_create_int64 =
          reinterpret_cast<napi_status (*)(napi_env, int64_t, napi_value*)>(
              raw_ptr_host.napi_create_int64_rawptr),
      .napi_create_string_latin1 = reinterpret_cast<napi_status (*)(
          napi_env, const char*, size_t, napi_value*)>(
          raw_ptr_host.napi_create_string_latin1_rawptr),
      .napi_create_string_utf8 = reinterpret_cast<napi_status (*)(
          napi_env, const char*, size_t, napi_value*)>(
          raw_ptr_host.napi_create_string_utf8_rawptr),
      .napi_create_string_utf16 = reinterpret_cast<napi_status (*)(
          napi_env, const char16_t*, size_t, napi_value*)>(
          raw_ptr_host.napi_create_string_utf16_rawptr),
      .napi_create_symbol =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_create_symbol_rawptr),
      .napi_create_function = reinterpret_cast<napi_status (*)(
          napi_env, const char*, size_t, napi_callback, void*, napi_value*)>(
          raw_ptr_host.napi_create_function_rawptr),
      .napi_create_error = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_value*)>(
          raw_ptr_host.napi_create_error_rawptr),
      .napi_create_type_error = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_value*)>(
          raw_ptr_host.napi_create_type_error_rawptr),
      .napi_create_range_error = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_value*)>(
          raw_ptr_host.napi_create_range_error_rawptr),
      .napi_typeof = reinterpret_cast<napi_status (*)(napi_env, napi_value,
                                                      napi_valuetype*)>(
          raw_ptr_host.napi_typeof_rawptr),
      .napi_get_value_double =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, double*)>(
              raw_ptr_host.napi_get_value_double_rawptr),
      .napi_get_value_int32 =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, int32_t*)>(
              raw_ptr_host.napi_get_value_int32_rawptr),
      .napi_get_value_uint32 =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, uint32_t*)>(
              raw_ptr_host.napi_get_value_uint32_rawptr),
      .napi_get_value_int64 =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, int64_t*)>(
              raw_ptr_host.napi_get_value_int64_rawptr),
      .napi_get_value_bool =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_get_value_bool_rawptr),
      .napi_get_value_string_latin1 = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, char*, size_t, size_t*)>(
          raw_ptr_host.napi_get_value_string_latin1_rawptr),
      .napi_get_value_string_utf8 = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, char*, size_t, size_t*)>(
          raw_ptr_host.napi_get_value_string_utf8_rawptr),
      .napi_get_value_string_utf16 = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, char16_t*, size_t, size_t*)>(
          raw_ptr_host.napi_get_value_string_utf16_rawptr),
      .napi_coerce_to_bool =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_coerce_to_bool_rawptr),
      .napi_coerce_to_number =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_coerce_to_number_rawptr),
      .napi_coerce_to_object =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_coerce_to_object_rawptr),
      .napi_coerce_to_string =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_coerce_to_string_rawptr),
      .napi_get_prototype =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_get_prototype_rawptr),
      .napi_get_property_names =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_get_property_names_rawptr),
      .napi_set_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_value)>(
          raw_ptr_host.napi_set_property_rawptr),
      .napi_has_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, bool*)>(
          raw_ptr_host.napi_has_property_rawptr),
      .napi_get_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_value*)>(
          raw_ptr_host.napi_get_property_rawptr),
      .napi_delete_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, bool*)>(
          raw_ptr_host.napi_delete_property_rawptr),
      .napi_has_own_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, bool*)>(
          raw_ptr_host.napi_has_own_property_rawptr),
      .napi_set_named_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, const char*, napi_value)>(
          raw_ptr_host.napi_set_named_property_rawptr),
      .napi_has_named_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, const char*, bool*)>(
          raw_ptr_host.napi_has_named_property_rawptr),
      .napi_get_named_property = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, const char*, napi_value*)>(
          raw_ptr_host.napi_get_named_property_rawptr),
      .napi_set_element = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, uint32_t, napi_value)>(
          raw_ptr_host.napi_set_element_rawptr),
      .napi_has_element = reinterpret_cast<napi_status (*)(napi_env, napi_value,
                                                           uint32_t, bool*)>(
          raw_ptr_host.napi_has_element_rawptr),
      .napi_get_element = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, uint32_t, napi_value*)>(
          raw_ptr_host.napi_get_element_rawptr),
      .napi_delete_element = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, uint32_t, bool*)>(
          raw_ptr_host.napi_delete_element_rawptr),
      .napi_define_properties = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, size_t, const napi_property_descriptor*)>(
          raw_ptr_host.napi_define_properties_rawptr),
      .napi_is_array =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_array_rawptr),
      .napi_get_array_length =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, uint32_t*)>(
              raw_ptr_host.napi_get_array_length_rawptr),
      .napi_strict_equals = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, bool*)>(
          raw_ptr_host.napi_strict_equals_rawptr),
      .napi_call_function = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, size_t, const napi_value*,
          napi_value*)>(raw_ptr_host.napi_call_function_rawptr),
      .napi_new_instance = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, size_t, const napi_value*, napi_value*)>(
          raw_ptr_host.napi_new_instance_rawptr),
      .napi_instanceof = reinterpret_cast<napi_status (*)(napi_env, napi_value,
                                                          napi_value, bool*)>(
          raw_ptr_host.napi_instanceof_rawptr),
      .napi_get_cb_info = reinterpret_cast<napi_status (*)(
          napi_env, napi_callback_info, size_t*, napi_value*, napi_value*,
          void**)>(raw_ptr_host.napi_get_cb_info_rawptr),
      .napi_get_new_target = reinterpret_cast<napi_status (*)(
          napi_env, napi_callback_info, napi_value*)>(
          raw_ptr_host.napi_get_new_target_rawptr),
      .napi_define_class = reinterpret_cast<napi_status (*)(
          napi_env, const char*, size_t, napi_callback, void*, size_t,
          const napi_property_descriptor*, napi_value*)>(
          raw_ptr_host.napi_define_class_rawptr),
      .napi_wrap = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, void*, node_api_basic_finalize, void*,
          napi_ref*)>(raw_ptr_host.napi_wrap_rawptr),
      .napi_unwrap =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, void**)>(
              raw_ptr_host.napi_unwrap_rawptr),
      .napi_remove_wrap =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, void**)>(
              raw_ptr_host.napi_remove_wrap_rawptr),
      .napi_create_external = reinterpret_cast<napi_status (*)(
          napi_env, void*, node_api_basic_finalize, void*, napi_value*)>(
          raw_ptr_host.napi_create_external_rawptr),
      .napi_get_value_external =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, void**)>(
              raw_ptr_host.napi_get_value_external_rawptr),
      .napi_create_reference = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, uint32_t, napi_ref*)>(
          raw_ptr_host.napi_create_reference_rawptr),
      .napi_delete_reference =
          reinterpret_cast<napi_status (*)(napi_env, napi_ref)>(
              raw_ptr_host.napi_delete_reference_rawptr),
      .napi_reference_ref =
          reinterpret_cast<napi_status (*)(napi_env, napi_ref, uint32_t*)>(
              raw_ptr_host.napi_reference_ref_rawptr),
      .napi_reference_unref =
          reinterpret_cast<napi_status (*)(napi_env, napi_ref, uint32_t*)>(
              raw_ptr_host.napi_reference_unref_rawptr),
      .napi_get_reference_value =
          reinterpret_cast<napi_status (*)(napi_env, napi_ref, napi_value*)>(
              raw_ptr_host.napi_get_reference_value_rawptr),
      .napi_open_handle_scope =
          reinterpret_cast<napi_status (*)(napi_env, napi_handle_scope*)>(
              raw_ptr_host.napi_open_handle_scope_rawptr),
      .napi_close_handle_scope =
          reinterpret_cast<napi_status (*)(napi_env, napi_handle_scope)>(
              raw_ptr_host.napi_close_handle_scope_rawptr),
      .napi_open_escapable_handle_scope = reinterpret_cast<napi_status (*)(
          napi_env, napi_escapable_handle_scope*)>(
          raw_ptr_host.napi_open_escapable_handle_scope_rawptr),
      .napi_close_escapable_handle_scope = reinterpret_cast<napi_status (*)(
          napi_env, napi_escapable_handle_scope)>(
          raw_ptr_host.napi_close_escapable_handle_scope_rawptr),
      .napi_escape_handle = reinterpret_cast<napi_status (*)(
          napi_env, napi_escapable_handle_scope, napi_value, napi_value*)>(
          raw_ptr_host.napi_escape_handle_rawptr),
      .napi_throw = reinterpret_cast<napi_status (*)(napi_env, napi_value)>(
          raw_ptr_host.napi_throw_rawptr),
      .napi_throw_error =
          reinterpret_cast<napi_status (*)(napi_env, const char*, const char*)>(
              raw_ptr_host.napi_throw_error_rawptr),
      .napi_throw_type_error =
          reinterpret_cast<napi_status (*)(napi_env, const char*, const char*)>(
              raw_ptr_host.napi_throw_type_error_rawptr),
      .napi_throw_range_error =
          reinterpret_cast<napi_status (*)(napi_env, const char*, const char*)>(
              raw_ptr_host.napi_throw_range_error_rawptr),
      .napi_is_error =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_error_rawptr),
      .napi_is_exception_pending =
          reinterpret_cast<napi_status (*)(napi_env, bool*)>(
              raw_ptr_host.napi_is_exception_pending_rawptr),
      .napi_get_and_clear_last_exception =
          reinterpret_cast<napi_status (*)(napi_env, napi_value*)>(
              raw_ptr_host.napi_get_and_clear_last_exception_rawptr),
      .napi_is_arraybuffer =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_arraybuffer_rawptr),
      .napi_create_arraybuffer = reinterpret_cast<napi_status (*)(
          napi_env, size_t, void**, napi_value*)>(
          raw_ptr_host.napi_create_arraybuffer_rawptr),
      .napi_create_external_arraybuffer = reinterpret_cast<napi_status (*)(
          napi_env, void*, size_t, node_api_basic_finalize, void*,
          napi_value*)>(raw_ptr_host.napi_create_external_arraybuffer_rawptr),
      .napi_get_arraybuffer_info = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, void**, size_t*)>(
          raw_ptr_host.napi_get_arraybuffer_info_rawptr),
      .napi_is_typedarray =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_typedarray_rawptr),
      .napi_create_typedarray = reinterpret_cast<napi_status (*)(
          napi_env, napi_typedarray_type, size_t, napi_value, size_t,
          napi_value*)>(raw_ptr_host.napi_create_typedarray_rawptr),
      .napi_get_typedarray_info = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_typedarray_type*, size_t*, void**,
          napi_value*, size_t*)>(raw_ptr_host.napi_get_typedarray_info_rawptr),
      .napi_create_dataview = reinterpret_cast<napi_status (*)(
          napi_env, size_t, napi_value, size_t, napi_value*)>(
          raw_ptr_host.napi_create_dataview_rawptr),
      .napi_is_dataview =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_dataview_rawptr),
      .napi_get_dataview_info = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, size_t*, void**, napi_value*, size_t*)>(
          raw_ptr_host.napi_get_dataview_info_rawptr),
      .napi_get_version =
          reinterpret_cast<napi_status (*)(node_api_basic_env, uint32_t*)>(
              raw_ptr_host.napi_get_version_rawptr),
      .napi_create_promise = reinterpret_cast<napi_status (*)(
          napi_env, napi_deferred*, napi_value*)>(
          raw_ptr_host.napi_create_promise_rawptr),
      .napi_resolve_deferred = reinterpret_cast<napi_status (*)(
          napi_env, napi_deferred, napi_value)>(
          raw_ptr_host.napi_resolve_deferred_rawptr),
      .napi_reject_deferred = reinterpret_cast<napi_status (*)(
          napi_env, napi_deferred, napi_value)>(
          raw_ptr_host.napi_reject_deferred_rawptr),
      .napi_is_promise =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_promise_rawptr),
      .napi_run_script =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, napi_value*)>(
              raw_ptr_host.napi_run_script_rawptr),
      .napi_adjust_external_memory = reinterpret_cast<napi_status (*)(
          node_api_basic_env, int64_t, int64_t*)>(
          raw_ptr_host.napi_adjust_external_memory_rawptr),
      .napi_create_date =
          reinterpret_cast<napi_status (*)(napi_env, double, napi_value*)>(
              raw_ptr_host.napi_create_date_rawptr),
      .napi_is_date =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_date_rawptr),
      .napi_get_date_value =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, double*)>(
              raw_ptr_host.napi_get_date_value_rawptr),
      .napi_add_finalizer = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, void*, node_api_basic_finalize, void*,
          napi_ref*)>(raw_ptr_host.napi_add_finalizer_rawptr),
      .napi_create_bigint_int64 =
          reinterpret_cast<napi_status (*)(napi_env, int64_t, napi_value*)>(
              raw_ptr_host.napi_create_bigint_int64_rawptr),
      .napi_create_bigint_uint64 =
          reinterpret_cast<napi_status (*)(napi_env, uint64_t, napi_value*)>(
              raw_ptr_host.napi_create_bigint_uint64_rawptr),
      .napi_create_bigint_words = reinterpret_cast<napi_status (*)(
          napi_env, int, size_t, const uint64_t*, napi_value*)>(
          raw_ptr_host.napi_create_bigint_words_rawptr),
      .napi_get_value_bigint_int64 = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, int64_t*, bool*)>(
          raw_ptr_host.napi_get_value_bigint_int64_rawptr),
      .napi_get_value_bigint_uint64 = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, uint64_t*, bool*)>(
          raw_ptr_host.napi_get_value_bigint_uint64_rawptr),
      .napi_get_value_bigint_words = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, int*, size_t*, uint64_t*)>(
          raw_ptr_host.napi_get_value_bigint_words_rawptr),
      .napi_get_all_property_names = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_key_collection_mode, napi_key_filter,
          napi_key_conversion, napi_value*)>(
          raw_ptr_host.napi_get_all_property_names_rawptr),
      .napi_set_instance_data = reinterpret_cast<napi_status (*)(
          node_api_basic_env, void*, napi_finalize, void*)>(
          raw_ptr_host.napi_set_instance_data_rawptr),
      .napi_get_instance_data =
          reinterpret_cast<napi_status (*)(node_api_basic_env, void**)>(
              raw_ptr_host.napi_get_instance_data_rawptr),
      .napi_detach_arraybuffer =
          reinterpret_cast<napi_status (*)(napi_env, napi_value)>(
              raw_ptr_host.napi_detach_arraybuffer_rawptr),
      .napi_is_detached_arraybuffer =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_detached_arraybuffer_rawptr),
      .napi_type_tag_object = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, const napi_type_tag*)>(
          raw_ptr_host.napi_type_tag_object_rawptr),
      .napi_check_object_type_tag = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, const napi_type_tag*, bool*)>(
          raw_ptr_host.napi_check_object_type_tag_rawptr),
      .napi_object_freeze =
          reinterpret_cast<napi_status (*)(napi_env, napi_value)>(
              raw_ptr_host.napi_object_freeze_rawptr),
      .napi_object_seal =
          reinterpret_cast<napi_status (*)(napi_env, napi_value)>(
              raw_ptr_host.napi_object_seal_rawptr),
      .napi_module_register = reinterpret_cast<void (*)(napi_module*)>(
          raw_ptr_host.napi_module_register_rawptr),
      .napi_fatal_error =
          reinterpret_cast<void (*)(const char*, size_t, const char*, size_t)>(
              raw_ptr_host.napi_fatal_error_rawptr),
      .napi_async_init = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_async_context*)>(
          raw_ptr_host.napi_async_init_rawptr),
      .napi_async_destroy =
          reinterpret_cast<napi_status (*)(napi_env, napi_async_context)>(
              raw_ptr_host.napi_async_destroy_rawptr),
      .napi_make_callback = reinterpret_cast<napi_status (*)(
          napi_env, napi_async_context, napi_value, napi_value, size_t,
          const napi_value*, napi_value*)>(
          raw_ptr_host.napi_make_callback_rawptr),
      .napi_create_buffer = reinterpret_cast<napi_status (*)(
          napi_env, size_t, void**, napi_value*)>(
          raw_ptr_host.napi_create_buffer_rawptr),
      .napi_create_external_buffer = reinterpret_cast<napi_status (*)(
          napi_env, size_t, void*, node_api_basic_finalize, void*,
          napi_value*)>(raw_ptr_host.napi_create_external_buffer_rawptr),
      .napi_create_buffer_copy = reinterpret_cast<napi_status (*)(
          napi_env, size_t, const void*, void**, napi_value*)>(
          raw_ptr_host.napi_create_buffer_copy_rawptr),
      .napi_is_buffer =
          reinterpret_cast<napi_status (*)(napi_env, napi_value, bool*)>(
              raw_ptr_host.napi_is_buffer_rawptr),
      .napi_get_buffer_info = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, void**, size_t*)>(
          raw_ptr_host.napi_get_buffer_info_rawptr),
      .napi_create_async_work = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_async_execute_callback,
          napi_async_complete_callback, void*, napi_async_work*)>(
          raw_ptr_host.napi_create_async_work_rawptr),
      .napi_delete_async_work =
          reinterpret_cast<napi_status (*)(napi_env, napi_async_work)>(
              raw_ptr_host.napi_delete_async_work_rawptr),
      .napi_queue_async_work = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_async_work)>(
          raw_ptr_host.napi_queue_async_work_rawptr),
      .napi_cancel_async_work = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_async_work)>(
          raw_ptr_host.napi_cancel_async_work_rawptr),
      .napi_get_node_version = reinterpret_cast<napi_status (*)(
          node_api_basic_env, const napi_node_version**)>(
          raw_ptr_host.napi_get_node_version_rawptr),
      .napi_get_uv_event_loop = reinterpret_cast<napi_status (*)(
          node_api_basic_env, struct uv_loop_s**)>(
          raw_ptr_host.napi_get_uv_event_loop_rawptr),
      .napi_fatal_exception =
          reinterpret_cast<napi_status (*)(napi_env, napi_value)>(
              raw_ptr_host.napi_fatal_exception_rawptr),
      .napi_add_env_cleanup_hook = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_cleanup_hook, void*)>(
          raw_ptr_host.napi_add_env_cleanup_hook_rawptr),
      .napi_remove_env_cleanup_hook = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_cleanup_hook, void*)>(
          raw_ptr_host.napi_remove_env_cleanup_hook_rawptr),
      .napi_open_callback_scope = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_async_context, napi_callback_scope*)>(
          raw_ptr_host.napi_open_callback_scope_rawptr),
      .napi_close_callback_scope =
          reinterpret_cast<napi_status (*)(napi_env, napi_callback_scope)>(
              raw_ptr_host.napi_close_callback_scope_rawptr),
      .napi_create_threadsafe_function = reinterpret_cast<napi_status (*)(
          napi_env, napi_value, napi_value, napi_value, size_t, size_t, void*,
          napi_finalize, void*, napi_threadsafe_function_call_js,
          napi_threadsafe_function*)>(
          raw_ptr_host.napi_create_threadsafe_function_rawptr),
      .napi_get_threadsafe_function_context =
          reinterpret_cast<napi_status (*)(napi_threadsafe_function, void**)>(
              raw_ptr_host.napi_get_threadsafe_function_context_rawptr),
      .napi_call_threadsafe_function = reinterpret_cast<napi_status (*)(
          napi_threadsafe_function, void*, napi_threadsafe_function_call_mode)>(
          raw_ptr_host.napi_call_threadsafe_function_rawptr),
      .napi_acquire_threadsafe_function =
          reinterpret_cast<napi_status (*)(napi_threadsafe_function)>(
              raw_ptr_host.napi_acquire_threadsafe_function_rawptr),
      .napi_release_threadsafe_function = reinterpret_cast<napi_status (*)(
          napi_threadsafe_function, napi_threadsafe_function_release_mode)>(
          raw_ptr_host.napi_release_threadsafe_function_rawptr),
      .napi_unref_threadsafe_function = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_threadsafe_function)>(
          raw_ptr_host.napi_unref_threadsafe_function_rawptr),
      .napi_ref_threadsafe_function = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_threadsafe_function)>(
          raw_ptr_host.napi_ref_threadsafe_function_rawptr),
      .napi_add_async_cleanup_hook = reinterpret_cast<napi_status (*)(
          node_api_basic_env, napi_async_cleanup_hook, void*,
          napi_async_cleanup_hook_handle*)>(
          raw_ptr_host.napi_add_async_cleanup_hook_rawptr),
      .napi_remove_async_cleanup_hook =
          reinterpret_cast<napi_status (*)(napi_async_cleanup_hook_handle)>(
              raw_ptr_host.napi_remove_async_cleanup_hook_rawptr),
      .napi_find_module_weak = reinterpret_cast<napi_module* (*)(const char*)>(
          raw_ptr_host.napi_find_module_rawptr),
  });

  inject_weak_node_api_host(sWeakHost);
}

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

}  // namespace napi
}  // namespace lynx
