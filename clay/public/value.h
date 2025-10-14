// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_PUBLIC_VALUE_H_
#define CLAY_PUBLIC_VALUE_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "clay/public/clay.h"

namespace lynx {
class ClayValueWrapper;
}

namespace clay {
class Value {
 public:
  enum Type {
    kNone = 0,  // Internal usage. Means invalid.
    kBool,
    kInt,
    kUInt,
    kLong,
    kFloat,
    kDouble,
    kString,
    kPointer,
    kArray,
    kArrayBuffer,
    kMap,
  };

  using Array = std::vector<Value>;
  using ArrayBuffer = std::vector<uint8_t>;
  using Map = std::unordered_map<std::string, Value>;

  explicit Value(bool v) : value_(v) {}
  explicit Value(int32_t v) : value_(v) {}
  explicit Value(uint32_t v) : value_(v) {}
  explicit Value(int64_t v) : value_(v) {}
  explicit Value(float v) : value_(v) {}
  explicit Value(double v) : value_(v) {}
  explicit Value(const char* v) : value_(std::string(v)) {}
  explicit Value(const std::string_view& v)
      : value_(std::string(v.data(), v.size())) {}
  explicit Value(const std::string& v) : value_(v) {}
  explicit Value(std::string&& v) : value_(std::move(v)) {}
  explicit Value(const ClayPointer& v) : value_(v) {}
  explicit Value(Array&& v) : value_(std::make_shared<Array>(std::move(v))) {}
  explicit Value(ArrayBuffer&& v)
      : value_(std::make_shared<ArrayBuffer>(std::move(v))) {}
  explicit Value(Map&& v) : value_(std::make_shared<Map>(std::move(v))) {}
  explicit Value(std::shared_ptr<Array> v) : value_(v) {}
  explicit Value(std::shared_ptr<ArrayBuffer> v) : value_(v) {}
  explicit Value(std::shared_ptr<Map> v) : value_(v) {}
  explicit Value(Type type) = delete;
  explicit Value(void* ptr, std::function<void(void*)> deleter)
      : value_(ClayPointer{ClayPointer::kClayPointerTypeExternal, nullptr}),
        wrapper_(ptr, std::move(deleter)) {}
  Value(std::initializer_list<Value>&& v);
  Value(std::initializer_list<std::pair<std::string, Value>>&& v);
  Value() = default;
  Value(Value&& that);
  Value& operator=(Value&& that);
  ~Value();

  // Prevent accidental copying
  Value& operator=(const Value&) = delete;
#ifndef CLAY_UNIT_TESTS
  Value(const Value&) = delete;
#else
  // only gmock used
  Value(const Value& that) : value_(that.value_) {}
#endif

  static Value Null() {
    return Value{ClayPointer{ClayPointer::kClayPointerTypeVoidPtr, nullptr}};
  }

  Type type() const { return static_cast<Type>(value_.index()); }
  bool IsNone() const { return type() == kNone; }
  bool IsNull() const { return type() == kPointer && GetPointer() == nullptr; }
  bool IsBool() const { return type() == kBool; }
  bool IsInt() const { return type() == kInt; }
  bool IsUint() const { return type() == kUInt; }
  bool IsLong() const { return type() == kLong; }
  bool IsFloat() const { return type() == kFloat; }
  bool IsDouble() const { return type() == kDouble; }
  bool IsString() const { return type() == kString; }
  bool IsPointer() const { return type() == kPointer; }
  bool IsArray() const { return type() == kArray; }
  bool IsArrayBuffer() const { return type() == kArrayBuffer; }
  bool IsMap() const { return type() == kMap; }

  bool GetBool() const { return value<bool>(); }
  int32_t GetInt() const { return value<int32_t>(); }
  uint32_t GetUint() const { return value<uint32_t>(); }
  int64_t GetLong() const { return value<int64_t>(); }
  float GetFloat() const { return value<float>(); }
  double GetDouble() const { return value<double>(); }
  const std::string& GetString() const { return value<std::string>(); }
  std::string& GetString() { return value<std::string>(); }
  const ArrayBuffer& GetArrayBuffer() const {
    return *value<std::shared_ptr<ArrayBuffer>>();
  }
  ArrayBuffer& GetArrayBuffer() {
    return *value<std::shared_ptr<ArrayBuffer>>();
  }
  const Array& GetArray() const { return *value<std::shared_ptr<Array>>(); }
  Array& GetArray() { return *value<std::shared_ptr<Array>>(); }
  const Map& GetMap() const { return *value<std::shared_ptr<Map>>(); }
  Map& GetMap() { return *value<std::shared_ptr<Map>>(); }
  ClayPointer::ClayPointerType GetPointerType() const {
    return value<ClayPointer>().type;
  }
  void* GetPointer() const { return value<ClayPointer>().pointer; }

  // Note: Changing from `std::get` to `std::get_if` to avoid compiler errors on
  // iOS 10.
  template <typename T>
  const T& value() const {
    auto* ptr = std::get_if<T>(&value_);
    return *ptr;
  }
  template <typename T>
  T& value() {
    auto* ptr = std::get_if<T>(&value_);
    return *ptr;
  }

  bool IsExternal() const {
    return type() == clay::Value::kPointer &&
           GetPointerType() == ClayPointer::kClayPointerTypeExternal;
  }
  std::unique_ptr<void, std::function<void(void*)>>& External() {
    return wrapper_;
  }

  const std::unique_ptr<void, std::function<void(void*)>>& GetWrapper() const {
    return wrapper_;
  }

  bool IsWrapperValid() const { return wrapper_ != nullptr; }

  void SetWrapper(std::unique_ptr<void, std::function<void(void*)>>&& wrapper) {
    wrapper_ = std::move(wrapper);
  }

 private:
  friend lynx::ClayValueWrapper;

  std::variant<std::monostate,                // kNone
               bool,                          // kBool
               int32_t,                       // kInt
               uint32_t,                      // kUInt
               int64_t,                       // kLong
               float,                         // kFloat
               double,                        // kDouble
               std::string,                   // kString
               ClayPointer,                   // kPointer
               std::shared_ptr<Array>,        // kArray
               std::shared_ptr<ArrayBuffer>,  // kArrayBuffer
               std::shared_ptr<Map>>          // kMap
      value_;

  std::unique_ptr<void, std::function<void(void*)>> wrapper_;
};

}  // namespace clay

#endif  // CLAY_PUBLIC_VALUE_H_
