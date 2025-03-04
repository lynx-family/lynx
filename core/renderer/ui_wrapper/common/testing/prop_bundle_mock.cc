// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/common/testing/prop_bundle_mock.h"

#include <utility>

#include "core/renderer/ui_wrapper/common/prop_bundle_creator_default.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {

namespace tasm {

PropBundleMock::PropBundleMock() : PropBundle() {}

std::unique_ptr<PropBundle> PropBundleMock::CreateForMock() {
  auto pda = std::make_unique<PropBundleMock>();
  return std::move(pda);
}

void PropBundleMock::SetNullProps(const char* key) {
  auto val = pub::ValueImplLepus(lepus::Value());
  SetProps(key, val);
}
void PropBundleMock::SetProps(const char* key, unsigned int value) {
  auto val = pub::ValueImplLepus(lepus::Value(value));
  SetProps(key, val);
}
void PropBundleMock::SetProps(const char* key, int value) {
  auto val = pub::ValueImplLepus(lepus::Value(value));
  SetProps(key, val);
}
void PropBundleMock::SetProps(const char* key, const char* value) {
  auto val = pub::ValueImplLepus(lepus::Value(value));
  SetProps(key, val);
}
void PropBundleMock::SetProps(const char* key, bool value) {
  auto val = pub::ValueImplLepus(lepus::Value(value));
  SetProps(key, val);
}
void PropBundleMock::SetProps(const char* key, double value) {
  auto val = pub::ValueImplLepus(lepus::Value(value));
  SetProps(key, val);
}
void PropBundleMock::SetProps(const char* key, const pub::Value& value) {
  auto value_impl = reinterpret_cast<const pub::ValueImplLepus*>(&value);
  if (value_impl != nullptr) {
    props_[std::string(key)] = value_impl->backend_value();
  }
}

void PropBundleMock::SetProps(const pub::Value& value) {
  value.ForeachMap([&](const pub::Value& k, const pub::Value& v) {
    auto value_impl = reinterpret_cast<const pub::ValueImplLepus*>(&v);
    props_[k.str().c_str()] = value_impl->backend_value();
  });
}

bool PropBundleMock::Contains(const char* key) const {
  return props_.find(key) != props_.end();
}

void PropBundleMock::SetEventHandler(const pub::Value& event) {}

void PropBundleMock::SetGestureDetector(const GestureDetector& detector) {}

void PropBundleMock::ResetEventHandler() {}

const std::map<std::string, lepus::Value>& PropBundleMock::GetPropsMap() const {
  return props_;
}

std::unique_ptr<PropBundle> PropBundleCreatorDefault::CreatePropBundle() {
  return std::make_unique<PropBundleMock>();
}

}  // namespace tasm

}  // namespace lynx
