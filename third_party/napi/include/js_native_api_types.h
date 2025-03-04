/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

#ifndef PRIMJS_SRC_JS_NATIVE_API_TYPES_H_
#define PRIMJS_SRC_JS_NATIVE_API_TYPES_H_

// This file needs to be compatible with C compilers.
// This is a public include file, and these includes have essentially
// became part of it's API.
#include <stddef.h>  // NOLINT(modernize-deprecated-headers)
#include <stdint.h>  // NOLINT(modernize-deprecated-headers)
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif

#if !defined __cplusplus || (defined(_MSC_VER) && _MSC_VER < 1900)
typedef uint16_t char16_t;
#endif

#ifndef NAPI_COMPILE_UNIT
#define NAPI_COMPILE_UNIT
#endif

#define NAPI_OPAQUE_STRUCT__2(name, unit) napi_##name##__##unit
#define NAPI_OPAQUE_STRUCT__1(name, unit) NAPI_OPAQUE_STRUCT__2(name, unit)
#define NAPI_OPAQUE_STRUCT(name) NAPI_OPAQUE_STRUCT__1(name, NAPI_COMPILE_UNIT)

// JSVM API types are all opaque pointers for ABI stability
// typedef undefined structs instead of void* for compile time type safety
typedef struct napi_env__* napi_env;
typedef struct napi_state__* napi_state;
typedef struct napi_value__* napi_value;

typedef struct NAPI_OPAQUE_STRUCT(context) * napi_context;
typedef struct NAPI_OPAQUE_STRUCT(runtime) * napi_runtime;
typedef struct NAPI_OPAQUE_STRUCT(ref) * napi_ref;
typedef struct NAPI_OPAQUE_STRUCT(context_scope) * napi_context_scope;
typedef struct NAPI_OPAQUE_STRUCT(handle_scope) * napi_handle_scope;
typedef struct NAPI_OPAQUE_STRUCT(error_scope) * napi_error_scope;
typedef struct NAPI_OPAQUE_STRUCT(escapable_handle_scope) *
    napi_escapable_handle_scope;
typedef struct NAPI_OPAQUE_STRUCT(callback_info) * napi_callback_info;
typedef struct NAPI_OPAQUE_STRUCT(deferred) * napi_deferred;
typedef struct NAPI_OPAQUE_STRUCT(class) * napi_class;

typedef struct NAPI_OPAQUE_STRUCT(async_work) * napi_async_work;
typedef struct NAPI_OPAQUE_STRUCT(threadsafe_function) *
    napi_threadsafe_function;

typedef enum {
  napi_deferred_resolve,
  napi_deferred_reject,
  napi_deferred_delete
} napi_deferred_release_mode;

typedef enum {
  napi_tsfn_nonblocking,
  napi_tsfn_blocking
} napi_threadsafe_function_call_mode;

typedef enum {
  napi_default = 0,
  napi_writable = 1 << 0,
  napi_enumerable = 1 << 1,
  napi_configurable = 1 << 2,
  // Used with napi_define_class to distinguish static properties
  // from instance properties. Ignored by napi_define_properties.
  napi_static = 1 << 10,

  // Default for class methods.
  napi_default_method = napi_writable | napi_configurable,

  // Default for object properties, like in JS obj[prop].
  napi_default_jsproperty = napi_writable | napi_enumerable | napi_configurable,
} napi_property_attributes;

typedef enum {
  // ES6 types (corresponds to typeof)
  napi_undefined,
  napi_null,
  napi_boolean,
  napi_number,
  napi_string,
  napi_symbol,
  napi_object,
  napi_function,
  napi_external,
  napi_bigint,
} napi_valuetype;

typedef enum {
  napi_int8_array,
  napi_uint8_array,
  napi_uint8_clamped_array,
  napi_int16_array,
  napi_uint16_array,
  napi_int32_array,
  napi_uint32_array,
  napi_float32_array,
  napi_float64_array,
  napi_bigint64_array,
  napi_biguint64_array,
} napi_typedarray_type;

typedef enum {
  napi_ok,
  napi_invalid_arg,
  napi_object_expected,
  napi_string_expected,
  napi_name_expected,
  napi_function_expected,
  napi_number_expected,
  napi_boolean_expected,
  napi_array_expected,
  napi_generic_failure,
  napi_pending_exception,
  napi_cancelled,
  napi_escape_called_twice,
  napi_handle_scope_mismatch,
  napi_callback_scope_mismatch,
  napi_queue_full,
  napi_closing,
  napi_bigint_expected,
  napi_date_expected,
  napi_arraybuffer_expected,
  napi_detachable_arraybuffer_expected,
  napi_conflict_instance_data,
  napi_context_scope_mismatch
} napi_status;

// Note: when adding a new enum value to `napi_status`, please also update
// `const int last_status` in `napi_get_last_error_info()' definition,
// in file js_native_api_v8.cc. Please also update the definition of
// `napi_status` in doc/api/n-api.md to reflect the newly added value(s).

typedef napi_value (*napi_callback)(napi_env env, napi_callback_info info);
typedef void (*napi_finalize)(napi_env env, void* finalize_data,
                              void* finalize_hint);

typedef void (*napi_async_execute_callback)(napi_env env, void* data);
typedef void (*napi_async_complete_callback)(napi_env env, napi_status status,
                                             void* data);
typedef void (*napi_threadsafe_function_call_js)(napi_env env, void* context,
                                                 void* data);

typedef struct {
  // One of utf8name or name should be NULL.
  const char* utf8name;
  napi_value name;

  napi_callback method;
  napi_callback getter;
  napi_callback setter;
  napi_value value;

  napi_property_attributes attributes;
  void* data;
} napi_property_descriptor;

typedef struct {
  const char* error_message;
  void* engine_reserved;
  uint32_t engine_error_code;
  napi_status error_code;
} napi_extended_error_info;

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif
#endif  // SRC_JS_NATIVE_API_TYPES_H_
