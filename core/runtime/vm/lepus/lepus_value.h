// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_VALUE_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_VALUE_H_

#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/log/logging.h"
#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/runtime/vm/lepus/marco.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#ifdef OS_IOS
#include "persistent-handle.h"
#else
#include "quickjs/include/persistent-handle.h"
#endif

namespace lynx {
namespace lepus {

class Value;

using JSValueIteratorCallback =
    base::MoveOnlyClosure<void, LEPUSContext*, LEPUSValue&, LEPUSValue&>;

using LepusValueIterator =
    base::MoveOnlyClosure<void, const lepus::Value&, const lepus::Value&>;

typedef void* point_t;
#define NormalNumberType(V) \
  V(Double, double)         \
  V(Int32, int32_t)         \
  V(UInt32, uint32_t)       \
  V(UInt64, uint64_t)

#define NumberType(V) NormalNumberType(V) V(Int64, int64_t)

#define ReferenceType(V)      \
  V(base::String, string)     \
  V(Table, table)             \
  V(Array, array)             \
  V(Closure, closure)         \
  V(LEPUSObject, lepusobject) \
  V(ByteArray, bytearray)     \
  V(Date, date)

/*
LepusNG will add more types:
  1. JSValue
    It include:  type_ > Value_TypeCount || type_ < 0
    It make lepus::Value can hold quickjs JSValue type
*/
enum ValueType {
  Value_Nil,
  Value_Double,
  Value_Bool,
  Value_String,
  Value_Table,
  Value_Array,
  Value_Closure,
  Value_CFunction,
  Value_CPointer,
  Value_Int32,
  Value_Int64,
  Value_UInt32,
  Value_UInt64,
  Value_NaN,
  Value_CDate,
  Value_RegExp,
  Value_JSObject,
  Value_Undefined,
  Value_ByteArray,
  Value_RefCounted,
  // Value_TypeCount is used for encoding jsvalue tag,
  // Adding a new Value_type needs to be inserted before 'Value_TypeCount'
  Value_PrimJsValue,
  Value_TypeCount,
};

class Context;
class CArray;
class CDate;
class Dictionary;
class Closure;
class RegExp;
class LEPUSValueHelper;
class ByteArray;
class QuickContext;
class RefCounted;
class LEPUSObject;

class ContextCell {
 public:
  ContextCell(lepus::QuickContext* qctx, LEPUSContext* ctx, LEPUSRuntime* rt)
      : gc_enable_(false), ctx_(ctx), rt_(rt), qctx_(qctx) {
    if (rt_) {
      gc_enable_ = LEPUS_IsGCModeRT(rt_);
    }
  };
  bool gc_enable_;
  LEPUSContext* ctx_;
  LEPUSRuntime* rt_;
  lepus::QuickContext* qctx_;
};

class CellManager {
 public:
  CellManager() : cells_(){};
  ~CellManager();
  ContextCell* AddCell(lepus::QuickContext* qctx);

 private:
  base::InlineVector<ContextCell*, 16> cells_;
};

typedef Value (*CFunction)(Context*);

class BASE_EXPORT_FOR_DEVTOOL Value {
 private:
  union {
    Dictionary* val_table_;

    // guaranteed to be non-null for Value_String
    base::RefCountedStringImpl* val_str_;
    lepus::LEPUSObject* val_jsobject_;
    lepus::ByteArray* val_bytearray_;
    lepus::RefCounted* val_ref_counted_;
    CArray* val_carray_;
    lepus::CDate* val_date_;
    lepus::RegExp* val_regexp_;
    Closure* val_closure_;

#define NumberStorage(name, type) type val_##type##_;
    NumberType(NumberStorage)
#undef NumberStorage

        bool val_bool_;
    void* val_ptr_ = nullptr;
    bool val_nan_;
  };

  ContextCell* cell_ = nullptr;
  union {
    ValueType type_ = Value_Nil;
    int32_t tag_;
  };

#if !defined(__aarch64__) || defined(OS_WIN) || DISABLE_NANBOX
  static constexpr int LEPUS_TAG_ADJUST = Value_TypeCount - LEPUS_TAG_FIRST + 1;

#define EncodeJSTag(t) ((t) + LEPUS_TAG_ADJUST)
#define DecodeJSTag(t) ((t)-LEPUS_TAG_ADJUST)
#endif
  GCPersistent* p_val_ = nullptr;

 public:
  explicit Value() = default;
  Value(const Value& value);
  Value(Value&& value) noexcept;

