// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/keyframe_effect.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include "base/include/log/logging.h"
#include "core/animation/animation.h"
#include "core/animation/animation_curve.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {

namespace {

bool ShouldPersistFillStyle(const starlight::AnimationData& data) {
  return data.fill_mode == starlight::AnimationFillModeType::kForwards ||
         data.fill_mode == starlight::AnimationFillModeType::kBoth;
}

gfx::AnimationTimingInput CreateCustomPropertyTimingInput(
    const Animation& animation, const gfx::AnimationData& data,
    gfx::TimingRunState run_state) {
  gfx::AnimationTimingInput input;
  input.animation_data = &data;
  input.duration = fml::TimeDelta::FromMilliseconds(data.duration);
  input.start_time = animation.start_time();
  input.pause_time = animation.pause_time();
  input.total_paused_duration = animation.total_paused_duration();
  input.is_paused = animation.GetState() == Animation::State::kPause &&
                    animation.pause_time() != fml::TimePoint::Min();
  input.run_state = run_state;
  return input;
}

std::tuple<bool, bool> UpdateCustomPropertyAnimationState(
    const Animation& animation, const gfx::AnimationData& data,
    fml::TimePoint monotonic_time, gfx::TimingRunState& run_state) {
  auto update = gfx::UpdateTimingState(
      CreateCustomPropertyTimingInput(animation, data, run_state),
      monotonic_time);
  run_state = update.run_state;
  return {update.start_event_due, update.end_event_due};
}

}  // namespace

KeyframeEffect::KeyframeEffect()
    : gfx_effect_(lynx::gfx::KeyframeEffect::Create()),
      animation_delegate_(nullptr) {}

std::unique_ptr<KeyframeEffect> KeyframeEffect::Create() {
  return std::make_unique<KeyframeEffect>();
}

void KeyframeEffect::SetStartTime(fml::TimePoint& time,
                                  bool reset_effect_state) {
  gfx_effect_->SetStartTime(time);
  if (!reset_effect_state) {
    return;
  }
  custom_property_run_state_ = gfx::TimingRunState::STARTING;
  custom_property_current_iteration_count_ = 0;
}

