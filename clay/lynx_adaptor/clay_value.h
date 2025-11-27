// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_CLAY_VALUE_H_
#define CLAY_LYNX_ADAPTOR_CLAY_VALUE_H_

#include <memory>
#include <string>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/lynx_adaptor/value_converter.h"
#include "clay/public/value.h"
#include "core/public/pub_value.h"

namespace lynx {

#define DeclarationTypeList(V) \
  V(Bool, bool)                \
  V(Double, double)            \
  V(Int32, int32_t)            \
  V(UInt32, uint32_t)          \
  V(Int64, int64_t)            \
  V(String, const std::string&)

class ClayValueWrapper : public pub::Value {
 public:
  explicit ClayValueWrapper(const clay::Value& value)
      : pub::Value(pub::ValueBackendType::ValueBackendTypeCustom),
        backend_value_(value) {}
  ~ClayValueWrapper() override {}

  // TODO(chenyouhui.duke): Design common type definition for pub value.
  int64_t Type() const override { return backend_value_.type(); }

  bool IsUndefined() const override { return backend_value_.IsNone(); }
  bool IsBool() const override { return backend_value_.IsBool(); }
  bool IsInt32() const override { return backend_value_.IsInt(); }
  bool IsInt64() const override { return backend_value_.IsLong(); }
  bool IsUInt32() const override { return backend_value_.IsUint(); }
  bool IsUInt64() const override { return backend_value_.IsLong(); }
  bool IsDouble() const override {
    return backend_value_.IsFloat() || backend_value_.IsDouble();
  }
  bool IsNumber() const override;
  bool IsNil() const override { return backend_value_.IsNull(); }
  bool IsString() const override { return backend_value_.IsString(); }
  bool IsArray() const override { return backend_value_.IsArray(); }
  bool IsArrayBuffer() const override { return backend_value_.IsArrayBuffer(); }
  bool IsMap() const override { return backend_value_.IsMap(); }
  bool IsFunction() const override { return false; }

  bool Bool() const override {
    FML_DCHECK(backend_value_.IsBool());
    return backend_value_.GetBool();
  }
  int32_t Int32() const override {
    FML_DCHECK(backend_value_.IsInt());
    return backend_value_.GetInt();
  }
  int64_t Int64() const override {
    FML_DCHECK(backend_value_.IsLong());
    return backend_value_.GetLong();
  }
  uint32_t UInt32() const override {
    FML_DCHECK(backend_value_.IsUint());
    return backend_value_.GetUint();
  }
  uint64_t UInt64() const override {
    FML_DCHECK(backend_value_.IsLong());
    return backend_value_.GetLong();
  }
  double Double() const override {
    FML_DCHECK(backend_value_.IsDouble());
    return backend_value_.GetDouble();
  }
  double Number() const override;
  uint8_t* ArrayBuffer() const override {
    return const_cast<uint8_t*>(backend_value_.GetArrayBuffer().data());
  }
  const std::string& str() const override;
  int Length() const override;
  bool IsEqual(const Value& value) const override;

  void ForeachArray(pub::ForeachArrayFunc func) const override;
  void ForeachMap(pub::ForeachMapFunc func) const override;
  std::unique_ptr<Value> GetValueAtIndex(uint32_t idx) const override;
  bool Erase(uint32_t idx) const override;
  std::unique_ptr<Value> GetValueForKey(const std::string& key) const override;
  bool Erase(const std::string& key) const override;
  bool Contains(const std::string& key) const override;
  bool PushValueToArray(const Value& value) override;
  bool PushValueToArray(std::unique_ptr<Value> value) override;
  bool PushNullToArray() override;
  bool PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value,
                              size_t length) override;
  bool PushBigIntToArray(const std::string& value) override;
  bool PushUInt64ToArray(uint64_t value) override;

#define NormalTypePushArrayImpl(name, type) \
  bool Push##name##ToArray(type value) override;
  DeclarationTypeList(NormalTypePushArrayImpl)
#undef NormalTypePushArrayImpl

      bool PushValueToMap(const std::string& key, const Value& value) override;
  bool PushValueToMap(const std::string& key,
                      std::unique_ptr<Value> value) override;
  bool PushNullToMap(const std::string& key) override;
  bool PushArrayBufferToMap(const std::string& key,
                            std::unique_ptr<uint8_t[]> value,
                            size_t length) override;

  bool PushUInt64ToMap(const std::string& key, uint64_t value) override;

#define NormalTypePushMapImpl(name, type) \
  bool Push##name##ToMap(const std::string& key, type value) override;
  DeclarationTypeList(NormalTypePushMapImpl)
#undef NormalTypePushMapImpl

      const clay::Value& backend_value() const {
    return backend_value_;
  }

 private:
  static std::unique_ptr<Value> wrap(const clay::Value& value);

  const clay::Value& backend_value_;
};

class ClayValue : public ClayValueWrapper {
 public:
  ClayValue(clay::Value&& value)
      : ClayValueWrapper(holder_), holder_(std::move(value)) {}
  ~ClayValue() override {}

 private:
  clay::Value holder_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_CLAY_VALUE_H_
