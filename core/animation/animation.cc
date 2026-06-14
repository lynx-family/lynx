// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation.h"

#include <math.h>

#include <cstdint>
#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/animation/constants.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker.h"

namespace {

constexpr int64_t kThirtyMinutesInSeconds = 1800;

}  // namespace

namespace lynx {
namespace animation {
namespace {

void SuppressSampleSideEffects(KeyframeEffect::KeyframeSampleResult& result) {
  result.should_send_start_event = false;
  result.should_send_end_event = false;
  result.iteration_events_due = 0;
  result.should_persist_fill_styles = false;
  result.should_clear_fill_styles = false;
}

}  // namespace

Animation::Animation(const base::String& name)
    : name_(name), keyframe_effect_(nullptr) {}

void Animation::Play(bool play_handles_initial_frame) {
  if (state_ == State::kPlay) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_PLAY);
  LOGI("Lynx Animation start, name is: " << name_.str());
  State temp_state = state_;
  if (temp_state == State::kIdle || temp_state == State::kStop) {
    ResetPauseTiming();
    ClearSampleHistory();
  } else {
    // Resume keeps the last valid sample history, but drops same-timestamp
    // cache.
    InvalidateSampleCache();
  }
  // Since `DoFrame` may reads and modifies state_, the change of state_ must be
  // completed before DoFrame is executed.
  state_ = State::kPlay;
  // The kIdle flag indicates that the animation has just been created and has
  // never been ticked before. Here we need to use dummy time to tick the
  // animation to ensure the style is correct.

  // This is a tricky code used to solve the UI flickering issue in some cases
  // on iOS. The root cause is that the operation of destroying an old animator
  // and ticking a newly created animator are not within the same UI operation,
  // causing them to take effect in different frames, resulting in flickering.
  // To solve this problem, these two operations need to occur within the same
  // UI operation. A tricky approach is used here, which involves using a dummy
  // time to actively tick the newly created animator. The more reasonable
  // approach is to delay the destruction of the old animator until the next
  // vsync, and then simultaneously perform the operations of destroying the old
  // animator and ticking the newly created animator on the next vsync.

  // TODO(WUJINTIAN): Remove these tricky code and defer the destruction of the
  // animator to the next vsync to solve the aforementioned problem.
  if (temp_state == State::kIdle && play_handles_initial_frame) {
    DoFrame(GetAnimationDummyStartTime());
    if (animation_delegate_) {
      animation_delegate_->FlushAnimatedStyle();
    }
  } else if (play_handles_initial_frame || temp_state != State::kIdle) {
    RequestNextFrame();
  } else {
    // kIdle with external initial-frame handling is used by the new styling
    // pipeline. It samples in the current style pass and schedules later ticks
    // from the outer pipeline after checking for running animations.
  }
}

void Animation::Pause() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_PAUSE);
  LOGI("Lynx Animation pause, name is: " << name_.str());
  if (state_ == State::kPause) {
    return;
  }
  InvalidateSampleCache();
  state_ = State::kPause;
}

void Animation::Stop() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_STOP);
  ClearSampleHistory();
  state_ = State::kStop;
}

void Animation::Destroy(bool need_clear_effect) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_DESTORY);
  ClearSampleHistory();
  ClearTransitionPreviousEndValue();
  if (need_clear_effect) {
    keyframe_effect_->ClearEffect();
  }
  if (state_ == State::kPlay || state_ == State::kPause) {
    SendCancelEvent();
    LOGI("Lynx Animation cancel, name is: " << name_.str());
  }
  state_ = State::kStop;
  if (animation_delegate_) {
    animation_delegate_->FlushAnimatedStyle();
  }
}

void Animation::CreateEventAndSend(const base::String& event) {
  if (element_->event_map().find(event) == element_->event_map().end() &&
      element_->lepus_event_map().find(event) ==
          element_->lepus_event_map().end() &&
      element_->global_bind_event_map().find(event) ==
          element_->global_bind_event_map().end()) {
    return;
  }
  auto dict = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kNewAnimator, "new_animator");
  BASE_STATIC_STRING_DECL(kAnimationType, "animation_type");
  BASE_STATIC_STRING_DECL(kAnimationName, "animation_name");
  dict->SetValue(kNewAnimator, true);
  dict->SetValue(kAnimationType,
                 is_transition_ ? BASE_STATIC_STRING(kTransitionAnimationName)
                                : BASE_STATIC_STRING(kKeyframeAnimationName));
  dict->SetValue(kAnimationName, this->get_animation_data().name);
  element_->element_manager()->SendAnimationEvent(
      event.str(), element_->impl_id(), lepus::Value(std::move(dict)));
}

