// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_TESTING_PROP_BUNDLE_MOCK_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_TESTING_PROP_BUNDLE_MOCK_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_set>

#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_property.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {

namespace tasm {

class PropBundleMock : public PropBundle {
 public:
  PropBundleMock();

  void SetNullProps(const char* key) override;
  void SetProps(const char* key, unsigned int value) override;
  void SetProps(const char* key, int value) override;
  void SetProps(const char* key, const char* value) override;
  void SetProps(const char* key, bool value) override;
  void SetProps(const char* key, double value) override;
  void SetProps(const char* key, const pub::Value& value) override;
  void SetProps(const pub::Value& value) override;
  void SetEventHandler(const pub::Value& event) override;
  void SetGestureDetector(const GestureDetector& detector) override;
  bool Contains(const char* key) const override;
  void ResetEventHandler() override;

  void SetNullPropsByID(CSSPropertyID id) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetNullProps(property_name);
  };

  void SetPropsByID(CSSPropertyID id, unsigned int value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID id, int value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID id, const char* value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID id, bool value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID id, double value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID id, const pub::Value& value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  };

  std::unique_ptr<PropBundle> ShallowCopy() override { return nullptr; }

  static std::unique_ptr<PropBundle> CreateForMock();

  const std::map<std::string, lepus::Value>& GetPropsMap() const;

 private:
  std::unordered_set<std::string> event_handler_;
  std::map<std::string, lepus::Value> props_;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_TESTING_PROP_BUNDLE_MOCK_H_
