// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepusng/jsvalue_helper.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/table.h"
#include "core/runtime/lepusng/quick_context.h"

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
  table.for_each([&](const auto& key, const auto& value) {
    keys[idx] = key.c_str();
    values[idx] = ToJsValue(ctx, value, deep_convert);
    ++idx;
  });
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
      RefType ref_type = static_cast<RefType>(val.tag);
      switch (ref_type) {
        case RefType::kLepusTable... RefType::kCSSFragment:
          if (deep_convert) {
            return RefCountedToJSValue(
                ctx, *reinterpret_cast<lepus::RefCounted*>(val.val_ptr));
          }
        case RefType::kJSIObject... RefType::kEvent:
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
#elif defined(LEPUS_NAN_BOXING)
      // FIXME(chenyouhui): Use LEPUS_NAN_BOXING that is defined in PrimJS
      // temporarily. The PrimJS should provide a more appropriate interface.
      LEPUSValue v = val.val_uint64;
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

lepus::Value LEPUSValueHelper::ToLepusArray(LEPUSContext* ctx,
                                            const LEPUSValue& val,
                                            int32_t flag) {
  auto arr = lepus::CArray::Create();
  JSValueIteratorCallback to_lepus_array_callback =
      [&arr, flag](LEPUSContext* ctx, const LEPUSValue& key,
                   const LEPUSValue& value) {
        arr->emplace_back(ToLepusValue(ctx, value, flag));
      };

  IteratorJsValue(ctx, val, &to_lepus_array_callback);
  return Value(std::move(arr));
}

lepus::Value LEPUSValueHelper::ToLepusTable(LEPUSContext* ctx,
                                            const LEPUSValue& val,
                                            int32_t flag) {
  auto tbl = lepus::Dictionary::Create();
  JSValueIteratorCallback to_lepus_table_callback =
      [&tbl, flag](LEPUSContext* ctx, const LEPUSValue& key,
                   const LEPUSValue& value) {
        tbl->SetValue(ToStdString(ctx, key), ToLepusValue(ctx, value, flag));
      };

  IteratorJsValue(ctx, val, &to_lepus_table_callback);
  return Value(std::move(tbl));
}  // namespace lepus

lepus::Value LEPUSValueHelper::ToLepusValue(LEPUSContext* ctx,
                                            const LEPUSValue& val,
                                            int32_t flag) {
  static lepus::Value empty_value;
  switch (LEPUS_VALUE_GET_TAG(val)) {
    case LEPUS_TAG_INT:
      return Value(LEPUS_VALUE_GET_INT(val));
    case LEPUS_TAG_BIG_INT: {
      int64_t int64;
      LEPUS_ToBigInt64(ctx, &int64, val);
      return Value(int64);
    } break;
    case LEPUS_TAG_FLOAT64: {
      double d;
      LEPUS_ToFloat64(ctx, &d, val);
      if (base::StringConvertHelper::IsInt64Double(d)) {
        return Value(static_cast<int64_t>(d));
      } else {
        return Value(d);
      }
    } break;
    case LEPUS_TAG_UNDEFINED: {
      lepus::Value ret;
      ret.SetUndefined();
      return ret;
    } break;
    case LEPUS_TAG_NULL:
      return Value();
    case LEPUS_TAG_BOOL:
      return Value(static_cast<bool>(LEPUS_VALUE_GET_BOOL(val)));
    case LEPUS_TAG_LEPUS_CPOINTER:
      return Value(JSCpointer(val));
    case LEPUS_TAG_STRING:
    case LEPUS_TAG_SEPARABLE_STRING:
      return Value(ToLepusString(ctx, val));
    case LEPUS_TAG_LEPUS_REF: {
      if (likely(flag == 0)) {
        return MK_JS_LEPUS_VALUE(ctx, val);
      } else if (flag == 1) {
        return Value::Clone(MK_JS_LEPUS_VALUE(ctx, val));
      } else {
        Value ret = MK_JS_LEPUS_VALUE(ctx, val);
        if (!ret.MarkConst()) {
          ret = Value::Clone(ret);
        }
        return ret;
      }
    } break;
    case LEPUS_TAG_OBJECT: {
      if (IsJsArray(ctx, val)) {
        return ToLepusArray(ctx, val, flag);
      } else if (IsJsFunction(ctx, val)) {
        if (flag == 0) {
          return MK_JS_LEPUS_VALUE(ctx, val);
        }
        return empty_value;
      } else {
        return ToLepusTable(ctx, val, flag);
      }
    } break;
    default:
      if (LEPUS_IsNumber(val)) {
        double d;
        LEPUS_ToFloat64(ctx, &d, val);
        if (base::StringConvertHelper::IsInt64Double(d)) {
          return Value(static_cast<int64_t>(d));
        } else {
          return Value(d);
        }
      }
      LOGE("ToLepusValue: unkown jsvalue type  " << GetType(ctx, val));
  }
  return empty_value;
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

const char* LEPUSValueHelper::GetType(LEPUSContext* ctx,
                                      const LEPUSValue& val) {
  switch (LEPUS_VALUE_GET_TAG(val)) {
    case LEPUS_TAG_BIG_INT:
      return "LEPUS_BIG_INT";
    case LEPUS_TAG_SYMBOL:
      return "LEPUS_TAG_SYMBOL";
    case LEPUS_TAG_STRING:
      return "LEPUS_TAG_STRING";
    case LEPUS_TAG_SEPARABLE_STRING:
      return "LEPUS_TAG_SEPARABLE_STRING";
    case LEPUS_TAG_SHAPE:
      return "LEPUS_TAG_SHAPE";
    case LEPUS_TAG_ASYNC_FUNCTION:
      return "LEPUS_TAG_ASYNC_FUNCTION";
    case LEPUS_TAG_VAR_REF:
      return "LEPUS_TAG_VAR_REF";
    case LEPUS_TAG_MODULE:
      return "LEPUS_TAG_MODULE";
    case LEPUS_TAG_FUNCTION_BYTECODE:
      return "LEPUS_TAG_FUNCTION_BYTECODE";
    case LEPUS_TAG_OBJECT: {
      if (IsJsArray(ctx, val)) {
        return "LEPUS_TAG_ARRAY";
      }
      return "LEPUS_TAG_OBJECT";
    } break;
    case LEPUS_TAG_INT:
      return "LEPUS_TAG_INT";
    case LEPUS_TAG_BOOL:
      return "LEPUS_TAG_BOOL";
    case LEPUS_TAG_NULL:
      return "LEPUS_TAG_NULL";
    case LEPUS_TAG_UNDEFINED:
      return "LEPUS_TAG_UNDEFINED";
    case LEPUS_TAG_UNINITIALIZED:
      return "LEPUS_TAG_UNINITIALIZED";
    case LEPUS_TAG_CATCH_OFFSET:
      return "LEPUS_TAG_CATCH_OFFSET";
    case LEPUS_TAG_EXCEPTION:
      return "LEPUS_TAG_EXCEPTION";
    case LEPUS_TAG_LEPUS_CPOINTER:
      return "LEPUS_TAG_LEPUS_CFUNCTION";
    case LEPUS_TAG_FLOAT64:
      return "LEPUS_TAG_FLOAT64";
  }
  return "";
}

void LEPUSValueHelper::PrintValue(std::ostream& s, LEPUSContext* ctx,
                                  const LEPUSValue& val, uint32_t prefix) {
#if ENABLE_PRINT_VALUE
  if (!IsJsObject(val)) {
    ToLepusValue(ctx, val).PrintValue(s);
    return;
  }

  if (LEPUS_IsError(ctx, val)) {
    auto error = MK_JS_LEPUS_VALUE(ctx, val);
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
  int32_t tag = 0;
  auto* ptr = LEPUS_GetLepusRefPoint(val);
  if (type == lynx_value_object) {
    if (old_type == Value_RefCounted) {
      tag = static_cast<int32_t>(
          reinterpret_cast<RefCounted*>(ptr)->GetRefType());
    } else if (old_type == Value_JSObject) {
      tag = static_cast<int32_t>(RefType::kJSIObject);
    }
  }
  reinterpret_cast<fml::RefCountedThreadSafeStorage*>(ptr)->AddRef();
  LEPUSLepusRef* ref =
      reinterpret_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(val));
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, ref->lepus_val);
  ref->lepus_val = LEPUS_UNDEFINED;
  return {.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr),
          .type = type,
          .tag = tag};
}

