// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/thread_local_animation_handler.h"

namespace lynx {
namespace animation {
namespace basic {
ThreadLocalAnimationHandler& ThreadLocalAnimationHandler::GetInstance() {
  static thread_local ThreadLocalAnimationHandler instance;
  return instance;
}

void ThreadLocalAnimationHandler::RequestNextFrame() {
  if (!frame_provider_) {
    return;
  }

  if (!has_requested_next_frame_) {
    frame_provider_->RequestNextFrame([this](const fml::TimePoint& frame_time) {
      // It must clear `has_requested_next_frame_` before invoking
      // `DoAnimationFrame`, because  `has_requested_next_frame_` will be set
      // during invoking `DoAnimationFrame`.
      has_requested_next_frame_ = false;
      DoAnimationFrame(frame_time);
    });
    has_requested_next_frame_ = true;
  }
}

void ThreadLocalAnimationHandler::AddAnimationFrameCallback(
    AnimationFrameCallback& callback) {
  animation_callbacks_.insert_or_assign(reinterpret_cast<uintptr_t>(&callback),
                                        callback.weak_from_this());
  RequestNextFrame();
}

void ThreadLocalAnimationHandler::DoAnimationFrame(
    const fml::TimePoint& frame_time) {
  std::unordered_map<uintptr_t, std::weak_ptr<AnimationFrameCallback>>
      temp_animation_callbacks;
  temp_animation_callbacks.swap(animation_callbacks_);
  for (auto& [ptr, weak_callback] : temp_animation_callbacks) {
    std::shared_ptr<AnimationFrameCallback> callback = weak_callback.lock();
    if (callback) {
      callback->DoAnimationFrame(frame_time);
    }
  }
}
}  // namespace basic
}  // namespace animation
}  // namespace lynx
