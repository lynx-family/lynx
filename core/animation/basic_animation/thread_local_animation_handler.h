// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_THREAD_LOCAL_ANIMATION_HANDLER_H_
#define CORE_ANIMATION_BASIC_ANIMATION_THREAD_LOCAL_ANIMATION_HANDLER_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "core/animation/basic_animation/animation_frame_callback.h"
#include "core/animation/basic_animation/animation_frame_callback_provider.h"

namespace lynx {
namespace animation {
namespace basic {
class ThreadLocalAnimationHandler {
 public:
  static ThreadLocalAnimationHandler& GetInstance();

  ThreadLocalAnimationHandler(const ThreadLocalAnimationHandler&) = delete;
  ThreadLocalAnimationHandler& operator=(const ThreadLocalAnimationHandler&) =
      delete;
  ThreadLocalAnimationHandler(ThreadLocalAnimationHandler&&) = delete;
  ThreadLocalAnimationHandler& operator=(ThreadLocalAnimationHandler&&) =
      delete;

  void SetFrameProvider(
      std::unique_ptr<AnimationFrameCallbackProvider> provider) {
    frame_provider_ = std::move(provider);
  }

  void AddAnimationFrameCallback(AnimationFrameCallback& callback);

  void RemoveAnimationFrameCallback(const AnimationFrameCallback* callback) {
    animation_callbacks_.erase(reinterpret_cast<uintptr_t>(callback));
  }

  void DoAnimationFrame(const fml::TimePoint& frame_time);

 private:
  ThreadLocalAnimationHandler() = default;

  void RequestNextFrame();

  std::unique_ptr<AnimationFrameCallbackProvider> frame_provider_;
  std::unordered_map<uintptr_t, std::weak_ptr<AnimationFrameCallback>>
      animation_callbacks_;
  bool has_requested_next_frame_{false};
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_THREAD_LOCAL_ANIMATION_HANDLER_H_
