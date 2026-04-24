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
#include "core/animation/animation_trace_event_def.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {

KeyframeEffect::KeyframeEffect()
    : gfx_effect_(lynx::gfx::KeyframeEffect::Create()),
      animation_delegate_(nullptr) {}

std::unique_ptr<KeyframeEffect> KeyframeEffect::Create() {
  return std::make_unique<KeyframeEffect>();
}

void KeyframeEffect::SetStartTime(fml::TimePoint& time) {
  gfx_effect_->SetStartTime(time);
}

void KeyframeEffect::SetPauseTime(fml::TimePoint& time) {
  gfx_effect_->SetPauseTime(time);
}

void KeyframeEffect::AddKeyframeModel(
    std::unique_ptr<KeyframeModel> keyframe_model) {
  if (!keyframe_model) {
    return;
  }
  gfx_effect_->AddKeyframeModel(keyframe_model->gfx_model_.get());
  keyframe_models_.push_back(std::move(keyframe_model));
}

gfx::KeyframeEffect::TickResult KeyframeEffect::TickKeyframeModel(
    fml::TimePoint monotonic_time) {
  auto tick_result = gfx_effect_->Tick(monotonic_time);
  if (animation_ != nullptr && tick_result.active_time) {
    animation_->MaybeReportOverTime(*tick_result.active_time);
  }
  ApplyTickResult(tick_result);
  return tick_result;
}

bool KeyframeEffect::HasFinishedAll() const {
  return gfx_effect_->HasFinishedAll();
}

void KeyframeEffect::ApplyTickResult(
    const lynx::gfx::KeyframeEffect::TickResult& tick_result) {
  tasm::StyleMap style_map;
  bool should_persist_fill_styles = false;
  bool should_clear_fill_styles = false;
  const bool is_transition_effect =
      animation_ != nullptr && animation_->GetTransitionFlag();
  style_map.reserve(keyframe_models_.size());

  if (animation_ != nullptr) {
    for (int i = 0; i < tick_result.iteration_events_due; ++i) {
      animation_->SendIterationEvent();
    }
  }

  for (const auto& sample : tick_result.samples) {
    auto* curve = static_cast<AnimationCurve*>(sample.curve);
    if (curve == nullptr) {
      continue;
    }
    if (animation_delegate_) {
      fml::TimeDelta t = sample.trimmed_time;
      tasm::CSSValue value = curve->GetValue(t);
      animation_delegate_->NotifyClientAnimated(
          style_map, value, static_cast<tasm::CSSPropertyID>(curve->Type()));
    }
  }

  if (!is_transition_effect && tick_result.end_event_due) {
    for (const auto& keyframe_model : keyframe_models_) {
      if (!keyframe_model || !keyframe_model->is_finished() ||
          !keyframe_model->HasAnimationData()) {
        continue;
      }
      if (keyframe_model->ShouldPersistFillStyle()) {
        should_persist_fill_styles = true;
      } else {
        should_clear_fill_styles = true;
      }
    }
  }

  if (animation_delegate_ != nullptr && !style_map.empty()) {
    animation_delegate_->UpdateFinalStyleMap(style_map);
  }
  if (!is_transition_effect && should_persist_fill_styles &&
      element_ != nullptr && !style_map.empty()) {
    element_->PersistAnimationFillStyles(style_map);
  }
  if (!is_transition_effect && should_clear_fill_styles &&
      element_ != nullptr && animation_ != nullptr) {
    for (const auto& id : animation_->GetRawStyleSet()) {
      element_->ClearPersistedAnimationFillStyle(id);
    }
  }

  if (animation_) {
    if (tick_result.start_event_due) {
      animation_->SendStartEvent();
      LOGI("Lynx Animation play, name is: " << animation_->name().str());
    }
    if (tick_result.end_event_due) {
      animation_->SendEndEvent();
      LOGI("Lynx Animation end, name is: " << animation_->name().str());
    }
  }
}

void KeyframeEffect::ClearEffectIfOutOfEffect(fml::TimePoint& monotonic_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_EFFECT_CLEAR_IF_OUT_OF_EFFECT);
  KeyframeModel* first_model = nullptr;
  for (const auto& keyframe_model : keyframe_models_) {
    if (keyframe_model) {
      first_model = keyframe_model.get();
      break;
    }
  }
  if (first_model == nullptr) {
    return;
  }
  DCHECK(gfx_effect_->HasFinishedAll());
  if (!first_model->InEffect(monotonic_time)) {
    ClearEffect();
  }
}

void KeyframeEffect::ClearEffect() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_EFFECT_CLEAR_EFFECT);
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
  gfx_effect_->EnsureFromAndToKeyframe();
}

void KeyframeEffect::NotifyElementSizeUpdated() {
  for (auto& keyframe_model : keyframe_models_) {
    if (keyframe_model) {
      keyframe_model->NotifyElementSizeUpdated();
    }
  }
}

void KeyframeEffect::NotifyUnitValuesUpdated(tasm::CSSValuePattern type) {
  for (auto& keyframe_model : keyframe_models_) {
    if (keyframe_model) {
      keyframe_model->NotifyUnitValuesUpdated(type);
    }
  }
}

}  // namespace animation
}  // namespace lynx
