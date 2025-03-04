// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/animation.h"

#include <math.h>

#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/animation/constants.h"
#include "core/shell/common/vsync_monitor.h"

namespace lynx {
namespace animation {
Animation::Animation(const std::string& name)
    : name_(name), keyframe_effect_(nullptr) {}

void Animation::Play() {
  if (state_ == State::kPlay) {
    return;
  }
  // Since `DoFrame` may reads and modifies state_, the change of state_ must be
  // completed before DoFrame is executed.
  State temp_state = state_;
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
  if (temp_state == State::kIdle) {
    DoFrame(GetAnimationDummyStartTime());
    if (animation_delegate_) {
      animation_delegate_->FlushAnimatedStyle();
    }
  } else {
    RequestNextFrame();
  }
}

void Animation::Pause() {
  if (state_ == State::kPause) {
    return;
  }
  state_ = State::kPause;
}

void Animation::Stop() { state_ = State::kStop; }

void Animation::Destroy(bool need_clear_effect) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Animation::Destroy");
  if (need_clear_effect) {
    keyframe_effect_->ClearEffect();
  }
  if (state_ == State::kPlay || state_ == State::kPause) {
    SendCancelEvent();
    LOGI("Animation cancel, name is: " << name_);
  }
  state_ = State::kStop;
  if (animation_delegate_) {
    animation_delegate_->FlushAnimatedStyle();
  }
}

void Animation::CreateEventAndSend(const char* event) {
  auto dict = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kNewAnimator, "new_animator");
  BASE_STATIC_STRING_DECL(kAnimationType, "animation_type");
  BASE_STATIC_STRING_DECL(kAnimationName, "animation_name");
  dict->SetValue(kNewAnimator, true);
  dict->SetValue(kAnimationType,
                 is_transition_ ? BASE_STATIC_STRING(kTransitionAnimationName)
                                : BASE_STATIC_STRING(kKeyframeAnimationName));
  dict->SetValue(kAnimationName, this->animation_data()->name);
  element_->element_manager()->SendAnimationEvent(
      event, element_->impl_id(), lepus::Value(std::move(dict)));
}

void Animation::SetKeyframeEffect(
    std::unique_ptr<KeyframeEffect> keyframe_effect) {
  keyframe_effect->SetAnimation(this);
  keyframe_effect_ = std::move(keyframe_effect);
}

void Animation::Tick(fml::TimePoint& time) {
  if (!keyframe_effect_) {
    return;
  }

  // If start_time_ is uninitialized or is a dummy time, we should update it.
  if (start_time_ == fml::TimePoint::Min() ||
      start_time_ == GetAnimationDummyStartTime()) {
    start_time_ = time;
    keyframe_effect_->SetStartTime(time);
  }

  keyframe_effect_->TickKeyframeModel(time);
}

void Animation::BindDelegate(AnimationDelegate* target) {
  animation_delegate_ = target;
}

bool Animation::HasFinishedAll(fml::TimePoint& time) {
  if (!keyframe_effect_ || keyframe_effect_->CheckHasFinished(time)) {
    return true;
  }
  return false;
}

void Animation::RequestNextFrame() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Animation::RequestNextFrame");
  if (animation_delegate_) {
    animation_delegate_->RequestNextFrame(
        std::weak_ptr<Animation>(shared_from_this()));
  }
}

void Animation::DoFrame(fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Animation::DoFrame",
              [this](lynx::perfetto::EventContext ctx) {
                auto* curveTypeInfo = ctx.event()->add_debug_annotations();
                curveTypeInfo->set_name("animationName");
                curveTypeInfo->set_string_value(name_);
              });
  if (frame_time != fml::TimePoint::Min()) {
    Tick(frame_time);
    if (HasFinishedAll(frame_time)) {
      LOGI("[animation] all keyframe effect has finished!");
      Stop();
    }
  }

  if (state_ == State::kPlay) {
    RequestNextFrame();
  } else if (state_ == State::kPause) {
    keyframe_effect_->SetPauseTime(frame_time);
  }
}

void Animation::UpdateAnimationData(starlight::AnimationData& data) {
  animation_data_ = data;
  if (keyframe_effect_) {
    keyframe_effect_->UpdateAnimationData(&animation_data_);
  }
}

void Animation::NotifyElementSizeUpdated() {
  if (keyframe_effect_) {
    keyframe_effect_->NotifyElementSizeUpdated();
  }
}

void Animation::NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern type) {
  if (keyframe_effect_) {
    keyframe_effect_->NotifyUnitValuesUpdatedToAnimation(type);
  }
}

fml::TimePoint& Animation::GetAnimationDummyStartTime() {
  static fml::TimePoint kAnimationDummyStartTime = fml::TimePoint();
  return kAnimationDummyStartTime;
}

void Animation::SendStartEvent() {
  CreateEventAndSend(is_transition_ ? kTransitionStartEventName
                                    : kKeyframeStartEventName);
}

void Animation::SendEndEvent() {
  CreateEventAndSend(is_transition_ ? kTransitionEndEventName
                                    : kKeyframeEndEventName);
}

void Animation::SendCancelEvent() {
  CreateEventAndSend(is_transition_ ? kTransitionCancelEventName
                                    : kKeyframeCancelEventName);
}

void Animation::SendIterationEvent() {
  CreateEventAndSend(kKeyframeIterationEventName);
}

}  // namespace animation
}  // namespace lynx
