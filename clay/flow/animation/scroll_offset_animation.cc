// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/animation/scroll_offset_animation.h"

#include <cmath>

#include "clay/flow/animation/animation_mutator.h"

namespace clay {

ScrollOffsetAnimation::~ScrollOffsetAnimation() {
  if (animator_->IsRunning()) {
    animator_->Cancel();
  }
}

void ScrollOffsetAnimation::StartIfNeeded() {
  if (has_started_) {
    return;
  }
  has_started_ = true;
  animator_->SetAnimationHandler(&animation_handler_);
  animator_->FlingInitialize();
  animator_->Start();
}

bool ScrollOffsetAnimation::IsRunning() const { return animator_->IsRunning(); }

bool ScrollOffsetAnimation::DoAnimationFrame(
    int64_t frame_time, AnimationMutator* animation_mutator) {
  ScrollOffsetMutator* mutator = animation_mutator->asScrollOffset();
  if (!mutator) {
    return true;
  }
  StartIfNeeded();
  if (!animator_->IsRunning()) {
    if (has_started_) {
      // Should apply the current scroll offset to `mutator` even when the
      // animation has reached the end, or the `mutator` may be got the wrong
      // value from UI layer.
      mutator->UpdateScrollOffset(current_scroll_offset_);
    }
    return true;
  }
  bool ignore_ui_repaint = true;
  animator_->DoAnimationFrame(frame_time);
  float value = animator_->GetValue();
  auto& max_offset_range = mutator->GetMaxOffsetRange();
  auto& visible_offset_range = mutator->GetVisibleOffsetRange();
  auto scroll_offset = mutator->GetScrollOffset();
  // If the value has reached the limit, it means that the animation should be
  // finished.
  if (direction_ == ScrollDirection::kVertical) {
    if (value <= max_offset_range.Top() || value >= max_offset_range.Bottom()) {
      animator_->Cancel();
    }
    value =
        std::clamp(value, max_offset_range.Top(), max_offset_range.Bottom());
  } else {
    if (value <= max_offset_range.Left() || value >= max_offset_range.Right()) {
      animator_->Cancel();
    }
    value =
        std::clamp(value, max_offset_range.Left(), max_offset_range.Right());
  }
  float update_value = value;
  // Clamped the value by visible offset range.
  if (direction_ == ScrollDirection::kVertical) {
    update_value = std::clamp(update_value, visible_offset_range.Top(),
                              visible_offset_range.Bottom());
  } else {
    update_value = std::clamp(update_value, visible_offset_range.Left(),
                              visible_offset_range.Right());
  }
  if (update_value != value) {
    ignore_ui_repaint = false;
  }
  update_value = std::round(-update_value);
  if (direction_ == ScrollDirection::kVertical) {
    current_scroll_offset_ = {scroll_offset.x, update_value};
  } else {
    current_scroll_offset_ = {update_value, scroll_offset.y};
  }
  mutator->UpdateScrollOffset(current_scroll_offset_);
  // Send the scroll animation events to the UI thread, and it will synchronize
  // the animation value to the ScrollView.
  mutator->OnScrolled(session_id_, value, ignore_ui_repaint);
  bool stopped = !animator_->IsRunning();
  if (stopped) {
    // If the animation is stopped, send the scroll end event to the UI thread.
    mutator->OnScrollEnd(session_id_, value, animator_->GetCurrentVelocity());
  }
  return stopped;
}
}  // namespace clay
