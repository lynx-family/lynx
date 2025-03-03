/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_LYNX_API_TYPES_H_
#define BASE_INCLUDE_VALUE_LYNX_API_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#ifndef LYNX_VALUE_COMPILE_UNIT
#define LYNX_VALUE_COMPILE_UNIT
#endif

#define LYNX_VALUE_OPAQUE_STRUCT__2(name, unit) lynx_##name##__##unit
#define LYNX_VALUE_OPAQUE_STRUCT__1(name, unit) \
  LYNX_VALUE_OPAQUE_STRUCT__2(name, unit)
#define LYNX_VALUE_OPAQUE_STRUCT(name) \
  LYNX_VALUE_OPAQUE_STRUCT__1(name, LYNX_VALUE_COMPILE_UNIT)

typedef struct lynx_value_ptr__* lynx_value_ptr;
typedef struct lynx_api_env__* lynx_api_env;
typedef struct lynx_api_state__* lynx_api_state;

typedef struct LYNX_VALUE_OPAQUE_STRUCT(api_context) * lynx_api_context;
typedef struct LYNX_VALUE_OPAQUE_STRUCT(api_runtime) * lynx_api_runtime;

typedef enum {
  lynx_api_ok,
  lynx_api_failed,
  lynx_api_invalid_arg,
  lynx_api_not_support,
  lynx_api_bool_expected,
  lynx_api_double_expected,
  lynx_api_int32_expected,
  lynx_api_uint32_expected,
  lynx_api_int64_expected,
  lynx_api_uint64_expected,
  lynx_api_string_expected,
  lynx_api_array_expected,
  lynx_api_map_expected,
  lynx_api_external_expected,
  lynx_api_arraybuffer_expected,
  lynx_api_function_expected,
  lynx_api_index_out_of_range,
  lynx_api_handle_scope_mismatch,
} lynx_api_status;

typedef struct {
  const char* error_message;
  uint32_t error_message_length;
  uint32_t engine_error_code;
  lynx_api_status error_code;
} lynx_api_extended_error_info;

#endif  // BASE_INCLUDE_VALUE_LYNX_API_TYPES_H_
