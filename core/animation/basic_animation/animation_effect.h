// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EFFECT_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EFFECT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/basic_animation/animation_effect_timing.h"
#include "core/animation/basic_animation/basic_keyframe_model.h"
#include "core/animation/basic_animation/keyframe.h"

namespace lynx {
namespace animation {
namespace basic {
class Animation;
class KeyframeModel;
class AnimationEffect {
 public:
  virtual ~AnimationEffect() = default;

  const AnimationEffectTiming& timing() const { return *timing_; }

  void UpdateTiming(std::unique_ptr<OptionalAnimationEffectTiming> timing) {
    timing_->UpdateTiming(std::move(timing));
  }

  void SetStartTime(const fml::TimePoint& time);

  void SetPauseTime(const fml::TimePoint& time);

  virtual void TickKeyframeModel(const fml::TimePoint& monotonic_time) = 0;

  void ClearEffect();

  bool CheckHasFinished(const fml::TimePoint& time);

  Animation* HostAnimation() { return animation_; }

  void BindHostAnimation(Animation* animation) { animation_ = animation; }

 protected:
  AnimationEffect() : timing_(AnimationEffectTiming::Create()) {}

  explicit AnimationEffect(std::unique_ptr<AnimationEffectTiming> timing)
      : timing_(std::move(timing)) {}

  explicit AnimationEffect(
      std::unique_ptr<OptionalAnimationEffectTiming> timing)
      : timing_(AnimationEffectTiming::Create(std::move(timing))) {}

  void UpdateAnimationData() {}

 protected:
  // The counter records the current iteration_count of the animation.
  int current_iteration_count_ = 0;

  std::unique_ptr<AnimationEffectTiming> timing_;
  std::unordered_map<std::string, std::unique_ptr<basic::KeyframeModel>>
      keyframe_models_;

 private:
  Animation* animation_{nullptr};
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EFFECT_H_
