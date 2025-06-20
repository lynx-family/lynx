// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_JSVALUE_HELPER_H_
#define CORE_RUNTIME_VM_LEPUS_JSVALUE_HELPER_H_
#include <ostream>
#include <string>

#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

#include <functional>
#include <memory>

#include "base/include/compiler_specific.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

#define ENABLE_PRINT_VALUE 1

#define WRAP_AS_JS_VALUE(val) lynx::lepus::LEPUSValueHelper::WrapAsJsValue(val)

#define MK_JS_LEPUS_VALUE(ctx, val) \
  lynx::lepus::LepusValueFactory(ctx).Create(val)

#define MK_JS_LEPUS_VALUE_WITH_CONVERT(ctx, val, deep_convert) \
  lynx::lepus::LepusValueFactory(ctx).Create(val, deep_convert)

namespace lynx {
namespace lepus {
class Value;

using JSValueIteratorCallback =
    base::MoveOnlyClosure<void, LEPUSContext*, LEPUSValue&, LEPUSValue&>;

class LEPUSValueHelper {
 public:
  static constexpr int64_t MAX_SAFE_INTEGER = 9007199254740991;

  static inline LEPUSValue CreateLepusRef(LEPUSContext* ctx,
                                          lepus::RefCountedBase* p,
                                          int32_t tag) {
    p->AddRef();
    return LEPUS_NewLepusWrap(ctx, p, tag);
  }

  static inline LEPUSValue ToJsValue(LEPUSContext* ctx, const lepus::Value& val,
                                     bool deep_convert = false) {
    if (val.IsJSValue()) {
      LEPUSValue v = WrapAsJsValue(val.value());
      LEPUS_DupValue(ctx, v);
      return v;
    }
    return ToJsValue(ctx, val.value(), deep_convert);
  }
  static LEPUSValue ToJsValue(LEPUSContext* ctx, const lynx_value& val,
                              bool deep_convert = false);

  static std::string LepusRefToStdString(LEPUSContext* ctx,
                                         const LEPUSValue& val);

  static std::string ToStdString(LEPUSContext* ctx, const LEPUSValue& val);

  /* The function is used for :
    1. convert jsvalue to lepus::Value when flag == 0;
    2. deep clone jsvalue to lepus::Value when flag == 1;
    3. shallow copy jsvalue to lepus::Value when flag == 2;
  flag default's value is 0
  */
  static lepus::Value ToLepusValue(LEPUSContext* ctx, const LEPUSValue& val,
                                   int32_t copy_flag = 0);

  /**
   * @brief This function converts value to base::String and initialize
   * string cache for it.
   */
  static base::String ToLepusString(LEPUSContext* ctx, const LEPUSValue& val) {
    return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
        ToLepusStringRefCountedImpl(ctx, val));
  }
  static base::RefCountedStringImpl* ToLepusStringRefCountedImpl(
      LEPUSContext* ctx, const LEPUSValue& val);

  static inline void IteratorJsValue(LEPUSContext* ctx, const LEPUSValue& val,
                                     JSValueIteratorCallback* pfunc) {
    if (!IsJsObject(val)) return;
    LEPUS_IterateObject(ctx, val, IteratorCallback,
                        reinterpret_cast<void*>(pfunc), nullptr);
  }

  static inline LEPUSValue NewInt32(LEPUSContext* ctx, int32_t val) {
    return LEPUS_NewInt32(ctx, val);
  }

  static inline LEPUSValue NewUint32(LEPUSContext* ctx, uint32_t val) {
    return LEPUS_NewInt64(ctx, val);  // may to be float/int32
  }

  static inline LEPUSValue NewInt64(LEPUSContext* ctx, int64_t val) {
    if (val < MAX_SAFE_INTEGER && val > -MAX_SAFE_INTEGER) {
      return LEPUS_NewInt64(ctx, val);
    } else {
      return LEPUS_NewBigUint64(ctx, static_cast<uint64_t>(val));  //
    }
  }

  static inline LEPUSValue NewUint64(LEPUSContext* ctx, uint64_t val) {
    if (val < MAX_SAFE_INTEGER) {
      return LEPUS_NewInt64(ctx, val);
    } else {
      return LEPUS_NewBigUint64(ctx, val);
    }
  }

  static inline LEPUSValue NewPointer(void* p) {
    return LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, p);
  }

  static inline LEPUSValue NewString(LEPUSContext* ctx, const char* name) {
    return LEPUS_NewString(ctx, name);
  }

