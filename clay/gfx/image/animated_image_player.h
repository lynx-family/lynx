// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_ANIMATED_IMAGE_PLAYER_H_
#define CLAY_GFX_IMAGE_ANIMATED_IMAGE_PLAYER_H_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/image/image_animation_listener.h"
#include "clay/gfx/image/platform_image.h"

namespace clay {

// Owns all playback calculations and state for one animated image instance.
// PlatformImageAnimation only exposes frame metadata and indexed decoding.
class AnimatedImagePlayer : public AnimationHandler::AnimationFrameCallback {
 public:
  AnimatedImagePlayer(std::unique_ptr<PlatformImageAnimation> animation,
                      fml::WeakPtr<AnimationHandler> animation_handler,
                      std::function<void()> frame_changed_callback,
                      std::function<bool()> visible_callback,
                      ImageAnimationListener* listener = nullptr);
  ~AnimatedImagePlayer() override;

  bool IsValid() const { return !frame_end_times_.empty(); }
  bool IsPlaying() const { return is_playing_; }
  int GetCurrentFrameIndex() const { return current_frame_index_; }
  std::shared_ptr<PlatformImageAnimation> GetAnimation() const {
    return animation_;
  }

  void SetAutoPlay(bool auto_play);
  // Zero (or a negative value) means infinite playback.
  void SetLoopCount(int loop_count);
  void StartAnimation();
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

  // frame_time is an externally supplied timestamp in milliseconds. The first
  // tick after start/resume anchors the timeline; no wall clock is consulted.
  // Repeated timestamps are idempotent. Rollback resamples the timeline without
  // replaying historical events. Missed loops coalesce into one loop callback.
  bool DoAnimationFrame(int64_t frame_time, bool update_values = true) override;
  bool ShouldReceiveAnimationFrame(int64_t current_time,
                                   int64_t* next_lifecycle_time) override;

  // Visibility suppresses ticks, not elapsed time. Wake scheduling when an
  // instance becomes visible again so its next tick catches up to frame_time.
  void EnsureAnimationScheduled();

 private:
  void RegisterFrameCallback();
  void UnregisterFrameCallback();
  void NotifyFrameChanged();

  std::shared_ptr<PlatformImageAnimation> animation_;
  fml::WeakPtr<AnimationHandler> animation_handler_;
  std::function<void()> frame_changed_callback_;
  std::function<bool()> visible_callback_;
  ImageAnimationListener* listener_ = nullptr;
  std::vector<int64_t> frame_end_times_;
  std::optional<int64_t> anchor_frame_time_;
  int64_t anchor_position_ = 0;
  int64_t position_ = 0;
  int64_t completed_loops_ = 0;
  uint64_t playback_generation_ = 0;
  int current_frame_index_ = 0;
  int loop_count_ = 0;
  bool is_playing_ = false;
  bool is_paused_ = false;
  bool is_registered_ = false;
  bool was_visible_ = false;
  fml::WeakPtrFactory<AnimatedImagePlayer> weak_factory_{this};
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_ANIMATED_IMAGE_PLAYER_H_