  explicit Value(const base::String& data);
  explicit Value(base::String&& data);
  explicit Value(const char* val);
  explicit Value(const std::string& str);
  explicit Value(std::string&& str);

  explicit Value(const fml::RefPtr<Dictionary>& data);
  explicit Value(fml::RefPtr<Dictionary>&& data);
  explicit Value(const fml::RefPtr<CArray>& data);
  explicit Value(fml::RefPtr<CArray>&& data);
  explicit Value(const fml::RefPtr<lepus::LEPUSObject>& data);
  explicit Value(fml::RefPtr<lepus::LEPUSObject>&& data);
  explicit Value(const fml::RefPtr<lepus::ByteArray>& data);
  explicit Value(fml::RefPtr<lepus::ByteArray>&& data);
  explicit Value(const fml::RefPtr<lepus::RefCounted>& data);
  explicit Value(fml::RefPtr<lepus::RefCounted>&& data);
  explicit Value(const fml::RefPtr<Closure>& data);
  explicit Value(fml::RefPtr<Closure>&& data);
  explicit Value(const fml::RefPtr<CDate>& data);
  explicit Value(fml::RefPtr<CDate>&& data);
  explicit Value(const fml::RefPtr<RegExp>& data);
  explicit Value(fml::RefPtr<RegExp>&& data);

  explicit Value(bool val);
  explicit Value(void* data);
  explicit Value(CFunction val);
  explicit Value(bool for_nan, bool val);

  inline bool IsCDate() const { return type_ == Value_CDate; }
  inline bool IsRegExp() const { return type_ == Value_RegExp; }

  fml::RefPtr<lepus::Closure> GetClosure() const;
  fml::RefPtr<lepus::CDate> Date() const;
  fml::RefPtr<lepus::RegExp> RegExp() const;

  void SetClosure(const fml::RefPtr<lepus::Closure>&);
  void SetClosure(fml::RefPtr<lepus::Closure>&&);
  void SetRegExp(const fml::RefPtr<lepus::RegExp>&);
  void SetRegExp(fml::RefPtr<lepus::RegExp>&&);
  void SetDate(const fml::RefPtr<lepus::CDate>&);
  void SetDate(fml::RefPtr<lepus::CDate>&&);

  inline void DupValue() const;
  void FreeValue();

  inline bool IsClosure() const { return type_ == Value_Closure; }
  inline bool IsCallable() const { return IsClosure() || IsJSFunction(); }

// add for compile
#define NumberConstructor(name, type) explicit Value(type data);

  NumberType(NumberConstructor)

      explicit Value(uint8_t data);
#undef NumberConstructor

#define SetNumberDefine(name, type) \
  void SetNumber(type value) {      \
    FreeValue();                    \
    val_##type##_ = value;          \
    type_ = Value_##name;           \
  }

  NumberType(SetNumberDefine)
#undef SetNumberDefine

      inline ValueType Type() const {
    return type_;
  }

  static Value Clone(const Value& src, bool clone_as_jsvalue = false);

  static Value ShallowCopy(const Value& src, bool clone_as_jsvalue = false);

  inline bool IsReference() const {
    return (type_ > Value_Bool && type_ < Value_CFunction) ||
           ((type_ >= Value_CDate && type_ <= Value_RefCounted &&
             type_ != Value_Undefined));
  }
  inline void* Ptr() const { return val_ptr_; }

  inline bool IsBool() const { return type_ == Value_Bool || IsJSBool(); }

  inline bool IsString() const { return type_ == Value_String || IsJSString(); }

  inline bool IsInt64() const { return type_ == Value_Int64 || IsJSInteger(); }

  inline bool IsNumber() const {
    return (type_ == Value_Double) ||
           (type_ >= Value_Int32 && type_ <= Value_UInt64) || IsJSNumber();
  }

  inline bool IsDouble() const { return type_ == Value_Double; }

  inline bool IsArray() const { return type_ == Value_Array; }

  inline bool IsTable() const { return type_ == Value_Table; }

  inline bool IsObject() const {
    if (IsTable()) return true;
    if (IsJSValue()) return IsJSTable();
    return false;
  }

  inline bool IsArrayOrJSArray() const {
    if (IsArray()) return true;
    if (IsJSValue()) return IsJSArray();
    return false;
  }

  inline bool IsCPointer() const {
    return type_ == Value_CPointer || IsJSCPointer();
  }

  inline bool IsRefCounted() const { return type_ == Value_RefCounted; }

