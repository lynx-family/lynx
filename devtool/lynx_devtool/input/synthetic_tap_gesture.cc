// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/input/synthetic_tap_gesture.h"

#include <algorithm>

#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {
namespace input {

SyntheticTapGesture::SyntheticTapGesture(float x, float y, int duration_ms,
                                         PointerSourceType source_type)
    : x_(x),
      y_(y),
      duration_ms_(std::max(0, duration_ms)),
      source_type_(source_type),
      pointer_id_(GenerateSyntheticPointerId()) {}

bool SyntheticTapGesture::Inject(PointerEventType type, int64_t frame_time_us,
                                 InputEventTarget* target) {
  PointerEvent event;
  event.source_type = source_type_;
  event.type = type;
  event.action_pointer_id = pointer_id_;
  event.timestamp_us = frame_time_us;
  event.buttons = type == PointerEventType::kDown ? kPrimaryButton : kNoButton;
  event.click_count = 1;

  Pointer pointer;
  pointer.id = pointer_id_;
  pointer.x = x_;
  pointer.y = y_;
  event.pointers.push_back(pointer);
  return target->InjectPointerEvent(event);
}

void SyntheticTapGesture::CancelActivePointer(int64_t frame_time_us,
                                              InputEventTarget* target) {
  if (state_ == State::kPendingRelease) {
    Inject(PointerEventType::kCancel, frame_time_us, target);
  }
}

void SyntheticTapGesture::Cancel(int64_t frame_time_us,
                                 InputEventTarget* target) {
  CancelActivePointer(frame_time_us, target);
  state_ = State::kTerminated;
}

SyntheticGestureResult SyntheticTapGesture::ForwardInputEvents(
    int64_t frame_time_us, InputEventTarget* target) {
  if (!target || state_ == State::kTerminated) {
    return state_ == State::kTerminated ? SyntheticGestureResult::kDone
                                        : SyntheticGestureResult::kFailed;
  }

  while (true) {
    switch (state_) {
      case State::kPendingPress:
        if (!Inject(PointerEventType::kDown, frame_time_us, target)) {
          return SyntheticGestureResult::kFailed;
        }
        press_time_us_ = frame_time_us;
        state_ = State::kPendingRelease;
        if (duration_ms_ > 0) {
          return SyntheticGestureResult::kRunning;
        }
        continue;
      case State::kPendingRelease:
        if (frame_time_us < press_time_us_ + duration_ms_ * 1000LL) {
          return SyntheticGestureResult::kRunning;
        }
        if (!Inject(PointerEventType::kUp, frame_time_us, target)) {
          CancelActivePointer(frame_time_us, target);
          return SyntheticGestureResult::kFailed;
        }
        state_ = State::kTerminated;
        return SyntheticGestureResult::kDone;
      case State::kTerminated:
        return SyntheticGestureResult::kDone;
    }
  }
}

}  // namespace input
}  // namespace devtool
}  // namespace lynx
