// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_RESTRICTED_VALUE_H_
#define CORE_RUNTIME_LEPUS_RESTRICTED_VALUE_H_

#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "base/include/base_defines.h"
#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/log/logging.h"
#include "base/include/type_traits_addon.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/lynx_value_extended.h"
#include "base/include/value/ref_type.h"

namespace lynx {
namespace lepus {

// RestrictedValue, as its name suggests, is only used in the lepusVM runtime
// environment. It cannot encapsulate NG values ​​like the Value in the base
// library. You should not use the RestrictedValue type directly in external
// code.
struct RestrictedValue {
 private:
  lynx_value value_;

 public:
  RestrictedValue() { value_.type = lynx_value_null; };
  ~RestrictedValue() { FreeValue(); }

  enum CreateAsUndefinedTag { kCreateAsUndefinedTag };
  explicit RestrictedValue(CreateAsUndefinedTag) {
    value_.type = lynx_value_undefined;
  }

  // It does not initialize any field of Value and the Value may be in some
  // invalid status(type not set, even not set to lynx_value_null) due to
  // random memory.
  enum UnsafeCreateAsUninitialized { kUnsafeCreateAsUninitialized };
  RestrictedValue(UnsafeCreateAsUninitialized) {}

  BASE_EXPORT BASE_INLINE RestrictedValue(const RestrictedValue& value) {
    value.DupValue();
    value_ = value.value_;
  }
  BASE_INLINE RestrictedValue(RestrictedValue&& value) noexcept {
    value_ = value.value_;
    value.value_.type = lynx_value_null;
  }
  RestrictedValue& operator=(const RestrictedValue& value);
  RestrictedValue& operator=(RestrictedValue&& value) noexcept;

  // Conversion with Value should be explicit.
  BASE_INLINE explicit RestrictedValue(const Value& value) {
    value.DupValue();
    value_ = value.value();
  }
  BASE_INLINE explicit RestrictedValue(Value&& value) noexcept {
    value_ = value.value();
    std::move(value).value().type = lynx_value_null;
  }
  BASE_EXPORT RestrictedValue& operator=(const Value& value);
  RestrictedValue& operator=(Value&& value) noexcept;

  explicit operator Value() const& {
    DupValue();
    return Value(value_);
  }
  explicit operator Value() && {
    Value result(value_);
    value_.type = lynx_value_null;
    return result;
  }

  // Full inlined `left = right`.
  static BASE_INLINE void Assign(RestrictedValue& left,
                                 const RestrictedValue& right) {
    if (likely(&left != &right)) {
      right.DupValue();
      left.FreeValue();
      left.value_ = right.value_;
    }
  }

  static BASE_INLINE void Assign(RestrictedValue& left, const Value& right) {
    right.DupValue();
    left.FreeValue();
    left.value_ = right.value();
  }

  static BASE_INLINE void Assign(Value& left, const RestrictedValue& right) {
    right.DupValue();
    left.FreeValue();
    std::move(left).value() = right.value_;
  }

  static BASE_INLINE void Assign(Value& left, RestrictedValue&& right) {
    left.FreeValue();
    std::move(left).value() = right.value_;
    right.value_.type = lynx_value_null;
  }

  explicit RestrictedValue(const lynx_value& value) : value_(value) {}

  explicit RestrictedValue(const base::String& data);
  explicit RestrictedValue(base::String&& data);
  explicit RestrictedValue(const char* data);
  explicit RestrictedValue(const std::string& data);
  explicit RestrictedValue(std::string&& data);

  explicit RestrictedValue(const fml::RefPtr<Dictionary>& data);
  explicit RestrictedValue(fml::RefPtr<Dictionary>&& data);
  explicit RestrictedValue(const fml::WeakRefPtr<Dictionary>& data);
  explicit RestrictedValue(const fml::RefPtr<CArray>& data);
  explicit RestrictedValue(fml::RefPtr<CArray>&& data);
  explicit RestrictedValue(const fml::WeakRefPtr<CArray>& data);
  explicit RestrictedValue(const fml::RefPtr<lepus::ByteArray>& data);
  explicit RestrictedValue(fml::RefPtr<lepus::ByteArray>&& data);
  explicit RestrictedValue(const fml::RefPtr<lepus::RefCounted>& data);
  explicit RestrictedValue(fml::RefPtr<lepus::RefCounted>&& data);

  enum CreateAsNanTag { kCreateAsNanTag };
  BASE_INLINE explicit RestrictedValue(CreateAsNanTag) {
    value_.type = lynx_value_nan;
    value_.val_bool = true;
  }

  BASE_INLINE explicit RestrictedValue(double val) {
    value_.type = lynx_value_double;
    value_.val_double = val;
  }

