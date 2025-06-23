// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/tap_gesture_handler.h"

#include "base/include/thread/timed_task.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
TapGestureHandler::TapGestureHandler(
    int sign, LynxContext* lynx_context,
    std::shared_ptr<GestureDetector> gesture_detector,
    std::weak_ptr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, lynx_context, gesture_detector,
                         gesture_arena_member),
      max_distance_(10),
      max_duration_(500),
      start_x_(0),
      start_y_(0),
      last_x_(0),
      last_y_(0),
      is_invoked_end_(false),
      is_tap_active_(false),
      timer_task_manager_(std::make_unique<base::TimedTaskManager>()) {
  HandleConfigMap(gesture_detector->gesture_config());
}

void TapGestureHandler::HandleConfigMap(const lepus::Value& config) {
  if (config.Contains(GestureConstants::MAX_DURATION)) {
    max_duration_ = config.GetProperty(GestureConstants::MAX_DURATION).Number();
  }
  if (config.Contains(GestureConstants::MAX_DISTANCE)) {
    max_distance_ =
        Dip2Px(config.GetProperty(GestureConstants::MAX_DISTANCE).Number());
  }
}

void TapGestureHandler::OnHandle(const ArkUI_UIInputEvent* event,
                                 std::shared_ptr<TouchEvent> lynx_touch_event,
                                 float fling_delta_x, float fling_delta_y) {
  last_touch_event_ = lynx_touch_event;
  if (event == nullptr) {
    Ignore();
    EndTap();
    return;
  }
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    EndTap();
    return;
  }

  switch (OH_ArkUI_UIInputEvent_GetAction(event)) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      start_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      start_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      is_invoked_end_ = false;
      Begin();
      OnBegin(start_x_, start_y_, lynx_touch_event);
      StartTap();
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      last_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      last_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      if (ShouldFail()) {
        Fail();
      }
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      last_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      last_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
        Fail();
      } else {
        Activate();
        OnStart(last_x_, last_y_, lynx_touch_event);
        OnEnd(last_x_, last_y_, lynx_touch_event);
      }
      EndTap();
      break;
    default:
      break;
  }
}

void TapGestureHandler::Fail() {
  if (status_ != GestureConstants::LYNX_STATE_FAIL) {
    status_ = GestureConstants::LYNX_STATE_FAIL;
    OnEnd(last_x_, last_y_, last_touch_event_);
  }
}

void TapGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
    OnEnd(last_x_, last_y_, last_touch_event_);
  }
}

void TapGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_end_ = false;
}
void TapGestureHandler::StartTap() {
  is_tap_active_ = true;

  auto task = [this]() {
    if (is_tap_active_) {
      Fail();
    }
  };
  timer_task_manager_->SetTimeout(std::move(task), max_duration_);
}

void TapGestureHandler::EndTap() { is_tap_active_ = false; }

bool TapGestureHandler::ShouldFail() {
  float dx = std::abs(last_x_ - start_x_);
  float dy = std::abs(last_y_ - start_y_);
  return dx > max_distance_ || dy > max_distance_;
}

void TapGestureHandler::OnBegin(float x, float y,
                                std::shared_ptr<TouchEvent> event) {
  if (!IsOnBeginEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_BEGIN,
                   GetEventParamsFromTouchEvent(event));
}

void TapGestureHandler::OnUpdate(float delta_x, float delta_y,
                                 std::shared_ptr<TouchEvent> event) {}

void TapGestureHandler::OnStart(float x, float y,
                                std::shared_ptr<TouchEvent> event) {
  if (!IsOnStartEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_START,
                   GetEventParamsFromTouchEvent(event));
}

void TapGestureHandler::OnEnd(float x, float y,
                              std::shared_ptr<TouchEvent> event) {
  if (!IsOnEndEnable() || is_invoked_end_) {
    return;
  }
  is_invoked_end_ = true;
  SendGestureEvent(GestureConstants::ON_END,
                   GetEventParamsFromTouchEvent(event));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
