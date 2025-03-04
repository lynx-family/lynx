// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_VALUE_WRAPPER_VALUE_IMPL_PIPER_H_
#define CORE_VALUE_WRAPPER_VALUE_IMPL_PIPER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/public/pub_value.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx {
namespace pub {

// piper value implementation
class ValueImplPiper : public Value {
 public:
  ValueImplPiper(piper::Runtime& rt, piper::Value&& value)
      : Value(ValueBackendType::ValueBackendTypePiper),
        rt_(rt),
        backend_value_(rt, std::move(value)) {}
  ValueImplPiper(piper::Runtime& rt, const piper::Value& value)
      : Value(ValueBackendType::ValueBackendTypePiper),
        rt_(rt),
        backend_value_(rt, value) {}

  int64_t Type() const override {
    switch (backend_value_.kind()) {
      case piper::Value::ValueKind::UndefinedKind:
        return lepus::Value_Undefined;
      case piper::Value::ValueKind::NullKind:
        return lepus::Value_Nil;
      case piper::Value::ValueKind::BooleanKind:
        return lepus::Value_Bool;
      case piper::Value::ValueKind::NumberKind:
        return lepus::Value_Double;
      case piper::Value::ValueKind::StringKind:
        return lepus::Value_String;
      case piper::Value::ValueKind::ObjectKind: {
        auto obj = backend_value_.getObject(rt_);
        if (obj.isArray(rt_)) {
          return lepus::Value_Array;
        }
        if (obj.isFunction(rt_)) {
          return 0;
        }
        return lepus::Value_Table;
      }
      default:
        DCHECK(false);
        return 0;
    }
  }

  template <typename T>
  bool IsIntegerInRange(T a, T b) const {
    if (!backend_value_.isNumber()) {
      return false;
    }
    double number = backend_value_.getNumber();
    int64_t integer = static_cast<int64_t>(number);
    if (static_cast<double>(integer) != number) {
      return false;
    }
    return static_cast<T>(integer) >= a && static_cast<T>(integer) <= b;
  }

