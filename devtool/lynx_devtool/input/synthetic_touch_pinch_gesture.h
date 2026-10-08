// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TOUCH_PINCH_GESTURE_H_
#define DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TOUCH_PINCH_GESTURE_H_

#include <cstdint>

#include "devtool/lynx_devtool/base/input_event.h"
#include "devtool/lynx_devtool/input/synthetic_gesture.h"

namespace lynx {
namespace devtool {
namespace input {

// Emits a two-finger touch pinch centered at (x, y). Each pointer travels at
// relative_speed_px_per_second until their separation changes by scale_factor.
class SyntheticTouchPinchGesture : public SyntheticGesture {
 public:
  SyntheticTouchPinchGesture(float x, float y, float scale_factor,
                             int relative_speed_px_per_second);

  SyntheticGestureResult ForwardInputEvents(int64_t frame_time_us,
                                            InputEventTarget* target) override;
  void Cancel(int64_t frame_time_us, InputEventTarget* target) override;

 private:
  enum class State {
    kPendingFirstPress,
    kPendingSecondPress,
    kMoving,
    kPendingSecondRelease,
    kPendingFirstRelease,
    kTerminated,
  };

  bool Inject(PointerEventType type, int32_t action_pointer_id,
              int64_t frame_time_us, float first_x, float second_x,
              bool include_second_pointer, InputEventTarget* target);
  void CancelActivePointers(int64_t frame_time_us, InputEventTarget* target);
  float CurrentHalfSpan(int64_t frame_time_us) const;

  float x_;
  float y_;
  float initial_half_span_;
  float final_half_span_;
  int64_t duration_us_;
  State state_{State::kPendingFirstPress};
  int32_t first_pointer_id_{0};
  int32_t second_pointer_id_{0};
  int64_t start_time_us_{0};
  bool first_pointer_active_{false};
  bool second_pointer_active_{false};
  bool emitted_final_move_{false};
};

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TOUCH_PINCH_GESTURE_H_
