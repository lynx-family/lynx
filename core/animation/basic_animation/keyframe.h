// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_KEYFRAME_H_
#define CORE_ANIMATION_BASIC_ANIMATION_KEYFRAME_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/time/time_point.h"
#include "core/animation/basic_animation/property_value.h"
#include "core/animation/utils/timing_function.h"
namespace lynx {
namespace animation {
namespace basic {
class Keyframe {
 public:
  using PropertyValueMap =
      std::unordered_map<std::string, std::unique_ptr<PropertyValue>>;

  Keyframe(double offset) : offset_(offset) {}
  virtual ~Keyframe() = default;

  void AddPropertyValue(std::unique_ptr<PropertyValue> property_value) {
    property_value_ = std::move(property_value);
  }

  std::optional<double>& offset() { return offset_; }
  void set_offset(const std::optional<double>& offset) { offset_ = offset; }
  const fml::TimeDelta Time() {
    return fml::TimeDelta::FromSecondsF(offset().value_or(0.0));
  }
  const TimingFunction& easing() const { return *easing_; }
  TimingFunction* timing_function() const { return easing_; }

  void set_easing(TimingFunction* easing) { easing_ = easing; }

  static std::unique_ptr<PropertyValue> interpolate(Keyframe* prev_keyframe,
                                                    Keyframe* next_keyframe,
                                                    double progress) {
    return prev_keyframe->property_value_->Interpolate(
        progress, next_keyframe->property_value_);
  };

 protected:
  std::optional<double> offset_;
  TimingFunction* easing_ = nullptr;
  std::unique_ptr<PropertyValue> property_value_;
};

class KeyframeToken : public Keyframe {
 public:
  explicit KeyframeToken(double offset) : Keyframe(offset) {}

  ~KeyframeToken() override = default;

  bool AffectsProperty(const std::string& name) {
    return property_values_->find(name) != property_values_->end();
  }

  void AddPropertyValueForToken(const std::string& property_name,
                                std::unique_ptr<PropertyValue> property_value) {
    if (!property_values_) {
      property_values_ = std::make_shared<PropertyValueMap>();
    }
    property_values_->insert_or_assign(property_name,
                                       std::move(property_value));
  }

  std::shared_ptr<PropertyValueMap> property_values() {
    return property_values_;
  }

 private:
  std::shared_ptr<PropertyValueMap> property_values_;
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_KEYFRAME_H_
