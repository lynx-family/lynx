// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_PROP_BUNDLE_IMPL_H_
#define CLAY_LYNX_ADAPTOR_PROP_BUNDLE_IMPL_H_

#include <sys/types.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "clay/public/value.h"
#include "clay/ui/gesture_handler/gesture_detector.h"
#include "core/public/prop_bundle.h"

namespace lynx {

class PropBundleImpl : public tasm::PropBundle {
 public:
  PropBundleImpl() = default;
  ~PropBundleImpl() override;
  void SetNullProps(const char* key) override;
  void SetProps(const char* key, unsigned int value) override;
  void SetProps(const char* key, int value) override;
  void SetProps(const char* key, const char* value) override;
  void SetProps(const char* key, bool value) override;
  void SetProps(const char* key, double value) override;
  void SetProps(const char* key, const pub::Value& value) override;
  void SetProps(const pub::Value& value) override;
  bool Contains(const char* key) const override;
  void SetEventHandler(const pub::Value& event) override;
  void SetGestureDetector(const tasm::GestureDetector& detector) override;
  void ResetEventHandler() override;

  void SetNullPropsByID(tasm::CSSPropertyID id) override;
  void SetPropsByID(tasm::CSSPropertyID id, unsigned int value) override;
  void SetPropsByID(tasm::CSSPropertyID id, int value) override;
  void SetPropsByID(tasm::CSSPropertyID id, const char* value) override;
  void SetPropsByID(tasm::CSSPropertyID id, bool value) override;
  void SetPropsByID(tasm::CSSPropertyID id, double value) override;
  void SetPropsByID(tasm::CSSPropertyID id, const pub::Value& value) override;
  void SetPropsByID(tasm::CSSPropertyID id, const uint8_t* data,
                    size_t size) override;

  void SetPropsByID(tasm::CSSPropertyID id, const uint32_t* data,
                    size_t size) override;
  fml::RefPtr<PropBundle> ShallowCopy() override { return nullptr; }

  const clay::Value::Map& map() const { return map_; }

  clay::Value::Map& mutable_map() { return map_; }

  const std::vector<std::string>& event_handlers() const {
    return event_handlers_;
  }

  const std::optional<clay::GestureMap>& GestureDetectorMap() const {
    return gesture_detector_map_;
  }

 private:
  clay::Value::Map map_;
  std::vector<std::string> event_handlers_;
  std::optional<clay::GestureMap> gesture_detector_map_;
};

class PropBundleCreatorClay : public tasm::PropBundleCreator {
 public:
  fml::RefPtr<tasm::PropBundle> CreatePropBundle() override;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_PROP_BUNDLE_IMPL_H_