  inline bool IsInt32() const { return type_ == Value_Int32; }
  inline bool IsUInt32() const { return type_ == Value_UInt32; }
  inline bool IsUInt64() const { return type_ == Value_UInt64; }
  inline bool IsNil() const { return (type_ == Value_Nil) || IsJsNull(); }
  inline bool IsUndefined() const {
    return type_ == Value_Undefined || IsJSUndefined();
  }
  inline bool IsCFunction() const { return type_ == Value_CFunction; }
  inline bool IsJSObject() const { return type_ == Value_JSObject; }
  inline bool IsByteArray() const { return type_ == Value_ByteArray; }
  inline bool IsNaN() const { return type_ == Value_NaN; }

  inline bool Bool() const {
    if (type_ != Value_Bool) return !IsFalse();
    return val_bool_;
  }
  inline bool NaN() const { return type_ == Value_NaN && val_nan_; }

  double Number() const;

  Value& operator=(const Value& value) {
    Copy(value);
    return *this;
  }
  Value& operator=(Value&& value) noexcept;

#define NumberValue(name, type) type name() const;
  NumberType(NumberValue)
#undef NumberValue

      /// @brief Returns a string view of the internal lepus string
      /// storage. The result is guaranteed to be null terminated.
      std::string_view StringView() const {
    return StdString();
  }

  /// @brief Equivalent to `String().c_str()` but with better performance and
  /// binary size because there is no temporary base::String object generated
  /// although it does not retain the underlying string impl. Use only when C
  /// string is required.
  const char* CString() const { return StdString().c_str(); }

  /// @brief Equivalent to `String().str()` but with better performance and
  /// binary size because there is no temporary base::String object generated
  /// although it does not retain the underlying string impl. Use this function
  /// for most scenarios.
  BASE_EXPORT const std::string& StdString() const;

  /// @Note
  /// If possible, use StringView(), StdString() or CString() instead of this
  /// because this function produce a temporary base::String instance although
  /// it does not retain the underlying ref-counted impl.
  ///
  /// @brief For values representing string, js string, boolean and js boolean
  /// this function returns a base::String instance which does not strong
  /// retains the underlying impl object. You can use the result instance for
  /// its string content safely in synchronous codes and when result is used for
  /// copy or move construction of other base::String instances, the underlying
  /// impl is retained normally by new instances.
  ///
  /// There is only one unsafe case which is assigning result of String()
  /// directly to a variable in asynchronous lambda's capture list.
  /// {
  ///   lepus::Value value;
  ///   do_async([s = value.String()] {
  ///     // s is not retaining the underlying string and is not safe to be used
  ///     // in asynchronous codes.
  ///   });
  /// }
  ///
  /// Instead you should write code like this so that the variable seen by
  /// lambda body retains the string when capture list is formed.
  /// {
  ///   lepus::Value value;
  ///   auto value_str = value.String();  // Not retaining the string
  ///   do_async([value_str] {            // Capture by copy.
  ///     // This value_str is copy constructed from outer value_str.
  ///   });
  /// }
  base::String String() const&;

  /// For rvalue this object, returns base::String which retains the underlying
  /// string impl to avoid dangling pointer. Consider a function which returns
  /// a Value. Without this overload, `s` would be using a dangling string impl.
  ///   static lepus::Value funcA(const char* s) {
  ///      return lepus::Value(s);
  ///   }
  ///   auto s = funcA("asdf").String();
  ///
  /// There is one case which matches this overload but not desired.
  ///   auto s = table_value.GetProperty("a").String();
  /// s would retain the string impl because String() is called for a rvalue
  /// returned by GetProperty(). In this case the base::String returned is not
  /// optimal but acceptable for safety reason.
  base::String String() &&;

  fml::RefPtr<lepus::Dictionary> Table() const;
  fml::RefPtr<lepus::CArray> Array() const;
  fml::RefPtr<lepus::LEPUSObject> LEPUSObject() const;
  fml::RefPtr<lepus::ByteArray> ByteArray() const;

  CFunction Function() const;
  void* CPoint() const;
  fml::RefPtr<lepus::RefCounted> RefCounted() const;

  void SetBool(bool);

  void SetString(const base::String&);
  void SetString(base::String&&);

  void SetTable(const fml::RefPtr<lepus::Dictionary>&);
  void SetTable(fml::RefPtr<lepus::Dictionary>&&);
  void SetArray(const fml::RefPtr<lepus::CArray>&);
  void SetArray(fml::RefPtr<lepus::CArray>&&);

  void SetJSObject(const fml::RefPtr<lepus::LEPUSObject>&);
  void SetJSObject(fml::RefPtr<lepus::LEPUSObject>&&);
  void SetByteArray(const fml::RefPtr<lepus::ByteArray>&);
  void SetByteArray(fml::RefPtr<lepus::ByteArray>&&);

