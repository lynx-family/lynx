// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_GESTURE_H_
#define DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_GESTURE_H_

#include <cstdint>

namespace lynx {
namespace devtool {
namespace input {

class InputEventTarget;

enum class SyntheticGestureResult {
  kRunning,
  kDone,
  kFailed,
};

class SyntheticGesture {
 public:
  virtual ~SyntheticGesture() = default;

  virtual SyntheticGestureResult ForwardInputEvents(
      int64_t frame_time_us, InputEventTarget* target) = 0;
  virtual void Cancel(int64_t frame_time_us, InputEventTarget* target) {}
};

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_GESTURE_H_
