// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/default_gesture_handler.h"

#include <limits>

#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
DefaultGestureHandler::DefaultGestureHandler(
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

void DefaultGestureHandler::HandleConfigMap(const lepus::Value& config) {}

void DefaultGestureHandler::OnHandle(
    const ArkUI_UIInputEvent* event,
    std::shared_ptr<TouchEvent> lynx_touch_event, float fling_delta_x,
    float fling_delta_y) {
  last_touch_event_ = lynx_touch_event;
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    OnEnd(last_x_, last_y_, last_touch_event_);
    return;
  }

  if (event != nullptr) {
    int32_t type = OH_ArkUI_UIInputEvent_GetAction(event);
    if (type == UI_TOUCH_EVENT_ACTION_DOWN) {
      last_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      last_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      is_invoked_end_ = false;
      Begin();
      OnBegin(last_x_, last_y_, lynx_touch_event);
    } else if (type == UI_TOUCH_EVENT_ACTION_MOVE) {
      float delta_x = last_x_ - OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      float delta_y = last_y_ - OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
      if (status_ == GestureConstants::LYNX_STATE_INIT) {
        OnBegin(last_x_, last_y_, lynx_touch_event);
        Activate();
      } else {
        if (ShouldFail(delta_x, delta_y)) {
          OnUpdate(delta_x, delta_y, lynx_touch_event);
          Fail();
          OnEnd(last_x_, last_y_, lynx_touch_event);
        } else {
          Activate();
          OnUpdate(delta_x, delta_y, lynx_touch_event);
        }
      }
      last_x_ = OH_ArkUI_PointerEvent_GetXByIndex(event, 0);
      last_y_ = OH_ArkUI_PointerEvent_GetYByIndex(event, 0);
    } else if (type == UI_TOUCH_EVENT_ACTION_UP) {
      if (status_ == GestureConstants::LYNX_STATE_ACTIVE &&
          fling_delta_x == std::numeric_limits<float>::min() &&
          fling_delta_y == std::numeric_limits<float>::min()) {
        Fail();
        OnEnd(0, 0, nullptr);
      }
    }
  } else {
    if (status_ == GestureConstants::LYNX_STATE_ACTIVE &&
        fling_delta_x == std::numeric_limits<float>::min() &&
        fling_delta_y == std::numeric_limits<float>::min()) {
      Fail();
      OnEnd(0, 0, nullptr);
      return;
    }
    if (ShouldFail(fling_delta_x, fling_delta_y)) {
      OnUpdate(fling_delta_x, fling_delta_y, nullptr);
      Fail();
      OnEnd(fling_delta_x, fling_delta_y, nullptr);
    } else {
      if (status_ == GestureConstants::LYNX_STATE_INIT) {
        OnBegin(last_x_, last_y_, lynx_touch_event);
        Activate();
        return;
      }
      OnUpdate(fling_delta_x, fling_delta_y, nullptr);
    }
  }
}

bool DefaultGestureHandler::ShouldFail(float delta_x, float delta_y) {
  auto member = gesture_arena_member_.lock();
  if (!member) {
    return true;
  }
  return !member->CanConsumeGesture(delta_x, delta_y);
}

lepus::Value DefaultGestureHandler::GetEventParamsInActive(
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

void DefaultGestureHandler::Fail() {
  if (status_ != GestureConstants::LYNX_STATE_FAIL) {
    status_ = GestureConstants::LYNX_STATE_FAIL;
  }
  if (last_touch_event_ == nullptr) {
    OnEnd(0, 0, nullptr);
  } else {
    OnEnd(last_x_, last_y_, last_touch_event_);
  }
}

void DefaultGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
  }
  if (last_touch_event_ == nullptr) {
    OnEnd(0, 0, nullptr);
  } else {
    OnEnd(last_x_, last_y_, last_touch_event_);
  }
}

void DefaultGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  last_x_ = 0;
  last_y_ = 0;
  is_invoked_begin_ = false;
  is_invoked_start_ = false;
  is_invoked_end_ = false;
}

void DefaultGestureHandler::OnBegin(float x, float y,
                                    std::shared_ptr<TouchEvent> event) {
  if (!IsOnBeginEnable() || is_invoked_begin_) {
    return;
  }
  is_invoked_begin_ = true;
  SendGestureEvent(GestureConstants::ON_BEGIN,
                   GetEventParamsInActive(event, 0, 0));
}

void DefaultGestureHandler::OnUpdate(float delta_x, float delta_y,
                                     std::shared_ptr<TouchEvent> event) {
  auto member = gesture_arena_member_.lock();
  if (member != nullptr) {
    member->ScrollBy(delta_x, delta_y);
  }
  if (!IsOnUpdateEnable()) {
    return;
  }
  SendGestureEvent(GestureConstants::ON_UPDATE,
                   GetEventParamsInActive(event, delta_x, delta_y));
}

void DefaultGestureHandler::OnStart(float x, float y,
                                    std::shared_ptr<TouchEvent> event) {
  if (!IsOnStartEnable() || is_invoked_start_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_start_ = true;
  SendGestureEvent(GestureConstants::ON_START,
                   GetEventParamsInActive(event, x, y));
}

void DefaultGestureHandler::OnEnd(float x, float y,
                                  std::shared_ptr<TouchEvent> event) {
  if (!IsOnEndEnable() || is_invoked_end_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_end_ = true;
  SendGestureEvent(GestureConstants::ON_END,
                   GetEventParamsInActive(event, 0, 0));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
