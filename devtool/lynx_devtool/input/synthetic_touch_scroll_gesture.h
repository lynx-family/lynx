// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TOUCH_SCROLL_GESTURE_H_
#define DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TOUCH_SCROLL_GESTURE_H_

#include <cstdint>

#include "devtool/lynx_devtool/base/input_event.h"
#include "devtool/lynx_devtool/input/synthetic_gesture.h"

namespace lynx {
namespace devtool {
namespace input {

struct SyntheticTouchScrollGestureParams {
  float x = 0.f;
  float y = 0.f;
  float x_distance = 0.f;
  float y_distance = 0.f;
  float x_overscroll = 0.f;
  float y_overscroll = 0.f;
  int speed = 800;
  bool prevent_fling = true;
  int repeat_count = 0;
  int repeat_delay_ms = 250;
};

// Emits a single-finger swipe followed by an optional reverse displacement.
// Coordinates and speed are expressed in window-local logical pixels.
class SyntheticTouchScrollGesture : public SyntheticGesture {
 public:
  explicit SyntheticTouchScrollGesture(
      const SyntheticTouchScrollGestureParams& params);

  // Used by the request handler to bound the sequence before converting its
  // durations to integer timestamps.
  static double TotalDurationUs(
      const SyntheticTouchScrollGestureParams& params);

  SyntheticGestureResult ForwardInputEvents(int64_t frame_time_us,
                                            InputEventTarget* target) override;
  void Cancel(int64_t frame_time_us, InputEventTarget* target) override;

 private:
  enum class State {
    kPendingPress,
    kMoving,
    kOverscrolling,
    kStopping,
    kWaitingForRepeat,
    kTerminated,
  };

  bool Inject(PointerEventType type, int64_t frame_time_us,
              InputEventTarget* target);
  SyntheticGestureResult Fail(int64_t frame_time_us, InputEventTarget* target);

  SyntheticTouchScrollGestureParams params_;
  State state_{State::kPendingPress};
  int64_t move_duration_us_;
  int64_t overscroll_duration_us_;
  int64_t phase_start_us_{0};
  int remaining_repeats_;
  Pointer pointer_;
  bool pointer_active_{false};
};

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TOUCH_SCROLL_GESTURE_H_
