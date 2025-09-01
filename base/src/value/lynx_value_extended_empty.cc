// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/lynx_value_extended.h"

lynx_api_status lynx_value_get_bool_ext(lynx_api_env env, lynx_value value,
                                        bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_double_ext(lynx_api_env env, lynx_value value,
                                          double* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_int32_ext(lynx_api_env env, lynx_value value,
                                         int32_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_int64_ext(lynx_api_env env, lynx_value value,
                                         int64_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_integer_ext(lynx_api_env env, lynx_value value,
                                          bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_integer_ext(lynx_api_env env, lynx_value value,
                                           int64_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_number_ext(lynx_api_env env, lynx_value value,
                                          double* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_string_ref_ext(lynx_api_env env,
                                              lynx_value value, void** result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_external_ext(lynx_api_env env, lynx_value value,
                                            void** result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_length_ext(lynx_api_env env, lynx_value value,
                                          uint32_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_array_ext(lynx_api_env env, lynx_value value,
                                        bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_set_element_ext(lynx_api_env env, lynx_value object,
                                           uint32_t index, lynx_value value) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_element_ext(lynx_api_env env, lynx_value object,
                                           uint32_t index, lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_map_ext(lynx_api_env env, lynx_value value,
                                      bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_set_named_property_ext(lynx_api_env env,
                                                  lynx_value object,
                                                  const char* utf8name,
                                                  lynx_value value) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_has_named_property_ext(lynx_api_env env,
                                                  lynx_value object,
                                                  const char* utf8name,
                                                  bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_named_property_ext(lynx_api_env env,
                                                  lynx_value object,
                                                  const char* utf8name,
                                                  lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_function_ext(lynx_api_env env, lynx_value value,
                                           bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_to_string_utf8_ext(lynx_api_env env,
                                              lynx_value value, void* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_typeof_ext(lynx_api_env env, lynx_value value,
                                      lynx_value_type* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_iterate_value_ext(
    lynx_api_env env, lynx_value object, lynx_value_iterator_callback callback,
    void* pfunc, void* raw_data) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_equals_ext(lynx_api_env env, lynx_value lhs,
                                      lynx_value rhs, bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_deep_copy_value_ext(lynx_api_env env, lynx_value src,
                                               lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_print_ext(lynx_api_env env, lynx_value value,
                                     void* stream,
                                     lynx_value_print_ext_callback callback) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_add_reference_ext(lynx_api_env env, lynx_value value,
                                             lynx_value_ref* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_add_reference_weak_ext(lynx_api_env env,
                                                  lynx_value value,
                                                  lynx_value_ref* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_move_reference_ext(lynx_api_env env,
                                              lynx_value src_val,
                                              lynx_value_ref src_ref,
                                              lynx_value_ref* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_remove_reference_ext(lynx_api_env env,
                                                lynx_value value,
                                                lynx_value_ref ref) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_has_ref_count_ext(lynx_api_env env, lynx_value val,
                                             bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_uninitialized_ext(lynx_api_env env,
                                                lynx_value val, bool* result) {
  return lynx_api_not_support;
}
