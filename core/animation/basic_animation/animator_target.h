// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATOR_TARGET_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATOR_TARGET_H_

#include <memory>
#include <string>

#include "core/animation/basic_animation/keyframe.h"
#include "core/animation/basic_animation/property_value.h"

namespace lynx {
namespace animation {
namespace basic {

class AnimatorTarget : public std::enable_shared_from_this<AnimatorTarget> {
 public:
  virtual ~AnimatorTarget() = default;

  virtual void UpdateAnimatedStyle(
      const Keyframe::PropertyValueMap& styles) = 0;

  // This interface is used to retrieve the value of a certain property from a
  // target. In the W3C standard, if a property's keyframes at the 0% and 100%
  // stages aren't specified, the current computed value of the property on the
  // target is used to construct the keyframes for the 0% and 100% stages.
  virtual std::unique_ptr<PropertyValue> GetStyle(
      const std::string& property_name) = 0;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATOR_TARGET_H_
