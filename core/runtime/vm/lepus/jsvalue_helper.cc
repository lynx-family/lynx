// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/jsvalue_helper.h"

#include <functional>
#include <utility>
#include <vector>

#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/table.h"
namespace lynx {
namespace lepus {

LEPUSValue LEPUSValueHelper::TableToJsValue(LEPUSContext* ctx,
                                            const Dictionary& table,
                                            bool deep_convert) {
  size_t idx = 0;
  int size = static_cast<int>(table.size());
  const char* keys[size];
  LEPUSValue values[size];
  HandleScope func_scope(LEPUS_GetRuntime(ctx));
  func_scope.PushLEPUSValueArrayHandle(values, size);
  for (const auto& [key, value] : table) {
    keys[idx] = key.c_str();
    values[idx] = ToJsValue(ctx, value, deep_convert);
    ++idx;
  }
  return LEPUS_NewObjectWithArgs(ctx, size, keys, values);
}

LEPUSValue LEPUSValueHelper::ArrayToJsValue(LEPUSContext* ctx,
                                            const CArray& array,
                                            bool deep_convert) {
  size_t idx = 0, size = array.vec_.size();
  LEPUSValue values[size];
  HandleScope func_scope(LEPUS_GetRuntime(ctx));
  func_scope.PushLEPUSValueArrayHandle(values, static_cast<int>(size));
  while (idx < size) {
    values[idx] = ToJsValue(ctx, array.vec_[idx], deep_convert);
    ++idx;
  }
  return LEPUS_NewArrayWithArgs(ctx, static_cast<int>(size), values);
}

LEPUSValue LEPUSValueHelper::RefCountedToJSValue(
    LEPUSContext* ctx, const RefCounted& ref_counted) {
  return LEPUS_NewObject(ctx);
}

LEPUSValue LEPUSValueHelper::ToJsValue(LEPUSContext* ctx,
                                       const lepus::Value& val,
                                       bool deep_convert) {
  return ToJsValue(ctx, val.value(), deep_convert);
}

LEPUSValue LEPUSValueHelper::ToJsValue(LEPUSContext* ctx, const lynx_value& val,
                                       bool deep_convert) {
  switch (val.type) {
    case lynx_value_null:
      return LEPUS_NULL;
    case lynx_value_undefined:
      return LEPUS_UNDEFINED;
    case lynx_value_double:
      return LEPUS_NewFloat64(ctx, val.val_double);
    case lynx_value_bool:
      return LEPUS_NewBool(ctx, val.val_bool);
    case lynx_value_string: {
      auto* str = reinterpret_cast<base::RefCountedStringImpl*>(val.val_ptr);
      return LEPUS_NewStringLen(ctx, str->c_str(), str->length());
    }
    case lynx_value_int32:
      return LEPUS_NewInt32(ctx, val.val_int32);
    case lynx_value_int64:
      return NewInt64(ctx, val.val_int64);
    case lynx_value_uint32:
      return NewUint32(ctx, val.val_uint32);
    case lynx_value_uint64:
      return NewUint64(ctx, val.val_uint64);
    case lynx_value_nan:
    case lynx_value_function:
      assert(false);
      break;
    case lynx_value_external:
      return NewPointer(val.val_ptr);
    case lynx_value_map:
      if (deep_convert) {
        auto* table = reinterpret_cast<lepus::Dictionary*>(val.val_ptr);
        return TableToJsValue(ctx, *table, true);
      }
    case lynx_value_array:
      if (deep_convert) {
        auto* arr = reinterpret_cast<lepus::CArray*>(val.val_ptr);
        return ArrayToJsValue(ctx, *arr, true);
      }
    case lynx_value_arraybuffer:
      return CreateLepusRef(ctx,
                            reinterpret_cast<lepus::RefCounted*>(val.val_ptr),
                            lepus::Value::LegacyTypeFromLynxValue(val));
    case lynx_value_object: {
      CustomRefCountedType ref_type =
          static_cast<CustomRefCountedType>(val.tag);
      switch (ref_type) {
        case CustomRefCountedType::kRefCounted:
          if (deep_convert) {
            return RefCountedToJSValue(
                ctx, *reinterpret_cast<lepus::RefCounted*>(val.val_ptr));
          }
        case CustomRefCountedType::kJSObject:
          return CreateLepusRef(
              ctx, reinterpret_cast<lepus::RefCounted*>(val.val_ptr),
              lepus::Value::LegacyTypeFromLynxValue(val));

        default:
          assert(false);
          break;
      }
    } break;
    case lynx_value_extended: {
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
      LEPUSValue v = (LEPUSValue){.as_int64 = val.val_int64};
#else
      LEPUSValue v = LEPUS_MKPTR(val.tag, val.val_ptr);
#endif
      return LEPUS_DupValue(ctx, v);
    }
    default:
      break;
  }
  return LEPUS_UNDEFINED;
}

std::string LEPUSValueHelper::LepusRefToStdString(LEPUSContext* ctx,
                                                  const LEPUSValue& val) {
  if (!IsLepusRef(val)) return "undefined";
  LEPUSLepusRef* pref = static_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));

