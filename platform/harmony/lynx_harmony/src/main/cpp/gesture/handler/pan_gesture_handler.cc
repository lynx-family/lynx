// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/pan_gesture_handler.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
PanGestureHandler::PanGestureHandler(
    int sign, LynxContext* lynx_context,
    std::shared_ptr<GestureDetector> gesture_detector,
    std::weak_ptr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, lynx_context, gesture_detector,
                         gesture_arena_member),
      min_distance_(0),
      start_x_(0),
      start_y_(0),
      last_x_(0),
      last_y_(0),
      is_invoked_begin_(false),
      is_invoked_start_(false),
      is_invoked_end_(false) {
  HandleConfigMap(gesture_detector->gesture_config());
}

void PanGestureHandler::HandleConfigMap(const lepus::Value& config) {
  if (config.Contains(GestureConstants::MIN_DISTANCE)) {
    min_distance_ =
        Dip2Px(config.GetProperty(GestureConstants::MIN_DISTANCE).Number());
  }
}

void PanGestureHandler::OnHandle(const ArkUI_UIInputEvent* event,
                                 std::shared_ptr<TouchEvent> lynx_touch_event,
                                 float fling_delta_x, float fling_delta_y) {
  last_touch_event_ = lynx_touch_event;
  if (event == nullptr) {
    Ignore();
    return;
  }
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    return;
  }

  switch (OH_ArkUI_UIInputEvent_GetAction(event)) {
    case UI_TOUCH_EVENT_ACTION_DOWN:
      start_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      start_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      is_invoked_end_ = false;
      Begin();
      OnBegin(start_x_, start_y_, lynx_touch_event);
      break;
    case UI_TOUCH_EVENT_ACTION_MOVE:
      last_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      last_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      if (status_ == GestureConstants::LYNX_STATE_INIT) {
        Begin();
        OnBegin(last_x_, last_y_, lynx_touch_event);
      }
      if (ShouldActivate()) {
        OnStart(last_x_, start_y_, lynx_touch_event);
        Activate();
      }
      if (status_ == GestureConstants::LYNX_STATE_ACTIVE) {
        OnUpdate(last_x_, last_y_, lynx_touch_event);
      } else if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
        OnEnd(last_x_, last_y_, lynx_touch_event);
      }
      break;
    case UI_TOUCH_EVENT_ACTION_UP:
      Fail();
      OnEnd(last_x_, last_y_, lynx_touch_event);
      break;
    default:
      break;
  }
}

bool PanGestureHandler::ShouldActivate() {
  float dx = std::abs(last_x_ - start_x_);
  float dy = std::abs(last_y_ - start_y_);
  return dx > min_distance_ || dy > min_distance_;
}

void PanGestureHandler::Fail() {
  BaseGestureHandler::Fail();
  OnEnd(last_x_, last_y_, last_touch_event_);
}

void PanGestureHandler::End() {
  BaseGestureHandler::End();
  OnEnd(last_x_, last_y_, last_touch_event_);
}

void PanGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_begin_ = false;
  is_invoked_start_ = false;
  is_invoked_end_ = false;
}

lepus::Value PanGestureHandler::GetEventParamsInActive(
    const std::shared_ptr<TouchEvent>& touch_event) {
  auto event_params_ = lepus::Dictionary::Create();

  event_params_->SetValue("scrollX", gesture_arena_member_.lock()->ScrollX());
  event_params_->SetValue("scrollY", gesture_arena_member_.lock()->ScrollY());
  event_params_->SetValue("isAtStart",
                          gesture_arena_member_.lock()->IsAtBorder(true));
  event_params_->SetValue("isAtEnd",
                          gesture_arena_member_.lock()->IsAtBorder(false));
  auto additional_params = GetEventParamsFromTouchEvent(touch_event);
  for (auto& param : *additional_params.Table()) {
    event_params_->SetValue(param.first, std::move(param.second));
  }
  return lepus::Value(std::move(event_params_));
}

void PanGestureHandler::OnBegin(float x, float y,
                                std::shared_ptr<TouchEvent> event) {
  if (!IsOnBeginEnable() || is_invoked_begin_) {
    return;
  }
  is_invoked_begin_ = true;
  SendGestureEvent(GestureConstants::ON_BEGIN, GetEventParamsInActive(event));
}

void PanGestureHandler::OnUpdate(float delta_x, float delta_y,
                                 std::shared_ptr<TouchEvent> event) {
  if (!IsOnUpdateEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_UPDATE, GetEventParamsInActive(event));
}

void PanGestureHandler::OnStart(float x, float y,
                                std::shared_ptr<TouchEvent> event) {
  if (!IsOnStartEnable() || is_invoked_start_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_start_ = true;
  SendGestureEvent(GestureConstants::ON_START, GetEventParamsInActive(event));
}

void PanGestureHandler::OnEnd(float x, float y,
                              std::shared_ptr<TouchEvent> event) {
  if (!IsOnEndEnable() || is_invoked_end_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_end_ = true;
  SendGestureEvent(GestureConstants::ON_END, GetEventParamsInActive(event));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
