// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/lynx_value_extended.h"

lynx_api_status lynx_value_get_bool(lynx_api_env env, lynx_value value,
                                    bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_double(lynx_api_env env, lynx_value value,
                                      double* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_int32(lynx_api_env env, lynx_value value,
                                     int32_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_int64(lynx_api_env env, lynx_value value,
                                     int64_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_integer(lynx_api_env env, lynx_value value,
                                      bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_integer(lynx_api_env env, lynx_value value,
                                       int64_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_number(lynx_api_env env, lynx_value value,
                                      double* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_string_ref(lynx_api_env env, lynx_value value,
                                          void** result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_external(lynx_api_env env, lynx_value value,
                                        void** result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_length(lynx_api_env env, lynx_value value,
                                      uint32_t* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_array(lynx_api_env env, lynx_value value,
                                    bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_set_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value value) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_map(lynx_api_env env, lynx_value value,
                                  bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_set_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value value) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_has_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_function(lynx_api_env env, lynx_value value,
                                       bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_to_string_utf8(lynx_api_env env, lynx_value value,
                                          void* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_typeof(lynx_api_env env, lynx_value value,
                                  lynx_value_type* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_iterate_value(lynx_api_env env, lynx_value object,
                                         lynx_value_iterator_callback callback,
                                         void* pfunc, void* raw_data) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_equals(lynx_api_env env, lynx_value lhs,
                                  lynx_value rhs, bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_deep_copy_value(lynx_api_env env, lynx_value src,
                                           lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_print(lynx_api_env env, lynx_value value,
                                 void* stream,
                                 lynx_value_print_callback callback) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_add_reference(lynx_api_env env, lynx_value value,
                                         lynx_value_ref* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_add_reference_weak(lynx_api_env env,
                                              lynx_value value,
                                              lynx_value_ref* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_move_reference(lynx_api_env env, lynx_value src_val,
                                          lynx_value_ref src_ref,
                                          lynx_value_ref* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_remove_reference(lynx_api_env env, lynx_value value,
                                            lynx_value_ref ref) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_has_ref_count(lynx_api_env env, lynx_value val,
                                         bool* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_is_uninitialized(lynx_api_env env, lynx_value val,
                                            bool* result) {
  return lynx_api_not_support;
}
