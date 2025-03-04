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

#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

#define ENABLE_PRINT_VALUE 1

namespace lynx {
namespace lepus {
class Value;

class LEPUSValueHelper {
 public:
  static constexpr int64_t MAX_SAFE_INTEGER = 9007199254740991;

  static inline LEPUSValue CreateLepusRef(LEPUSContext* ctx,
                                          lepus::RefCounted* p, int32_t tag) {
    p->AddRef();
    return LEPUS_NewLepusWrap(ctx, p, tag);
  }

  static LEPUSValue ToJsValue(LEPUSContext* ctx, const lepus::Value& val,
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

  static inline lepus::Value DeepCopyJsValue(LEPUSContext* ctx,
                                             const LEPUSValue& src,
                                             bool copy_as_jsvalue) {
    if (copy_as_jsvalue) {
      Value ret(ctx, LEPUS_DeepCopy(ctx, src));
      return ret;
    }
    return ToLepusValue(ctx, src, 1);
  }

  /*
   The function is used for shallowCopy a JSValue to LepusValue. ref type will
   be shallow copy.
   */
  static inline lepus::Value ShallowCopyJsValue(LEPUSContext* ctx,
                                                const LEPUSValue& src,
                                                bool copy_as_jsvalue) {
    if (copy_as_jsvalue) {
      Value ret(ctx, LEPUS_DeepCopy(ctx, src));
      return ret;
    }
    return ToLepusValue(ctx, src, 2);
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

  static LEPUSClassID GetRefCountedClassID(LEPUSContext* ctx,
                                           const LEPUSValue& val);

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
    LEPUSValue v = prop.ToJSValue(ctx);
    HandleScope block_scope(ctx, &v, HANDLE_TYPE_LEPUS_VALUE);
    return !!LEPUS_SetPropertyUint32(ctx, obj, idx, v);
  }

  static inline bool SetProperty(LEPUSContext* ctx, LEPUSValue obj,
                                 const base::String& key,
                                 const lepus::Value& val) {
    LEPUSValue v = val.ToJSValue(ctx);
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

  static inline bool IsLepusEqualJsValue(LEPUSContext* ctx,
                                         const lepus::Value& src,
                                         const LEPUSValue& dst) {
    if (IsArray(ctx, dst)) {  // dst is arrary
      if (!src.IsArray()) return false;
      return IsLepusEqualJsArray(ctx, src.Array().get(), dst);
    } else if (IsObject(dst)) {  // dst is object. including js object and lepus
                                 // table ref
      if (!src.IsTable()) return false;
      return IsLepusEqualJsObject(ctx, src.Table().get(), dst);
    } else if (IsJsFunction(ctx, dst)) {
      return false;
    }
    // the last need to be translated to lepus::Value for doing equal,
    // and the dst is not array or object, so the convert is light
    return src == ToLepusValue(ctx, dst);
  }

  static bool IsJsValueEqualJsValue(LEPUSContext* ctx, const LEPUSValue& left,
                                    const LEPUSValue& right);

  static void PrintValue(std::ostream& s, LEPUSContext* ctx,
                         const LEPUSValue& val, uint32_t prefix = 1);
  static void Print(LEPUSContext* ctx, const LEPUSValue& val);
  static const char* GetType(LEPUSContext* ctx, const LEPUSValue& val);

  static LEPUSValue TableToJsValue(LEPUSContext*, const Dictionary&, bool);

  static LEPUSValue ArrayToJsValue(LEPUSContext* ctx, const CArray& val, bool);

  static LEPUSValue RefCountedToJSValue(LEPUSContext* ctx, const RefCounted&);

 private:
  static inline void IteratorCallback(LEPUSContext* ctx, LEPUSValue key,
                                      LEPUSValue value, void* pfunc,
                                      void* raw_data) {
    reinterpret_cast<JSValueIteratorCallback*>(pfunc)->operator()(ctx, key,
                                                                  value);
  }

  static bool IsLepusEqualJsArray(LEPUSContext* ctx, lepus::CArray* src,
                                  const LEPUSValue& dst);

  static bool IsLepusEqualJsObject(LEPUSContext* ctx, lepus::Dictionary* src,
                                   const LEPUSValue& dst);

  static lepus::Value ToLepusArray(LEPUSContext* ctx, const LEPUSValue& val,
                                   int32_t flag = 0);

  static lepus::Value ToLepusTable(LEPUSContext* ctx, const LEPUSValue& val,
                                   int32_t flag = 0);
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_JSVALUE_HELPER_H_
