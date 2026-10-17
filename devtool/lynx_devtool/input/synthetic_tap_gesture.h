// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TAP_GESTURE_H_
#define DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TAP_GESTURE_H_

#include <cstdint>

#include "devtool/lynx_devtool/base/input_event.h"
#include "devtool/lynx_devtool/input/synthetic_gesture.h"

namespace lynx {
namespace devtool {
namespace input {

class SyntheticTapGesture : public SyntheticGesture {
 public:
  SyntheticTapGesture(float x, float y, int duration_ms,
                      PointerSourceType source_type);

  SyntheticGestureResult ForwardInputEvents(int64_t frame_time_us,
                                            InputEventTarget* target) override;
  void Cancel(int64_t frame_time_us, InputEventTarget* target) override;

 private:
  enum class State {
    kPendingPress,
    kPendingRelease,
    kTerminated,
  };

  bool Inject(PointerEventType type, int64_t frame_time_us,
              InputEventTarget* target);
  void CancelActivePointer(int64_t frame_time_us, InputEventTarget* target);

  float x_;
  float y_;
  int duration_ms_;
  PointerSourceType source_type_;
  State state_{State::kPendingPress};
  int32_t pointer_id_{0};
  int64_t press_time_us_{0};
};

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_TAP_GESTURE_H_
