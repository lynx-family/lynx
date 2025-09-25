// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_COMMON_GESTURE_EXTRA_BUNDLE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_COMMON_GESTURE_EXTRA_BUNDLE_H_

namespace lynx {
namespace tasm {

namespace harmony {

class GestureExtraBundle {
 public:
  GestureExtraBundle();

  // DIRECTION_HORIZONTAL = -1 DIRECTION_VERTICAL = 1
  // DIRECTION_UNDETERMINED = 0
  int GestureDirection() const { return gesture_direction_; }
  void SetGestureDirection(int direction) { gesture_direction_ = direction; }

  // The offset x that needs to be consumed by the simultaneous gesture
  float SimultaneousDeltaX() const { return simultaneous_delta_x_; }
  void SetSimultaneousDeltaX(float delta_x) { simultaneous_delta_x_ = delta_x; }

  // The offset y that needs to be consumed by the simultaneous gesture
  float SimultaneousDeltaY() const { return simultaneous_delta_y_; }
  void SetSimultaneousDeltaY(float delta_y) { simultaneous_delta_y_ = delta_y; }

  bool IsNeedConsumedSimultaneousGesture() const {
    return is_need_consumed_simultaneous_gesture_;
  }
  void SetIsNeedConsumedSimultaneousGesture(bool need_consumed) {
    is_need_consumed_simultaneous_gesture_ = need_consumed;
  }

  // need consume simultaneous gesture or not
  void ResetSimultaneousDelta();

 private:
  int gesture_direction_ = 0;
  float simultaneous_delta_x_ = 0.0f;
  float simultaneous_delta_y_ = 0.0f;
  bool is_need_consumed_simultaneous_gesture_ = false;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_COMMON_GESTURE_EXTRA_BUNDLE_H_