  static inline int32_t GetLength(LEPUSContext* ctx, const LEPUSValue& val) {
    return LEPUS_GetLength(ctx, val);
  }

  static inline bool IsLepusRef(const LEPUSValue& val) {
    return LEPUS_IsLepusRef(val);
  }
  static inline bool IsLepusJSObject(const LEPUSValue& val) {
    return LEPUS_GetLepusRefTag(val) == Value_JSObject;
  }

  static inline bool IsLepusArray(const LEPUSValue& val) {
    return LEPUS_GetLepusRefTag(val) == Value_Array;
  }

  static inline bool IsLepusTable(const LEPUSValue& val) {
    return LEPUS_GetLepusRefTag(val) == Value_Table;
  }

  static inline bool IsLepusByteArray(const LEPUSValue& val) {
    return LEPUS_GetLepusRefTag(val) == Value_ByteArray;
  }

  static inline bool IsJSCpointer(const LEPUSValue& val) {
    return LEPUS_VALUE_GET_TAG(val) == LEPUS_TAG_LEPUS_CPOINTER;
  }

  static inline void* JSCpointer(const LEPUSValue& val) {
    return LEPUS_VALUE_GET_PTR(val);
  }

  static inline LEPUSObject* GetLepusJSObject(const LEPUSValue& val) {
    return reinterpret_cast<LEPUSObject*>(LEPUS_GetLepusRefPoint(val));
  }

  static inline ByteArray* GetLepusByteArray(const LEPUSValue& val) {
    return reinterpret_cast<ByteArray*>(LEPUS_GetLepusRefPoint(val));
  }

  static inline Dictionary* GetLepusTable(const LEPUSValue& val) {
    return reinterpret_cast<Dictionary*>(LEPUS_GetLepusRefPoint(val));
  }
  static inline CArray* GetLepusArray(const LEPUSValue& val) {
    return reinterpret_cast<CArray*>(LEPUS_GetLepusRefPoint(val));
  }

  static inline lepus::RefCounted* GetRefCounted(const LEPUSValue& val) {
    return reinterpret_cast<lepus::RefCounted*>(LEPUS_GetLepusRefPoint(val));
  }

  static inline bool IsJsObject(const LEPUSValue& val) {
    return LEPUS_IsObject(val);
  }

  static inline bool IsObject(const LEPUSValue& val) {
    return LEPUS_IsObject(val) || (IsLepusTable(val));
  }

  static inline bool IsJsArray(LEPUSContext* ctx, const LEPUSValue& val) {
    return LEPUS_IsArray(ctx, val);
  }

  static inline bool IsArray(LEPUSContext* ctx, const LEPUSValue& val) {
    return LEPUS_IsArray(ctx, val) || (IsLepusArray(val));
  }

  static inline bool IsJSString(const LEPUSValue& val) {
    return LEPUS_IsString(val);
  }

  static inline bool IsUndefined(const LEPUSValue& val) {
    return LEPUS_IsUndefined(val);
  }

  static inline bool IsJsFunction(LEPUSContext* ctx, const LEPUSValue& val) {
    return LEPUS_IsFunction(ctx, val);
  }

  static inline bool SetProperty(LEPUSContext* ctx, LEPUSValue obj,
                                 uint32_t idx, const LEPUSValue& prop) {
    return !!LEPUS_SetPropertyUint32(ctx, obj, idx, prop);
  }

  static inline bool SetProperty(LEPUSContext* ctx, LEPUSValue obj,
                                 const char* name, const LEPUSValue& prop) {
    return !!LEPUS_SetPropertyStr(ctx, obj, name, prop);
  }

  static inline bool SetProperty(LEPUSContext* ctx, LEPUSValue obj,
                                 uint32_t idx, const lepus::Value& prop) {
    LEPUSValue v = ToJsValue(ctx, prop);
    HandleScope block_scope(ctx, &v, HANDLE_TYPE_LEPUS_VALUE);
    return !!LEPUS_SetPropertyUint32(ctx, obj, idx, v);
  }

  static inline bool SetProperty(LEPUSContext* ctx, LEPUSValue obj,
                                 const base::String& key,
                                 const lepus::Value& val) {
    LEPUSValue v = ToJsValue(ctx, val);
    HandleScope block_scope(ctx, &v, HANDLE_TYPE_LEPUS_VALUE);
    return !!LEPUS_SetPropertyStr(ctx, obj, key.c_str(), v);
  }

  static inline LEPUSValue GetPropertyJsValue(LEPUSContext* ctx,
                                              const LEPUSValue& obj,
                                              const char* name) {
    return LEPUS_GetPropertyStr(ctx, obj, name);
  }