  switch (pref->tag) {
    case Value_Array: {
      Value lepus_value;
      lepus_value.SetArray(fml::RefPtr<lepus::CArray>(GetLepusArray(val)));
      std::ostringstream s;
      s << lepus_value;
      return s.str();
    } break;
    case Value_Table: {
      return "[object Object]";
    } break;
    case Value_JSObject: {
      return "[object JSObject]";
    } break;
    case Value_ByteArray: {
      return "[object ByteArray]";
    } break;
    default:
      return "";
  }
}

std::string LEPUSValueHelper::ToStdString(LEPUSContext* ctx,
                                          const LEPUSValue& val) {
  if (LEPUS_IsUndefined(val)) {
    return "";
  }
  DCHECK(ctx);
  if (IsLepusRef(val)) {
    return LepusRefToStdString(ctx, val);
  } else if (LEPUS_VALUE_IS_STRING(val)) {
    auto* str = LEPUS_GetStringUtf8(ctx, LEPUS_VALUE_GET_STRING(val));
    if (str) return str;
  }
  size_t len;
  const char* chr = LEPUS_ToCStringLen(ctx, &len, val);
  if (chr) {
    std::string ret(chr, len);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, chr);
    return ret;
  }
  return "";
}

base::RefCountedStringImpl* LEPUSValueHelper::ToLepusStringRefCountedImpl(
    LEPUSContext* ctx, const LEPUSValue& val) {
  if (ctx == nullptr) return nullptr;
  void* cache = LEPUS_IsGCMode(ctx) ? LEPUS_GetStringCache_GC(val)
                                    : LEPUS_GetStringCache(val);
  if (cache == nullptr) {
    auto ptr =
        base::RefCountedStringImpl::Unsafe::RawCreate(ToStdString(ctx, val));
    LEPUS_SetStringCache(ctx, val, ptr);
    cache = ptr;
    ptr->Release();
  }
  return reinterpret_cast<base::RefCountedStringImpl*>(cache);
}