Value LEPUSValueHelper::CreateObject(Context* ctx) {
  if (ctx && ctx->IsLepusNGContext()) {
    LEPUSContext* lctx = QuickContext::Cast(ctx)->context();
    return MK_JS_LEPUS_VALUE(lctx, LEPUS_NewObject(lctx));
  }
  return Value(lepus::Dictionary::Create());
}

Value LEPUSValueHelper::CreateArray(Context* ctx) {
  if (ctx && ctx->IsLepusNGContext()) {
    auto* lctx = QuickContext::Cast(ctx)->context();
    return MK_JS_LEPUS_VALUE(lctx, LEPUS_NewArray(lctx));
  }
  return Value(lepus::CArray::Create());
}

lepus::Value LepusValueFactory::Create(const LEPUSValue& val) {
  if (LEPUS_IsLepusRef(val)) {
    return lepus::Value(
        LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx_, val));
  }
  return lepus::Value(QuickContext::GetContextCellFromCtx(ctx_)->env_,
                      LEPUS_VALUE_GET_INT64(val),
                      LEPUSValueHelper::CalculateTag(val));
}

lepus::Value LepusValueFactory::Create(LEPUSValue&& val) {
  if (LEPUS_IsLepusRef(val)) {
    lynx_value value =
        LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx_, val);
    if (!LEPUS_IsGCMode(ctx_)) LEPUS_FreeValue(ctx_, val);
    val = LEPUS_UNDEFINED;
    return lepus::Value(std::move(value));
  }
  int32_t tag = LEPUSValueHelper::CalculateTag(val);
  return lepus::Value(QuickContext::GetContextCellFromCtx(ctx_)->env_,
                      MAKE_LYNX_VALUE(val, tag));
}

