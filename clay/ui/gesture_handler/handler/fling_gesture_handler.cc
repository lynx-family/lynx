// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/fling_gesture_handler.h"

#include <limits>
#include <utility>

#include "clay/public/value.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"

namespace clay {

FlingGestureHandler::FlingGestureHandler(
    int sign, PageView* page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, page_view, gesture_detector,
                         gesture_arena_member),
      is_invoked_begin_(false),
      is_invoked_start_(false),
      is_invoked_end_(false) {
  HandleConfigMap(gesture_detector->config_map());
}

void FlingGestureHandler::HandleConfigMap(const Value& config) {}

void FlingGestureHandler::OnHandle(
    const PointerEvent* pointer_event, float fling_delta_x, float fling_delta_y,
    bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  if (pointer_event != nullptr &&
      (pointer_event->type == PointerEvent::EventType::kDownEvent ||
       pointer_event->type == PointerEvent::EventType::kMoveEvent)) {
    // If the event is not empty, it means the finger on the screen, no need to
    // handle fling gesture
    Ignore();
    return;
  }
  if (pointer_event != nullptr &&
      pointer_event->type == PointerEvent::EventType::kUpEvent) {
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

  OnUpdate(fling_delta_x, fling_delta_y, nullptr, extra_bundle);
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

Value FlingGestureHandler::GenerateAdditionalEventParams(float delta_x,
                                                         float delta_y) {
  Value::Map res;
  res["scrollX"] = Value(gesture_arena_member_->ScrollX());
  res["scrollY"] = Value(gesture_arena_member_->ScrollY());
  res["deltaX"] = Value(delta_x);
  res["deltaY"] = Value(delta_y);
  res["isAtStart"] = Value(gesture_arena_member_->IsAtBorder(true));
  res["isAtEnd"] = Value(gesture_arena_member_->IsAtBorder(false));
  return Value(std::move(res));
}

void FlingGestureHandler::OnBegin(float x, float y,
                                  const PointerEvent* pointer_event) {
  if (!IsOnBeginEnable() || is_invoked_begin_) {
    return;
  }
  is_invoked_begin_ = true;
  Value addition_params = GenerateAdditionalEventParams(x, y);
  SendGestureEvent(GestureConstants::ON_BEGIN, pointer_event, addition_params);
}

void FlingGestureHandler::OnUpdate(
    float delta_x, float delta_y, const PointerEvent* pointer_event,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  if (!IsOnUpdateEnable()) {
    return;
  }
  Value addition_params = GenerateAdditionalEventParams(delta_x, delta_y);
  SendGestureEvent(GestureConstants::ON_UPDATE, pointer_event, addition_params);
}

void FlingGestureHandler::OnStart(float x, float y,
                                  const PointerEvent* pointer_event) {
  if (!IsOnStartEnable() || is_invoked_start_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_start_ = true;
  Value addition_params = GenerateAdditionalEventParams(x, y);
  SendGestureEvent(GestureConstants::ON_START, pointer_event, addition_params);
}

void FlingGestureHandler::OnEnd(float x, float y,
                                const PointerEvent* pointer_event) {
  if (!IsOnEndEnable() || is_invoked_end_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_end_ = true;
  Value addition_params = GenerateAdditionalEventParams(x, y);
  SendGestureEvent(GestureConstants::ON_END, pointer_event, addition_params);
}

}  // namespace clay
