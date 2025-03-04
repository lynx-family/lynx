// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PUB_VALUE_H_
#define CORE_PUBLIC_PUB_VALUE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/closure.h"

namespace lynx {
namespace pub {

enum class ValueBackendType {
  ValueBackendTypeInvalid,
  ValueBackendTypeLepus,
  ValueBackendTypePiper,
  ValueBackendTypeCustom,
  ValueBackendTypeDarwin,
  ValueBackendTypeJava,
  ValueBackendTypeNapi,
};

class Value;

using ForeachMapFunc =
    base::MoveOnlyClosure<void, const pub::Value&, const pub::Value&>;
using ForeachArrayFunc =
    base::MoveOnlyClosure<void, int64_t, const pub::Value&>;

class ScopedCircleChecker;

class Value {
 public:
  virtual ~Value() = default;

  ValueBackendType backend_type() const { return backend_type_; }

  // Type
  virtual int64_t Type() const = 0;
  virtual bool IsUndefined() const = 0;
  virtual bool IsBool() const = 0;
  virtual bool IsInt32() const = 0;
  virtual bool IsInt64() const = 0;
  virtual bool IsUInt32() const = 0;
  virtual bool IsUInt64() const = 0;
  virtual bool IsDouble() const = 0;
  virtual bool IsNumber() const = 0;

  virtual bool IsNil() const = 0;
  virtual bool IsString() const = 0;
  virtual bool IsArray() const = 0;
  virtual bool IsArrayBuffer() const { return false; }
  virtual bool IsMap() const = 0;
  virtual bool IsFunction() const = 0;

  // Getter
  virtual bool Bool() const = 0;
  virtual double Double() const = 0;
  virtual int32_t Int32() const = 0;
  virtual uint32_t UInt32() const = 0;
  virtual int64_t Int64() const = 0;
  virtual uint64_t UInt64() const = 0;
  virtual double Number() const = 0;
  virtual uint8_t* ArrayBuffer() const { return nullptr; }
  virtual const std::string& str() const = 0;
  virtual int Length() const = 0;
  virtual bool IsEqual(const Value& value) const { return false; }

  // Iterator
  virtual void ForeachArray(pub::ForeachArrayFunc func) const = 0;
  virtual void ForeachMap(pub::ForeachMapFunc func) const = 0;

  // Find
  virtual std::unique_ptr<Value> GetValueAtIndex(uint32_t idx) const = 0;
  virtual bool Erase(uint32_t idx) const = 0;
  virtual std::unique_ptr<Value> GetValueForKey(
      const std::string& key) const = 0;
  virtual bool Erase(const std::string& key) const = 0;
  virtual bool Contains(const std::string& key) const = 0;

  // Setter
  virtual bool PushValueToArray(const Value& value) { return false; }
  virtual bool PushValueToArray(std::unique_ptr<Value> value) { return false; }
  virtual bool PushNullToArray() { return false; }
  virtual bool PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value,
                                      size_t length) {
    return false;
  }
  virtual bool PushStringToArray(const std::string& value) { return false; }
  virtual bool PushBigIntToArray(const std::string& value) { return false; }
  virtual bool PushBoolToArray(bool value) { return false; }
  virtual bool PushDoubleToArray(double value) { return false; }
  virtual bool PushInt32ToArray(int32_t value) { return false; }
  virtual bool PushUInt32ToArray(uint32_t value) { return false; }
  virtual bool PushInt64ToArray(int64_t value) { return false; }
  virtual bool PushUInt64ToArray(uint64_t value) { return false; }

  virtual bool PushValueToMap(const std::string& key, const Value& value) {
    return false;
  }
  virtual bool PushValueToMap(const std::string& key,
                              std::unique_ptr<Value> value) {
    return false;
  }
  virtual bool PushNullToMap(const std::string& key) { return false; }
  virtual bool PushArrayBufferToMap(const std::string& key,
                                    std::unique_ptr<uint8_t[]> value,
                                    size_t length) {
    return false;
  }
  virtual bool PushStringToMap(const std::string& key,
                               const std::string& value) {
    return false;
  }

  virtual bool PushBigIntToMap(const std::string& key,
                               const std::string& value) {
    return false;
  }
  virtual bool PushBoolToMap(const std::string& key, bool value) {
    return false;
  }
  virtual bool PushDoubleToMap(const std::string& key, double value) {
    return false;
  }
  virtual bool PushInt32ToMap(const std::string& key, int32_t value) {
    return false;
  }
  virtual bool PushUInt32ToMap(const std::string& key, uint32_t value) {
    return false;
  }
  virtual bool PushInt64ToMap(const std::string& key, int64_t value) {
    return false;
  }
  virtual bool PushUInt64ToMap(const std::string& key, uint64_t value) {
    return false;
  }

  // Verify
  virtual bool CheckCircle(
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector,
      int depth) const {
    return false;
  };
  virtual std::unique_ptr<Value> Clone() const { return nullptr; }

 protected:
  explicit Value(ValueBackendType backend_type) : backend_type_(backend_type) {}
  ValueBackendType backend_type_;
};

/*
 * PubValueFactory is the factory used to create a concrete implementation of
 * pub Value. Different pub Value data backends can provide different Factory
 * implementations.
 */
class PubValueFactory {
 public:
  virtual std::unique_ptr<Value> CreateArray() = 0;
  virtual std::unique_ptr<Value> CreateMap() = 0;
  virtual std::unique_ptr<Value> CreateBool(bool value) = 0;
  virtual std::unique_ptr<Value> CreateNumber(double value) = 0;
  virtual std::unique_ptr<Value> CreateString(const std::string& value) = 0;
  virtual std::unique_ptr<Value> CreateArrayBuffer(
      std::unique_ptr<uint8_t[]> value, size_t length) = 0;
  virtual ~PubValueFactory() {}
};

class ScopedCircleChecker {
 public:
  ScopedCircleChecker() = default;
  ~ScopedCircleChecker() {
    if (!scoped_value_vector_ || scoped_value_vector_->empty()) {
      return;
    }
    scoped_value_vector_->pop_back();
  }

  static std::unique_ptr<std::vector<std::unique_ptr<pub::Value>>>
  InitVectorIfNecessary(const pub::Value& value) {
    if (value.backend_type() != pub::ValueBackendType::ValueBackendTypePiper) {
      return nullptr;
    }
    auto prev_value_vector =
        std::make_unique<std::vector<std::unique_ptr<pub::Value>>>();
    prev_value_vector->push_back(value.Clone());
    return prev_value_vector;
  }

  bool CheckCircleOrCacheValue(
      std::vector<std::unique_ptr<pub::Value>>* prev_value_vector,
      const pub::Value& value, int depth) {
    if (prev_value_vector == nullptr ||
        value.backend_type() != ValueBackendType::ValueBackendTypePiper) {
      return false;
    }
    if (value.CheckCircle(prev_value_vector, depth)) {
      return true;
    }
    prev_value_vector->push_back(value.Clone());
    scoped_value_vector_ = prev_value_vector;
    return false;
  }

 private:
  std::vector<std::unique_ptr<pub::Value>>* scoped_value_vector_ = nullptr;
};
}  // namespace pub
}  // namespace lynx

#endif  // CORE_PUBLIC_PUB_VALUE_H_