lepus::Value LepusValueFactory::Create(const lepus::Value& value,
                                       bool deep_convert) {
  // TODO(frendy): fast path of lepus value to lepus value
  LEPUSValue val = LEPUSValueHelper::ToJsValue(ctx_, value, deep_convert);
  if (LEPUS_IsLepusRef(val)) {
    lynx_value value =
        LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx_, val);
    if (!LEPUS_IsGCMode(ctx_)) LEPUS_FreeValue(ctx_, val);
    val = LEPUS_UNDEFINED;
    return lepus::Value(std::move(value));
  }
  int32_t tag = LEPUSValueHelper::CalculateTag(val);
  return lepus::Value(QuickContext::GetContextCellFromCtx(ctx_)->env_,
                      MAKE_LYNX_VALUE(val, tag));
}

std::unique_ptr<lepus::Value> LepusValueFactory::CreatePtr(
    const LEPUSValue& val) {
  if (LEPUS_IsLepusRef(val)) {
    return std::make_unique<lepus::Value>(
        LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx_, val));
  }
  int32_t tag = LEPUSValueHelper::CalculateTag(val);
  return std::make_unique<lepus::Value>(
      QuickContext::GetContextCellFromCtx(ctx_)->env_,
      lynx_value(MAKE_LYNX_VALUE(val, tag)));
}

std::unique_ptr<lepus::Value> LepusValueFactory::CreatePtr(LEPUSValue&& val) {
  if (LEPUS_IsLepusRef(val)) {
    lynx_value value =
        LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx_, val);
    if (!LEPUS_IsGCMode(ctx_)) LEPUS_FreeValue(ctx_, val);
    val = LEPUS_UNDEFINED;
    return std::make_unique<lepus::Value>(std::move(value));
  }
  int32_t tag = LEPUSValueHelper::CalculateTag(val);
  return std::make_unique<lepus::Value>(
      QuickContext::GetContextCellFromCtx(ctx_)->env_,
      lynx_value(MAKE_LYNX_VALUE(val, tag)));
}

}  // namespace lepus
}  // namespace lynx
