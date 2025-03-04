// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_KEYFRAME_EFFECT_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_KEYFRAME_EFFECT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/basic_animation/animation_effect.h"
#include "core/animation/basic_animation/animation_effect_timing.h"
#include "core/animation/basic_animation/animator_target.h"
#include "core/animation/basic_animation/basic_keyframe_model.h"
#include "core/animation/basic_animation/keyframe.h"
#include "core/animation/utils/timing_function.h"

namespace lynx {
namespace animation {
namespace basic {
class KeyframeEffect : public AnimationEffect {
 public:
  ~KeyframeEffect() override = default;

  static std::unique_ptr<KeyframeEffect> Create(
      std::vector<std::unique_ptr<KeyframeToken>> keyframes,
      const std::shared_ptr<AnimatorTarget>& target) {
    auto effect = std::unique_ptr<KeyframeEffect>(
        new KeyframeEffect(std::move(keyframes), target));
    effect->MakeKeyframeModel();
    return effect;
  }

  static std::unique_ptr<KeyframeEffect> Create(
      std::vector<std::unique_ptr<KeyframeToken>> keyframes,
      const std::shared_ptr<AnimatorTarget>& target,
      std::unique_ptr<AnimationEffectTiming> timing) {
    auto effect = std::unique_ptr<KeyframeEffect>(
        new KeyframeEffect(std::move(keyframes), target, std::move(timing)));
    effect->MakeKeyframeModel();
    return effect;
  }

  static std::unique_ptr<KeyframeEffect> Create(
      std::vector<std::unique_ptr<KeyframeToken>> keyframes,
      const std::shared_ptr<AnimatorTarget>& target,
      std::unique_ptr<OptionalAnimationEffectTiming> timing) {
    auto effect = std::unique_ptr<KeyframeEffect>(
        new KeyframeEffect(std::move(keyframes), target, std::move(timing)));
    effect->MakeKeyframeModel();
    return effect;
  }

  std::unordered_map<std::string, std::unique_ptr<basic::KeyframeModel>>&
  keyframe_models() {
    return keyframe_models_;
  }

  void MakeKeyframeModel();

  void TickKeyframeModel(const fml::TimePoint& monotonic_time) override;

 private:
  KeyframeEffect(std::vector<std::unique_ptr<KeyframeToken>> keyframes,
                 const std::shared_ptr<AnimatorTarget>& target);

  KeyframeEffect(std::vector<std::unique_ptr<KeyframeToken>> keyframes,
                 const std::shared_ptr<AnimatorTarget>& target,
                 std::unique_ptr<AnimationEffectTiming> timing);

  KeyframeEffect(std::vector<std::unique_ptr<KeyframeToken>> keyframes,
                 const std::shared_ptr<AnimatorTarget>& target,
                 std::unique_ptr<OptionalAnimationEffectTiming> timing);

  std::weak_ptr<AnimatorTarget> target_;
  std::vector<std::unique_ptr<KeyframeToken>> keyframes_token_map_;

  Keyframe::PropertyValueMap property_value_map_;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_KEYFRAME_EFFECT_H_