  BASE_INLINE explicit RestrictedValue(int32_t val) {
    value_.type = lynx_value_int32;
    value_.val_int32 = val;
  }

  BASE_INLINE explicit RestrictedValue(uint32_t val) {
    value_.type = lynx_value_uint32;
    value_.val_uint32 = val;
  }

  BASE_INLINE explicit RestrictedValue(int64_t val) {
    value_.type = lynx_value_int64;
    value_.val_int64 = val;
  }

  BASE_INLINE explicit RestrictedValue(uint64_t val) {
    value_.type = lynx_value_uint64;
    value_.val_uint64 = val;
  }

  BASE_INLINE explicit RestrictedValue(uint8_t val) {
    value_.type = lynx_value_uint32;
    value_.val_uint32 = val;
  }

  BASE_INLINE explicit RestrictedValue(bool val) {
    value_.type = lynx_value_bool;
    value_.val_bool = val;
  }

  BASE_INLINE explicit RestrictedValue(void* data) {
    value_.type = lynx_value_external;
    value_.val_ptr = reinterpret_cast<lynx_value_ptr>(data);
  }

  explicit RestrictedValue(CFunction val);
  explicit RestrictedValue(CFunctionBuiltin val);

  const lynx_value& value() const& { return value_; }
  lynx_value& value() && { return value_; }

  BASE_INLINE void DupValue() const {
    if (IsReference()) {
      reinterpret_cast<fml::RefCountedThreadSafeStorage*>(value_.val_ptr)
          ->AddRef();
    }
  }
  BASE_INLINE void FreeValue() {
    if (IsReference()) {
      reinterpret_cast<fml::RefCountedThreadSafeStorage*>(value_.val_ptr)
          ->Release();
    }
  }

  BASE_INLINE ValueType Type() const {
    return Value::LegacyTypeFromLynxValue(value_);
  }

  BASE_INLINE lepus::RefType RefType() const {
    return static_cast<lepus::RefType>(value_.tag);
  }

  BASE_INLINE bool IsReference() const {
    return (value_.type >= lynx_value_string &&
            value_.type <= lynx_value_object);
  }

  BASE_INLINE bool IsBool() const { return value_.type == lynx_value_bool; }

  BASE_INLINE bool IsString() const { return value_.type == lynx_value_string; }

  BASE_INLINE bool IsInt64() const { return value_.type == lynx_value_int64; }

  BASE_INLINE bool IsNumber() const {
    return value_.type >= lynx_value_double && value_.type <= lynx_value_uint64;
  }

  BASE_INLINE bool IsDouble() const { return value_.type == lynx_value_double; }

  BASE_INLINE bool IsArray() const { return value_.type == lynx_value_array; }

  BASE_INLINE bool IsTable() const { return value_.type == lynx_value_map; }

  BASE_INLINE bool IsObject() const { return IsTable(); }

  BASE_INLINE bool IsCPointer() const {
    return value_.type == lynx_value_external;
  }

  BASE_INLINE bool IsByteArray() const {
    return value_.type == lynx_value_arraybuffer;
  }

  BASE_INLINE bool IsRefCounted() const {
    return value_.type == lynx_value_object &&
           value_.tag < static_cast<int32_t>(RefType::kJSIObject);
  }

  BASE_INLINE bool IsInt32() const { return value_.type == lynx_value_int32; }
  BASE_INLINE bool IsUInt32() const { return value_.type == lynx_value_uint32; }
  BASE_INLINE bool IsUInt64() const { return value_.type == lynx_value_uint64; }
  BASE_INLINE bool IsNil() const { return value_.type == lynx_value_null; }
  BASE_INLINE bool IsUndefined() const {
    return value_.type == lynx_value_undefined;
  }
  BASE_INLINE bool IsCFunction() const {
    return value_.type == lynx_value_function;
  }
  BASE_INLINE CFunctionType GetCFunctionType() const {
    return static_cast<CFunctionType>(value_.tag);
  }

  BASE_INLINE bool IsBuiltinFunctionTable() const {
    return value_.type == lynx_value_function_table;
  }

  BASE_INLINE bool IsNaN() const { return value_.type == lynx_value_nan; }

  BASE_INLINE bool IsTrue() const { return !IsFalse(); }
  bool IsFalse() const;

  BASE_INLINE bool IsEmpty() const {
    return (value_.type == lynx_value_null) ||
           (value_.type == lynx_value_undefined);
  }

  BASE_INLINE bool IsCDate() const {
    return value_.type == lynx_value_object &&
           value_.tag == static_cast<int32_t>(RefType::kCDate);
  }

