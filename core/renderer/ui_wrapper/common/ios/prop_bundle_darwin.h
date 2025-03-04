// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UI_WRAPPER_COMMON_IOS_PROP_BUNDLE_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_COMMON_IOS_PROP_BUNDLE_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <vector>

#include "core/public/prop_bundle.h"
#include "core/public/pub_value.h"
#include "core/renderer/css/css_property.h"

namespace lynx {
namespace tasm {

class PropBundleCreatorDarwin : public PropBundleCreator {
 public:
  std::unique_ptr<PropBundle> CreatePropBundle() override;
};

class PropBundleDarwin : public PropBundle {
 public:
  void SetNullProps(const char* key) override;
  void SetProps(const char* key, uint value) override;
  void SetProps(const char* key, int value) override;
  void SetProps(const char* key, const char* value) override;
  void SetProps(const char* key, bool value) override;
  void SetProps(const char* key, double value) override;
  void SetProps(const char* key, const pub::Value& value) override;
  void SetProps(const pub::Value& value) override;
  void SetEventHandler(const pub::Value& event) override;
  void SetGestureDetector(const GestureDetector& detector) override;
  bool Contains(const char* key) const override;

  void SetNullPropsByID(CSSPropertyID prop_id) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetNullProps(property_name);
  };

  void SetPropsByID(CSSPropertyID prop_id, unsigned int value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID prop_id, int value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID prop_id, const char* value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID prop_id, bool value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID prop_id, double value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetProps(property_name, value);
  };

  void SetPropsByID(CSSPropertyID prop_id, const pub::Value& value) override {
    auto property_name = CSSProperty::GetPropertyNameCStr(prop_id);
    SetProps(property_name, value);
  };

  void ResetEventHandler() override;
  std::unique_ptr<PropBundle> ShallowCopy() override;

  inline NSDictionary* dictionary() { return [propMap copy]; }
  inline NSSet* event_set() { return eventSet; }
  inline NSSet* lepus_event_set() { return lepusEventSet; }
  inline NSSet* gesture_detector_set() { return gestureDetectorSet; }

 private:
  friend class PropBundleCreatorDarwin;
  PropBundleDarwin();
  void AssembleArray(NSMutableArray* array, const pub::Value& value,
                     std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
                     int depth = 0, bool need_handle_null_value = true);
  void AssembleMap(NSMutableDictionary* map, const char* key, const pub::Value& value,
                   std::vector<std::unique_ptr<pub::Value>>* prev_value_vector = nullptr,
                   int depth = 0, bool need_handle_null_value = true);
  NSMutableDictionary* propMap;
  NSMutableSet* eventSet;
  NSMutableSet* lepusEventSet;
  NSMutableSet* gestureDetectorSet;
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_UI_WRAPPER_COMMON_IOS_PROP_BUNDLE_DARWIN_H_
