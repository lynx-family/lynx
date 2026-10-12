// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/input/synthetic_touch_pinch_gesture.h"

#include <algorithm>
#include <cmath>

#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {
namespace input {
namespace {

// Half the initial distance from the pinch anchor to each synthetic pointer:
// the two pointers start 2 * kInitialPinchHalfSpanPx apart on a horizontal line
// through the anchor, and the final span is that value times scale_factor. It
// is a fixed logical-pixel constant, deliberately not derived from any device
// or display parameter, because the emitted PointerEvents are consumed by
// Lynx's own input pipeline, which imposes no touch-slop threshold; a constant
// keeps the gesture simple and reproducible.
//
// Make this device-aware (e.g. derive it from touch slop or clamp a minimum
// span, as Chromium's SyntheticTouchscreenPinchGesture does) when either:
//   - the events are routed to a native recognizer (Android
//     ScaleGestureDetector, iOS UIPinchGestureRecognizer) with a minimum-span /
//     slop requirement -- a scale_factor near 1 moves each pointer only
//     kInitialPinchHalfSpanPx * |scale_factor - 1| px and may fall below slop
//     and never be recognized; or
//   - very small scale_factors must stay valid, where the final span shrinks
//     toward zero and both pointers collapse onto the anchor.
constexpr float kInitialPinchHalfSpanPx = 50.f;

int64_t DurationUs(float initial_half_span, float final_half_span,
                   int relative_speed_px_per_second) {
  const int safe_speed = std::max(1, relative_speed_px_per_second);
  // CDP relativeSpeed describes the changing span between the two pointers,
  // not the distance travelled by one pointer.
  const double span_delta_px =
      2.0 * std::abs(static_cast<double>(final_half_span - initial_half_span));
  return static_cast<int64_t>(
      std::ceil(span_delta_px * 1000000.0 / static_cast<double>(safe_speed)));
}

}  // namespace

SyntheticTouchPinchGesture::SyntheticTouchPinchGesture(
    float x, float y, float scale_factor, int relative_speed_px_per_second)
    : x_(x),
      y_(y),
      initial_half_span_(kInitialPinchHalfSpanPx),
      final_half_span_(kInitialPinchHalfSpanPx * scale_factor),
      duration_us_(DurationUs(initial_half_span_, final_half_span_,
                              relative_speed_px_per_second)),
      first_pointer_id_(GenerateSyntheticPointerId()),
      second_pointer_id_(GenerateSyntheticPointerId()) {}

bool SyntheticTouchPinchGesture::Inject(PointerEventType type,
                                        int32_t action_pointer_id,
                                        int64_t frame_time_us, float first_x,
                                        float second_x,
                                        bool include_second_pointer,
                                        InputEventTarget* target) {
  PointerEvent event;
  // This gesture models a touchscreen pinch. Mouse/touchpad pinch uses a
  // different native shape: ctrl-modified scroll events emitted by a separate
  // gesture.
  event.source_type = PointerSourceType::kTouch;
  event.type = type;
  event.action_pointer_id = action_pointer_id;
  event.timestamp_us = frame_time_us;
  event.buttons = type == PointerEventType::kDown ? kPrimaryButton : kNoButton;

  Pointer first_pointer;
  first_pointer.id = first_pointer_id_;
  first_pointer.x = first_x;
  first_pointer.y = y_;
  event.pointers.push_back(first_pointer);

  if (include_second_pointer) {
    Pointer second_pointer;
    second_pointer.id = second_pointer_id_;
    second_pointer.x = second_x;
    second_pointer.y = y_;
    event.pointers.push_back(second_pointer);
  }
  return target->InjectPointerEvent(event);
}

float SyntheticTouchPinchGesture::CurrentHalfSpan(int64_t frame_time_us) const {
  if (duration_us_ == 0 || frame_time_us >= start_time_us_ + duration_us_) {
    return final_half_span_;
  }
  const double elapsed = std::max<int64_t>(0, frame_time_us - start_time_us_);
  const float progress =
      static_cast<float>(elapsed / static_cast<double>(duration_us_));
  return initial_half_span_ +
         (final_half_span_ - initial_half_span_) * progress;
}

void SyntheticTouchPinchGesture::CancelActivePointers(
    int64_t frame_time_us, InputEventTarget* target) {
  if (!target || !first_pointer_active_) {
    return;
  }
  const float half_span = second_pointer_active_
                              ? CurrentHalfSpan(frame_time_us)
                              : initial_half_span_;
  Inject(PointerEventType::kCancel, first_pointer_id_, frame_time_us,
         x_ - half_span, x_ + half_span, second_pointer_active_, target);
  first_pointer_active_ = false;
  second_pointer_active_ = false;
}

void SyntheticTouchPinchGesture::Cancel(int64_t frame_time_us,
                                        InputEventTarget* target) {
  CancelActivePointers(frame_time_us, target);
  state_ = State::kTerminated;
}

SyntheticGestureResult SyntheticTouchPinchGesture::ForwardInputEvents(
    int64_t frame_time_us, InputEventTarget* target) {
  if (!target || state_ == State::kTerminated) {
    return state_ == State::kTerminated ? SyntheticGestureResult::kDone
                                        : SyntheticGestureResult::kFailed;
  }

  while (true) {
    switch (state_) {
      case State::kPendingFirstPress:
        if (final_half_span_ == initial_half_span_) {
          state_ = State::kTerminated;
          return SyntheticGestureResult::kDone;
        }
        if (!Inject(PointerEventType::kDown, first_pointer_id_, frame_time_us,
                    x_ - initial_half_span_, x_ + initial_half_span_, false,
                    target)) {
          return SyntheticGestureResult::kFailed;
        }
        first_pointer_active_ = true;
        state_ = State::kPendingSecondPress;
        continue;
      case State::kPendingSecondPress:
        if (!Inject(PointerEventType::kDown, second_pointer_id_, frame_time_us,
                    x_ - initial_half_span_, x_ + initial_half_span_, true,
                    target)) {
          CancelActivePointers(frame_time_us, target);
          return SyntheticGestureResult::kFailed;
        }
        second_pointer_active_ = true;
        start_time_us_ = frame_time_us;
        state_ = State::kMoving;
        if (duration_us_ > 0) {
          return SyntheticGestureResult::kRunning;
        }
        continue;
      case State::kMoving: {
        const float half_span = CurrentHalfSpan(frame_time_us);
        if (frame_time_us < start_time_us_ + duration_us_) {
          if (!Inject(PointerEventType::kMove, first_pointer_id_, frame_time_us,
                      x_ - half_span, x_ + half_span, true, target)) {
            CancelActivePointers(frame_time_us, target);
            return SyntheticGestureResult::kFailed;
          }
          return SyntheticGestureResult::kRunning;
        }
        if (!emitted_final_move_ && final_half_span_ != initial_half_span_) {
          if (!Inject(PointerEventType::kMove, first_pointer_id_, frame_time_us,
                      x_ - final_half_span_, x_ + final_half_span_, true,
                      target)) {
            CancelActivePointers(frame_time_us, target);
            return SyntheticGestureResult::kFailed;
          }
          emitted_final_move_ = true;
        }
        state_ = State::kPendingSecondRelease;
        continue;
      }
      case State::kPendingSecondRelease:
        if (!Inject(PointerEventType::kUp, second_pointer_id_, frame_time_us,
                    x_ - final_half_span_, x_ + final_half_span_, true,
                    target)) {
          CancelActivePointers(frame_time_us, target);
          return SyntheticGestureResult::kFailed;
        }
        second_pointer_active_ = false;
        state_ = State::kPendingFirstRelease;
        continue;
      case State::kPendingFirstRelease:
        if (!Inject(PointerEventType::kUp, first_pointer_id_, frame_time_us,
                    x_ - final_half_span_, x_ + final_half_span_, false,
                    target)) {
          CancelActivePointers(frame_time_us, target);
          return SyntheticGestureResult::kFailed;
        }
        first_pointer_active_ = false;
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