void LEPUSValueHelper::PrintValue(std::ostream& s, LEPUSContext* ctx,
                                  const LEPUSValue& val, uint32_t prefix) {
#if ENABLE_PRINT_VALUE
  if (!IsJsObject(val)) {
    ContextCell* cell = Context::GetContextCellFromCtx(ctx);
    if (LEPUS_IsLepusRef(val)) {
      lepus::Value v1(cell->lynx_api_env_,
                      ConstructLepusRefToLynxValue(ctx, val));
      v1.ToLepusValue().PrintValue(s);
    } else {
      ToBaseValue(cell->lynx_api_env_, MAKE_LYNX_VALUE_FROM_LEPUS_VALUE(val))
          .PrintValue(s);
    }
    return;
  }

  if (LEPUS_IsError(ctx, val)) {
    auto error = lepus::Value(ctx, val);
    BASE_STATIC_STRING_DECL(kStack, "stack");
    s << error.ToString() << "\n" << error.GetProperty(kStack).ToString();
    return;
  }

#define PRINT_PREFIX(prefix)              \
  for (uint32_t i = 0; i < prefix; i++) { \
    s << "  ";                            \
  }
  uint32_t current_idx = 0;
  uint32_t size = GetLength(ctx, val);
  if (IsJsArray(ctx, val)) {
    s << "[\n";
    while (current_idx < size) {
      PRINT_PREFIX(prefix)
      LEPUSValue prop = GetPropertyJsValue(ctx, val, current_idx);
      // prop kept by val
      PrintValue(s, ctx, prop, prefix + 1);
      if (!ctx || !LEPUS_IsGCMode(ctx)) {
        LEPUS_FreeValue(ctx, prop);
      }
      if (++current_idx != size) {
        s << ",";
      }
      s << "\n";
    }
    PRINT_PREFIX(prefix - 1)
    s << "]";
  } else if (IsJsObject(val)) {
    s << "{\n";
    JSValueIteratorCallback print_jstable =
        [&s, &current_idx, &size, prefix](
            LEPUSContext* ctx, const LEPUSValue& key, const LEPUSValue& val) {
          PRINT_PREFIX(prefix)
          s << ToStdString(ctx, key) << ": ";
          PrintValue(s, ctx, val, prefix + 1);
          if (++current_idx != size) {
            s << ",";
          }
          s << "\n";
        };
    IteratorJsValue(ctx, val, &print_jstable);
    PRINT_PREFIX(prefix - 1)
    s << "}";
  }
#undef PRINT_PREFIX
#endif
}

void LEPUSValueHelper::Print(LEPUSContext* ctx, const LEPUSValue& val) {
#if ENABLE_PRINT_VALUE
  std::ostringstream s;
  PrintValue(s, ctx, val);
  LOGI(s.str() << std::endl);
#endif
}

lynx_value LEPUSValueHelper::ConstructLepusRefToLynxValue(
    LEPUSContext* ctx, const LEPUSValue& val) {
  ValueType old_type = static_cast<ValueType>(LEPUS_GetLepusRefTag(val));
  lynx_value_type type = lepus::Value::ToLynxValueType(old_type);
  int64_t tag = 0;
  if (type == lynx_value_object) {
    if (old_type == Value_RefCounted) {
      tag = static_cast<int64_t>(CustomRefCountedType::kRefCounted);
    } else if (old_type == Value_JSObject) {
      tag = static_cast<int64_t>(CustomRefCountedType::kJSObject);
    }
  }
  auto* ptr = LEPUS_GetLepusRefPoint(val);
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(ptr)->AddRef();
  LEPUSLepusRef* ref =
      reinterpret_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, ref->lepus_val);
  ref->lepus_val = LEPUS_UNDEFINED;
  return {.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr),
          .type = type,
          .tag = tag};
}