  BASE_INLINE bool IsRegExp() const {
    return value_.type == lynx_value_object &&
           value_.tag == static_cast<int32_t>(RefType::kRegExp);
  }

  BASE_INLINE bool IsClosure() const {
    return value_.type == lynx_value_object &&
           value_.tag == static_cast<int32_t>(RefType::kClosure);
  }

  BASE_INLINE bool IsCallable() const { return IsClosure(); }

  BASE_INLINE void* Ptr() const { return value_.val_ptr; }

  BASE_INLINE bool Bool() const {
    if (value_.type != lynx_value_bool) return !IsFalse();
    return value_.val_bool;
  }

  BASE_INLINE bool NaN() const {
    return value_.type == lynx_value_nan && value_.val_bool;
  }

  double Number() const;

#define NumberValue(name, type) type name() const;
  NumberType(NumberValue)
#undef NumberValue

      /// @brief Returns a string view of the internal lepus string
      /// storage. The result is guaranteed to be null terminated.
      std::string_view StringView() const {
    return StdString();
  }

  const char* CString() const { return StdString().c_str(); }
  const std::string& StdString() const;
  base::String String() const&;
  base::String String() &&;

  fml::WeakRefPtr<Dictionary> Table() const&;
  BASE_EXPORT fml::WeakRefPtr<CArray> Array() const&;
  fml::WeakRefPtr<lepus::ByteArray> ByteArray() const&;
  BASE_EXPORT fml::WeakRefPtr<lepus::RefCounted> RefCounted() const&;

  fml::RefPtr<Dictionary> Table() &&;
  fml::RefPtr<CArray> Array() &&;
  fml::RefPtr<lepus::ByteArray> ByteArray() &&;
  fml::RefPtr<lepus::RefCounted> RefCounted() &&;

  BASE_INLINE CFunction Function() const {
    DCHECK(value_.type == lynx_value_function &&
           value_.tag == static_cast<int32_t>(CFunctionType_Default));
    return reinterpret_cast<CFunction>(Ptr());
  }
  BASE_INLINE CFunctionBuiltin FunctionBuiltin() const {
    DCHECK(value_.type == lynx_value_function &&
           value_.tag == static_cast<int32_t>(CFunctionType_Builtin));
    return reinterpret_cast<CFunctionBuiltin>(Ptr());
  }
  BASE_INLINE BuiltinFunctionTable* FunctionTable() const {
    DCHECK(value_.type == lynx_value_function_table);
    return reinterpret_cast<BuiltinFunctionTable*>(Ptr());
  }
  void* CPoint() const;

  BASE_INLINE void SetNil() {
    FreeValue();
    value_.type = lynx_value_null;
  }

  void SetNan(bool value) {
    FreeValue();
    value_.type = lynx_value_nan;
    value_.val_bool = value;
  }

  BASE_INLINE void SetUndefined() {
    FreeValue();
    value_.type = lynx_value_undefined;
  }

  void SetBool(bool val) {
    FreeValue();
    value_.type = lynx_value_bool;
    value_.val_bool = val;
  }

  void SetNumber(double val) {
    FreeValue();
    value_.type = lynx_value_double;
    value_.val_double = val;
  }

  void SetNumber(int32_t val) {
    FreeValue();
    value_.type = lynx_value_int32;
    value_.val_int32 = val;
  }

  void SetNumber(uint32_t val) {
    FreeValue();
    value_.type = lynx_value_uint32;
    value_.val_uint32 = val;
  }

  void SetNumber(int64_t val) {
    FreeValue();
    value_.type = lynx_value_int64;
    value_.val_int64 = val;
  }

  void SetNumber(uint64_t val) {
    FreeValue();
    value_.type = lynx_value_uint64;
    value_.val_uint64 = val;
  }

  void SetString(const base::String&);
  void SetString(base::String&&);

  void SetTable(const fml::RefPtr<Dictionary>&);
  void SetTable(fml::RefPtr<Dictionary>&&);

  void SetArray(const fml::RefPtr<CArray>&);
  void SetArray(fml::RefPtr<CArray>&&);

  void SetByteArray(const fml::RefPtr<lepus::ByteArray>&);
  void SetByteArray(fml::RefPtr<lepus::ByteArray>&&);

  void SetRefCounted(const fml::RefPtr<lepus::RefCounted>&);
  void SetRefCounted(fml::RefPtr<lepus::RefCounted>&&);

  bool SetProperty(uint32_t idx, const RestrictedValue& val);
  bool SetProperty(uint32_t idx, RestrictedValue&& val);
  bool SetProperty(const base::String& key, const RestrictedValue& val);
  bool SetProperty(base::String&& key, const RestrictedValue& val);
  bool SetProperty(base::String&& key, RestrictedValue&& val);

