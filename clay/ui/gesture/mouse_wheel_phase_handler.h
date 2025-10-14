// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_UI_GESTURE_MOUSE_WHEEL_PHASE_HANDLER_H_
#define CLAY_UI_GESTURE_MOUSE_WHEEL_PHASE_HANDLER_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/timer.h"
#include "clay/ui/event/gesture_event.h"

namespace clay {

constexpr fml::TimeDelta kDefaultMouseWheelLatchingTransaction =
    fml::TimeDelta::FromMilliseconds(500);

/**
 *           |
 *           v
 * +-------------------+
 * |       None        |
 * +-------------------+
 *           |
 *           | (MouseWheelEvent)
 *           v
 * +-------------------+  (mouse move too much)   +------------------+
 * |    StartScroll    |------------------------->|    EndScroll     |
 * +-------------------+      (timeout)           +------------------+
 *           |                                             ^
 *           | (MouseWheelEvent)                           |
 *           v                                             |
 * +-------------------+        (mouse move too much)      |
 * |      Scroll       |-----------------------------------+
 * +-------------------+            (timeout)
 *      ^          |
 *      |          v
 *      |          |
 *      +----------+
 *   (MouseWheelEvent)
 */
class MouseWheelPhaseHandler final {
 public:
  class Delegate {
   public:
    virtual void SendSyntheticWheelEventWithPhaseEnd(const PointerEvent&) = 0;
  };

  MouseWheelPhaseHandler(fml::RefPtr<fml::TaskRunner>, Delegate*);

  // Set after initialization if you want to use different transaction latching.
  void SetMouseWheelEndDispatchTimeout(fml::TimeDelta timeout) {
    mouse_wheel_end_dispatch_timeout_ = timeout;
  }

  void UpdatePhaseAndScheduleEndEvent(PointerEvent&);

  void DispatchWheelEndEventIfNeeded(const PointerEvent&, bool force = false);

 private:
  void ScheduleMouseWheelEndDispatching(fml::TimeDelta timeout);
  void SendSyntheticWheelEventWithPhaseEnd();
  bool IsWithinSlopRegion(const PointerEvent& mouse_hover_event);

  fml::OneshotTimer mouse_wheel_end_dispatcher_timer_;
  fml::TimeDelta mouse_wheel_end_dispatch_timeout_{
      kDefaultMouseWheelLatchingTransaction};
  PointerEvent initial_mouse_wheel_event_;
  PointerEvent last_mouse_wheel_event_;
  Delegate* delegate_{nullptr};
  bool likely_scroll_x_{false};
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_MOUSE_WHEEL_PHASE_HANDLER_H_
