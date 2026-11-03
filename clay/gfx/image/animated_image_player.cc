// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/animated_image_player.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace clay {

AnimatedImagePlayer::AnimatedImagePlayer(
    std::unique_ptr<PlatformImageAnimation> animation,
    fml::WeakPtr<AnimationHandler> animation_handler,
    std::function<void()> frame_changed_callback,
    std::function<bool()> visible_callback, ImageAnimationListener* listener)
    : animation_(std::move(animation)),
      animation_handler_(std::move(animation_handler)),
      frame_changed_callback_(std::move(frame_changed_callback)),
      visible_callback_(std::move(visible_callback)),
      listener_(listener) {
  if (!animation_) {
    return;
  }
  int64_t end_time = 0;
  const int frame_count = animation_->GetFrameCount();
  for (int i = 0; i < frame_count; ++i) {
    // Normalize invalid delays here so every platform uses the same timeline.
    const int64_t duration =
        std::max<int64_t>(1, animation_->GetFrameDuration(i));
    if (duration > std::numeric_limits<int64_t>::max() - end_time) {
      frame_end_times_.clear();
      return;
    }
    end_time += duration;
    frame_end_times_.push_back(end_time);
  }
}

AnimatedImagePlayer::~AnimatedImagePlayer() { UnregisterFrameCallback(); }

void AnimatedImagePlayer::SetAutoPlay(bool auto_play) {
  if (auto_play == is_playing_) {
    return;
  }
  if (auto_play) {
    StartAnimation();
  } else {
    StopAnimation();
  }
}

void AnimatedImagePlayer::SetLoopCount(int loop_count) {
  loop_count_ = std::max(0, loop_count);
  ++playback_generation_;
}

void AnimatedImagePlayer::StartAnimation() {
  if (!IsValid()) {
    return;
  }
  UnregisterFrameCallback();
  const uint64_t generation = ++playback_generation_;
  anchor_frame_time_.reset();
  anchor_position_ = 0;
  position_ = 0;
  completed_loops_ = 0;
  current_frame_index_ = 0;
  is_playing_ = true;
  is_paused_ = false;
  auto weak = weak_factory_.GetWeakPtr();
  if (listener_) {
    listener_->OnStartPlay();
  }
  if (!weak || generation != playback_generation_) {
    return;
  }
  NotifyFrameChanged();
  if (weak && generation == playback_generation_) {
    RegisterFrameCallback();
  }
}

void AnimatedImagePlayer::StopAnimation() {
  ++playback_generation_;
  is_playing_ = false;
  is_paused_ = false;
  UnregisterFrameCallback();
}

void AnimatedImagePlayer::PauseAnimation() {
  if (!is_playing_) {
    return;
  }
  ++playback_generation_;
  anchor_position_ = position_;
  anchor_frame_time_.reset();
  is_playing_ = false;
  is_paused_ = true;
  UnregisterFrameCallback();
}

void AnimatedImagePlayer::ResumeAnimation() {
  if (!IsValid() || !is_paused_) {
    return;
  }
  const uint64_t generation = ++playback_generation_;
  is_playing_ = true;
  is_paused_ = false;
  auto weak = weak_factory_.GetWeakPtr();
  if (listener_) {
    listener_->OnStartPlay();
  }
  if (weak && generation == playback_generation_) {
    RegisterFrameCallback();
  }
}

bool AnimatedImagePlayer::DoAnimationFrame(int64_t frame_time,
                                           bool update_values) {
  if (!is_playing_ || !IsValid()) {
    return true;
  }
  if (!update_values || frame_time < 0) {
    return false;
  }
  if (!anchor_frame_time_) {
    anchor_frame_time_ = frame_time;
  }
  const int64_t delta = frame_time - *anchor_frame_time_;
  // Saturate forward jumps and clamp seeks before the playback origin.
  position_ = delta > std::numeric_limits<int64_t>::max() - anchor_position_
                  ? std::numeric_limits<int64_t>::max()
                  : std::max<int64_t>(0, anchor_position_ + delta);
  const int64_t duration = frame_end_times_.back();
  const int64_t loops = position_ / duration;
  const bool finished = loop_count_ > 0 && loops >= loop_count_;
  const int64_t completed = finished ? loop_count_ : loops;
  const bool loop_complete = completed > completed_loops_;
  completed_loops_ = std::max(completed_loops_, completed);
  const int frame_index =
      finished
          ? static_cast<int>(frame_end_times_.size()) - 1
          : std::upper_bound(frame_end_times_.begin(), frame_end_times_.end(),
                             position_ % duration) -
                frame_end_times_.begin();
  const bool frame_changed = current_frame_index_ != frame_index;
  current_frame_index_ = frame_index;
  if (finished) {
    is_playing_ = false;
    UnregisterFrameCallback();
  }
  const uint64_t generation = playback_generation_;
  auto weak = weak_factory_.GetWeakPtr();
  if (frame_changed) {
    NotifyFrameChanged();
  }
  if (!weak || generation != playback_generation_) {
    return !weak || !is_playing_;
  }
  if (listener_ && loop_complete) {
    listener_->OnCurrentLoopComplete();
  }
  if (!weak || generation != playback_generation_) {
    return !weak || !is_playing_;
  }
  if (listener_ && finished) {
    listener_->OnFinalLoopComplete();
  }
  return !weak || !is_playing_;
}

bool AnimatedImagePlayer::ShouldReceiveAnimationFrame(
    int64_t current_time, int64_t* next_lifecycle_time) {
  if (next_lifecycle_time) {
    *next_lifecycle_time = -1;
  }
  was_visible_ = !visible_callback_ || visible_callback_();
  return is_playing_ && was_visible_;
}

void AnimatedImagePlayer::EnsureAnimationScheduled() {
  if (is_playing_ && !was_visible_ &&
      (!visible_callback_ || visible_callback_())) {
    // A hidden callback remains registered but no longer requests vsync.
    UnregisterFrameCallback();
    RegisterFrameCallback();
  }
}

void AnimatedImagePlayer::RegisterFrameCallback() {
  if (is_playing_ && !is_registered_ && animation_handler_) {
    is_registered_ = true;
    animation_handler_->AddAnimationFrameCallback(this, 0);
  }
}

void AnimatedImagePlayer::UnregisterFrameCallback() {
  if (is_registered_ && animation_handler_) {
    animation_handler_->RemoveCallback(this);
  }
  is_registered_ = false;
}

void AnimatedImagePlayer::NotifyFrameChanged() {
  if (frame_changed_callback_) {
    frame_changed_callback_();
  }
}

}  // namespace clay
