// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/tap_gesture_handler.h"

#include "base/include/fml/time/time_delta.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

TapGestureHandler::TapGestureHandler(
    int sign, PageView* page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, page_view, gesture_detector,
                         gesture_arena_member),
      max_distance_(10),
      max_duration_(500),
      start_point_{0, 0},
      last_point_{0, 0},
      is_invoked_end_(false),
      is_tap_active_(false),
      timer_(fml::OneshotTimer(page_view->GetTaskRunner())) {
  HandleConfigMap(gesture_detector->config_map());
}

void TapGestureHandler::HandleConfigMap(const Value& config) {
  if (!config.IsMap()) return;
  const auto& map = config.GetMap();
  if (auto iter = map.find(GestureConstants::MAX_DURATION); iter != map.end()) {
    max_duration_ = iter->second.GetLong();
  }
  if (auto iter = map.find(GestureConstants::MAX_DISTANCE); iter != map.end()) {
    max_distance_ =
        page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetFloat());
  }
}

void TapGestureHandler::OnHandle(
    const PointerEvent* pointer_event, float fling_delta_x, float fling_delta_y,
    bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  last_point_event_ = pointer_event;
  if (pointer_event == nullptr) {
    Ignore();
    EndTap();
    return;
  }
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    EndTap();
    return;
  }

  switch (pointer_event->type) {
    case PointerEvent::EventType::kDownEvent:
      start_point_ = pointer_event->position;
      is_invoked_end_ = false;
      Begin();
      OnBegin(start_point_.x(), start_point_.y(), pointer_event);
      StartTap();
      break;
    case PointerEvent::EventType::kMoveEvent:
      last_point_ = pointer_event->position;
      if (ShouldFail()) {
        Fail();
      }
      break;
    case PointerEvent::EventType::kUpEvent:
      last_point_ = pointer_event->position;
      if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
        Fail();
      } else {
        Activate();
        OnStart(last_point_.x(), last_point_.y(), pointer_event);
        OnEnd(last_point_.x(), last_point_.y(), pointer_event);
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
    OnEnd(last_point_.x(), last_point_.y(), last_point_event_);
  }
}

void TapGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
    OnEnd(last_point_.x(), last_point_.y(), last_point_event_);
  }
}

void TapGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_end_ = false;
}
void TapGestureHandler::StartTap() {
  is_tap_active_ = true;
  timer_.Start(fml::TimeDelta::FromMilliseconds(max_duration_), [this]() {
    if (is_tap_active_) {
      Fail();
    }
  });
}

void TapGestureHandler::EndTap() { is_tap_active_ = false; }

bool TapGestureHandler::ShouldFail() {
  FloatPoint delta = last_point_ - start_point_;
  return std::abs(delta.x()) > max_distance_ ||
         std::abs(delta.y()) > max_distance_;
}

void TapGestureHandler::OnBegin(float x, float y,
                                const PointerEvent* pointer_event) {
  if (!IsOnBeginEnable()) {
    return;
  }
  Value empty;
  SendGestureEvent(GestureConstants::ON_BEGIN, pointer_event, empty);
}

void TapGestureHandler::OnUpdate(
    float delta_x, float delta_y, const PointerEvent* pointer_event,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {}

void TapGestureHandler::OnStart(float x, float y,
                                const PointerEvent* pointer_event) {
  if (!IsOnStartEnable()) {
    return;
  }
  Value empty;
  SendGestureEvent(GestureConstants::ON_START, pointer_event, empty);
}

void TapGestureHandler::OnEnd(float x, float y,
                              const PointerEvent* pointer_event) {
  if (!IsOnEndEnable() || is_invoked_end_) {
    return;
  }
  is_invoked_end_ = true;
  Value empty;
  SendGestureEvent(GestureConstants::ON_END, pointer_event, empty);
}

}  // namespace clay
