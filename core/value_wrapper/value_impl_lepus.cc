// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/value_impl_lepus.h"

#include <memory>

#include "core/base/js_constants.h"
#include "core/runtime/common/utils.h"

namespace lynx {
namespace pub {
// PubValueFactory default implementation
std::unique_ptr<Value> PubValueFactoryDefault::CreateArray() {
  return std::make_unique<PubLepusValue>(lepus::Value(lepus::CArray::Create()));
}

std::unique_ptr<Value> PubValueFactoryDefault::CreateMap() {
  return std::make_unique<PubLepusValue>(
      lepus::Value(lepus::Dictionary::Create()));
}

std::unique_ptr<Value> PubValueFactoryDefault::CreateBool(bool value) {
  return std::make_unique<PubLepusValue>(lepus::Value(value));
}

std::unique_ptr<Value> PubValueFactoryDefault::CreateNumber(double value) {
  return std::make_unique<PubLepusValue>(lepus::Value(value));
}

std::unique_ptr<Value> PubValueFactoryDefault::CreateString(
    const std::string& value) {
  return std::make_unique<PubLepusValue>(lepus::Value(value));
}

std::unique_ptr<Value> PubValueFactoryDefault::CreateArrayBuffer(
    std::unique_ptr<uint8_t[]> value, size_t length) {
  return std::make_unique<PubLepusValue>(
      lepus::Value(lepus::ByteArray::Create(std::move(value), length)));
}

}  // namespace pub

}  // namespace lynx