lepus::Value LEPUSValueHelper::ToBaseValue(lynx_api_env env,
                                           const lynx_value& val,
                                           int32_t flag) {
  static lepus::Value empty_value;
  if (!env) {
    return empty_value;
  }
  if (val.type != lynx_value_extended) {
    if (likely(flag == 0)) {
      return lepus::Value(env, val);
    } else if (flag == 1) {
      return lepus::Value::Clone(lepus::Value(env, val));
    } else {
      lepus::Value ret(env, val);
      if (!ret.MarkConst()) {
        ret = lepus::Value::Clone(ret);
      }
      return ret;
    }
  }
  lynx_value_type type;
  env->lynx_value_typeof(env, val, &type);
  switch (type) {
    case lynx_value_null:
      return lepus::Value();
    case lynx_value_undefined: {
      lepus::Value ret;
      ret.SetUndefined();
      return ret;
    }
    case lynx_value_bool: {
      bool ret;
      env->lynx_value_get_bool(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_double: {
      double ret;
      env->lynx_value_get_double(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_int32: {
      int32_t ret;
      env->lynx_value_get_int32(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_uint32: {
      uint32_t ret;
      env->lynx_value_get_uint32(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_int64: {
      int64_t ret;
      env->lynx_value_get_int64(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_uint64: {
      uint64_t ret;
      env->lynx_value_get_uint64(env, val, &ret);
      return lepus::Value(ret);
    }
    case lynx_value_nan:
      return lepus::Value(true, true);
    case lynx_value_string: {
      void* str;
      env->lynx_value_get_string_ref(env, val, &str);
      auto* base_str = reinterpret_cast<base::RefCountedStringImpl*>(str);
      return lepus::Value(
          base::String::Unsafe::ConstructWeakRefStringFromRawRef(base_str));
    }
    case lynx_value_array: {
      return ToBaseArray(env, val, flag);
    }
    case lynx_value_map: {
      return ToBaseMap(env, val, flag);
    }
    case lynx_value_function: {
      if (flag == 0) {
        return lepus::Value(env, val);
      }
      return empty_value;
    }
    case lynx_value_arraybuffer:
    case lynx_value_object:
    case lynx_value_external:
    case lynx_value_extended:
      LOGE("unkown type:" << type);
      break;
  }

  return empty_value;
}

lepus::Value LEPUSValueHelper::ToBaseArray(lynx_api_env env,
                                           const lynx_value& val,
                                           int32_t flag) {
  auto arr = lepus::CArray::Create();
  ExtendedValueIteratorCallback callback =
      [&arr, flag](lynx_api_env env, const lynx_value& key,
                   const lynx_value& value) {
        arr->emplace_back(ToBaseValue(env, value, flag));
      };
  IterateLynxValue(env, val, &callback);
  return lepus::Value(std::move(arr));
}

lepus::Value LEPUSValueHelper::ToBaseMap(lynx_api_env env,
                                         const lynx_value& val, int32_t flag) {
  auto map = lepus::Dictionary::Create();
  ExtendedValueIteratorCallback callback =
      [&map, flag](lynx_api_env env, const lynx_value& key,
                   const lynx_value& value) {
        std::string str;
        env->lynx_value_to_string_utf8(env, key, &str);
        map->SetValue(std::move(str), ToBaseValue(env, value, flag));
      };
  IterateLynxValue(env, val, &callback);
  return lepus::Value(std::move(map));
}

bool LEPUSValueHelper::IsBaseValueEqualToLynxValue(lynx_api_env env,
                                                   const lepus::Value& src,
                                                   const lynx_value& dst) {
  lynx_value_type type;
  env->lynx_value_typeof(env, dst, &type);
  if (type == lynx_value_array) {
    if (!src.IsArray()) return false;
    return IsBaseArrayEqualToLynxArray(env, src.Array().get(), dst);
  } else if (type == lynx_value_map) {
    if (!src.IsTable()) return false;
    return IsBaseDictEqualToLynxDict(env, src.Table().get(), dst);
  } else if (type == lynx_value_function) {
    return false;
  }

  return src == ToBaseValue(env, dst);
}

bool LEPUSValueHelper::IsBaseArrayEqualToLynxArray(lynx_api_env env,
                                                   lepus::CArray* src,
                                                   const lynx_value& dst) {
  uint32_t len;
  env->lynx_value_get_array_length(env, dst, &len);
  if (src->size() != static_cast<size_t>(len)) {
    return false;
  }
  for (uint32_t i = 0; i < src->size(); i++) {
    lynx_value val;
    lynx_api_status status = env->lynx_value_get_element(env, dst, i, &val);
    if (status != lynx_api_ok) return false;
    lepus::Value dst_element(env, std::move(val));
    if (src->get(i) != dst_element) return false;
  }
  return true;
}

bool LEPUSValueHelper::IsBaseDictEqualToLynxDict(lynx_api_env env,
                                                 lepus::Dictionary* src,
                                                 const lynx_value& dst) {
  uint32_t len;
  env->lynx_value_get_length(env, dst, &len);
  if (src->size() != static_cast<size_t>(len)) {
    return false;
  }
  for (auto& it : *src) {
    lynx_value val;
    lynx_api_status status =
        env->lynx_value_get_named_property(env, dst, it.first.c_str(), &val);
    if (status != lynx_api_ok) return false;
    lepus::Value dst_property(env, std::move(val));
    if (it.second != dst_property) return false;
  }
  return true;
}

}  // namespace lepus
}  // namespace lynx
