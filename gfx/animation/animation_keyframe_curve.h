// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_KEYFRAME_CURVE_H_
#define GFX_ANIMATION_ANIMATION_KEYFRAME_CURVE_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/timing_function.h"

namespace lynx {
namespace gfx {

class AnimationCurve {
 public:
  virtual ~AnimationCurve() = default;

  fml::TimeDelta Duration() const {
    if (keyframes_.size() < 2) {
      return fml::TimeDelta();
    }
    return (keyframes_.back()->Time() - keyframes_.front()->Time()) *
           scaled_duration();
  }

  const TimingFunction* timing_function() const {
    return timing_function_.get();
  }
  TimingFunction* timing_function() { return timing_function_.get(); }

  void SetTimingFunction(std::unique_ptr<TimingFunction> timing_function) {
    timing_function_ = std::move(timing_function);
  }

  double scaled_duration() const { return scaled_duration_; }
  void set_scaled_duration(double scaled_duration) {
    scaled_duration_ = scaled_duration;
  }

  size_t get_keyframes_size() const { return keyframes_.size(); }

  void AddKeyframe(std::unique_ptr<Keyframe> keyframe) {
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

  void EnsureFromAndToKeyframe() {
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

  virtual std::unique_ptr<Keyframe> MakeEmptyKeyframe(
      const fml::TimeDelta& offset) = 0;

 protected:
  AnimationCurve() = default;

  std::unique_ptr<TimingFunction> timing_function_;
  double scaled_duration_{1.0};
  std::vector<std::unique_ptr<Keyframe>> keyframes_;
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_KEYFRAME_CURVE_H_
