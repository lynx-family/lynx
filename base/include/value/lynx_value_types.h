/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_LYNX_VALUE_TYPES_H_
#define BASE_INCLUDE_VALUE_LYNX_VALUE_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#include "base/include/value/lynx_api_types.h"

typedef struct LYNX_VALUE_OPAQUE_STRUCT(value_ref) * lynx_value_ref;
typedef struct LYNX_VALUE_OPAQUE_STRUCT(value_handle_scope) *
    lynx_value_handle_scope;
typedef struct LYNX_VALUE_OPAQUE_STRUCT(value_callback_info) *
    lynx_value_callback_info;

typedef void (*lynx_value_finalizer)(lynx_api_env env, void* finalize_data,
                                     void* finalize_hint);

typedef enum {
  lynx_value_null,
  lynx_value_undefined,
  lynx_value_bool,
  lynx_value_double,
  lynx_value_int32,
  lynx_value_uint32,
  lynx_value_int64,
  lynx_value_uint64,
  lynx_value_nan,
  lynx_value_string,
  lynx_value_array,
  lynx_value_map,
  lynx_value_arraybuffer,
  lynx_value_function,
  lynx_value_object,
  lynx_value_external,  // external raw pointer
  lynx_value_extended,
} lynx_value_type;

struct lynx_value {
  union {
    bool val_bool;
    double val_double;
    int32_t val_int32;
    uint32_t val_uint32;
    int64_t val_int64;
    uint64_t val_uint64;
    lynx_value_ptr val_ptr;
  };

  lynx_value_type type = lynx_value_null;
  int64_t tag = 0;
};

typedef lynx_value (*lynx_value_callback)(lynx_api_env env,
                                          const lynx_value_callback_info info);

#endif  // BASE_INCLUDE_VALUE_LYNX_VALUE_TYPES_H_
