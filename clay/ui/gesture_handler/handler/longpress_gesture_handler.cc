// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/longpress_gesture_handler.h"

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/timer.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

LongPressGestureHandler::LongPressGestureHandler(
    int sign, PageView* page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, page_view, gesture_detector,
                         gesture_arena_member),
      max_distance_(10),
      min_duration_(500),
      start_point_{0, 0},
      last_point_{0, 0},
      is_invoked_end_(false),
      timer_(fml::OneshotTimer(page_view->GetTaskRunner())) {
  HandleConfigMap(gesture_detector->config_map());
}

void LongPressGestureHandler::HandleConfigMap(const Value& config) {
  if (!config.IsMap()) return;
  const auto& map = config.GetMap();
  if (auto iter = map.find(GestureConstants::MIN_DURATION); iter != map.end()) {
    min_duration_ = iter->second.GetLong();
  }
  if (auto iter = map.find(GestureConstants::MAX_DISTANCE); iter != map.end()) {
    if (iter->second.IsInt()) {
      max_distance_ =
          page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetInt());
    } else if (iter->second.IsFloat()) {
      max_distance_ =
          page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetFloat());
    } else if (iter->second.IsDouble()) {
      max_distance_ =
          page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetDouble());
    } else {
      FML_DLOG(ERROR) << "Unsupported type:" << iter->second.type()
                      << " for maxDistance";
    }
  }
}

void LongPressGestureHandler::OnHandle(
    const PointerEvent* pointer_event, float fling_delta_x, float fling_delta_y,
    bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  last_pointer_event_ = pointer_event;
  if (pointer_event == nullptr) {
    Ignore();
    return;
  }
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    EndLongPress();
    return;
  }

  switch (pointer_event->type) {
    case PointerEvent::EventType::kDownEvent:
      start_point_ = pointer_event->position;
      is_invoked_end_ = false;
      Begin();
      OnBegin(start_point_.x(), start_point_.y(), pointer_event);
      StartLongPress();
      break;
    case PointerEvent::EventType::kMoveEvent:
      last_point_ = pointer_event->position;
      if (ShouldFail()) {
        Fail();
        EndLongPress();
      }
      break;
    case PointerEvent::EventType::kUpEvent:
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
    OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
  }
}

void LongPressGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
    OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
  }
}

void LongPressGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  is_invoked_end_ = false;
}
void LongPressGestureHandler::StartLongPress() {
  timer_.Start(fml::TimeDelta::FromMilliseconds(min_duration_), [this]() {
    if (status_ != GestureConstants::LYNX_STATE_FAIL &&
        status_ != GestureConstants::LYNX_STATE_ACTIVE) {
      Activate();
      OnStart(last_point_.x(), last_point_.y(), last_pointer_event_);
    }
  });
}

void LongPressGestureHandler::EndLongPress() { timer_.Stop(); }

bool LongPressGestureHandler::ShouldFail() {
  FloatPoint delta = last_point_ - start_point_;
  float dx = std::abs(delta.x());
  float dy = std::abs(delta.y());
  return dx > max_distance_ || dy > max_distance_;
}

void LongPressGestureHandler::OnBegin(float x, float y,
                                      const PointerEvent* pointer_event) {
  if (!IsOnBeginEnable()) {
    return;
  }
  Value empty;
  SendGestureEvent(GestureConstants::ON_BEGIN, pointer_event, empty);
}

void LongPressGestureHandler::OnUpdate(
    float delta_x, float delta_y, const PointerEvent* pointer_event,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  // empty implementation, because long press gesture is not continuous gesture
}

void LongPressGestureHandler::OnStart(float x, float y,
                                      const PointerEvent* pointer_event) {
  if (!IsOnStartEnable()) {
    return;
  }
  Value empty;
  SendGestureEvent(GestureConstants::ON_START, pointer_event, empty);
}

void LongPressGestureHandler::OnEnd(float x, float y,
                                    const PointerEvent* pointer_event) {
  if (!IsOnEndEnable() || is_invoked_end_) {
    return;
  }
  is_invoked_end_ = true;
  Value empty;
  SendGestureEvent(GestureConstants::ON_END, pointer_event, empty);
}

}  // namespace clay
