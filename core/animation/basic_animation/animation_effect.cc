// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/animation_effect.h"

#include "core/animation/basic_animation/basic_keyframe_model.h"

namespace lynx {
namespace animation {
namespace basic {

void AnimationEffect::SetStartTime(const fml::TimePoint &time) {
  for (auto &model : keyframe_models_) {
    model.second->set_start_time(time);
  }
}

void AnimationEffect::SetPauseTime(const fml::TimePoint &time) {
  for (auto &model : keyframe_models_) {
    model.second->SetRunState(KeyframeModel::PAUSED, time);
  }
}

bool AnimationEffect::CheckHasFinished(const fml::TimePoint &monotonic_time) {
  if (!keyframe_models_.empty()) {
    if (keyframe_models_.begin()->second->is_finished() &&
        !keyframe_models_.begin()->second->InEffect(monotonic_time)) {
      ClearEffect();
    }
    return keyframe_models_.begin()->second->is_finished();
  }
  return true;
}

void AnimationEffect::ClearEffect() {
  // TODO(wangyifei.20010605): Will be implemented later.
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
