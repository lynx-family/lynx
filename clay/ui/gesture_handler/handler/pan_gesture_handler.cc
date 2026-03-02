// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/pixel_helper.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

PanGestureHandler::PanGestureHandler(
    int sign, PageView* page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, page_view, gesture_detector,
                         std::move(gesture_arena_member)),
      min_distance_(0),
      start_point_{0, 0},
      last_point_{0, 0},
      is_invoked_begin_(false),
      is_invoked_start_(false),
      is_invoked_end_(false) {
  HandleConfigMap(gesture_detector->config_map());
}

void PanGestureHandler::HandleConfigMap(const Value& config) {
  if (!config.IsMap()) return;
  const auto& map = config.GetMap();
  if (auto iter = map.find(GestureConstants::MIN_DISTANCE); iter != map.end()) {
    min_distance_ =
        page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetFloat());
  }
}

void PanGestureHandler::OnHandle(
    const PointerEvent* pointer_event, float fling_delta_x, float fling_delta_y,
    bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  last_pointer_event_ = pointer_event;
  if (pointer_event == nullptr) {
    Ignore();
    return;
  }
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    return;
  }

  switch (pointer_event->type) {
    case PointerEvent::EventType::kDownEvent:
      start_point_ = pointer_event->position;
      is_invoked_end_ = false;
      Begin();
      OnBegin(start_point_.x(), start_point_.y(), pointer_event);
      break;
    case PointerEvent::EventType::kMoveEvent:
      last_point_ = pointer_event->position;
      if (status_ == GestureConstants::LYNX_STATE_INIT) {
        Begin();
        OnBegin(last_point_.x(), last_point_.y(), pointer_event);
      }
      if (ShouldActivate()) {
        Activate();
        OnStart(last_point_.x(), last_point_.y(), pointer_event);
      }
      if (status_ == GestureConstants::LYNX_STATE_ACTIVE) {
        OnUpdate(last_point_.x(), last_point_.y(), pointer_event, extra_bundle);
      } else if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
        OnEnd(last_point_.x(), last_point_.y(), pointer_event);
      }
      break;
    case PointerEvent::EventType::kUpEvent:
      Fail();
      OnEnd(last_point_.x(), last_point_.y(), pointer_event);
      break;
    default:
      break;
  }
}

bool PanGestureHandler::ShouldActivate() {
  FloatPoint delta = last_point_ - start_point_;
  return std::abs(delta.x()) > min_distance_ ||
         std::abs(delta.y()) > min_distance_;
}

void PanGestureHandler::Fail() {
  BaseGestureHandler::Fail();
  OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
}

void PanGestureHandler::End() {
  BaseGestureHandler::End();
  OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
}

void PanGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_begin_ = false;
  is_invoked_start_ = false;
  is_invoked_end_ = false;
}

Value PanGestureHandler::GenerateAdditionalEventParams() {
  Value::Map res;
  res["scrollX"] = Value(gesture_arena_member_->ScrollX());
  res["scrollY"] = Value(gesture_arena_member_->ScrollY());
  res["isAtStart"] = Value(gesture_arena_member_->IsAtBorder(true));
  res["isAtEnd"] = Value(gesture_arena_member_->IsAtBorder(false));
  return Value(std::move(res));
}

void PanGestureHandler::OnBegin(float x, float y,
                                const PointerEvent* pointer_event) {
  if (!IsOnBeginEnable() || is_invoked_begin_) {
    return;
  }
  is_invoked_begin_ = true;
  Value additional_params = GenerateAdditionalEventParams();
  SendGestureEvent(GestureConstants::ON_BEGIN, pointer_event,
                   additional_params);
}

void PanGestureHandler::OnUpdate(
    float delta_x, float delta_y, const PointerEvent* pointer_event,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  if (!IsOnUpdateEnable()) {
    return;
  }
  Value additional_params = GenerateAdditionalEventParams();
  SendGestureEvent(GestureConstants::ON_UPDATE, pointer_event,
                   additional_params);
}

void PanGestureHandler::OnStart(float x, float y,
                                const PointerEvent* pointer_event) {
  if (!IsOnStartEnable() || is_invoked_start_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_start_ = true;
  Value additional_params = GenerateAdditionalEventParams();
  SendGestureEvent(GestureConstants::ON_START, pointer_event,
                   additional_params);
}

void PanGestureHandler::OnEnd(float x, float y,
                              const PointerEvent* pointer_event) {
  if (!IsOnEndEnable() || is_invoked_end_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_end_ = true;
  Value additional_params = GenerateAdditionalEventParams();
  SendGestureEvent(GestureConstants::ON_END, pointer_event, additional_params);
}

}  // namespace clay