  int GetLength() const;
  RestrictedValue GetProperty(uint32_t idx) const;
  RestrictedValue GetProperty(const base::String& key) const;

  BASE_INLINE friend bool operator==(const Value& left,
                                     const RestrictedValue& right) {
    char tmp[sizeof(RestrictedValue)];
    RestrictedValue& tmp_value = *reinterpret_cast<RestrictedValue*>(tmp);
    std::move(tmp_value).value() = left.value();
    return tmp_value == right;
  }
  friend bool operator==(const RestrictedValue& left, const Value& right) {
    return right == left;
  }
  friend bool operator!=(const Value& left, const RestrictedValue& right) {
    return !(left == right);
  }
  friend bool operator!=(const RestrictedValue& left, const Value& right) {
    return !(left == right);
  }

  friend bool operator==(const RestrictedValue& left,
                         const RestrictedValue& right);
  friend bool operator!=(const RestrictedValue& left,
                         const RestrictedValue& right) {
    return !(left == right);
  }

  // A flag telling base containers such as `base::Vector<>` to optimize
  // for reallocate, insert and erase.
  using TriviallyRelocatable = bool;

  friend struct Unsafe;

  // In a secure environment, provide an efficient interface for modifying data
  // to optimize the execution efficiency of the VMContext.
  struct Unsafe {
    Unsafe() = delete;

    struct TypeSure {
      static BASE_INLINE fml::WeakRefPtr<lepus::RefCounted> GetRefCounted(
          const RestrictedValue& value) {
        return fml::WeakRefPtr<lepus::RefCounted>(
            reinterpret_cast<lepus::RefCounted*>(value.value_.val_ptr));
      }

      static BASE_INLINE int64_t GetInt64(const RestrictedValue& value) {
        return value.value_.val_int64;
      }

      static BASE_INLINE void SetInt64(RestrictedValue& value, int64_t v) {
        value.value_.val_int64 = v;
      }

      static BASE_INLINE double GetNumberFast(const RestrictedValue& value) {
        // Avoid switch table for double and int64 type.
        if (likely(value.value_.type == lynx_value_double)) {
          return value.value_.val_double;
        } else if (value.value_.type == lynx_value_int64) {
          return value.value_.val_int64;
        } else {
          return value.Number();
        }
      }

      static BASE_INLINE double GetNumberDoubleFast(
          const RestrictedValue& value) {
        // Avoid switch table for double.
        if (likely(value.value_.type == lynx_value_double)) {
          return value.value_.val_double;
        } else {
          return value.Number();
        }
      }

      static bool IsNumberDoubleFast(const RestrictedValue& value,
                                     double& result) {
        auto type = value.value_.type;
        if (likely(type == lynx_value_double)) {
          result = value.value_.val_double;
          return true;
        } else if (type == lynx_value_int64) {
          result = value.value_.val_int64;
          return true;
        } else {
          switch (type) {
            case lynx_value_int32:
              result = value.value_.val_int32;
              return true;
            case lynx_value_uint32:
              result = value.value_.val_uint32;
              return true;
            case lynx_value_uint64:
              result = value.value_.val_uint64;
              return true;
            default:
              return false;
          }
        }
      }

      static BASE_INLINE fml::WeakRefPtr<Dictionary> GetTable(
          const RestrictedValue& value) {
        return fml::WeakRefPtr<Dictionary>(
            reinterpret_cast<Dictionary*>(value.value_.val_ptr));
      }

      static BASE_INLINE fml::WeakRefPtr<CArray> GetArray(
          const RestrictedValue& value) {
        return fml::WeakRefPtr<CArray>(
            reinterpret_cast<CArray*>(value.value_.val_ptr));
      }

      static BASE_INLINE base::String GetString(const RestrictedValue& value) {
        return base::String::Unsafe::ConstructWeakRefStringFromRawRef(
            reinterpret_cast<base::RefCountedStringImpl*>(
                value.value_.val_ptr));
      }

      static BASE_INLINE const std::string& GetStdString(
          const RestrictedValue& value) {
        return reinterpret_cast<base::RefCountedStringImpl*>(
                   value.value_.val_ptr)
            ->str();
      }
    };

    struct NoFree {
      static BASE_INLINE void SetNumber(RestrictedValue& value, double v) {
        value.value_.type = lynx_value_double;
        value.value_.val_double = v;
      }

      static BASE_INLINE void SetNumber(RestrictedValue& value, int64_t v) {
        value.value_.type = lynx_value_int64;
        value.value_.val_int64 = v;
      }
    };
  };
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_RESTRICTED_VALUE_H_
