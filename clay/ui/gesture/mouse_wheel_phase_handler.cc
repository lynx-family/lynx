// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/mouse_wheel_phase_handler.h"

#include "clay/fml/logging.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/macros.h"

namespace clay {

constexpr float kMouseWheelLatchingSlopDistance = 10.0;

MouseWheelPhaseHandler::MouseWheelPhaseHandler(
    fml::RefPtr<fml::TaskRunner> task_runner, Delegate* delegate)
    : mouse_wheel_end_dispatcher_timer_(task_runner), delegate_(delegate) {}

void MouseWheelPhaseHandler::UpdatePhaseAndScheduleEndEvent(
    PointerEvent& mouse_wheel_event) {
  FML_DCHECK(mouse_wheel_event.type == PointerEvent::EventType::kSignalEvent);

  if (mouse_wheel_end_dispatcher_timer_.Stopped()) {
    GESTURE_LOG << "mouse wheel transaction begin";
    mouse_wheel_event.signal_kind = PointerEvent::SignalKind::kStartScroll;
    if (std::abs(mouse_wheel_event.scroll_delta_x) >
        std::abs(mouse_wheel_event.scroll_delta_y)) {
      likely_scroll_x_ = true;
      mouse_wheel_event.scroll_delta_y = 0;
    } else {
      likely_scroll_x_ = false;
      mouse_wheel_event.scroll_delta_x = 0;
    }
    initial_mouse_wheel_event_ = mouse_wheel_event;
    ScheduleMouseWheelEndDispatching(mouse_wheel_end_dispatch_timeout_);
  } else {  // !Stopped
    mouse_wheel_event.signal_kind = PointerEvent::SignalKind::kScroll;
    if (likely_scroll_x_) {
      mouse_wheel_event.scroll_delta_y = 0;
    } else {
      mouse_wheel_event.scroll_delta_x = 0;
    }
    ScheduleMouseWheelEndDispatching(mouse_wheel_end_dispatch_timeout_);
  }
  last_mouse_wheel_event_ = mouse_wheel_event;
}

void MouseWheelPhaseHandler::DispatchWheelEndEventIfNeeded(
    const PointerEvent& mouse_event, bool force) {
  if (force) {
    GESTURE_LOG << "mouse wheel transaction end by force";
    mouse_wheel_end_dispatcher_timer_.FireImmediately();
  } else if (!mouse_wheel_end_dispatcher_timer_.Stopped() &&
             mouse_event.type == PointerEvent::EventType::kHoverEvent &&
             !IsWithinSlopRegion(mouse_event)) {
    FML_DCHECK(mouse_event.device == PointerEvent::DeviceType::kMouse);
    GESTURE_LOG << "mouse wheel transaction end due to move too much";
    mouse_wheel_end_dispatcher_timer_.FireImmediately();
  }
}

void MouseWheelPhaseHandler::ScheduleMouseWheelEndDispatching(
    fml::TimeDelta timeout) {
  mouse_wheel_end_dispatcher_timer_.Start(
      timeout, [this] { SendSyntheticWheelEventWithPhaseEnd(); });
}

void MouseWheelPhaseHandler::SendSyntheticWheelEventWithPhaseEnd() {
  GESTURE_LOG << "mouse wheel transaction end";
  PointerEvent event = last_mouse_wheel_event_;
  event.signal_kind = PointerEvent::SignalKind::kEndScroll;
  event.synthesized = true;
  event.scroll_delta_x = 0;
  event.scroll_delta_y = 0;
  if (delegate_) {
    delegate_->SendSyntheticWheelEventWithPhaseEnd(event);
  }
  last_mouse_wheel_event_ = {};
}

bool MouseWheelPhaseHandler::IsWithinSlopRegion(
    const PointerEvent& mouse_hover_event) {
  return mouse_hover_event.pan_delta.distance() <=
         kMouseWheelLatchingSlopDistance;
}

}  // namespace clay
