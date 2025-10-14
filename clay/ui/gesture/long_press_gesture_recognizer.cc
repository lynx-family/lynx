// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/long_press_gesture_recognizer.h"

#include <memory>

#include "clay/ui/gesture/macros.h"

namespace clay {

void LongPressGestureRecognizer::Reset() {
  down_event_.reset();
  up_event_.reset();
  state_ = LongPressState::kUnknown;
}

void LongPressGestureRecognizer::HandlePrimaryPointerEvent(
    const PointerEvent& event) {
  if (event.type == PointerEvent::EventType::kUpEvent) {
    up_event_ = std::make_unique<PointerEvent>(event);
    if (state_ == LongPressState::kDetected) {
      if (on_long_press_end_) {
        on_long_press_end_(event);
      }
    } else {
      ResolveAll(GestureDisposition::kReject);
    }
    Reset();
  } else if (event.type == PointerEvent::EventType::kDownEvent) {
    // We should reset to the initial state on touch down. This is needed to fix
    // the issue where long press gesture cannot be recognized correctly after a
    // drag gesture.
    Reset();
    down_event_ = std::make_unique<PointerEvent>(event);
    if (on_long_press_down_) {
      on_long_press_down_(event);
    }
  } else if (event.type == PointerEvent::EventType::kMoveEvent) {
    if (state_ == LongPressState::kDetected && on_long_press_move_) {
      on_long_press_move_(event);
    }
  } else if (event.type == PointerEvent::EventType::kCancel) {
    ResolveAll(GestureDisposition::kReject);
    Reset();
  }
}

void LongPressGestureRecognizer::OnGestureAccepted(int pointer_id) {
  // Ignore winning. We care about long press timeout.
}

void LongPressGestureRecognizer::OnResolveAll(GestureDisposition disposition) {
  GESTURE_LOG << GetMemberTag() << this << " OnResolveAll with disposition:"
              << static_cast<int>(disposition);
  if (disposition == GestureDisposition::kReject) {
    if (state_ == LongPressState::kDetected) {
      Reset();
    } else {
      state_ = LongPressState::kDefunct;
      if (on_long_press_cancel_) {
        on_long_press_cancel_();
      }
    }
  }
}

void LongPressGestureRecognizer::OnTimeout() {
  GESTURE_LOG << GetMemberTag() << this
              << " OnTimeout current state:" << static_cast<int>(state_);
  if (state_ == LongPressState::kDefunct) {
    return;
  }

  state_ = LongPressState::kDetected;

  ResolveAll(GestureDisposition::kAccept);

  if (on_long_press_start_ && down_event_) {
    on_long_press_start_(*down_event_);
  }
}

}  // namespace clay
