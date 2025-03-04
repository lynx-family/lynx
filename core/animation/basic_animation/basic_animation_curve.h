// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATION_CURVE_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATION_CURVE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/basic_animation/animation_effect_timing.h"
#include "core/animation/basic_animation/keyframe.h"

namespace lynx {
namespace animation {
namespace basic {
class AnimationEffect;
class AnimationCurve {
 public:
  AnimationCurve() = default;
  ~AnimationCurve() = default;

  AnimationCurve(const std::string& property_value_id,
                 basic::AnimationEffect* effect)
      : property_value_id_(property_value_id), effect_(effect) {}

 public:
  static std::unique_ptr<AnimationCurve> Create(
      const std::string& property_value_id, basic::AnimationEffect* effect);

  void EnsureFromAndToKeyframe();

  void AddKeyframe(std::unique_ptr<Keyframe> keyframe);

  std::unique_ptr<Keyframe> MakeEmptyKeyframe(double offset);

  std::unique_ptr<PropertyValue> GetValue(fml::TimeDelta& t);

  TimingFunction* timing_function() { return timing_function_; }

  void SetTimingFunction(TimingFunction* timing_function) {
    timing_function_ = timing_function;
  }

  const std::string& property_value_id() { return property_value_id_; }

  void SetPropertyValueId(const std::string& property_value_id) {
    property_value_id_ = property_value_id;
  }

  AnimationCurve(const AnimationCurve&) = delete;
  AnimationCurve& operator=(const AnimationCurve&) = delete;

 protected:
  TimingFunction* timing_function_;
  std::vector<std::unique_ptr<basic::Keyframe>> keyframes_;

 private:
  std::string property_value_id_;
  basic::AnimationEffect* effect_;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATION_CURVE_H_
