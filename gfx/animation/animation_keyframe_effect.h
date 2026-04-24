// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_KEYFRAME_EFFECT_H_
#define GFX_ANIMATION_ANIMATION_KEYFRAME_EFFECT_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "gfx/animation/animation_keyframe_model.h"

namespace lynx {
namespace animation {
class KeyframeEffect;
}
namespace gfx {

class KeyframeEffect {
 public:
  struct Sample {
    AnimationCurve* curve{nullptr};
    fml::TimeDelta trimmed_time;
  };

  struct TickResult {
    bool start_event_due{false};
    bool end_event_due{false};
    int iteration_events_due{0};
    bool has_finished_all{false};
    std::optional<fml::TimeDelta> active_time;
    std::vector<Sample> samples;
  };

  static std::unique_ptr<KeyframeEffect> Create();

  KeyframeEffect();
  ~KeyframeEffect() = default;

  KeyframeEffect(const KeyframeEffect&) = delete;
  KeyframeEffect& operator=(const KeyframeEffect&) = delete;

  void AddKeyframeModel(KeyframeModel* model);

  void SetStartTime(fml::TimePoint& time);
  void SetPauseTime(fml::TimePoint& time);

  void EnsureFromAndToKeyframe();

  TickResult Tick(fml::TimePoint monotonic_time);

 private:
  friend class lynx::animation::KeyframeEffect;

  bool HasFinishedAll() const;

  std::vector<KeyframeModel*> models_;
  int current_iteration_count_{0};
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_KEYFRAME_EFFECT_H_