  static inline LEPUSValue GetPropertyJsValue(LEPUSContext* ctx,
                                              const LEPUSValue& obj,
                                              uint32_t idx) {
    return LEPUS_GetPropertyUint32(ctx, obj, idx);
  }

  static inline bool HasProperty(LEPUSContext* ctx, const LEPUSValue& obj,
                                 const base::String& key) {
    HandleScope func_scope(ctx);
    LEPUSAtom atom = LEPUS_NewAtom(ctx, key.c_str());
    func_scope.PushLEPUSAtom(atom);
    int32_t ret = LEPUS_HasProperty(ctx, obj, atom);
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeAtom(ctx, atom);
    return !!ret;
  }

  static void PrintValue(std::ostream& s, LEPUSContext* ctx,
                         const LEPUSValue& val, uint32_t prefix = 1);
  static void Print(LEPUSContext* ctx, const LEPUSValue& val);
  static const char* GetType(LEPUSContext* ctx, const LEPUSValue& val);

  static LEPUSValue TableToJsValue(LEPUSContext*, const Dictionary&, bool);

  static LEPUSValue ArrayToJsValue(LEPUSContext* ctx, const CArray& val, bool);

  static LEPUSValue RefCountedToJSValue(LEPUSContext* ctx, const RefCounted&);
  static lynx_value ConstructLepusRefToLynxValue(LEPUSContext* ctx,
                                                 const LEPUSValue& val);
  static lepus::Value CreateObject(Context* ctx);
  static lepus::Value CreateArray(Context* ctx);

  static ALWAYS_INLINE LEPUSValue WrapAsJsValue(const lynx_value& val) {
    if (val.type != lynx_value_extended) return LEPUS_UNDEFINED;
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
    return (LEPUSValue){.as_int64 = val.val_int64};
#elif defined(LEPUS_NAN_BOXING)
    return val.val_uint64;
#else
    return LEPUS_MKPTR(static_cast<int8_t>((val.tag & 0xff)), val.val_ptr);
#endif
  }

  static ALWAYS_INLINE int32_t CalculateTag(const LEPUSValue& val) {
    int64_t val_tag = LEPUS_VALUE_GET_NORM_TAG(val);
    int32_t tag =
        (static_cast<int32_t>(LEPUSValueTagToLynxValueType(val_tag)) << 16) |
        (val_tag & 0xff);
    return tag;
  }

  static inline lynx_value_type LEPUSValueTagToLynxValueType(int64_t tag) {
    switch (tag) {
      case LEPUS_TAG_INT:
        return lynx_value_int32;
      case LEPUS_TAG_LEPUS_CPOINTER:
        return lynx_value_external;
      case LEPUS_TAG_STRING:
      case LEPUS_TAG_SEPARABLE_STRING:
        return lynx_value_string;
      case LEPUS_TAG_FLOAT64:
        return lynx_value_double;
      case LEPUS_TAG_BOOL:
        return lynx_value_bool;
      case LEPUS_TAG_BIG_INT:
        return lynx_value_int64;
      case LEPUS_TAG_NULL:
        return lynx_value_null;
      case LEPUS_TAG_UNDEFINED:
        return lynx_value_undefined;
      case LEPUS_TAG_OBJECT:
        return lynx_value_object;
      default:
        return lynx_value_extended;
    }
  }

 private:
  static inline void IteratorCallback(LEPUSContext* ctx, LEPUSValue key,
                                      LEPUSValue value, void* pfunc,
                                      void* raw_data) {
    reinterpret_cast<JSValueIteratorCallback*>(pfunc)->operator()(ctx, key,
                                                                  value);
  }

  static lepus::Value ToLepusArray(LEPUSContext* ctx, const LEPUSValue& val,
                                   int32_t flag = 0);

  static lepus::Value ToLepusTable(LEPUSContext* ctx, const LEPUSValue& val,
                                   int32_t flag = 0);
};

class LepusValueFactory {
 public:
  explicit LepusValueFactory(LEPUSContext* ctx) : ctx_(ctx) {}

  lepus::Value Create(const LEPUSValue& val);
  lepus::Value Create(LEPUSValue&& val);
  lepus::Value Create(const lepus::Value& value, bool deep_convert = false);
  std::unique_ptr<lepus::Value> CreatePtr(const LEPUSValue& val);
  std::unique_ptr<lepus::Value> CreatePtr(LEPUSValue&& val);

 private:
  LEPUSContext* ctx_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_JSVALUE_HELPER_H_