  void SetRefCounted(const fml::RefPtr<lepus::RefCounted>&);
  void SetRefCounted(fml::RefPtr<lepus::RefCounted>&&);

  bool SetProperty(uint32_t idx, const Value& val);
  bool SetProperty(uint32_t idx, Value&& val);
  bool SetProperty(const base::String& key, const Value& val);
  bool SetProperty(base::String&& key, const Value& val);
  bool SetProperty(base::String&& key, Value&& val);

  Value GetProperty(uint32_t idx) const;
  Value GetProperty(const base::String& key) const;

  int GetLength() const;
  bool Contains(const base::String& key) const;
  static void MergeValue(lepus::Value& target, const lepus::Value& update);
  static bool UpdateValueByPath(lepus::Value& target,
                                const lepus::Value& update,
                                const base::Vector<std::string>& path);

  static lepus::Value CreateObject(Context* ctx = nullptr);
  static lepus::Value CreateArray(Context* ctx = nullptr);

  bool MarkConst() const;

  Value(LEPUSContext* ctx, const LEPUSValue& val);
  Value(LEPUSContext* ctx, LEPUSValue&& val);
  BASE_EXPORT bool IsJSValue() const;

  LEPUSContext* context() const { return cell_ ? cell_->ctx_ : nullptr; }

  LEPUSValue ToJSValue(LEPUSContext* ctx, bool deep_convert = false) const;

  /*
   deep convert jsvalue to lepus::Value when deep_convert == true;
   */
  Value ToLepusValue(bool deep_convert = false) const;

  inline LEPUSValue WrapJSValue() const {
    if (!IsJSValue()) return LEPUS_UNDEFINED;
#if defined(__aarch64__) && !defined(OS_WIN) && !DISABLE_NANBOX
    return (LEPUSValue){.as_int64 = val_int64_t_};
#else
    return LEPUS_MKPTR(DecodeJSTag(tag_), val_ptr_);
#endif
  }

  inline bool IsJSCPointer() const {
    return IsJSValue() && LEPUS_VALUE_IS_LEPUS_CPOINTER(WrapJSValue());
  }

  inline void* LEPUSCPointer() const {
    DCHECK(IsJSCPointer());
    return LEPUS_VALUE_GET_CPOINTER(WrapJSValue());
  }

  bool IsJSArray() const;
  bool IsJSTable() const;

  inline bool IsJSBool() const {
    return IsJSValue() && LEPUS_VALUE_IS_BOOL(WrapJSValue());
  }
  inline bool LEPUSBool() const {
    if (!IsJSBool()) return false;
    return LEPUS_VALUE_GET_BOOL(WrapJSValue());
  }
  inline bool IsJSString() const {
    return IsJSValue() && LEPUS_IsString(WrapJSValue());
  }

  inline bool IsJSUndefined() const {
    return IsJSValue() && LEPUS_VALUE_IS_UNDEFINED(WrapJSValue());
  }

  inline bool IsJSNumber() const {
    auto value = WrapJSValue();
    return IsJSValue() &&
           (LEPUS_VALUE_IS_INT(value) || LEPUS_VALUE_IS_FLOAT64(value) ||
            LEPUS_VALUE_IS_BIG_INT(value));
  }

  inline bool IsJsNull() const {
    return IsJSValue() && LEPUS_VALUE_IS_NULL(WrapJSValue());
  }

  double LEPUSNumber() const;
  bool IsJSInteger() const;
  bool IsJSFunction() const;
  int GetJSLength() const;
  bool IsJSFalse() const;
  int64_t JSInteger() const;
  std::string ToString() const;

  void SetCPoint(void*);
  void SetCFunction(CFunction);
  void SetNan(bool);
  ~Value();

  bool IsTrue() const { return !IsFalse(); }

  bool IsFalse() const {
    return type_ == Value_Nil || type_ == Value_NaN ||
           type_ == Value_Undefined || (type_ == Value_Bool && !Bool()) ||
           (IsNumber() && Number() == 0) ||
           (type_ == Value_String && StringView().empty()) || IsJSFalse();
  }
  inline bool IsEmpty() const {
    return (type_ == Value_Nil) || (type_ == Value_Undefined) ||
           IsJSUndefined() || IsJsNull();
  }

  inline void SetNil() {
    FreeValue();
    type_ = Value_Nil;
    val_ptr_ = nullptr;
  }

  inline void SetUndefined() {
    FreeValue();
    type_ = Value_Undefined;
    val_ptr_ = nullptr;
  }

