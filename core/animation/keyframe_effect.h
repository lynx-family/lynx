// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_KEYFRAME_EFFECT_H_
#define CORE_ANIMATION_KEYFRAME_EFFECT_H_

#include <memory>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "core/animation/animation_curve.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/keyframe_model.h"
#include "gfx/animation/animation_keyframe_effect.h"

namespace lynx {

namespace tasm {
class Element;
class CSSKeyframesToken;
}  // namespace tasm

namespace animation {
class Animation;

class KeyframeEffect {
 public:
  KeyframeEffect();
  virtual ~KeyframeEffect() = default;

  gfx::KeyframeEffect::TickResult TickKeyframeModel(
      fml::TimePoint monotonic_time);

  void AddKeyframeModel(std::unique_ptr<KeyframeModel> keyframe_model);

  KeyframeModel* GetKeyframeModelByCurveType(AnimationCurve::CurveType type);

  void SetAnimation(Animation* animation) { animation_ = animation; }

  void SetStartTime(fml::TimePoint& time);

  void SetPauseTime(fml::TimePoint& time);

  static std::unique_ptr<KeyframeEffect> Create();
  void BindAnimationDelegate(AnimationDelegate* target) {
    animation_delegate_ = target;
  }
  void BindElement(tasm::Element* element) { element_ = element; }
  void ClearEffectIfOutOfEffect(fml::TimePoint& time);

  void ClearEffect();

  void UpdateAnimationData(starlight::AnimationData* data);

  void EnsureFromAndToKeyframe();

  bool HasFinishedAll() const;

  Animation* GetAnimation() { return animation_; }

  std::vector<std::unique_ptr<KeyframeModel>>& keyframe_models() {
    return keyframe_models_;
  }

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdated(tasm::CSSValuePattern);

 private:
  friend class Animation;
  void ApplyTickResult(const gfx::KeyframeEffect::TickResult& tick_result);

  tasm::Element* element_{nullptr};
  std::vector<std::unique_ptr<KeyframeModel>> keyframe_models_;
  std::unique_ptr<lynx::gfx::KeyframeEffect> gfx_effect_;
  AnimationDelegate* animation_delegate_;
  Animation* animation_{nullptr};
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_KEYFRAME_EFFECT_H_
