// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_LYNX_VALUE_EXTENDED_H_
#define BASE_INCLUDE_VALUE_LYNX_VALUE_EXTENDED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "base/include/value/lynx_value_types.h"

#define MAKE_LYNX_VALUE(val, tag)                                  \
  {                                                                \
    .val_int64 = static_cast<int64_t>(LEPUS_VALUE_GET_INT64(val)), \
    .type = lynx_value_extended, .tag = tag                        \
  }
lynx_api_status lynx_value_get_bool_ext(lynx_api_env env, lynx_value value,
                                        bool* result);
lynx_api_status lynx_value_get_double_ext(lynx_api_env env, lynx_value value,
                                          double* result);
lynx_api_status lynx_value_get_int32_ext(lynx_api_env env, lynx_value value,
                                         int32_t* result);
lynx_api_status lynx_value_get_int64_ext(lynx_api_env env, lynx_value value,
                                         int64_t* result);
lynx_api_status lynx_value_is_integer_ext(lynx_api_env env, lynx_value value,
                                          bool* result);
lynx_api_status lynx_value_get_integer_ext(lynx_api_env env, lynx_value value,
                                           int64_t* result);
lynx_api_status lynx_value_get_number_ext(lynx_api_env env, lynx_value value,
                                          double* result);
lynx_api_status lynx_value_get_string_ref_ext(lynx_api_env env,
                                              lynx_value value, void** result);
lynx_api_status lynx_value_get_external_ext(lynx_api_env env, lynx_value value,
                                            void** result);
lynx_api_status lynx_value_get_length_ext(lynx_api_env env, lynx_value value,
                                          uint32_t* result);
lynx_api_status lynx_value_is_array_ext(lynx_api_env env, lynx_value value,
                                        bool* result);
lynx_api_status lynx_value_set_element_ext(lynx_api_env env, lynx_value object,
                                           uint32_t index, lynx_value value);
lynx_api_status lynx_value_get_element_ext(lynx_api_env env, lynx_value object,
                                           uint32_t index, lynx_value* result);
lynx_api_status lynx_value_is_map_ext(lynx_api_env env, lynx_value value,
                                      bool* result);
lynx_api_status lynx_value_set_named_property_ext(lynx_api_env env,
                                                  lynx_value object,
                                                  const char* utf8name,
                                                  lynx_value value);
lynx_api_status lynx_value_has_named_property_ext(lynx_api_env env,
                                                  lynx_value object,
                                                  const char* utf8name,
                                                  bool* result);
lynx_api_status lynx_value_get_named_property_ext(lynx_api_env env,
                                                  lynx_value object,
                                                  const char* utf8name,
                                                  lynx_value* result);
lynx_api_status lynx_value_is_function_ext(lynx_api_env env, lynx_value value,
                                           bool* result);
lynx_api_status lynx_value_to_string_utf8_ext(lynx_api_env env,
                                              lynx_value value, void* result);
lynx_api_status lynx_value_typeof_ext(lynx_api_env env, lynx_value value,
                                      lynx_value_type* result);
lynx_api_status lynx_value_iterate_value_ext(
    lynx_api_env env, lynx_value object, lynx_value_iterator_callback callback,
    void* pfunc, void* raw_data);
lynx_api_status lynx_value_equals_ext(lynx_api_env env, lynx_value lhs,
                                      lynx_value rhs, bool* result);
lynx_api_status lynx_value_deep_copy_value_ext(lynx_api_env env, lynx_value src,
                                               lynx_value* result);
lynx_api_status lynx_value_print_ext(lynx_api_env env, lynx_value value,
                                     void* stream,
                                     lynx_value_print_ext_callback callback);
lynx_api_status lynx_value_add_reference_ext(lynx_api_env env, lynx_value value,
                                             lynx_value_ref* result);
lynx_api_status lynx_value_add_reference_weak_ext(lynx_api_env env,
                                                  lynx_value value,
                                                  lynx_value_ref* result);
lynx_api_status lynx_value_move_reference_ext(lynx_api_env env,
                                              lynx_value src_val,
                                              lynx_value_ref src_ref,
                                              lynx_value_ref* result);
lynx_api_status lynx_value_remove_reference_ext(lynx_api_env env,
                                                lynx_value value,
                                                lynx_value_ref ref);
lynx_api_status lynx_value_has_ref_count_ext(lynx_api_env env, lynx_value val,
                                             bool* result);
lynx_api_status lynx_value_is_uninitialized_ext(lynx_api_env env,
                                                lynx_value val, bool* result);

#ifdef __cplusplus
}
#endif

#endif  // BASE_INCLUDE_VALUE_LYNX_VALUE_EXTENDED_H_
