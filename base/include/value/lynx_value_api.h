/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_LYNX_VALUE_API_H_
#define BASE_INCLUDE_VALUE_LYNX_VALUE_API_H_

#include "base/include/value/lynx_value_types.h"

#define FOR_EACH_LYNX_VALUE_CALL(V) \
  V(typeof)                         \
  V(create_undefined)               \
  V(create_null)                    \
  V(create_bool)                    \
  V(create_double)                  \
  V(create_int32)                   \
  V(create_uint32)                  \
  V(create_int64)                   \
  V(create_uint64)                  \
  V(create_string_utf8)             \
  V(create_array)                   \
  V(create_map)                     \
  V(create_arraybuffer)             \
  V(create_function)                \
  V(get_bool)                       \
  V(get_double)                     \
  V(get_int32)                      \
  V(get_uint32)                     \
  V(get_int64)                      \
  V(get_uint64)                     \
  V(get_number)                     \
  V(get_external)                   \
  V(get_string_utf8)                \
  V(is_array)                       \
  V(get_array_length)               \
  V(set_element)                    \
  V(has_element)                    \
  V(get_element)                    \
  V(delete_element)                 \
  V(get_property_names)             \
  V(set_named_property)             \
  V(has_named_property)             \
  V(get_named_property)             \
  V(delete_named_property)          \
  V(is_arraybuffer)                 \
  V(get_arraybuffer_info)           \
  V(call_function)                  \
  V(get_callback_info)              \
  V(set_instance_data)              \
  V(get_instance_data)              \
  V(equals)                         \
  V(create_reference)               \
  V(delete_reference)               \
  V(get_reference_value)            \
  V(open_handle_scope)              \
  V(close_handle_scope)             \
  V(get_length)                     \
  V(deep_copy_value)                \
  V(has_string_ref)                 \
  V(get_string_ref)

struct lynx_api_env__ {
  lynx_api_state state;
  lynx_api_context ctx;
  lynx_api_runtime rt;

  // type
  lynx_api_status (*lynx_value_typeof)(lynx_api_env env, lynx_value value,
                                       lynx_value_type* result);

  lynx_api_status (*lynx_value_create_undefined)(lynx_api_env env,
                                                 lynx_value* result);
  lynx_api_status (*lynx_value_create_null)(lynx_api_env env,
                                            lynx_value* result);
  lynx_api_status (*lynx_value_create_bool)(lynx_api_env env, bool value,
                                            lynx_value* result);

  // creator
  lynx_api_status (*lynx_value_create_double)(lynx_api_env env, double value,
                                              lynx_value* result);
  lynx_api_status (*lynx_value_create_int32)(lynx_api_env env, int32_t value,
                                             lynx_value* result);
  lynx_api_status (*lynx_value_create_uint32)(lynx_api_env env, uint32_t value,
                                              lynx_value* result);
  lynx_api_status (*lynx_value_create_int64)(lynx_api_env env, int64_t value,
                                             lynx_value* result);
  lynx_api_status (*lynx_value_create_uint64)(lynx_api_env env, uint64_t value,
                                              lynx_value* result);
  lynx_api_status (*lynx_value_create_string_utf8)(lynx_api_env env,
                                                   const char* str,
                                                   size_t length,
                                                   lynx_value* result);
  lynx_api_status (*lynx_value_create_array)(lynx_api_env env,
                                             lynx_value* result);
  lynx_api_status (*lynx_value_create_map)(lynx_api_env env,
                                           lynx_value* result);
  lynx_api_status (*lynx_value_create_arraybuffer)(lynx_api_env env,
                                                   size_t byte_length,
                                                   void** data,
                                                   lynx_value* result);
  lynx_api_status (*lynx_value_create_function)(lynx_api_env env,
                                                const char* utf8_name,
                                                size_t length,
                                                lynx_value_callback callback,
                                                void* data, lynx_value* result);

  // getter
  lynx_api_status (*lynx_value_get_bool)(lynx_api_env env, lynx_value value,
                                         bool* result);
  lynx_api_status (*lynx_value_get_double)(lynx_api_env env, lynx_value value,
                                           double* result);
  lynx_api_status (*lynx_value_get_int32)(lynx_api_env env, lynx_value value,
                                          int32_t* result);
  lynx_api_status (*lynx_value_get_uint32)(lynx_api_env env, lynx_value value,
                                           uint32_t* result);
  lynx_api_status (*lynx_value_get_int64)(lynx_api_env env, lynx_value value,
                                          int64_t* result);
  lynx_api_status (*lynx_value_get_uint64)(lynx_api_env env, lynx_value value,
                                           uint64_t* result);
  lynx_api_status (*lynx_value_get_number)(lynx_api_env env, lynx_value value,
                                           double* result);
  lynx_api_status (*lynx_value_get_external)(lynx_api_env env, lynx_value value,
                                             void** result);
  // If buf is nullptr, the *result will be the length of arraybuffer value.
  // Otherwise, the *result will be the minimum of bufsize and length of
  // arraybuffer value
  lynx_api_status (*lynx_value_get_string_utf8)(lynx_api_env env,
                                                lynx_value value, char* buf,
                                                size_t bufsize, size_t* result);

