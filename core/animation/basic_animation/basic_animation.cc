// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/basic_animation.h"

#include "core/animation/basic_animation/animation_event_listener.h"
#include "core/animation/basic_animation/animation_frame_callback_provider.h"
#include "core/animation/basic_animation/thread_local_animation_handler.h"

namespace lynx {
namespace animation {
namespace basic {
void Animation::Play() {
  if (state_ == State::kPlay) {
    return;
  }
  State temp_state = state_;
  state_ = State::kPlay;
  if (temp_state == State::kIdle) {
    DoAnimationFrame(GetAnimationDummyStartTime());
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
  if (need_clear_effect) {
    effect_->ClearEffect();
  }
  if (state_ == State::kPlay || state_ == State::kPause) {
    // send cancel event
    SendAnimationEvent(EventType::Cancel);
  }
  state_ = State::kStop;
}

void Animation::DoAnimationFrame(const fml::TimePoint &frame_time) {
  if (frame_time != fml::TimePoint::Min()) {
    Tick(frame_time);
    if (HasFinishAll(frame_time)) {
      Stop();
    }
  }
  if (state_ == State::kPlay) {
    RequestNextFrame();
  } else if (state_ == State::kPause) {
    effect_->SetPauseTime(frame_time);
  }
}

void Animation::Tick(const fml::TimePoint &time) {
  if (!effect_) {
    return;
  }
  if (start_time_ == fml::TimePoint::Min() ||
      start_time_ == GetAnimationDummyStartTime()) {
    start_time_ = time;
    effect_->SetStartTime(time);
  }
  effect_->TickKeyframeModel(time);
}

bool Animation::HasFinishAll(const fml::TimePoint &time) {
  if (!effect_ || effect_->CheckHasFinished(time)) {
    return true;
  }
  return false;
}

fml::TimePoint &Animation::GetAnimationDummyStartTime() {
  static fml::TimePoint kAnimationDummyStartTime = fml::TimePoint();
  return kAnimationDummyStartTime;
}

void Animation::RequestNextFrame() {
  auto strong_provider = animation_frame_callback_provider_.lock();
  // If the provider exists in the animation, use it with priority. Otherwise,
  // use the global provider.
  if (strong_provider) {
    strong_provider->RequestNextFrame(
        [weak_ptr = weak_from_this()](const fml::TimePoint &frame_time) {
          auto strong_ptr = weak_ptr.lock();
          if (strong_ptr) {
            strong_ptr->DoAnimationFrame(frame_time);
          }
        });
    return;
  }
  ThreadLocalAnimationHandler::GetInstance().AddAnimationFrameCallback(*this);
}

void Animation::SendAnimationEvent(EventType type) {
  auto strong_ptr = listener_.lock();
  if (strong_ptr) {
    strong_ptr->OnAnimationEvent(*this, type);
  } else {
    LOGE("Animation already has been destroyed.");
  }
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
