// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_ANIMATION_SCROLL_OFFSET_ANIMATION_H_
#define CLAY_FLOW_ANIMATION_SCROLL_OFFSET_ANIMATION_H_

#include <memory>
#include <utility>

#include "clay/gfx/animation/fling_animator.h"
#include "clay/gfx/scroll_direction.h"
#include "skity/geometry/vector.hpp"

namespace clay {
class AnimationMutator;

// The scroll offset animation is used to run raster scroll animation. It's
// managed by `ScrollOffsetMutator`.
class ScrollOffsetAnimation {
 public:
  ScrollOffsetAnimation(int32_t session_id, ScrollDirection direction,
                        std::unique_ptr<clay::FlingAnimator> animator)
      : session_id_(session_id),
        direction_(direction),
        animator_(std::move(animator)) {}
  ~ScrollOffsetAnimation();

  clay::FlingAnimator* GetAnimator() { return animator_.get(); }

  void StartIfNeeded();

  bool IsRunning() const;
  bool DoAnimationFrame(int64_t frame_time, AnimationMutator* mutator);

 private:
  int32_t session_id_;
  ScrollDirection direction_;
  std::unique_ptr<clay::FlingAnimator> animator_;
  clay::AnimationHandler animation_handler_;
  bool has_started_ = false;
  skity::Vec2 current_scroll_offset_;
};

}  // namespace clay
#endif  // CLAY_FLOW_ANIMATION_SCROLL_OFFSET_ANIMATION_H_