void KeyframeEffect::SetPauseTime(fml::TimePoint& time) {
  gfx_effect_->SetPauseTime(time);
  custom_property_run_state_ = gfx::TimingRunState::PAUSED;
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

KeyframeEffect::KeyframeSampleResult KeyframeEffect::SampleKeyframeModel(
    fml::TimePoint monotonic_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_EFFECT_SAMPLE_KEYFRAME_MODEL);
  KeyframeSampleResult result;
  auto tick_result = gfx_effect_->Tick(monotonic_time);
  if (keyframe_models_.empty() && has_custom_property_keyframes_ &&
      animation_ != nullptr) {
    auto gfx_animation_data = ToGfxAnimationData(*animation_->animation_data());
    std::tie(tick_result.start_event_due, tick_result.end_event_due) =
        UpdateCustomPropertyAnimationState(*animation_, gfx_animation_data,
                                           monotonic_time,
                                           custom_property_run_state_);
    auto timing_input = CreateCustomPropertyTimingInput(
        *animation_, gfx_animation_data, custom_property_run_state_);
    auto active_time = gfx::CalculateActiveTime(timing_input, monotonic_time);
    if (active_time != fml::TimeDelta::Min()) {
      tick_result.active_time = active_time;
      const int old_iteration_count = custom_property_current_iteration_count_;
      auto trimmed = gfx::TrimTimeToCurrentIteration(
          timing_input, monotonic_time,
          custom_property_current_iteration_count_);
      custom_property_current_iteration_count_ =
          trimmed.current_iteration_count;
      tick_result.iteration_events_due += gfx::CountIterationEventsDue(
          old_iteration_count, custom_property_current_iteration_count_);
    }
  }
  if (animation_ != nullptr && tick_result.active_time) {
    animation_->MaybeReportOverTime(*tick_result.active_time);
  }
  result.styles.reserve(tick_result.samples.size());
  result.should_send_start_event = tick_result.start_event_due;
  result.should_send_end_event = tick_result.end_event_due;
  result.iteration_events_due = tick_result.iteration_events_due;

  for (const auto& sample : tick_result.samples) {
    auto* curve = static_cast<AnimationCurve*>(sample.curve);
    if (curve == nullptr) {
      continue;
    }
    if (animation_delegate_) {
      fml::TimeDelta t = sample.trimmed_time;
      tasm::CSSValue value = curve->GetValue(t);
      animation_delegate_->NotifyClientAnimated(
          result.styles, value,
          static_cast<tasm::CSSPropertyID>(curve->Type()));
    }
  }

  const bool is_transition_effect =
      animation_ != nullptr && animation_->GetTransitionFlag();
  if (!is_transition_effect && tick_result.end_event_due) {
    for (const auto& keyframe_model : keyframe_models_) {
      if (!keyframe_model || !keyframe_model->is_finished() ||
          !keyframe_model->HasAnimationData()) {
        continue;
      }
      if (keyframe_model->ShouldPersistFillStyle()) {
        result.should_persist_fill_styles = true;
      } else {
        result.should_clear_fill_styles = true;
      }
    }
    if (has_custom_property_keyframes_ && animation_ != nullptr &&
        animation_->animation_data() != nullptr) {
      if (ShouldPersistFillStyle(*animation_->animation_data())) {
        result.should_persist_fill_styles = true;
      } else {
        result.should_clear_fill_styles = true;
      }
    }
  }

  return result;
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

bool KeyframeEffect::CheckHasFinished(fml::TimePoint& monotonic_time,
                                      bool clear_effect) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_EFFECT_CHECK_HAS_FINISHED);
  if (!keyframe_models_.empty()) {
    const bool has_finished_all = gfx_effect_->HasFinishedAll();
    if (has_finished_all && clear_effect) {
      ClearEffectIfOutOfEffect(monotonic_time);
    }
    return has_finished_all;
  }
  if (has_custom_property_keyframes_ && animation_ != nullptr) {
    auto gfx_animation_data = ToGfxAnimationData(*animation_->animation_data());
    auto timing_input = CreateCustomPropertyTimingInput(
        *animation_, gfx_animation_data, custom_property_run_state_);
    return gfx::CalculatePhase(timing_input, monotonic_time) ==
           gfx::TimingPhase::AFTER;
  }
  return true;
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

void KeyframeEffect::SampleCustomPropertyKeyframes(
    fml::TimePoint monotonic_time,
    const tasm::CSSKeyframesCustomPropertyContent& keyframes,
    tasm::CustomPropertiesMap& out_styles) const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              KEYFRAME_EFFECT_SAMPLE_CUSTOM_PROPERTY_KEYFRAMES);
  if (animation_ == nullptr || keyframes.empty()) {
    return;
  }
  auto gfx_animation_data = ToGfxAnimationData(*animation_->animation_data());
  auto timing_input = CreateCustomPropertyTimingInput(
      *animation_, gfx_animation_data, custom_property_run_state_);
  if (gfx::CalculateActiveTime(timing_input, monotonic_time) ==
      fml::TimeDelta::Min()) {
    return;
  }
  const auto duration =
      fml::TimeDelta::FromMilliseconds(gfx_animation_data.duration);
  double progress = 1.0;
  if (duration > fml::TimeDelta()) {
    auto trimmed = gfx::TrimTimeToCurrentIteration(
        timing_input, monotonic_time, custom_property_current_iteration_count_);
    progress = std::clamp(
        trimmed.time.ToMillisecondsF() / duration.ToMillisecondsF(), 0.0, 1.0);
  }
  const std::shared_ptr<tasm::CustomPropertiesMap>* selected = nullptr;
  for (const auto& [offset, styles] : keyframes) {
    if (offset <= progress && styles != nullptr) {
      selected = &styles;
    }
  }
  if (selected == nullptr || *selected == nullptr) {
    return;
  }
  for (const auto& [key, value] : **selected) {
    out_styles.insert_or_assign(key, value);
  }
}

}  // namespace animation
}  // namespace lynx
