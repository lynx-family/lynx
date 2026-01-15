// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_NATIVE_PROP_BUNDLE_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_NATIVE_PROP_BUNDLE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/flex_optional.h"
#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx {

namespace tasm {

class GestureDetector;

class NativePropBundle : public PropBundle {
  using PropMap = std::unordered_map<std::string, lepus::Value>;

 public:
  NativePropBundle();
  explicit NativePropBundle(
      const PropMap& props,
      const base::flex_optional<std::vector<lepus::Value>>& event);
  ~NativePropBundle() override;

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
  fml::RefPtr<PropBundle> ShallowCopy() override;

  void SetNullPropsByID(CSSPropertyID id) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetNullProps(property_name);
  }

  void SetPropsByID(CSSPropertyID id, unsigned int value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  }

  void SetPropsByID(CSSPropertyID id, int value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  }

  void SetPropsByID(CSSPropertyID id, const char* value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  }

  void SetPropsByID(CSSPropertyID id, bool value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  }

  void SetPropsByID(CSSPropertyID id, double value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  }

  void SetPropsByID(CSSPropertyID id, const pub::Value& value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(id);
    SetProps(property_name, value);
  }

  void SetPropsByID(CSSPropertyID id, const uint8_t* data,
                    size_t size) override;

  void SetPropsByID(CSSPropertyID id, const uint32_t* data,
                    size_t size) override;

  const PropMap& GetProps() const { return props_; }

  const base::flex_optional<std::vector<lepus::Value>>& GetEvents() const {
    return event_handler_;
  }

  const std::optional<GestureMap>& GetGestureDetectors() const {
    return gesture_detector_map_;
  }

  bool IsNative() const override { return true; }

 protected:
  PropMap props_;
  base::flex_optional<std::vector<lepus::Value>> event_handler_;
  base::flex_optional<GestureMap> gesture_detector_map_;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_NATIVE_PROP_BUNDLE_H_
