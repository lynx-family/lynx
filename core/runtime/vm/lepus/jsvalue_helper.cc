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
  switch (auto type = val.Type()) {
    case Value_Nil:
      return LEPUS_NULL;
    case Value_Undefined:
      return LEPUS_UNDEFINED;
    case Value_Double:
      return LEPUS_NewFloat64(ctx, val.val_double_);
    case Value_Bool:
      return LEPUS_NewBool(ctx, val.val_bool_);
    case Value_String:
      return LEPUS_NewStringLen(ctx, val.val_str_->c_str(),
                                val.val_str_->length());
    case Value_Int32:
      return LEPUS_NewInt32(ctx, val.val_int32_t_);
    case Value_Int64:
      return NewInt64(ctx, val.val_int64_t_);
    case Value_UInt32:
      return NewUint32(ctx, val.val_uint32_t_);
    case Value_UInt64:
      return NewUint64(ctx, val.val_uint64_t_);
    case Value_NaN:
    case Value_CDate:
    case Value_RegExp:
    case Value_Closure:
    case Value_CFunction:
      assert(false);
      break;
    case Value_CPointer:
      return NewPointer(val.val_ptr_);
    case Value_Table:
      if (deep_convert) {
        return TableToJsValue(ctx, *val.val_table_, true);
      }
    case Value_Array:
      if (deep_convert) {
        return ArrayToJsValue(ctx, *val.val_carray_, true);
      }
    case Value_RefCounted:
      if (deep_convert) {
        return RefCountedToJSValue(ctx, *val.val_ref_counted_);
      }
    case Value_JSObject:
    case Value_ByteArray:
      return CreateLepusRef(ctx, static_cast<lepus::RefCounted*>(val.val_ptr_),
                            type);
    default: {
      if (val.IsJSValue()) {
        return LEPUS_DupValue(ctx, val.WrapJSValue());
      }
    };
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
      LEPUS_ToInt64(ctx, &int64, val);
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
        return lepus::Value(ctx, val);
      } else if (flag == 1) {
        return Value::Clone(lepus::Value(ctx, val));
      } else {
        Value ret(ctx, val);
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
          return lepus::Value(ctx, val);
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

bool LEPUSValueHelper::IsLepusEqualJsArray(LEPUSContext* ctx,
                                           lepus::CArray* src,
                                           const LEPUSValue& dst) {
  if (src->size() != static_cast<size_t>(GetLength(ctx, dst))) {
    return false;
  }
  for (uint32_t i = 0; i < src->size(); i++) {
    lepus::Value dst_element(ctx, GetPropertyJsValue(ctx, dst, i));
    if (src->get(i) != dst_element) return false;
  }
  return true;
}

bool LEPUSValueHelper::IsLepusEqualJsObject(LEPUSContext* ctx,
                                            lepus::Dictionary* src,
                                            const LEPUSValue& dst) {
  if (src->size() != static_cast<size_t>(GetLength(ctx, dst))) {
    return false;
  }
  for (auto& it : *src) {
    lepus::Value dst_property(ctx,
                              GetPropertyJsValue(ctx, dst, it.first.c_str()));
    if (it.second != dst_property) return false;
  }
  return true;
}

bool LEPUSValueHelper::IsJsValueEqualJsValue(LEPUSContext* ctx,
                                             const LEPUSValue& left,
                                             const LEPUSValue& right) {
  return LEPUS_VALUE_GET_BOOL(LEPUS_DeepEqual(ctx, left, right));
}

const char* LEPUSValueHelper::GetType(LEPUSContext* ctx,
                                      const LEPUSValue& val) {
  switch (LEPUS_VALUE_GET_TAG(val)) {
    case LEPUS_TAG_BIG_INT:
      return "LEPUS_BIG_INT";
    case LEPUS_TAG_BIG_FLOAT:
      return "LEPUS_BIG_FLOAT";
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

LEPUSClassID LEPUSValueHelper::GetRefCountedClassID(LEPUSContext* ctx,
                                                    const LEPUSValue& val) {
  auto ref_counted_class_id = RefCounted::GetClassID();
  if (LEPUS_GetClassID(ctx, val) == ref_counted_class_id) {
    return ref_counted_class_id;
  }
  return 0;
}

}  // namespace lepus
}  // namespace lynx