void Animation::SetKeyframeEffect(
    std::unique_ptr<KeyframeEffect> keyframe_effect) {
  keyframe_effect->SetAnimation(this);
  need_report_over_time_ = true;
  keyframe_effect_ = std::move(keyframe_effect);
}

bool Animation::Tick(fml::TimePoint& time) {
  if (!keyframe_effect_) {
    return true;
  }
  // If start_time_ is uninitialized or is a dummy time, we should update it.
  if (start_time_ == fml::TimePoint::Min() ||
      start_time_ == GetAnimationDummyStartTime()) {
    const bool reset_effect_state = start_time_ == fml::TimePoint::Min();
    start_time_ = time;
    keyframe_effect_->SetStartTime(time, reset_effect_state);
  }
  return keyframe_effect_->TickKeyframeModel(time).has_finished_all;
}

void Animation::MaybeReportOverTime(fml::TimeDelta active_time) {
  if (!need_report_over_time_ ||
      active_time.ToSeconds() <= kThirtyMinutesInSeconds) {
    return;
  }
  need_report_over_time_ = false;
  ReportAnimationOverTime();
}

void Animation::ReportAnimationOverTime() {
  if (!tasm::LynxEnv::GetInstance().EnableAnimationInfoReport()) {
    return;
  }
  tasm::report::EventTracker::OnEvent([css_animation_name =
                                           animation_data_.name.str()](
                                          tasm::report::MoveOnlyEvent& event) {
    event.SetName("lynxsdk_animation_report_event");
    event.SetProps("report_event_type", "css_animation_time_out_30_minutes");
    event.SetProps("css_animation_name", css_animation_name);
    event.SetProps("total_duration_seconds", kThirtyMinutesInSeconds);
  });
}

void Animation::BindDelegate(AnimationDelegate* target) {
  animation_delegate_ = target;
}

void Animation::RequestNextFrame() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_REQUEST_NEXT_FRAME);
  if (animation_delegate_) {
    animation_delegate_->RequestNextFrame(
        std::weak_ptr<Animation>(shared_from_this()));
  }
}

void Animation::DoFrame(fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ANIMATION_DOFRAME,
              [this](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("animationName");
                curveTypeInfo->set_string_value(name_.str());
              });
  if (frame_time != fml::TimePoint::Min()) {
    if (Tick(frame_time)) {
      Stop();
      if (keyframe_effect_) {
        keyframe_effect_->ClearEffectIfOutOfEffect(frame_time);
      }
      ClearTransitionPreviousEndValue();
    }

    if (state_ == State::kPlay) {
      RequestNextFrame();
    } else if (state_ == State::kPause) {
      keyframe_effect_->SetPauseTime(frame_time);
    }
  }
}

