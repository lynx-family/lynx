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

fml::RefPtr<PropBundle> PropBundleMock::CreateForMock() {
  auto pda = fml::MakeRefCounted<PropBundleMock>();
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
  props_[std::string(key)] =
      lynx::pub::ValueUtils::ConvertValueToLepusValue(value);
}

void PropBundleMock::SetProps(const pub::Value& value) {
  value.ForeachMap([&](const pub::Value& k, const pub::Value& v) {
    props_[k.str().c_str()] =
        lynx::pub::ValueUtils::ConvertValueToLepusValue(v);
  });
}

bool PropBundleMock::Contains(const char* key) const {
  return props_.find(key) != props_.end();
}

void PropBundleMock::SetEventHandler(const pub::Value& event) {}

void PropBundleMock::SetGestureDetector(const GestureDetector& detector) {}

void PropBundleMock::ResetEventHandler() {}

void PropBundleMock::SetPropsByID(CSSPropertyID id, const uint8_t* data,
                                  size_t size) {
  auto array = lepus::Value(lepus::CArray::Create());
  for (size_t i = 0; i < size; ++i) {
    array.SetProperty(i, lepus::Value(data[i]));
  }
  auto property_name = CSSProperty::GetPropertyNameCStr(id);
  SetProps(property_name, pub::ValueImplLepus(lepus::Value(std::move(array))));
}

void PropBundleMock::SetPropsByID(CSSPropertyID id, const uint32_t* data,
                                  size_t size) {
  auto array = lepus::Value(lepus::CArray::Create());
  for (size_t i = 0; i < size; ++i) {
    array.SetProperty(i, lepus::Value(data[i]));
  }
  auto property_name = CSSProperty::GetPropertyNameCStr(id);
  SetProps(property_name, pub::ValueImplLepus(lepus::Value(std::move(array))));
}

const std::map<std::string, lepus::Value>& PropBundleMock::GetPropsMap() const {
  return props_;
}

fml::RefPtr<PropBundle> PropBundleCreatorDefault::CreatePropBundle() {
  return fml::MakeRefCounted<PropBundleMock>();
}

}  // namespace tasm

}  // namespace lynx
