// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/input/synthetic_touch_scroll_gesture.h"

#include <algorithm>
#include <cmath>

#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {
namespace input {
namespace {

// Feed stationary moves through the native velocity tracker before releasing
// the pointer, so ACTION_UP does not start a fling.
constexpr int64_t kStopDurationUs = 100000;

double DurationUs(float dx, float dy, int speed) {
  return std::ceil(
      std::hypot(static_cast<double>(dx), static_cast<double>(dy)) * 1000000.0 /
      std::max(1, speed));
}

}  // namespace

SyntheticTouchScrollGesture::SyntheticTouchScrollGesture(
    const SyntheticTouchScrollGestureParams& params)
    : params_(params),
      move_duration_us_(
          DurationUs(params.x_distance, params.y_distance, params.speed)),
      overscroll_duration_us_(
          DurationUs(params.x_overscroll, params.y_overscroll, params.speed)),
      remaining_repeats_(params.repeat_count) {}

double SyntheticTouchScrollGesture::TotalDurationUs(
    const SyntheticTouchScrollGestureParams& params) {
  const double duration =
      DurationUs(params.x_distance, params.y_distance, params.speed) +
      DurationUs(params.x_overscroll, params.y_overscroll, params.speed);
  if (duration == 0) {
    return 0;
  }
  return (duration + (params.prevent_fling ? kStopDurationUs : 0)) *
             (static_cast<double>(params.repeat_count) + 1) +
         static_cast<double>(params.repeat_delay_ms) * 1000 *
             params.repeat_count;
}

bool SyntheticTouchScrollGesture::Inject(PointerEventType type,
                                         int64_t frame_time_us,
                                         InputEventTarget* target) {
  PointerEvent event;
  event.source_type = PointerSourceType::kTouch;
  event.type = type;
  event.timestamp_us = frame_time_us;
  event.action_pointer_id = pointer_.id;
  event.buttons =
      type == PointerEventType::kDown || type == PointerEventType::kMove
          ? kPrimaryButton
          : kNoButton;
  event.pointers.push_back(pointer_);
  return target->InjectPointerEvent(event);
}

void SyntheticTouchScrollGesture::Cancel(int64_t frame_time_us,
                                         InputEventTarget* target) {
  if (pointer_active_ && target) {
    Inject(PointerEventType::kCancel, frame_time_us, target);
  }
  pointer_active_ = false;
  state_ = State::kTerminated;
}

SyntheticGestureResult SyntheticTouchScrollGesture::Fail(
    int64_t frame_time_us, InputEventTarget* target) {
  Cancel(frame_time_us, target);
  return SyntheticGestureResult::kFailed;
}

SyntheticGestureResult SyntheticTouchScrollGesture::ForwardInputEvents(
    int64_t frame_time_us, InputEventTarget* target) {
  if (state_ == State::kTerminated) {
    return SyntheticGestureResult::kDone;
  }
  if (!target) {
    return Fail(frame_time_us, target);
  }
  while (true) {
    switch (state_) {
      case State::kPendingPress:
        if (move_duration_us_ == 0 && overscroll_duration_us_ == 0) {
          state_ = State::kTerminated;
          return SyntheticGestureResult::kDone;
        }
        pointer_.id = GenerateSyntheticPointerId();
        pointer_.x = params_.x;
        pointer_.y = params_.y;
        if (!Inject(PointerEventType::kDown, frame_time_us, target)) {
          return Fail(frame_time_us, target);
        }
        pointer_active_ = true;
        phase_start_us_ = frame_time_us;
        state_ = move_duration_us_ > 0 ? State::kMoving : State::kOverscrolling;
        return SyntheticGestureResult::kRunning;
      case State::kMoving:
      case State::kOverscrolling: {
        const bool moving = state_ == State::kMoving;
        const int64_t duration =
            moving ? move_duration_us_ : overscroll_duration_us_;
        const double progress =
            std::min(1.0, static_cast<double>(std::max<int64_t>(
                              0, frame_time_us - phase_start_us_)) /
                              duration);
        // CDP distances have the same sign as finger motion; overscroll is a
        // separate displacement in the opposite direction.
        pointer_.x =
            static_cast<double>(params_.x) +
            (moving ? params_.x_distance * progress
                    : params_.x_distance - params_.x_overscroll * progress);
        pointer_.y =
            static_cast<double>(params_.y) +
            (moving ? params_.y_distance * progress
                    : params_.y_distance - params_.y_overscroll * progress);
        if (!Inject(PointerEventType::kMove, frame_time_us, target)) {
          return Fail(frame_time_us, target);
        }
        if (progress < 1.0) {
          return SyntheticGestureResult::kRunning;
        }
        phase_start_us_ = frame_time_us;
        if (moving && overscroll_duration_us_ > 0) {
          state_ = State::kOverscrolling;
          return SyntheticGestureResult::kRunning;
        }
        state_ = State::kStopping;
        if (params_.prevent_fling) {
          return SyntheticGestureResult::kRunning;
        }
        continue;
      }
      case State::kStopping:
        if (params_.prevent_fling) {
          if (!Inject(PointerEventType::kMove, frame_time_us, target)) {
            return Fail(frame_time_us, target);
          }
          if (frame_time_us - phase_start_us_ < kStopDurationUs) {
            return SyntheticGestureResult::kRunning;
          }
        }
        if (!Inject(PointerEventType::kUp, frame_time_us, target)) {
          return Fail(frame_time_us, target);
        }
        pointer_active_ = false;
        if (remaining_repeats_ == 0) {
          state_ = State::kTerminated;
          return SyntheticGestureResult::kDone;
        }
        --remaining_repeats_;
        phase_start_us_ = frame_time_us;
        state_ = State::kWaitingForRepeat;
        return SyntheticGestureResult::kRunning;
      case State::kWaitingForRepeat:
        if (frame_time_us - phase_start_us_ <
            static_cast<int64_t>(params_.repeat_delay_ms) * 1000) {
          return SyntheticGestureResult::kRunning;
        }
        state_ = State::kPendingPress;
        continue;
      case State::kTerminated:
        return SyntheticGestureResult::kDone;
    }
  }
}

}  // namespace input
}  // namespace devtool
}  // namespace lynx
