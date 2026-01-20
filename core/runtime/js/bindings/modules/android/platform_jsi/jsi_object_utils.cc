// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/android/platform_jsi/jsi_object_utils.h"

#include "core/base/js_constants.h"

namespace lynx {
namespace runtime {
namespace js {
namespace platform_jsi {

std::unique_ptr<JSIObject> JSIObject::Null() {
  return std::unique_ptr<JSIObject>(new JSIObjectHostObject(nullptr));
}

std::unique_ptr<JSIObject> JSIObject::Create(bool value) {
  return std::unique_ptr<JSIObject>(new JSIObjectBool(value));
}
std::unique_ptr<JSIObject> JSIObject::Create(double value) {
  return std::unique_ptr<JSIObject>(new JSIObjectNumber(value));
}
std::unique_ptr<JSIObject> JSIObject::Create(jlong value) {
  return std::unique_ptr<JSIObject>(new JSIObjectJLong(value));
}
std::unique_ptr<JSIObject> JSIObject::Create(std::string value) {
  return std::unique_ptr<JSIObject>(new JSIObjectString(std::move(value)));
}
std::unique_ptr<JSIObject> JSIObject::Create(
    std::shared_ptr<HostObject> value) {
  return std::unique_ptr<JSIObject>(new JSIObjectHostObject(std::move(value)));
}

std::unique_ptr<JSIObject> JSIObject::Create(
    std::vector<std::unique_ptr<JSIObject>> value) {
  return std::unique_ptr<JSIObject>(new JSIObjectArray(std::move(value)));
}

std::optional<Value> JSIObjectBool::ConvertToValue(Runtime *rt) {
  return std::optional<Value>(value_);
}

std::optional<Value> JSIObjectNumber::ConvertToValue(Runtime *rt) {
  return std::optional<Value>(value_);
}

std::optional<Value> JSIObjectJLong::ConvertToValue(Runtime *rt) {
  if (value_ < kMinJavaScriptNumber || value_ > kMaxJavaScriptNumber) {
    auto bigint_opt = BigInt::createWithString(*rt, std::to_string(value_));
    return bigint_opt ? Value(std::move(*bigint_opt)) : Value::undefined();
  }
  return Value(static_cast<double>(value_));
}

std::optional<Value> JSIObjectString::ConvertToValue(Runtime *rt) {
  return String::createFromUtf8(*rt, value_);
}

std::optional<Value> JSIObjectHostObject::ConvertToValue(Runtime *rt) {
  return value_ ? Object::createFromHostObject(*rt, value_) : Value::null();
}

std::optional<Value> JSIObjectArray::ConvertToValue(Runtime *rt) {
  auto array = Array::createWithLength(*rt, value_.size());
  int32_t index = 0;
  for (const auto &item : value_) {
    array->setValueAtIndex(*rt, index,
                           item ? *item->ConvertToValue(rt) : Value::null());
    ++index;
  }
  return std::move(*array);
}

}  // namespace platform_jsi
}  // namespace js
}  // namespace runtime
}  // namespace lynx
