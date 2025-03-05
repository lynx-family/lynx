/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define LYNX_VALUE_COMPILE_UNIT lepusng

#include "core/runtime/vm/lepus/lynx_value_lepusng.h"

#include <algorithm>

#include "core/runtime/vm/lepus/jsvalue_helper.h"

#define MAKE_LYNX_VALUE_FROM_LEPUS_VALUE(val)                                \
  {                                                                          \
    .val_ptr = reinterpret_cast<lynx_value_ptr>(LEPUS_VALUE_GET_INT64(val)), \
    .type = lynx_value_extended, .tag = LEPUS_VALUE_GET_TAG(val)             \
  }

// lynx_value_context
struct lynx_api_context__lepusng {
  lynx_api_context__lepusng(lynx_api_env env, LEPUSContext* ctx)
      : env(env), rt{LEPUS_GetRuntime(ctx)}, ctx{ctx} {
    env->ctx = this;
  }

  lynx_api_env env;
  LEPUSRuntime* rt{};
  LEPUSContext* ctx{};
};

// attach interface
#define DECLARE_METHOD(API)                                                    \
  static std::remove_pointer<decltype(lynx_api_env__::lynx_value_##API)>::type \
      lynx_value_##API;

FOR_EACH_LYNX_VALUE_CALL(DECLARE_METHOD)

#undef DECLARE_METHOD

void lynx_attach_value_lepusng(lynx_api_env env, LEPUSContext* ctx) {
#define SET_METHOD(API) env->lynx_value_##API = lynx_value_##API;

  FOR_EACH_LYNX_VALUE_CALL(SET_METHOD)

#undef SET_METHOD
  env->ctx = new lynx_api_context__lepusng(env, ctx);
}

void lynx_detach_value_lepusng(lynx_api_env env) {
  delete env->ctx;
  env->ctx = nullptr;
}

// implementations
namespace {

inline LEPUSValue WrapJSValue(const lynx_value& value) {
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
  return (LEPUSValue){.as_int64 = value.val_int64};
#else
  return LEPUS_MKPTR(value.tag, value.val_ptr);
#endif
}
}  // namespace

lynx_api_status lynx_value_typeof(lynx_api_env env, lynx_value value,
                                  lynx_value_type* result) {
  if (value.type != lynx_value_extended) {
    return lynx_api_invalid_arg;
  }
  // Keep the same logic as ToLepusValue in LEPUSValueHelper
  switch (value.tag) {
    case LEPUS_TAG_INT:
      *result = lynx_value_int32;
      break;
    case LEPUS_TAG_BIG_INT:
      *result = lynx_value_int64;
      break;
    case LEPUS_TAG_FLOAT64: {
      double d;
      LEPUS_ToFloat64(env->ctx->ctx, &d, WrapJSValue(value));
      if (lynx::base::StringConvertHelper::IsInt64Double(d)) {
        *result = lynx_value_int64;
      } else {
        *result = lynx_value_double;
      }
    } break;
    case LEPUS_TAG_UNDEFINED:
      *result = lynx_value_undefined;
      break;
    case LEPUS_TAG_NULL:
      *result = lynx_value_null;
      break;
    case LEPUS_TAG_BOOL:
      *result = lynx_value_bool;
      break;
    case LEPUS_TAG_LEPUS_CPOINTER:
      *result = lynx_value_external;
      break;
    case LEPUS_TAG_STRING:
    case LEPUS_TAG_SEPARABLE_STRING:
      *result = lynx_value_string;
      break;
    case LEPUS_TAG_LEPUS_REF:
      *result = lynx_value_object;
      break;
    case LEPUS_TAG_OBJECT: {
      auto js_value = WrapJSValue(value);
      LEPUSContext* context = reinterpret_cast<LEPUSContext*>(env->ctx->ctx);
      if (LEPUS_IsFunction(context, js_value)) {
        *result = lynx_value_function;
      } else {
        *result = lynx_value_object;
      }
    } break;
    default:
      LEPUSValue js_value = WrapJSValue(value);
      if (LEPUS_IsNumber(js_value)) {
        double d;
        LEPUS_ToFloat64(env->ctx->ctx, &d, js_value);
        if (lynx::base::StringConvertHelper::IsInt64Double(d)) {
          *result = lynx_value_int64;
        } else {
          *result = lynx_value_double;
        }
      }
      LOGE("lynx_value_typeof: unkown jsvalue type  " << value.tag);
  }
  return lynx_api_ok;
}

// For interfaces that are not needed in the LEPUSValue situation, leave the
// implementation empty for now and return lynx_api_not_support
lynx_api_status lynx_value_create_undefined(lynx_api_env env,
                                            lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_null(lynx_api_env env, lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_bool(lynx_api_env env, bool value,
                                       lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_double(lynx_api_env env, double value,
                                         lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_int32(lynx_api_env env, int32_t value,
                                        lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_uint32(lynx_api_env env, uint32_t value,
                                         lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_int64(lynx_api_env env, int64_t value,
                                        lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_uint64(lynx_api_env env, uint64_t value,
                                         lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_string_utf8(lynx_api_env env, const char* str,
                                              size_t length,
                                              lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_array(lynx_api_env env, lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_map(lynx_api_env env, lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_arraybuffer(lynx_api_env env,
                                              size_t byte_length, void** data,
                                              lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_create_function(lynx_api_env env,
                                           const char* utf8_name, size_t length,
                                           lynx_value_callback callback,
                                           void* data, lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_bool(lynx_api_env env, lynx_value value,
                                    bool* result) {
  auto js_value = WrapJSValue(value);
  if (!LEPUS_VALUE_IS_BOOL(js_value)) {
    *result = false;
    return lynx_api_bool_expected;
  }
  *result = LEPUS_VALUE_GET_BOOL(js_value);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_double(lynx_api_env env, lynx_value value,
                                      double* result) {
  auto js_value = WrapJSValue(value);
  if (!LEPUS_VALUE_IS_FLOAT64(js_value)) {
    *result = 0.f;
    return lynx_api_double_expected;
  }
  *result = LEPUS_VALUE_GET_FLOAT64(js_value);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_int32(lynx_api_env env, lynx_value value,
                                     int32_t* result) {
  auto js_value = WrapJSValue(value);
  if (!LEPUS_VALUE_IS_INT(js_value)) {
    return lynx_api_int32_expected;
  }
  *result = LEPUS_VALUE_GET_INT(js_value);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_uint32(lynx_api_env env, lynx_value value,
                                      uint32_t* result) {
  *result = 0;
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_int64(lynx_api_env env, lynx_value value,
                                     int64_t* result) {
  int ret = LEPUS_ToInt64(env->ctx->ctx, result, WrapJSValue(value));
  if (ret == -1) {
    return lynx_api_int64_expected;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_uint64(lynx_api_env env, lynx_value value,
                                      uint64_t* result) {
  *result = 0;
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_number(lynx_api_env env, lynx_value value,
                                      double* result) {
  int ret = LEPUS_ToFloat64(env->ctx->ctx, result, WrapJSValue(value));
  if (ret == -1) {
    return lynx_api_invalid_arg;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_external(lynx_api_env env, lynx_value value,
                                        void** result) {
  if (value.tag != LEPUS_TAG_LEPUS_CPOINTER) {
    *result = nullptr;
    return lynx_api_external_expected;
  }
  *result = LEPUS_VALUE_GET_PTR(WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_string_utf8(lynx_api_env env, lynx_value value,
                                           char* buf, size_t bufsize,
                                           size_t* result) {
  auto js_value = WrapJSValue(value);
  if (!LEPUS_VALUE_IS_STRING(js_value)) {
    return lynx_api_string_expected;
  }
  auto* ctx = env->ctx->ctx;
  size_t length;
  const char* str = LEPUS_ToCStringLen(ctx, &length, js_value);

  if (buf == nullptr) {
    *result = length;
  } else {
    size_t size{std::min(length, bufsize - 1)};
    std::copy(str, str + size, buf);
    buf[size] = '\0';
    if (result != nullptr) {
      *result = size;
    }
  }

  if (!LEPUS_IsGCMode(ctx)) {
    LEPUS_FreeCString(ctx, str);
  }

  return lynx_api_ok;
}

lynx_api_status lynx_value_is_array(lynx_api_env env, lynx_value value,
                                    bool* result) {
  int result_is = LEPUS_IsArray(env->ctx->ctx, WrapJSValue(value));
  if (result_is == -1) {
    *result = false;
    return lynx_api_failed;
  }
  *result = result_is;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_array_length(lynx_api_env env, lynx_value value,
                                            uint32_t* result) {
  LEPUSValue js_value = WrapJSValue(value);
  if (!LEPUS_IsArray(env->ctx->ctx, js_value)) {
    return lynx_api_array_expected;
  }
  int ret = lepus_get_length32(env->ctx->ctx, result, js_value);
  if (ret == -1) {
    return lynx_api_failed;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_set_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value value) {
  auto js_value = WrapJSValue(value);
  auto* ctx = env->ctx->ctx;
  LEPUS_DupValue(ctx, js_value);
  int ret = LEPUS_SetPropertyUint32(ctx, WrapJSValue(object), index, js_value);
  if (ret == -1) {
    return lynx_api_failed;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_has_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, bool* result) {
  LEPUSValue val =
      LEPUS_GetPropertyUint32(env->ctx->ctx, WrapJSValue(object), index);
  if (LEPUS_IsException(val)) {
    *result = false;
    return lynx_api_failed;
  }
  *result = !LEPUS_IsUndefined(val);
  LEPUS_FreeValue(env->ctx->ctx, val);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value* result) {
  LEPUSValue val =
      LEPUS_GetPropertyUint32(env->ctx->ctx, WrapJSValue(object), index);
  // The caller needs to manage val's memory by itself
  *result = MAKE_LYNX_VALUE_FROM_LEPUS_VALUE(val);
  return lynx_api_ok;
}

lynx_api_status lynx_value_delete_element(lynx_api_env env, lynx_value object,
                                          uint32_t index, bool* result) {
  auto* ctx = env->ctx->ctx;
  JSAtom atom = LEPUS_NewAtomUInt32(ctx, index);
  int ret =
      LEPUS_DeleteProperty(ctx, WrapJSValue(object), atom, LEPUS_PROP_THROW);
  LEPUS_FreeAtom(ctx, atom);
  if (ret == -1) {
    *result = false;
    return lynx_api_failed;
  }
  *result = ret;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_property_names(lynx_api_env env,
                                              const lynx_value object,
                                              lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_set_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value value) {
  auto js_value = WrapJSValue(value);
  auto* ctx = env->ctx->ctx;
  LEPUS_DupValue(ctx, js_value);
  int ret = lynx::lepus::LEPUSValueHelper::SetProperty(ctx, WrapJSValue(object),
                                                       utf8name, js_value);
  if (ret == -1) {
    return lynx_api_failed;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_has_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              bool* result) {
  *result = lynx::lepus::LEPUSValueHelper::HasProperty(
      env->ctx->ctx, WrapJSValue(object), utf8name);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value* result) {
  LEPUSValue val =
      LEPUS_GetPropertyStr(env->ctx->ctx, WrapJSValue(object), utf8name);
  // The caller needs to manage val's memory by itself
  *result = MAKE_LYNX_VALUE_FROM_LEPUS_VALUE(val);
  return lynx_api_ok;
}

lynx_api_status lynx_value_delete_named_property(lynx_api_env env,
                                                 lynx_value object,
                                                 const char* name) {
  auto* ctx = env->ctx->ctx;
  JSAtom atom = LEPUS_NewAtom(ctx, name);
  int ret =
      LEPUS_DeleteProperty(ctx, WrapJSValue(object), atom, LEPUS_PROP_THROW);
  LEPUS_FreeAtom(ctx, atom);
  if (ret == -1) {
    return lynx_api_failed;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_is_arraybuffer(lynx_api_env env, lynx_value value,
                                          bool* result) {
  *result = LEPUS_IsArrayBuffer(WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_arraybuffer_info(lynx_api_env env,
                                                lynx_value arraybuffer,
                                                void** data,
                                                size_t* byte_length) {
  size_t size;
  uint8_t* bytes =
      LEPUS_GetArrayBuffer(env->ctx->ctx, &size, WrapJSValue(arraybuffer));

  if (bytes == nullptr) {
    if (data) {
      *data = nullptr;
    }
    if (byte_length) {
      *byte_length = 0;
    }
    return lynx_api_failed;
  }

  if (data) {
    *data = static_cast<void*>(bytes);
  }
  if (byte_length) {
    *byte_length = size;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_call_function(lynx_api_env env, lynx_value recv,
                                         lynx_value func, size_t argc,
                                         const lynx_value* argv,
                                         lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_callback_info(
    lynx_api_env env, const lynx_value_callback_info info, size_t* argc,
    lynx_value* argv, lynx_value* this_arg, void** data) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_instance_data(lynx_api_env env, uint64_t key,
                                             void** result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_set_instance_data(lynx_api_env env, uint64_t key,
                                             void* data,
                                             lynx_value_finalizer finalizer,
                                             void* finalize_hint) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_equals(lynx_api_env env, lynx_value lhs,
                                  lynx_value rhs, bool* result) {
  LEPUSValue val =
      LEPUS_DeepEqual(env->ctx->ctx, WrapJSValue(lhs), WrapJSValue(rhs));
  *result = LEPUS_VALUE_GET_BOOL(val);
  return lynx_api_ok;
}

lynx_api_status lynx_value_create_reference(lynx_api_env env, lynx_value value,
                                            uint32_t initial_refcount,
                                            lynx_value_ref* result) {
  LEPUSValue val = WrapJSValue(value);
  if (LEPUS_IsGCMode(env->ctx->ctx)) {
    auto* p_val = new GCPersistent();
    p_val->Reset(env->ctx->rt, val);
    *result = reinterpret_cast<lynx_value_ref>(p_val);
  } else {
    LEPUS_DupValue(env->ctx->ctx, val);
    *result = reinterpret_cast<lynx_value_ref>(&value);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_delete_reference(lynx_api_env env,
                                            lynx_value_ref ref) {
  if (LEPUS_IsGCMode(env->ctx->ctx)) {
    auto* p_val = reinterpret_cast<GCPersistent*>(ref);
    if (p_val == nullptr) {
      return lynx_api_invalid_arg;
    }
    p_val->Reset(env->ctx->rt);
    delete p_val;
  } else {
    LEPUSValue val = WrapJSValue(*(reinterpret_cast<lynx_value*>(ref)));
    LEPUS_FreeValueRT(env->ctx->rt, val);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_move_reference(lynx_api_env env, lynx_value src_val,
                                          lynx_value_ref src_ref,
                                          lynx_value_ref* result) {
  if (LEPUS_IsGCMode(env->ctx->ctx)) {
    GCPersistent* dst_ref_val = nullptr;
    if (*result) {
      dst_ref_val = reinterpret_cast<GCPersistent*>(*result);
      dst_ref_val->Reset(env->ctx->rt);
    }
    if (src_ref) {
      auto* src_ref_val = reinterpret_cast<GCPersistent*>(*result);
      dst_ref_val = (dst_ref_val == nullptr) ? new GCPersistent() : dst_ref_val;
      dst_ref_val->Reset(env->ctx->rt, src_ref_val->Get());
      src_ref_val->Reset(env->ctx->rt);
    } else {
      dst_ref_val = (dst_ref_val == nullptr) ? new GCPersistent() : dst_ref_val;
      dst_ref_val->Reset(env->ctx->rt, WrapJSValue(src_val));
    }
    *result = reinterpret_cast<lynx_value_ref>(dst_ref_val);
  } else {
    *result = src_ref;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_reference_value(lynx_api_env env,
                                               lynx_value_ref ref,
                                               lynx_value* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_open_handle_scope(lynx_api_env env,
                                             lynx_value_handle_scope* result) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_close_handle_scope(lynx_api_env env,
                                              lynx_value_handle_scope scope) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_add_finalizer(lynx_api_env env, lynx_value value,
                                         void* finalize_data,
                                         lynx_value_finalizer finalizer,
                                         void* finalize_hint) {
  return lynx_api_not_support;
}

lynx_api_status lynx_value_get_length(lynx_api_env env, lynx_value value,
                                      uint32_t* result) {
  *result = (uint32_t)LEPUS_GetLength(env->ctx->ctx, WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_deep_copy_value(lynx_api_env env, lynx_value src,
                                           lynx_value* result) {
  LEPUSValue dst = LEPUS_DeepCopy(env->ctx->ctx, WrapJSValue(src));
  *result = MAKE_LYNX_VALUE_FROM_LEPUS_VALUE(dst);
  return lynx_api_ok;
}

lynx_api_status lynx_value_has_string_ref(lynx_api_env env, lynx_value value,
                                          bool* result) {
  auto js_value = WrapJSValue(value);
  if (!LEPUS_VALUE_IS_STRING(js_value)) {
    *result = false;
    return lynx_api_string_expected;
  }
  *result = true;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_string_ref(lynx_api_env env, lynx_value value,
                                          void** result) {
  auto js_value = WrapJSValue(value);
  if (!LEPUS_VALUE_IS_STRING(js_value)) {
    *result = nullptr;
    return lynx_api_failed;
  }
  *result = lynx::lepus::LEPUSValueHelper::ToLepusStringRefCountedImpl(
      env->ctx->ctx, js_value);
  return lynx_api_ok;
}