  bool IsUndefined() const override { return backend_value_.isUndefined(); }
  bool IsBool() const override { return backend_value_.isBool(); }
  bool IsInt32() const override { return false; }
  bool IsInt64() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return ValueUtils::IsBigInt(rt_, obj);
    }
    return false;
  }
  bool IsUInt32() const override { return false; }
  bool IsUInt64() const override { return false; }
  bool IsDouble() const override { return backend_value_.isNumber(); }
  bool IsNumber() const override { return backend_value_.isNumber(); }

  bool IsNil() const override { return backend_value_.isNull(); }
  bool IsString() const override { return backend_value_.isString(); }
  bool IsArray() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return obj.isArray(rt_);
    }
    return false;
  }
  bool IsArrayBuffer() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return obj.isArrayBuffer(rt_);
    }
    return false;
  }
  bool IsMap() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return !obj.isArray(rt_) && !obj.isArrayBuffer(rt_) &&
             !obj.isFunction(rt_) && !ValueUtils::IsBigInt(rt_, obj);
    }
    return false;
  }
  bool IsFunction() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return obj.isFunction(rt_);
    }
    return false;
  }

  bool Bool() const override { return backend_value_.getBool(); }
  int32_t Int32() const override {
    return static_cast<int32_t>(backend_value_.getNumber());
  }
  int64_t Int64() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      std::string result;
      if (ValueUtils::ConvertBigIntToStringIfNecessary(rt_, obj, result)) {
        return static_cast<int64_t>(std::strtoll(result.c_str(), nullptr, 0));
      }
    }
    return static_cast<int64_t>(backend_value_.getNumber());
  }
  uint32_t UInt32() const override {
    return static_cast<uint32_t>(backend_value_.getNumber());
  }
  uint64_t UInt64() const override {
    return static_cast<uint64_t>(backend_value_.getNumber());
  }
  double Double() const override { return backend_value_.getNumber(); }
  double Number() const override { return backend_value_.getNumber(); }
  uint8_t* ArrayBuffer() const override {
    piper::Object obj = backend_value_.getObject(rt_);
    piper::ArrayBuffer array_buffer = obj.getArrayBuffer(rt_);
    return array_buffer.data(rt_);
  }

  const std::string& str() const override {
    if (backend_value_.isString()) {
      if (cached_str_.empty()) {
        const_cast<ValueImplPiper*>(this)->cached_str_ =
            backend_value_.getString(rt_).utf8(rt_);
      }
      return cached_str_;
    } else if (backend_value_.isBool()) {
      static std::string true_str("true");
      static std::string false_str("false");
      return backend_value_.getBool() ? true_str : false_str;
    }
    // When backend_value_ is not a string type, the value returned by String()
    // method will be free after leaving this function scope. Returning
    // String()->str() directly will result in heap-use-after-free error.
    // So we return a static string here.
    static std::string empty("");
    return empty;
  }
  int Length() const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      if (obj.isArray(rt_)) {
        return static_cast<int>(
            std::move(obj).asArray(rt_)->size(rt_).value_or(0));
      }
      if (obj.isArrayBuffer(rt_)) {
        return static_cast<int>(obj.getArrayBuffer(rt_).length(rt_));
      }
      auto val = obj.getProperty(rt_, "length");
      if (val && val->isNumber()) {
        return static_cast<int>(val->getNumber());
      }
    } else if (backend_value_.isString()) {
      auto string = backend_value_.getString(rt_);
      return static_cast<int>(string.utf8(rt_).size());
    }
    return 0;
  }

  bool IsEqual(const Value& value) const override {
    if (value.backend_type() != ValueBackendType::ValueBackendTypePiper) {
      return false;
    }
    return piper::Value::strictEquals(
        rt_, backend_value_,
        static_cast<const ValueImplPiper&>(value).backend_value_);
  }

  void ForeachArray(ForeachArrayFunc func) const override {
    if (!backend_value_.isObject()) {
      return;
    }
    auto obj = backend_value_.getObject(rt_);
    if (!obj.isArray(rt_)) {
      return;
    }
    piper::Array array = obj.getArray(rt_);
    auto size_opt = array.size(rt_);
    if (!size_opt) {
      LOGE("There is error in ForeachArray: can't find the size.");
      return;
    }
    for (size_t i = 0; i < *size_opt; ++i) {
      auto prop = array.getValueAtIndex(rt_, i);
      if (!prop) {
        LOGE("ForeachArray index[" + std::to_string(i) + "] is null.");
        return;
      }
      ValueImplPiper impl_value(rt_, *prop);
      func(i, impl_value);
    }
  }

  void ForeachMap(ForeachMapFunc func) const override {
    if (!backend_value_.isObject()) {
      return;
    }
    auto obj = backend_value_.getObject(rt_);
    if (obj.isArray(rt_)) {
      return;
    }
    auto names = obj.getPropertyNames(rt_);
    if (!names) {
      rt_.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
          "There is error in ForeachMap: getPropertyNames failed."));
      return;
    }
    auto size = (*names).size(rt_);
    if (!size) {
      LOGE("There is error in ForeachMap: can't find the size.");
      return;
    }
    for (size_t i = 0; i < *size; ++i) {
      auto item = (*names).getValueAtIndex(rt_, i);
      if (!item) {
        LOGE("ForeachMap key[" + std::to_string(i) + "] is null.");
        return;
      }
      piper::String name = item->getString(rt_);
      auto prop = obj.getProperty(rt_, name);
      if (!prop) {
        LOGE("ForeachMap value[" + name.utf8(rt_) + "] is null.");
        return;
      }
      ValueImplPiper impl_key(rt_, name);
      ValueImplPiper impl_value(rt_, *prop);
      func(impl_key, impl_value);
    }
  }

  std::unique_ptr<Value> GetValueAtIndex(uint32_t idx) const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      if (obj.isArray(rt_)) {
        auto val = std::move(obj).asArray(rt_)->getValueAtIndex(rt_, idx);
        if (val) {
          return std::make_unique<ValueImplPiper>(rt_, std::move(*val));
        }
      }
    }
    // Returns an empty Value if it's not a array to keep consistent with
    // piper::Value
    return std::make_unique<ValueImplPiper>(rt_, piper::Value());
  }

  bool Erase(uint32_t idx) const override {
    DCHECK(false);
    return false;
  }

  std::unique_ptr<Value> GetValueForKey(const std::string& key) const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      auto val = obj.getProperty(rt_, key.c_str());
      if (val) {
        return std::make_unique<ValueImplPiper>(rt_, std::move(*val));
      }
    }
    // Returns an empty Value if it's not a map to keep consistent with
    // piper::Value
    return std::make_unique<ValueImplPiper>(rt_, piper::Value());
  }

  bool Erase(const std::string& key) const override {
    DCHECK(false);
    return false;
  }

  bool Contains(const std::string& key) const override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return obj.hasProperty(rt_, key.c_str());
    }
    return false;
  }

  bool PushValueToArray(const Value& value) override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      if (obj.isArray(rt_)) {
        auto array = std::move(obj).asArray(rt_);
        return array->setValueAtIndex(
            rt_, array->size(rt_).value_or(0),
            ValueUtils::ConvertValueToPiperValue(rt_, value));
      }
    }
    return false;
  }

  bool PushValueToMap(const std::string& key, const Value& value) override {
    if (backend_value_.isObject()) {
      auto obj = backend_value_.getObject(rt_);
      return obj.setProperty(rt_, key.c_str(),
                             ValueUtils::ConvertValueToPiperValue(rt_, value));
    }
    return false;
  }

  bool CheckCircle(std::vector<std::unique_ptr<pub::Value>>* prev_value_vector,
                   int depth) const override {
    if (prev_value_vector == nullptr || prev_value_vector->empty()) {
      return false;
    }
    static constexpr int kMaxDepth = 50;
    if (!rt_.IsEnableCircularDataCheck() || !rt_.IsCircularDataCheckUnset() ||
        depth < kMaxDepth) {
      return false;
    }

    for (auto& item : *prev_value_vector) {
      if (IsEqual(*item)) {
        auto message = std::string("JS circular reference is found.");
        rt_.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(message));
        LOGE(message);
        return true;
      }
    }
    return false;
  };

  std::unique_ptr<Value> Clone() const override {
    auto value = piper::Value(rt_, backend_value_);
    auto piper_value = std::make_unique<ValueImplPiper>(rt_, value);
    return piper_value;
  }

  piper::Runtime& rt() { return rt_; }
  const piper::Value& backend_value() const { return backend_value_; }

 private:
  piper::Runtime& rt_;
  piper::Value backend_value_;
  std::string cached_str_;
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_VALUE_WRAPPER_VALUE_IMPL_PIPER_H_
