// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define LYNX_VALUE_COMPILE_UNIT lepusng

#include "base/include/value/lynx_value_extended.h"

#include <string>

#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_context_cell.h"
#include "core/runtime/vm/lepus/lynx_api_context_lepusng.h"
#ifdef OS_IOS
#include "persistent-handle.h"
#else
#include "quickjs/include/persistent-handle.h"
#endif

struct iterator_raw_data {
  lynx_api_env env;
  void* pfunc;
  void* raw_data;
};

namespace {
inline LEPUSValue WrapJSValue(const lynx_value& value) {
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
  return (LEPUSValue){.as_int64 = value.val_int64};
#elif defined(LEPUS_NAN_BOXING)
  // FIXME(chenyouhui): Use LEPUS_NAN_BOXING that is defined in PrimJS
  // temporarily. The PrimJS should provide a more appropriate interface.
  return value.val_uint64;
#else
  return LEPUS_MKPTR(static_cast<int8_t>((value.tag & 0xff)), value.val_ptr);
#endif
}

inline lynx_value MakeLynxValue(const LEPUSValue& val) {
  int64_t val_tag = LEPUS_VALUE_GET_NORM_TAG(val);
  int32_t tag =
      (static_cast<int32_t>(
           lynx::lepus::LEPUSValueHelper::LEPUSValueTagToLynxValueType(val_tag))
       << 16) |
      (val_tag & 0xff);
  return MAKE_LYNX_VALUE(val, tag);
}

}  // namespace

lynx_api_env lynx_value_api_new_env(LEPUSContext* ctx) {
  auto* env = new lynx_api_env__();
  env->ctx = new lynx_api_context__lepusng(env, ctx);
  return env;
}

void lynx_value_api_delete_env(lynx_api_env env) {
  if (!env) return;
  if (env->ctx) {
    delete env->ctx;
  }
  delete env;
}

void lynx_value_api_detach_context_from_env(lynx_api_env env) {
  if (!env || !env->ctx) return;
  delete env->ctx;
  env->ctx = nullptr;
}

LEPUSContext* lynx_value_api_get_context_from_env(lynx_api_env env) {
  if (env) {
    return env->ctx->ctx;
  }
  return nullptr;
}

