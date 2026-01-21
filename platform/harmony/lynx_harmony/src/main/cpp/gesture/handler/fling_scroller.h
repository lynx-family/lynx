// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_FLING_SCROLLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_FLING_SCROLLER_H_

#include <arkui/ui_input_event.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/value/base_value.h"
#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"

namespace lynx {
namespace base {
class TimedTaskManager;
}
namespace tasm {
class GestureDetector;
struct GestureCallback;

namespace harmony {
class LynxContext;
class GestureArenaMember;

using FlingCallback = std::function<void(uint8_t, float, float)>;

class FlingScroller : public std::enable_shared_from_this<FlingScroller> {
 public:
  // Enum class for representing different types of gestures.
  enum class FlingState : uint8_t {
    IDLE = 0,
    FLING = 1,
  };
  FlingScroller();

  ~FlingScroller();

  void Start(float velocity_x, float velocity_y, FlingCallback callback);

  void Stop();

  bool IsIdle();

 private:
  std::shared_ptr<animation::basic::LynxBasicAnimator> fling_scroller_animator_;
  int64_t last_time_;
  uint8_t current_fling_state_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_FLING_SCROLLER_H_
