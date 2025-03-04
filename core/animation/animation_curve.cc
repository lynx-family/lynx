// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation_curve.h"

#include "core/animation/keyframed_animation_curve.h"

namespace lynx {
namespace animation {

void AnimationCurve::NotifyElementSizeUpdated() {
  for (auto& keyframe : keyframes_) {
    if (keyframe) {
      keyframe->NotifyElementSizeUpdated();
    }
  }
}

void AnimationCurve::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern type) {
  for (auto& keyframe : keyframes_) {
    if (keyframe) {
      keyframe->NotifyUnitValuesUpdatedToAnimation(type);
    }
  }
}

fml::TimeDelta AnimationCurve::Duration() const {
  return (keyframes_.back()->Time() - keyframes_.front()->Time()) *
         scaled_duration();
}

void AnimationCurve::AddKeyframe(std::unique_ptr<Keyframe> keyframe) {
  // Usually, the keyframes will be added in order, so this loop would be
  // unnecessary and we should skip it if possible.
  if (!keyframes_.empty() && keyframe != nullptr &&
      keyframe->Time() < keyframes_.back()->Time()) {
    for (size_t i = 0; i < keyframes_.size(); ++i) {
      if (keyframe->Time() < keyframes_.at(i)->Time()) {
        keyframes_.insert(keyframes_.begin() + i, std::move(keyframe));
        return;
      }
    }
  }

  keyframes_.push_back(std::move(keyframe));
}

// There may be no from(0%) and to(100%) keyframe. If so, we add a empty one.
void AnimationCurve::EnsureFromAndToKeyframe() {
  static const fml::TimeDelta kFromTimeOffset =
      fml::TimeDelta::FromSecondsF(0.0f);
  static const fml::TimeDelta kToTimeOffset =
      fml::TimeDelta::FromSecondsF(1.0f);
  if (keyframes_.empty() || (keyframes_.front()->Time() != kFromTimeOffset)) {
    AddKeyframe(MakeEmptyKeyframe(kFromTimeOffset));
  }
  if (keyframes_.empty() || (keyframes_.back()->Time() != kToTimeOffset)) {
    AddKeyframe(MakeEmptyKeyframe(kToTimeOffset));
  }
}

std::unique_ptr<Keyframe> LayoutAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return LayoutKeyframe::Create(offset, nullptr);
}

std::unique_ptr<Keyframe> OpacityAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return OpacityKeyframe::Create(offset, nullptr);
}

std::unique_ptr<Keyframe> ColorAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return ColorKeyframe::Create(offset, nullptr);
}

std::unique_ptr<Keyframe> FloatAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return FloatKeyframe::Create(offset, nullptr);
}

std::unique_ptr<Keyframe> FilterAnimationCurve::MakeEmptyKeyframe(
    const fml::TimeDelta& offset) {
  return FilterKeyframe::Create(offset, nullptr);
}

}  // namespace animation
}  // namespace lynx
