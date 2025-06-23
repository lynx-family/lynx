// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PROP_BUNDLE_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PROP_BUNDLE_HARMONY_H_

#include <napi/native_api.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {

namespace tasm {

class GestureDetector;

class PropBundleHarmony : public PropBundle {
  using PropMap = std::unordered_map<std::string, lepus::Value>;

 public:
  PropBundleHarmony();
  explicit PropBundleHarmony(
      const PropMap& props,
      const std::optional<std::vector<lepus::Value>>& event);
  ~PropBundleHarmony() override;

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

  void SetPropsByID(CSSPropertyID id, const uint8_t* data,
                    size_t size) override;

  void SetPropsByID(CSSPropertyID id, const uint32_t* data,
                    size_t size) override;
  napi_value GetJSProps() const;

  napi_value GetJSEventHandler() const;

  static napi_value CreateNapiValue(napi_env env, const lepus::Value& value);

  static void Init(napi_env env) { env_ = env; }

  const PropMap& GetProps() const { return props_; }

  const std::optional<std::vector<lepus::Value>>& GetEvents() const {
    return event_handler_;
  }

  const std::optional<GestureMap>& GetGestureDetectors() const {
    return gesture_detector_map_;
  }

 private:
  static napi_env env_;
  static void AssembleArray(napi_env env, napi_value array, uint32_t index,
                            const lepus::Value& value);
  static void AssembleMap(napi_env env, napi_value object, const char* key,
                          const lepus::Value& value);
  static napi_value GetNapiValue(napi_ref ref);
  PropMap props_;
  std::optional<std::vector<lepus::Value>> event_handler_;
  std::optional<GestureMap> gesture_detector_map_;
};

class PropBundleCreatorHarmony : public PropBundleCreator {
 public:
  fml::RefPtr<PropBundle> CreatePropBundle() override;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_HARMONY_PROP_BUNDLE_HARMONY_H_