  // array
  lynx_api_status (*lynx_value_is_array)(lynx_api_env env, lynx_value value,
                                         bool* result);
  lynx_api_status (*lynx_value_get_array_length)(lynx_api_env env,
                                                 lynx_value value,
                                                 uint32_t* result);
  lynx_api_status (*lynx_value_set_element)(lynx_api_env env, lynx_value object,
                                            uint32_t index, lynx_value value);
  lynx_api_status (*lynx_value_has_element)(lynx_api_env env, lynx_value object,
                                            uint32_t index, bool* result);
  lynx_api_status (*lynx_value_get_element)(lynx_api_env env, lynx_value object,
                                            uint32_t index, lynx_value* result);
  lynx_api_status (*lynx_value_delete_element)(lynx_api_env env,
                                               lynx_value object,
                                               uint32_t index, bool* result);

  // map
  lynx_api_status (*lynx_value_get_property_names)(lynx_api_env env,
                                                   const lynx_value object,
                                                   lynx_value* result);
  lynx_api_status (*lynx_value_set_named_property)(lynx_api_env env,
                                                   lynx_value object,
                                                   const char* utf8name,
                                                   lynx_value value);
  lynx_api_status (*lynx_value_has_named_property)(lynx_api_env env,
                                                   lynx_value object,
                                                   const char* utf8name,
                                                   bool* result);
  lynx_api_status (*lynx_value_get_named_property)(lynx_api_env env,
                                                   lynx_value object,
                                                   const char* utf8name,
                                                   lynx_value* result);
  lynx_api_status (*lynx_value_delete_named_property)(lynx_api_env env,
                                                      lynx_value object,
                                                      const char* name);

  // arraybuffer
  lynx_api_status (*lynx_value_is_arraybuffer)(lynx_api_env env,
                                               lynx_value value, bool* result);
  lynx_api_status (*lynx_value_get_arraybuffer_info)(lynx_api_env env,
                                                     lynx_value arraybuffer,
                                                     void** data,
                                                     size_t* byte_length);

  // function
  lynx_api_status (*lynx_value_call_function)(lynx_api_env env, lynx_value recv,
                                              lynx_value func, size_t argc,
                                              const lynx_value* argv,
                                              lynx_value* result);
  lynx_api_status (*lynx_value_get_callback_info)(
      lynx_api_env env, const lynx_value_callback_info info, size_t* argc,
      lynx_value* argv, lynx_value* this_arg, void** data);

  // instance data
  lynx_api_status (*lynx_value_set_instance_data)(
      lynx_api_env env, uint64_t key, void* data,
      lynx_value_finalizer finalizer, void* finalize_hint);
  lynx_api_status (*lynx_value_get_instance_data)(lynx_api_env env,
                                                  uint64_t key, void** result);

  // equals
  lynx_api_status (*lynx_value_equals)(lynx_api_env env, lynx_value lhs,
                                       lynx_value rhs, bool* result);

  // reference
  lynx_api_status (*lynx_value_create_reference)(lynx_api_env env,
                                                 lynx_value value,
                                                 uint32_t initial_refcount,
                                                 lynx_value_ref* result);
  lynx_api_status (*lynx_value_delete_reference)(lynx_api_env env,
                                                 lynx_value_ref ref);
  lynx_api_status (*lynx_value_get_reference_value)(lynx_api_env env,
                                                    lynx_value_ref ref,
                                                    lynx_value* result);

  // handle scope
  lynx_api_status (*lynx_value_open_handle_scope)(
      lynx_api_env env, lynx_value_handle_scope* result);
  lynx_api_status (*lynx_value_close_handle_scope)(
      lynx_api_env env, lynx_value_handle_scope scope);

  // finalizer
  lynx_api_status (*lynx_value_add_finalizer)(lynx_api_env env,
                                              lynx_value value,
                                              void* finalize_data,
                                              lynx_value_finalizer finalizer,
                                              void* finalize_hint);

  // others
  lynx_api_status (*lynx_value_get_length)(lynx_api_env env, lynx_value value,
                                           uint32_t* result);

  lynx_api_status (*lynx_value_deep_copy_value)(lynx_api_env env,
                                                lynx_value src,
                                                lynx_value* result);

  lynx_api_status (*lynx_value_has_string_ref)(lynx_api_env env,
                                               lynx_value value, bool* result);
  // Get a string object raw pointer.
  lynx_api_status (*lynx_value_get_string_ref)(lynx_api_env env,
                                               lynx_value value, void** result);
};

#endif  // BASE_INCLUDE_VALUE_LYNX_VALUE_API_H_