KeyframeEffect::KeyframeSampleResult Animation::SampleAt(
    fml::TimePoint& frame_time) {
  KeyframeEffect::KeyframeSampleResult result;
  // Invalid time and missing effect produce no sampled style changes.
  if (frame_time == fml::TimePoint::Min() || !keyframe_effect_) {
    return result;
  }
  if (state_ == State::kStop) {
    // A just-finished animation may be sampled repeatedly at the same timestamp
    // in one pipeline pass. Keep style output stable, but do not replay events
    // or fill side effects.
    if (has_cached_sample_ && cached_sample_time_ == frame_time) {
      result = cached_sample_result_;
      SuppressSampleSideEffects(result);
    }
    return result;
  }

  if (state_ == State::kPause && frame_time == GetAnimationDummyStartTime() &&
      !has_last_sample_) {
    // Dummy time is only a style recalculation placeholder. Without a previous
    // real sample to freeze, it must not initialize pause timing or advance
    // timeline state.
    return result;
  }

  // Resume from pause by excluding the paused interval from active time.
  if (state_ == State::kPlay && was_paused_) {
    total_paused_duration_ =
        total_paused_duration_ + (frame_time - pause_time_);
    was_paused_ = false;
  }

  // Convert an unset or dummy start time into the timestamp used for sampling.
  // Replacing a dummy start time should not restart effect state, because dummy
  // time only primes the initial style before the first real frame arrives.
  if (start_time_ == fml::TimePoint::Min() ||
      start_time_ == GetAnimationDummyStartTime()) {
    const bool reset_effect_state = start_time_ == fml::TimePoint::Min();
    start_time_ = frame_time;
    keyframe_effect_->SetStartTime(frame_time, reset_effect_state);
  }

  // Resolve the timestamp that should be sampled. Paused animations keep
  // sampling at pause_time_ so repeated resolves return a frozen style.
  fml::TimePoint sample_time = frame_time;
  if (state_ == State::kPause) {
    // The new styling pipeline may resolve a paused animation with dummy time
    // during style recalculation. Reuse the last real sample instead of letting
    // dummy time move the timeline or replay one-shot effects.
    if (frame_time == GetAnimationDummyStartTime() && has_last_sample_) {
      pause_time_ = last_sample_time_;
      was_paused_ = true;
      keyframe_effect_->SetPauseTime(pause_time_);
      result = last_sample_result_;
      SuppressSampleSideEffects(result);
      has_cached_sample_ = true;
      cached_sample_time_ = pause_time_;
      cached_sample_result_ = result;
      return result;
    }
    if (!was_paused_ || pause_time_ == fml::TimePoint::Min()) {
      pause_time_ = frame_time;
      was_paused_ = true;
    }
    sample_time = pause_time_;
    keyframe_effect_->SetPauseTime(sample_time);
  }

  if (has_cached_sample_ && cached_sample_time_ == sample_time) {
    // Repeated sampling at the same timestamp must not advance timeline state
    // or re-emit one-shot events. Reuse the sampled styles but clear event
    // and fill side-effect bits.
    result = cached_sample_result_;
    SuppressSampleSideEffects(result);
    return result;
  }

  // Perform a fresh sample. The result is saved for repeated sampling at this
  // timestamp and, while history is kept, for future paused dummy-time
  // resolves.
  result = keyframe_effect_->SampleKeyframeModel(sample_time);
  has_last_sample_ = true;
  last_sample_time_ = sample_time;
  last_sample_result_ = result;

  // Keep lifecycle side effects consistent with the sampled state.
  if (state_ == State::kPause) {
    keyframe_effect_->SetPauseTime(pause_time_);
  } else if (keyframe_effect_->CheckHasFinished(sample_time, false)) {
    Stop();
    ClearTransitionPreviousEndValue();
  }

  has_cached_sample_ = true;
  cached_sample_time_ = sample_time;
  cached_sample_result_ = result;
  return result;
}

void Animation::UpdateAnimationData(starlight::AnimationData& data) {
  InvalidateSampleCache();
  animation_data_ = data;
  if (keyframe_effect_) {
    keyframe_effect_->UpdateAnimationData(&animation_data_);
  }
}

void Animation::NotifyElementSizeUpdated() {
  InvalidateSampleCache();
  if (keyframe_effect_) {
    keyframe_effect_->NotifyElementSizeUpdated();
  }
}

void Animation::NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern type) {
  InvalidateSampleCache();
  if (keyframe_effect_) {
    keyframe_effect_->NotifyUnitValuesUpdated(type);
  }
}

void Animation::ResetPauseTiming() {
  pause_time_ = fml::TimePoint::Min();
  total_paused_duration_ = fml::TimeDelta::Zero();
  was_paused_ = false;
}

void Animation::InvalidateSampleCache() {
  has_cached_sample_ = false;
  cached_sample_time_ = fml::TimePoint::Min();
  cached_sample_result_ = KeyframeEffect::KeyframeSampleResult();
}

void Animation::ClearSampleHistory() {
  InvalidateSampleCache();
  has_last_sample_ = false;
  last_sample_time_ = fml::TimePoint::Min();
  last_sample_result_ = KeyframeEffect::KeyframeSampleResult();
}

fml::TimePoint& Animation::GetAnimationDummyStartTime() {
  static fml::TimePoint kAnimationDummyStartTime = fml::TimePoint();
  return kAnimationDummyStartTime;
}

void Animation::ClearTransitionPreviousEndValue() {
  if (is_transition_) {
    element_->ClearTransitionPreviousEndValue(name());
  }
}

void Animation::SendStartEvent() {
  CreateEventAndSend(is_transition_
                         ? BASE_STATIC_STRING(kTransitionStartEventName)
                         : BASE_STATIC_STRING(kKeyframeStartEventName));
}

void Animation::SendEndEvent() {
  CreateEventAndSend(is_transition_
                         ? BASE_STATIC_STRING(kTransitionEndEventName)
                         : BASE_STATIC_STRING(kKeyframeEndEventName));
}

void Animation::SendCancelEvent() {
  CreateEventAndSend(is_transition_
                         ? BASE_STATIC_STRING(kTransitionCancelEventName)
                         : BASE_STATIC_STRING(kKeyframeCancelEventName));
}

void Animation::SendIterationEvent() {
  CreateEventAndSend(BASE_STATIC_STRING(kKeyframeIterationEventName));
}

}  // namespace animation
}  // namespace lynx
