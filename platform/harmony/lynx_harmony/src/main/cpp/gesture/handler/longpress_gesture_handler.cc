// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/longpress_gesture_handler.h"

#include "base/include/thread/timed_task.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
LongPressGestureHandler::LongPressGestureHandler(
    int sign, LynxContext* lynx_context,
    std::shared_ptr<GestureDetector> gesture_detector,
    std::weak_ptr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, lynx_context, gesture_detector,
                         gesture_arena_member),
      max_distance_(10),
      min_duration_(500),
      start_x_(0),
      start_y_(0),
      last_x_(0),
      last_y_(0),
      is_invoked_end_(false),
      timer_task_manager_(std::make_unique<base::TimedTaskManager>()) {
  HandleConfigMap(gesture_detector->gesture_config());
}

void LongPressGestureHandler::HandleConfigMap(const lepus::Value& config) {
  if (config.Contains(GestureConstants::MIN_DURATION)) {
    min_duration_ = config.GetProperty(GestureConstants::MIN_DURATION).Number();
  }
  if (config.Contains(GestureConstants::MAX_DISTANCE)) {
    max_distance_ =
        Dip2Px(config.GetProperty(GestureConstants::MAX_DISTANCE).Number());
  }
}

void LongPressGestureHandler::OnHandle(
    const ArkUI_UIInputEvent* event,
    std::shared_ptr<TouchEvent> lynx_touch_event, float fling_delta_x,
    float fling_delta_y) {
  last_touch_event_ = lynx_touch_event;
  if (event == nullptr) {
    Ignore();
    return;
  }
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    EndLongPress();
    return;
  }

  switch (OH_ArkUI_UIInputEvent_GetAction(event)) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      start_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      start_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      is_invoked_end_ = false;
      Begin();
      OnBegin(start_x_, start_y_, lynx_touch_event);
      StartLongPress();
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      last_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      last_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      if (ShouldFail()) {
        Fail();
        EndLongPress();
      }
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      EndLongPress();
      Fail();
      break;
    default:
      break;
  }
}

void LongPressGestureHandler::Fail() {
  if (status_ != GestureConstants::LYNX_STATE_FAIL) {
    status_ = GestureConstants::LYNX_STATE_FAIL;
    OnEnd(last_x_, last_y_, last_touch_event_);
  }
}

void LongPressGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
    OnEnd(last_x_, last_y_, last_touch_event_);
  }
}

void LongPressGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_end_ = false;
}
void LongPressGestureHandler::StartLongPress() {
  auto task = [this]() {
    if (status_ != GestureConstants::LYNX_STATE_FAIL &&
        status_ != GestureConstants::LYNX_STATE_ACTIVE) {
      Activate();
      OnStart(last_x_, last_y_, last_touch_event_);
    }
  };
  last_task_id_ =
      timer_task_manager_->SetTimeout(std::move(task), min_duration_);
}

void LongPressGestureHandler::EndLongPress() {
  timer_task_manager_->StopTask(last_task_id_);
}

bool LongPressGestureHandler::ShouldFail() {
  float dx = std::abs(last_x_ - start_x_);
  float dy = std::abs(last_y_ - start_y_);
  return dx > max_distance_ || dy > max_distance_;
}

void LongPressGestureHandler::OnBegin(float x, float y,
                                      std::shared_ptr<TouchEvent> event) {
  if (!IsOnBeginEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_BEGIN,
                   GetEventParamsFromTouchEvent(event));
}

void LongPressGestureHandler::OnUpdate(float delta_x, float delta_y,
                                       std::shared_ptr<TouchEvent> event) {
  // empty implementation, because long press gesture is not continuous gesture
}

void LongPressGestureHandler::OnStart(float x, float y,
                                      std::shared_ptr<TouchEvent> event) {
  if (!IsOnStartEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_START,
                   GetEventParamsFromTouchEvent(event));
}

void LongPressGestureHandler::OnEnd(float x, float y,
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
