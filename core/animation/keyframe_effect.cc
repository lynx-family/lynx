// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/keyframe_effect.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/animation/animation.h"
#include "core/animation/animation_curve.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {

KeyframeEffect::KeyframeEffect() : animation_delegate_(nullptr) {}

std::unique_ptr<KeyframeEffect> KeyframeEffect::Create() {
  return std::make_unique<KeyframeEffect>();
}

void KeyframeEffect::SetStartTime(fml::TimePoint& time) {
  for (auto& keyframe_model : keyframe_models_) {
    keyframe_model->set_start_time(time);
  }
}

void KeyframeEffect::SetPauseTime(fml::TimePoint& time) {
  for (auto& keyframe_model : keyframe_models_) {
    keyframe_model->SetRunState(KeyframeModel::PAUSED, time);
  }
}

void KeyframeEffect::AddKeyframeModel(
    std::unique_ptr<KeyframeModel> keyframe_model) {
  keyframe_models_.push_back(std::move(keyframe_model));
}

void KeyframeEffect::TickKeyframeModel(fml::TimePoint monotonic_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframeEffect::TickKeyframeModel");
  // Collect animated style of this animation
  tasm::StyleMap style_map;
  bool should_send_start_event = false;
  bool should_send_end_event = false;

  style_map.reserve(keyframe_models_.size());
  for (auto& keyframe_model : keyframe_models_) {
    // #1. Update the model state and collect animation event information.
    std::tie(should_send_start_event, should_send_end_event) =
        keyframe_model->UpdateState(monotonic_time);

    // #2. Collect animation styles
    if (!keyframe_model->InEffect(monotonic_time)) {
      continue;
    }
    AnimationCurve* curve = keyframe_model->curve();
    // The counter records whether the iteration_count has changed.
    int temp_count = current_iteration_count_;
    // #2.1 Calculate trimmed time to current iteration
    fml::TimeDelta trimmed = keyframe_model->TrimTimeToCurrentIteration(
        monotonic_time, current_iteration_count_);
    if (current_iteration_count_ != temp_count) {
      animation_->SendIterationEvent();
    }

    // #2.2 Calculate animation styles according to trimmed time.
    if (animation_delegate_) {
      tasm::CSSValue value = curve->GetValue(trimmed);
      animation_delegate_->NotifyClientAnimated(
          style_map, value, static_cast<tasm::CSSPropertyID>(curve->Type()));
    }
  }
  // #3. Flush all animation styles to element.
  if (animation_delegate_ != nullptr && !style_map.empty()) {
    animation_delegate_->UpdateFinalStyleMap(style_map);
  }

  // #4. Send animation event.
  if (animation_) {
    if (should_send_start_event) {
      animation_->SendStartEvent();
      LOGI("Animation start, name is: " << animation_->name());
    }
    if (should_send_end_event) {
      animation_->SendEndEvent();
      LOGI("Animation end, name is: " << animation_->name());
    }
  }
}

bool KeyframeEffect::CheckHasFinished(fml::TimePoint& monotonic_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframeEffect::CheckHasFinished");
  // As all keyframe models share the same animation parameters, once one of
  // them finishes, all others will also finish. Therefore, here we only need to
  // check if the first keyframe model has finished.
  if (!keyframe_models_.empty()) {
    if (keyframe_models_[0]->is_finished() &&
        !keyframe_models_[0]->InEffect(monotonic_time)) {
      ClearEffect();
    }
    return keyframe_models_[0]->is_finished();
  }
  return true;
}

void KeyframeEffect::ClearEffect() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "KeyframeEffect::ClearEffect");
  if (animation_delegate_) {
    animation_delegate_->SetNeedsAnimationStyleRecalc(animation_->name());
  }
}

KeyframeModel* KeyframeEffect::GetKeyframeModelByCurveType(
    AnimationCurve::CurveType type) {
  for (auto& keyframe_model : keyframe_models_) {
    if (keyframe_model->animation_curve()->Type() == type) {
      return keyframe_model.get();
    }
  }
  return nullptr;
}

void KeyframeEffect::UpdateAnimationData(starlight::AnimationData* data) {
  for (auto& keyframe_model : keyframe_models_) {
    if (keyframe_model) {
      keyframe_model->UpdateAnimationData(data);
    }
  }
}

void KeyframeEffect::EnsureFromAndToKeyframe() {
  for (auto& keyframe_model : keyframe_models_) {
    keyframe_model->EnsureFromAndToKeyframe();
  }
}

void KeyframeEffect::NotifyElementSizeUpdated() {
  for (auto& keyframe_model : keyframe_models_) {
    if (keyframe_model) {
      keyframe_model->NotifyElementSizeUpdated();
    }
  }
}

void KeyframeEffect::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern type) {
  for (auto& keyframe_model : keyframe_models_) {
    if (keyframe_model) {
      keyframe_model->NotifyUnitValuesUpdatedToAnimation(type);
    }
  }
}

}  // namespace animation
}  // namespace lynx
