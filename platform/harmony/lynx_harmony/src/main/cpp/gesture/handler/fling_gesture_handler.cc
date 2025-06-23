// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/fling_gesture_handler.h"

#include <limits>

#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
FlingGestureHandler::FlingGestureHandler(
    int sign, LynxContext* lynx_context,
    std::shared_ptr<GestureDetector> gesture_detector,
    std::weak_ptr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, lynx_context, gesture_detector,
                         gesture_arena_member),

      is_invoked_begin_(false),
      is_invoked_start_(false),
      is_invoked_end_(false) {
  HandleConfigMap(gesture_detector->gesture_config());
}

void FlingGestureHandler::HandleConfigMap(const lepus::Value& config) {}

void FlingGestureHandler::OnHandle(const ArkUI_UIInputEvent* event,
                                   std::shared_ptr<TouchEvent> lynx_touch_event,
                                   float fling_delta_x, float fling_delta_y) {
  int32_t type = OH_ArkUI_UIInputEvent_GetAction(event);

  if (event != nullptr && (type == UI_TOUCH_EVENT_ACTION_DOWN ||
                           type == UI_TOUCH_EVENT_ACTION_MOVE)) {
    // If the event is not empty, it means the finger on the screen, no need to
    // handle fling gesture
    Ignore();
    return;
  }
  if (event != nullptr && type == UI_TOUCH_EVENT_ACTION_UP) {
    Begin();
    OnBegin(0, 0, nullptr);
    return;
  }

  if (status_ >= GestureConstants::LYNX_STATE_FAIL &&
      status_ <= GestureConstants::LYNX_STATE_END) {
    OnEnd(0, 0, nullptr);
    return;
  }

  if (fling_delta_x == std::numeric_limits<float>::min() &&
      fling_delta_y == std::numeric_limits<float>::min()) {
    Fail();
    OnEnd(0, 0, nullptr);
    return;
  }

  if (status_ == GestureConstants::LYNX_STATE_INIT ||
      status_ == GestureConstants::LYNX_STATE_UNDETERMINED) {
    Begin();
    Activate();
    OnBegin(0, 0, nullptr);
    OnStart(0, 0, nullptr);
    return;
  }

  OnUpdate(fling_delta_x, fling_delta_y, nullptr);
}

lepus::Value FlingGestureHandler::GetEventParamsInActive(
    const std::shared_ptr<TouchEvent>& touch_event, float delta_x,
    float delta_y) {
  auto event_params_ = lepus::Dictionary::Create();

  event_params_->SetValue("scrollX", gesture_arena_member_.lock()->ScrollX());
  event_params_->SetValue("scrollY", gesture_arena_member_.lock()->ScrollY());
  event_params_->SetValue("deltaX", delta_x);
  event_params_->SetValue("deltaY", delta_y);
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

void FlingGestureHandler::Fail() {
  if (status_ != GestureConstants::LYNX_STATE_FAIL) {
    status_ = GestureConstants::LYNX_STATE_FAIL;
    OnEnd(0, 0, nullptr);
  }
}

void FlingGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
    OnEnd(0, 0, nullptr);
  }
}

void FlingGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_begin_ = false;
  is_invoked_start_ = false;
  is_invoked_end_ = false;
}

void FlingGestureHandler::OnBegin(float x, float y,
                                  std::shared_ptr<TouchEvent> event) {
  if (!IsOnBeginEnable() || is_invoked_begin_) {
    return;
  }
  is_invoked_begin_ = true;
  SendGestureEvent(GestureConstants::ON_BEGIN,
                   GetEventParamsInActive(event, x, y));
}

void FlingGestureHandler::OnUpdate(float delta_x, float delta_y,
                                   std::shared_ptr<TouchEvent> event) {
  if (!IsOnUpdateEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_UPDATE,
                   GetEventParamsInActive(event, delta_x, delta_y));
}

void FlingGestureHandler::OnStart(float x, float y,
                                  std::shared_ptr<TouchEvent> event) {
  if (!IsOnStartEnable() || is_invoked_start_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_start_ = true;
  SendGestureEvent(GestureConstants::ON_START,
                   GetEventParamsInActive(event, x, y));
}

void FlingGestureHandler::OnEnd(float x, float y,
                                std::shared_ptr<TouchEvent> event) {
  if (!IsOnEndEnable() || is_invoked_end_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_end_ = true;
  SendGestureEvent(GestureConstants::ON_END,
                   GetEventParamsInActive(event, x, y));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
