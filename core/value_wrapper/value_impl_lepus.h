// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_VALUE_IMPL_LEPUS_H_
#define CORE_VALUE_WRAPPER_VALUE_IMPL_LEPUS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/public/pub_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/byte_array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx {
namespace pub {

#define DeclarationTypeList(V) \
  V(Bool, bool)                \
  V(Double, double)            \
  V(Int32, int32_t)            \
  V(UInt32, uint32_t)          \
  V(Int64, int64_t)            \
  V(UInt64, uint64_t)          \
  V(String, const std::string&)

// lepus value implementation
class ValueImplLepus : public Value {
 public:
  explicit ValueImplLepus(const lepus::Value& value)
      : Value(ValueBackendType::ValueBackendTypeLepus), backend_value_(value) {}
  explicit ValueImplLepus(lepus::Value&& value)
      : Value(ValueBackendType::ValueBackendTypeLepus),
        backend_value_(std::move(value)) {}

  ~ValueImplLepus() override = default;

  int64_t Type() const override {
    return static_cast<int64_t>(backend_value_.Type());
  }
  bool IsUndefined() const override { return backend_value_.IsUndefined(); }
  bool IsBool() const override { return backend_value_.IsBool(); }
  bool IsInt32() const override { return backend_value_.IsInt32(); }
  bool IsInt64() const override { return backend_value_.IsInt64(); }
  bool IsUInt32() const override { return backend_value_.IsUInt32(); }
  bool IsUInt64() const override { return backend_value_.IsUInt64(); }
  bool IsDouble() const override { return backend_value_.IsDouble(); }
  bool IsNumber() const override { return backend_value_.IsNumber(); }

  bool IsNil() const override { return backend_value_.IsNil(); }
  bool IsString() const override { return backend_value_.IsString(); }
  bool IsArray() const override {
    return backend_value_.IsArray() ||
           (backend_value_.IsJSValue() && backend_value_.IsJSArray());
  }
  bool IsArrayBuffer() const override { return backend_value_.IsByteArray(); }
  bool IsMap() const override {
    return backend_value_.IsTable() ||
           (backend_value_.IsJSValue() && backend_value_.IsJSTable());
  }
  bool IsFunction() const override {
    return backend_value_.IsJSFunction() || backend_value_.IsCFunction();
  }

  bool Bool() const override { return backend_value_.Bool(); }
  int32_t Int32() const override { return backend_value_.Int32(); }
  int64_t Int64() const override { return backend_value_.Int64(); }
  uint32_t UInt32() const override { return backend_value_.UInt32(); }
  uint64_t UInt64() const override { return backend_value_.UInt64(); }
  double Double() const override { return backend_value_.Double(); }
  double Number() const override { return backend_value_.Number(); }
  uint8_t* ArrayBuffer() const override {
    return backend_value_.ByteArray()->GetPtr();
  }

  const std::string& str() const override { return backend_value_.StdString(); }
  int Length() const override {
    if (backend_value_.IsJSValue()) {
      return backend_value_.GetJSLength();
    } else if (backend_value_.IsByteArray()) {
      return static_cast<int>(backend_value_.ByteArray()->GetLength());
    } else {
      return backend_value_.GetLength();
    }
  }

  bool IsEqual(const Value& value) const override {
    if (value.backend_type() != pub::ValueBackendType::ValueBackendTypeLepus) {
      return false;
    }
    return backend_value_.IsEqual(
        (reinterpret_cast<const ValueImplLepus*>(&value))->backend_value());
  }

  void ForeachArray(pub::ForeachArrayFunc func) const override {
    tasm::ForEachLepusValue(backend_value_, [callback = std::move(func)](
                                                const lepus::Value& key,
                                                const lepus::Value& value) {
      pub::ValueImplLepus impl_value(value);
      callback(key.Int64(), impl_value);
    });
  };

  void ForeachMap(pub::ForeachMapFunc func) const override {
    tasm::ForEachLepusValue(backend_value_, [callback = std::move(func)](
                                                const lepus::Value& key,
                                                const lepus::Value& value) {
      pub::ValueImplLepus impl_key(key);
      pub::ValueImplLepus impl_value(value);
      callback(impl_key, impl_value);
    });
  };

  std::unique_ptr<Value> GetValueAtIndex(uint32_t idx) const override {
    if (!IsArray()) {
      // Returns an empty Value if it's not a array to keep consistent with
      // lepus::Value
      return std::make_unique<ValueImplLepus>(lepus::Value());
    }
    return std::make_unique<ValueImplLepus>(backend_value_.GetProperty(idx));
  }

  bool Erase(uint32_t idx) const override {
    if (!IsArray()) {
      return false;
    }
    return backend_value_.Array()->Erase(idx);
  }

