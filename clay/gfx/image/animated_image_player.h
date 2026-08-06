// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_ANIMATED_IMAGE_PLAYER_H_
#define CLAY_GFX_IMAGE_ANIMATED_IMAGE_PLAYER_H_

#include <functional>
#include <memory>

#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/timer.h"
#include "clay/gfx/image/platform_image.h"

namespace clay {

// Owns the mutable playback state for one animated image instance. The
// PlatformImage that creates the animation remains shareable and immutable.
class AnimatedImagePlayer {
 public:
  AnimatedImagePlayer(std::unique_ptr<PlatformImageAnimation> animation,
                      fml::RefPtr<fml::TaskRunner> task_runner,
                      std::function<void()> frame_changed_callback,
                      std::function<bool()> visible_callback);
  ~AnimatedImagePlayer();

  bool IsValid() const { return animation_ != nullptr; }
  bool IsPlaying() const { return is_playing_; }
  std::shared_ptr<PlatformImageAnimation> GetAnimation() const {
    return animation_;
  }

  void SetAutoPlay(bool auto_play);
  void SetLoopCount(int loop_count);
  void StartAnimation();
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

  // Restarts frame scheduling after an instance becomes visible again.
  void EnsureAnimationScheduled();

 private:
  void NotifyFrameChanged();
  void StartNextFrameTimer();

  std::shared_ptr<PlatformImageAnimation> animation_;
  std::unique_ptr<fml::OneshotTimer> frame_timer_;
  std::function<void()> frame_changed_callback_;
  std::function<bool()> visible_callback_;
  bool is_playing_ = false;
  bool is_timer_running_ = false;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_ANIMATED_IMAGE_PLAYER_H_
