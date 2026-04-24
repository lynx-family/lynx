// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_KEYFRAME_MODEL_H_
#define CORE_ANIMATION_KEYFRAME_MODEL_H_

#include <cmath>
#include <memory>
#include <string>
#include <tuple>

#include "core/animation/animation_curve.h"
#include "core/renderer/css/css_value.h"
#include "core/style/animation_data.h"
#include "gfx/animation/animation_keyframe_model.h"

namespace lynx {
namespace animation {

class KeyframeEffect;

class KeyframeModel {
 public:
  static std::unique_ptr<KeyframeModel> Create(
      std::unique_ptr<AnimationCurve> curve);

  bool InEffect(fml::TimePoint monotonic_time) const;

  bool is_finished() const { return gfx_model_->is_finished(); }
  AnimationCurve* animation_curve() { return curve(); }

  void UpdateAnimationData(starlight::AnimationData* data);

  void EnsureFromAndToKeyframe();

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdated(tasm::CSSValuePattern);

  bool HasAnimationData() const {
    return gfx_model_->animation_data() != nullptr;
  }
  starlight::AnimationData get_animation_data() const {
    return *animation_data_;
  }
  bool ShouldPersistFillStyle() const;

  explicit KeyframeModel(std::unique_ptr<AnimationCurve> curve);

 private:
  AnimationCurve* curve() {
    return static_cast<AnimationCurve*>(gfx_model_->curve());
  }

  const AnimationCurve* curve() const {
    return static_cast<const AnimationCurve*>(gfx_model_->curve());
  }

  friend class KeyframeEffect;
  std::unique_ptr<lynx::gfx::KeyframeModel> gfx_model_;
  starlight::AnimationData* animation_data_{nullptr};
  gfx::AnimationData gfx_animation_data_;
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_KEYFRAME_MODEL_H_