  std::unique_ptr<Value> GetValueForKey(const std::string& key) const override {
    if (!IsMap()) {
      // Returns an empty Value if it's not a map to keep consistent with
      // lepus::Value
      return std::make_unique<ValueImplLepus>(lepus::Value());
    }
    return std::make_unique<ValueImplLepus>(backend_value_.GetProperty(key));
  }

  bool Erase(const std::string& key) const override {
    if (!IsMap()) {
      return false;
    }
    return backend_value_.Table()->Erase(key);
  }

  bool Contains(const std::string& key) const override {
    if (!IsMap()) {
      return false;
    }
    return backend_value_.Contains(key);
  }

  bool PushValueToArray(const Value& value) override {
    if (!IsArray()) {
      return false;
    }
    return backend_value_.Array()->emplace_back(
        ValueUtils::ConvertValueToLepusValue(value));
  }

  bool PushValueToArray(std::unique_ptr<Value> value) override {
    if (!IsArray()) {
      return false;
    }
    return backend_value_.Array()->emplace_back(
        ValueUtils::ConvertValueToLepusValue(*(value.get())));
  }

  bool PushNullToArray() override {
    if (!IsArray()) {
      return false;
    }
    return backend_value_.Array()->emplace_back();
  }

  bool PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value,
                              size_t length) override {
    if (!IsArray()) {
      return false;
    }
    return backend_value_.Array()->emplace_back(
        lepus::ByteArray::Create(std::move(value), length));
  }

  bool PushBigIntToArray(const std::string& value) override {
    if (!IsArray()) {
      return false;
    }
    auto int_value =
        static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
    return backend_value_.Array()->emplace_back(int_value);
  }

#define NormalTypePushArrayImpl(name, type)             \
  bool Push##name##ToArray(type value) override {       \
    if (!IsArray()) {                                   \
      return false;                                     \
    }                                                   \
    return backend_value_.Array()->emplace_back(value); \
  }
  DeclarationTypeList(NormalTypePushArrayImpl)
#undef NormalTypePushArrayImpl

      bool PushValueToMap(const std::string& key, const Value& value) override {
    if (!IsMap()) {
      return false;
    }
    return backend_value_.Table()->SetValue(
        key, ValueUtils::ConvertValueToLepusValue(value));
  }

  bool PushValueToMap(const std::string& key,
                      std::unique_ptr<Value> value) override {
    if (!IsMap()) {
      return false;
    }
    return backend_value_.Table()->SetValue(
        key, ValueUtils::ConvertValueToLepusValue(*(value.get())));
  }

  bool PushNullToMap(const std::string& key) override {
    if (!IsMap()) {
      return false;
    }
    return backend_value_.Table()->SetValue(key);
  }

  bool PushArrayBufferToMap(const std::string& key,
                            std::unique_ptr<uint8_t[]> value,
                            size_t length) override {
    if (!IsMap()) {
      return false;
    }
    return backend_value_.Table()->SetValue(
        key, lepus::ByteArray::Create(std::move(value), length));
  }

  bool PushBigIntToMap(const std::string& key,
                       const std::string& value) override {
    if (!IsMap()) {
      return false;
    }
    auto int_value =
        static_cast<int64_t>(std::strtoll(value.c_str(), nullptr, 0));
    return PushInt64ToMap(key, int_value);
  }

#define NormalTypePushMapImpl(name, type)                               \
  bool Push##name##ToMap(const std::string& key, type value) override { \
    if (!IsMap()) {                                                     \
      return false;                                                     \
    }                                                                   \
    return backend_value_.Table()->SetValue(key, value);                \
  }
  DeclarationTypeList(NormalTypePushMapImpl)
#undef NormalTypePushMapImpl

      const lepus::Value& backend_value() const {
    return backend_value_;
  }

  bool CheckCircle(std::vector<std::unique_ptr<pub::Value>>* prev_value_vector,
                   int depth) const override {
    NOTREACHED();
    return false;
  }
  std::unique_ptr<Value> Clone() const override {
    NOTREACHED();
    return nullptr;
  }

 private:
  lepus::Value backend_value_;
};

class PubValueFactoryDefault : public PubValueFactory {
 public:
  std::unique_ptr<Value> CreateArray() override;
  std::unique_ptr<Value> CreateMap() override;
  std::unique_ptr<Value> CreateBool(bool value) override;
  std::unique_ptr<Value> CreateNumber(double value) override;
  std::unique_ptr<Value> CreateString(const std::string& value) override;
  std::unique_ptr<Value> CreateArrayBuffer(std::unique_ptr<uint8_t[]> value,
                                           size_t length) override;
  ~PubValueFactoryDefault() {}
};

}  // namespace pub
}  // namespace lynx

using PubLepusValue = lynx::pub::ValueImplLepus;

#endif  // CORE_VALUE_WRAPPER_VALUE_IMPL_LEPUS_H_