  bool IsEqual(const Value& value) const;
  BASE_EXPORT_FOR_DEVTOOL friend bool operator==(const Value& left,
                                                 const Value& right);

  BASE_EXPORT_FOR_DEVTOOL friend bool operator!=(const Value& left,
                                                 const Value& right) {
    return !(left == right);
  }

  friend Value operator+(const Value& left, const Value& right) {
    Value value;
    if (left.IsNumber() && right.IsNumber()) {
      if (left.IsInt64() && right.IsInt64()) {
        value.SetNumber(left.Int64() + right.Int64());
      } else {
        value.SetNumber(left.Number() + right.Number());
      }
    }
    return value;
  }

  friend Value operator-(const Value& left, const Value& right) {
    Value value;
    if (left.IsNumber() && right.IsNumber()) {
      if (left.IsInt64() && right.IsInt64()) {
        value.SetNumber(left.Int64() - right.Int64());
      } else {
        value.SetNumber(left.Number() - right.Number());
      }
    }
    return value;
  }

  friend Value operator*(const Value& left, const Value& right) {
    Value value;
    if (left.IsNumber() && right.IsNumber()) {
      if (left.IsInt64() && right.IsInt64()) {
        value.SetNumber(left.Int64() * right.Int64());
      } else {
        value.SetNumber(left.Number() * right.Number());
      }
    }
    return value;
  }

  friend Value operator/(const Value& left, const Value& right) {
    Value value;
    if (left.IsNumber() && right.IsNumber()) {
      if (left.IsInt64() && right.IsInt64()) {
        value.SetNumber(left.Int64() / right.Int64());
      } else {
        value.SetNumber(left.Number() / right.Number());
      }
    }
    return value;
  }

  friend Value operator%(const Value& left, const Value& right) {
    Value value;
    if (left.IsNumber() && right.IsNumber()) {
      value.SetNumber((int64_t)(left.Number()) % ((int64_t)right.Number()));
    }
    return value;
  }

  Value& operator+=(const Value& value) {
    if (IsNumber() && value.IsNumber()) {
      if (IsInt64() && value.IsInt64()) {
        SetNumber(Int64() + value.Int64());
      } else {
        SetNumber(Number() + value.Number());
      }
    }
    return *this;
  }

  Value& operator-=(const Value& value) {
    if (IsNumber() && value.IsNumber()) {
      if (IsInt64() && value.IsInt64()) {
        SetNumber(Int64() - value.Int64());
      } else {
        SetNumber(Number() - value.Number());
      }
    }
    return *this;
  }

  Value& operator*=(const Value& value) {
    if (IsNumber() && value.IsNumber()) {
      if (IsInt64() && value.IsInt64()) {
        SetNumber(Int64() * value.Int64());
      } else {
        SetNumber(Number() * value.Number());
      }
    }
    return *this;
  }

  Value& operator/=(const Value& value) {
    if (IsNumber() && value.IsNumber()) {
      if (IsInt64() && value.IsInt64()) {
        SetNumber(Int64() / value.Int64());
      } else {
        SetNumber(Number() / value.Number());
      }
    }
    return *this;
  }

  Value& operator%=(const Value& value) {
    if (IsNumber() && value.IsNumber()) {
      SetNumber((int64_t)Number() % (int64_t)value.Number());
    }
    return *this;
  }

  void Print() const;
  void PrintValue(std::ostream& output, bool ignore_other = false,
                  bool pretty = false) const;
  friend std::ostream& operator<<(std::ostream& output, const lepus::Value& v) {
    v.PrintValue(output);
    return output;
  }

  // override operator<< for class LogStream
  friend base::logging::LogStream& operator<<(base::logging::LogStream& output,
                                              const lepus::Value& v) {
    std::ostringstream output_lepus_value;
    v.PrintValue(output_lepus_value);
    output << output_lepus_value;
    return output;
  }

  void IteratorJSValue(const LepusValueIterator& callback) const;
  friend lepus::LEPUSValueHelper;

 private:
  void Copy(const Value& value);

  void ConstructValueFromLepusRef(LEPUSContext* ctx, const LEPUSValue& val);

  Value GetPropertyFromTableOrArray(const std::string& key) const;
  bool SetPropertyToTableOrArray(const std::string& key, const Value& update);

  static void ToLepusValueRecursively(Value& value, bool deep_convert);
  static Value CloneRecursively(const Value& src, bool clone_as_jsvalue);
};
}  // namespace lepus
}  // namespace lynx
typedef lynx::lepus::Value lepus_value;
#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_VALUE_H_
