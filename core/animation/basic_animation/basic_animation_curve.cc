// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/basic_animation_curve.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/animation/basic_animation/animation_effect.h"
#include "core/animation/basic_animation/keyframe.h"

namespace lynx {
namespace animation {
namespace basic {

fml::TimeDelta TransformedAnimationTime(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    TimingFunction* timing_function, double scaled_duration,
    fml::TimeDelta time) {
  if (timing_function) {
    fml::TimeDelta start_time = keyframes.front()->Time() * scaled_duration;
    fml::TimeDelta duration =
        (keyframes.back()->Time() - keyframes.front()->Time()) *
        scaled_duration;
    double progress = static_cast<double>(time.ToMicroseconds() -
                                          start_time.ToMicroseconds()) /
                      static_cast<double>(duration.ToMicroseconds());

    time = (duration * timing_function->GetValue(progress)) + start_time;
  }

  return time;
}

size_t GetActiveKeyframe(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time) {
  DCHECK(keyframes.size() >= 2);
  size_t i = 0;
  for (; i < keyframes.size() - 2; ++i) {  // Last keyframe is never active.
    if (time < (keyframes[i + 1]->Time() * scaled_duration)) break;
  }

  return i;
}

double TransformedKeyframeProgress(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time, size_t i) {
  double in_time = time.ToNanosecondsF();
  double time1 = keyframes[i]->Time().ToNanosecondsF() * scaled_duration;
  double time2 = keyframes[i + 1]->Time().ToNanosecondsF() * scaled_duration;

  if (std::fabs(time2 - time1) < std::numeric_limits<double>::epsilon()) {
    return 1.0;
  }
  double progress = (in_time - time1) / (time2 - time1);

  if (keyframes[i]->timing_function()) {
    progress = keyframes[i]->timing_function()->GetValue(progress);
  }

  return progress;
}

std::unique_ptr<AnimationCurve> AnimationCurve::Create(
    const std::string& property_value_id, basic::AnimationEffect* effect) {
  return std::make_unique<AnimationCurve>(property_value_id, effect);
}

void AnimationCurve::EnsureFromAndToKeyframe() {
  static const double kFromTimeOffset = 0.0f;
  static const double kToTimeOffset = 1.0f;
  if (keyframes_.empty() || keyframes_.front()->offset() != kFromTimeOffset) {
    AddKeyframe(MakeEmptyKeyframe(kFromTimeOffset));
  }
  if (keyframes_.empty() || keyframes_.front()->offset() != kToTimeOffset) {
    AddKeyframe(MakeEmptyKeyframe(kToTimeOffset));
  }
}

void AnimationCurve::AddKeyframe(std::unique_ptr<Keyframe> keyframe) {
  if (!keyframes_.empty() && keyframe != nullptr &&
      keyframe->offset() < keyframes_.back()->offset()) {
    for (size_t i = 0; i < keyframes_.size(); ++i) {
      if (keyframe->offset() < keyframes_.at(i)->offset()) {
        keyframes_.insert(keyframes_.begin() + i, std::move(keyframe));
        return;
      }
    }
  }
  keyframes_.push_back(std::move(keyframe));
}

std::unique_ptr<Keyframe> AnimationCurve::MakeEmptyKeyframe(double offset) {
  return std::make_unique<Keyframe>(offset);
}

std::unique_ptr<PropertyValue> AnimationCurve::GetValue(fml::TimeDelta& t) {
  double duration = effect_->timing().duration().ToSecondsF();
  t = TransformedAnimationTime(keyframes_, timing_function_, duration, t);
  size_t i = GetActiveKeyframe(keyframes_, duration, t);
  double progress = TransformedKeyframeProgress(keyframes_, duration, t, i);
  Keyframe* prev_keyframe = keyframes_[i].get();
  Keyframe* next_keyframe = keyframes_[i + 1].get();
  return Keyframe::interpolate(prev_keyframe, next_keyframe, progress);
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