lynx_api_status lynx_value_get_bool(lynx_api_env env, lynx_value value,
                                    bool* result) {
  *result = LEPUS_VALUE_GET_BOOL(WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_double(lynx_api_env env, lynx_value value,
                                      double* result) {
  *result = LEPUS_VALUE_GET_FLOAT64(WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_int32(lynx_api_env env, lynx_value value,
                                     int32_t* result) {
  *result = LEPUS_VALUE_GET_INT(WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_int64(lynx_api_env env, lynx_value value,
                                     int64_t* result) {
  auto js_value = WrapJSValue(value);
  if (LEPUS_VALUE_IS_BIG_INT(js_value)) {
    int ret = LEPUS_ToInt64(env->ctx->ctx, result, js_value);
    if (ret != -1) {
      return lynx_api_ok;
    }
  } else if (LEPUS_VALUE_IS_FLOAT64(js_value)) {
    double d;
    LEPUS_ToFloat64(env->ctx->ctx, &d, js_value);
    if (lynx::base::StringConvertHelper::IsInt64Double(d)) {
      *result = static_cast<int64_t>(d);
      return lynx_api_ok;
    }
  }
  *result = 0;
  return lynx_api_int64_expected;
}

lynx_api_status lynx_value_is_integer(lynx_api_env env, lynx_value value,
                                      bool* result) {
  LEPUSValue temp_val = WrapJSValue(value);
  if (LEPUS_IsInteger(temp_val)) {
    *result = true;
    return lynx_api_ok;
  }
  if (LEPUS_IsNumber(temp_val)) {
    double val;
    LEPUS_ToFloat64(env->ctx->ctx, &val, temp_val);
    if (lynx::base::StringConvertHelper::IsInt64Double(val)) {
      *result = true;
      return lynx_api_ok;
    }
  }
  *result = false;
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_integer(lynx_api_env env, lynx_value value,
                                       int64_t* result) {
  LEPUSValue temp_val = WrapJSValue(value);
  if (LEPUS_VALUE_GET_NORM_TAG(temp_val) == LEPUS_TAG_INT) {
    *result = LEPUS_VALUE_GET_INT(temp_val);
    return lynx_api_ok;
  }
  if (LEPUS_IsInteger(temp_val)) {
    LEPUS_ToInt64(env->ctx->ctx, result, temp_val);
  } else {
    DCHECK(LEPUS_IsNumber(temp_val));
    double val;
    LEPUS_ToFloat64(env->ctx->ctx, &val, temp_val);
    *result = static_cast<int64_t>(val);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_number(lynx_api_env env, lynx_value value,
                                      double* result) {
  auto js_value = WrapJSValue(value);
  if (LEPUS_VALUE_IS_INT(js_value)) {
    *result = LEPUS_VALUE_GET_INT(js_value);
  } else if (LEPUS_VALUE_IS_FLOAT64(js_value)) {
    *result = LEPUS_VALUE_GET_FLOAT64(js_value);
  } else if (LEPUS_VALUE_IS_BIG_INT(js_value)) {
    LEPUS_ToFloat64(env->ctx->ctx, result, js_value);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_string_ref(lynx_api_env env, lynx_value value,
                                          void** result) {
  auto val = WrapJSValue(value);
  auto* ctx = env->ctx->ctx;
  void* cache = LEPUS_IsGCMode(ctx) ? LEPUS_GetStringCache_GC(val)
                                    : LEPUS_GetStringCache(val);
  if (cache == nullptr) {
    auto ptr = lynx::base::RefCountedStringImpl::Unsafe::RawCreate(
        lynx::lepus::LEPUSValueHelper::ToStdString(ctx, val));
    LEPUS_SetStringCache(ctx, val, ptr);
    cache = ptr;
    ptr->Release();
  }
  *result = reinterpret_cast<lynx::base::RefCountedStringImpl*>(cache);
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_external(lynx_api_env env, lynx_value value,
                                        void** result) {
  *result = LEPUS_VALUE_GET_CPOINTER(WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_length(lynx_api_env env, lynx_value value,
                                      uint32_t* result) {
  *result = (uint32_t)LEPUS_GetLength(env->ctx->ctx, WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_is_array(lynx_api_env env, lynx_value value,
                                    bool* result) {
  LEPUSValue js_value = WrapJSValue(value);
  *result = LEPUS_IsArray(env->ctx->ctx, js_value) ||
            LEPUS_GetLepusRefTag(js_value) == lynx::lepus::Value_Array;
  return lynx_api_ok;
}

lynx_api_status lynx_value_set_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value value) {
  LEPUSValue js_value;
  auto* ctx = env->ctx->ctx;
  if (value.type == lynx_value_extended) {
    js_value = WrapJSValue(value);
    LEPUS_DupValue(ctx, js_value);
  } else {
    js_value = lynx::lepus::LEPUSValueHelper::ToJsValue(ctx, value);
  }
  HandleScope block_scope(ctx, &js_value, HANDLE_TYPE_LEPUS_VALUE);
  int ret = LEPUS_SetPropertyUint32(ctx, WrapJSValue(object), index, js_value);
  if (ret == -1) {
    return lynx_api_failed;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_get_element(lynx_api_env env, lynx_value object,
                                       uint32_t index, lynx_value* result) {
  LEPUSValue val =
      LEPUS_GetPropertyUint32(env->ctx->ctx, WrapJSValue(object), index);
  if (LEPUS_IsLepusRef(val)) {
    *result = lynx::lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(
        env->ctx->ctx, val);
    if (!LEPUS_IsGCMode(env->ctx->ctx)) LEPUS_FreeValue(env->ctx->ctx, val);
  } else {
    *result = MakeLynxValue(val);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_is_map(lynx_api_env env, lynx_value value,
                                  bool* result) {
  LEPUSValue js_value = WrapJSValue(value);
  *result = LEPUS_IsObject(js_value) ||
            (LEPUS_GetLepusRefTag(js_value) == lynx::lepus::Value_Table);
  return lynx_api_ok;
}

lynx_api_status lynx_value_set_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value value) {
  LEPUSValue js_value;
  auto* ctx = env->ctx->ctx;
  if (value.type == lynx_value_extended) {
    js_value = WrapJSValue(value);
    LEPUS_DupValue(ctx, js_value);
  } else {
    js_value = lynx::lepus::LEPUSValueHelper::ToJsValue(ctx, value);
  }
  HandleScope block_scope(ctx, &js_value, HANDLE_TYPE_LEPUS_VALUE);
  int ret = LEPUS_SetPropertyStr(ctx, WrapJSValue(object), utf8name, js_value);
  if (ret == -1) {
    return lynx_api_failed;
  }
  return lynx_api_ok;
}
lynx_api_status lynx_value_has_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              bool* result) {
  auto* ctx = env->ctx->ctx;
  HandleScope func_scope(ctx);
  LEPUSAtom atom = LEPUS_NewAtom(ctx, utf8name);
  func_scope.PushLEPUSAtom(atom);
  int32_t ret = LEPUS_HasProperty(ctx, WrapJSValue(object), atom);
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeAtom(ctx, atom);
  *result = !!ret;
  return lynx_api_ok;
}
lynx_api_status lynx_value_get_named_property(lynx_api_env env,
                                              lynx_value object,
                                              const char* utf8name,
                                              lynx_value* result) {
  LEPUSValue val =
      LEPUS_GetPropertyStr(env->ctx->ctx, WrapJSValue(object), utf8name);
  if (LEPUS_IsLepusRef(val)) {
    *result = lynx::lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(
        env->ctx->ctx, val);
    if (!LEPUS_IsGCMode(env->ctx->ctx)) LEPUS_FreeValue(env->ctx->ctx, val);
  } else {
    *result = MakeLynxValue(val);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_is_function(lynx_api_env env, lynx_value value,
                                       bool* result) {
  *result = LEPUS_IsFunction(env->ctx->ctx, WrapJSValue(value));
  return lynx_api_ok;
}

lynx_api_status lynx_value_to_string_utf8(lynx_api_env env, lynx_value value,
                                          void* result) {
  LEPUSValue val = WrapJSValue(value);
  if (LEPUS_IsUndefined(val)) {
    (*reinterpret_cast<std::string*>(result)) = "";
    return lynx_api_ok;
  }
  LEPUSContext* ctx = env->ctx->ctx;
  if (LEPUS_IsLepusRef(val)) {
    (*reinterpret_cast<std::string*>(result)) =
        lynx::lepus::LEPUSValueHelper::LepusRefToStdString(ctx, val);
    return lynx_api_ok;
  } else if (LEPUS_VALUE_IS_STRING(val)) {
    auto* str = LEPUS_GetStringUtf8(ctx, LEPUS_VALUE_GET_STRING(val));
    if (str) {
      (*reinterpret_cast<std::string*>(result)) = str;
      return lynx_api_ok;
    }
  }
  size_t len;
  const char* chr = LEPUS_ToCStringLen(ctx, &len, val);
  if (chr) {
    (*reinterpret_cast<std::string*>(result)) = std::string(chr, len);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, chr);
  } else {
    (*reinterpret_cast<std::string*>(result)) = "";
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_typeof(lynx_api_env env, lynx_value value,
                                  lynx_value_type* result) {
  if (value.type != lynx_value_extended) {
    return lynx_api_invalid_arg;
  }
  LEPUSValue val = WrapJSValue(value);
  switch (LEPUS_VALUE_GET_NORM_TAG(val)) {
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
    case LEPUS_TAG_LEPUS_REF: {
      int tag = LEPUS_GetLepusRefTag(WrapJSValue(value));
      *result = lynx::lepus::Value::ToLynxValueType(
          static_cast<lynx::lepus::ValueType>(tag));
    } break;
    case LEPUS_TAG_OBJECT: {
      auto js_value = WrapJSValue(value);
      LEPUSContext* ctx = env->ctx->ctx;
      if (LEPUS_IsFunction(ctx, js_value)) {
        *result = lynx_value_function;
      } else if (LEPUS_IsArray(ctx, js_value)) {
        *result = lynx_value_array;
      } else if (LEPUS_IsArrayBuffer(js_value)) {
        *result = lynx_value_arraybuffer;
      } else {
        *result = lynx_value_map;
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
      } else {
        *result = lynx_value_null;
        LOGE("lynx_value_typeof: unkown jsvalue type  " << value.tag);
      }
  }
  return lynx_api_ok;
}

static void iterator_callback(LEPUSContext* ctx, LEPUSValue key,
                              LEPUSValue value, void* pfunc, void* raw_data) {
  auto* data = reinterpret_cast<iterator_raw_data*>(raw_data);
  auto func = reinterpret_cast<lynx_value_iterator_callback>(pfunc);
  if (LEPUS_IsLepusRef(value)) {
    func(
        data->env, MakeLynxValue(key),
        lynx::lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx, value),
        data->pfunc, data->raw_data);
  } else {
    func(data->env, MakeLynxValue(key), MakeLynxValue(value), data->pfunc,
         data->raw_data);
  }
}

lynx_api_status lynx_value_iterate_value(lynx_api_env env, lynx_value object,
                                         lynx_value_iterator_callback callback,
                                         void* pfunc, void* raw_data) {
  iterator_raw_data data = {.env = env, .pfunc = pfunc, .raw_data = raw_data};
  LEPUS_IterateObject(env->ctx->ctx, WrapJSValue(object), iterator_callback,
                      reinterpret_cast<void*>(callback), &data);

  return lynx_api_ok;
}

lynx_api_status lynx_value_equals(lynx_api_env env, lynx_value lhs,
                                  lynx_value rhs, bool* result) {
  *result = LEPUS_VALUE_GET_BOOL(
      LEPUS_DeepEqual(env->ctx->ctx, WrapJSValue(lhs), WrapJSValue(rhs)));
  return lynx_api_ok;
}

lynx_api_status lynx_value_deep_copy_value(lynx_api_env env, lynx_value src,
                                           lynx_value* result) {
  LEPUSValue dst = LEPUS_DeepCopy(env->ctx->ctx, WrapJSValue(src));
  *result = MakeLynxValue(dst);
  return lynx_api_ok;
}

lynx_api_status lynx_value_print(lynx_api_env env, lynx_value value,
                                 void* stream,
                                 lynx_value_print_callback callback) {
  // TODO(frendy): implement with callback
  LEPUSValue val = WrapJSValue(value);
  std::ostream& s = *(reinterpret_cast<std::ostream*>(stream));
  lynx::lepus::LEPUSValueHelper::PrintValue(s, env->ctx->ctx, val);
  return lynx_api_ok;
}

lynx_api_status lynx_value_add_reference(lynx_api_env env, lynx_value value,
                                         lynx_value_ref* result) {
  LEPUSValue val = WrapJSValue(value);
  if (!LEPUS_IsGCMode(env->ctx->ctx)) {
    LEPUS_DupValueRT(env->ctx->rt, val);
    *result = nullptr;
  } else {
    GCPersistent* p_val = nullptr;
    if (*result != nullptr) {
      p_val = reinterpret_cast<GCPersistent*>(*result);
      p_val->Reset(env->ctx->rt);
    }
    p_val = (p_val == nullptr) ? new GCPersistent() : p_val;
    p_val->Reset(env->ctx->rt, val);
    *result = reinterpret_cast<lynx_value_ref>(p_val);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_move_reference(lynx_api_env env, lynx_value src_val,
                                          lynx_value_ref src_ref,
                                          lynx_value_ref* result) {
  if (!LEPUS_IsGCMode(env->ctx->ctx)) {
    *result = nullptr;
  } else {
    GCPersistent* dst_ref_val = nullptr;
    if (*result) {
      dst_ref_val = reinterpret_cast<GCPersistent*>(*result);
      dst_ref_val->Reset(env->ctx->rt);
    }
    if (src_ref) {
      auto* src_ref_val = reinterpret_cast<GCPersistent*>(src_ref);
      dst_ref_val = (dst_ref_val == nullptr) ? new GCPersistent() : dst_ref_val;
      dst_ref_val->Reset(env->ctx->rt, src_ref_val->Get());
      src_ref_val->Reset(env->ctx->rt);
    } else {
      dst_ref_val = (dst_ref_val == nullptr) ? new GCPersistent() : dst_ref_val;
      dst_ref_val->Reset(env->ctx->rt, WrapJSValue(src_val));
    }
    *result = reinterpret_cast<lynx_value_ref>(dst_ref_val);
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_remove_reference(lynx_api_env env, lynx_value value,
                                            lynx_value_ref ref) {
  if (env->ctx == nullptr) {
    // There are some case that LEPUSValue is released in JS Thread.
    return lynx_api_ok;
  }
  if (!LEPUS_IsGCModeRT(env->ctx->rt)) {
    LEPUSValue val = WrapJSValue(value);
    LEPUS_FreeValueRT(env->ctx->rt, val);
  } else {
    auto* p_val = reinterpret_cast<GCPersistent*>(ref);
    if (p_val == nullptr) {
      return lynx_api_invalid_arg;
    }
    p_val->Reset(env->ctx->rt);
    delete p_val;
  }
  return lynx_api_ok;
}

lynx_api_status lynx_value_has_ref_count(lynx_api_env env, lynx_value val,
                                         bool* result) {
  *result = LEPUS_VALUE_HAS_REF_COUNT(WrapJSValue(val));
  return lynx_api_ok;
}

lynx_api_status lynx_value_is_uninitialized(lynx_api_env env, lynx_value val,
                                            bool* result) {
  *result = LEPUS_VALUE_IS_UNINITIALIZED(WrapJSValue(val));
  return lynx_api_ok;
}
