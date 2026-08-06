// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image_player.h"

#include <utility>

namespace clay {

AnimatedImagePlayer::AnimatedImagePlayer(
    std::unique_ptr<PlatformImageAnimation> animation,
    fml::RefPtr<fml::TaskRunner> task_runner,
    std::function<void()> frame_changed_callback,
    std::function<bool()> visible_callback)
    : animation_(std::move(animation)),
      frame_timer_(std::make_unique<fml::OneshotTimer>(task_runner)),
      frame_changed_callback_(std::move(frame_changed_callback)),
      visible_callback_(std::move(visible_callback)) {}

AnimatedImagePlayer::~AnimatedImagePlayer() {
  frame_timer_->Stop();
  if (animation_) {
    animation_->StopAnimation();
  }
}

void AnimatedImagePlayer::SetAutoPlay(bool auto_play) {
  if (!animation_ || auto_play == is_playing_) {
    return;
  }
  if (auto_play) {
    StartAnimation();
  } else {
    StopAnimation();
  }
}

void AnimatedImagePlayer::SetLoopCount(int loop_count) {
  if (animation_) {
    animation_->SetLoopCount(loop_count);
  }
}

void AnimatedImagePlayer::StartAnimation() {
  if (!animation_) {
    return;
  }
  frame_timer_->Stop();
  is_timer_running_ = false;
  animation_->StopAnimation();
  animation_->StartAnimation();
  is_playing_ = true;
  NotifyFrameChanged();
}

void AnimatedImagePlayer::StopAnimation() {
  if (animation_) {
    animation_->StopAnimation();
  }
  frame_timer_->Stop();
  is_playing_ = false;
  is_timer_running_ = false;
}

void AnimatedImagePlayer::PauseAnimation() {
  if (animation_) {
    animation_->PauseAnimation();
  }
  frame_timer_->Stop();
  is_playing_ = false;
  is_timer_running_ = false;
}

void AnimatedImagePlayer::ResumeAnimation() {
  if (!animation_) {
    return;
  }
  animation_->ResumeAnimation();
  is_playing_ = true;
  NotifyFrameChanged();
}

void AnimatedImagePlayer::EnsureAnimationScheduled() {
  if (is_playing_ && !is_timer_running_) {
    NotifyFrameChanged();
  }
}

void AnimatedImagePlayer::NotifyFrameChanged() {
  if (!animation_ || !is_playing_) {
    return;
  }
  if (frame_changed_callback_) {
    frame_changed_callback_();
  }
  if (visible_callback_ && !visible_callback_()) {
    frame_timer_->Stop();
    is_timer_running_ = false;
    return;
  }
  StartNextFrameTimer();
}

void AnimatedImagePlayer::StartNextFrameTimer() {
  if (!animation_ || !is_playing_) {
    is_timer_running_ = false;
    return;
  }
  int64_t duration = animation_->GetDuration();
  if (duration <= 0) {
    is_timer_running_ = false;
    return;
  }
  is_timer_running_ = true;
  frame_timer_->Start(fml::TimeDelta::FromMilliseconds(duration), [this] {
    is_timer_running_ = false;
    if (!animation_ || !is_playing_) {
      return;
    }
    if (animation_->DrawFrame()) {
      NotifyFrameChanged();
    } else {
      is_playing_ = false;
    }
  });
}

}  // namespace clay
