// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/default_gesture_handler.h"

#include <cmath>
#include <limits>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/gesture_manager.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/gesture_arena_member.h"

namespace clay {

DefaultGestureHandler::DefaultGestureHandler(
    int sign, PageView* page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : BaseGestureHandler(sign, page_view, gesture_detector,
                         std::move(gesture_arena_member)),
      is_invoked_begin_(false),
      is_invoked_start_(false),
      is_invoked_end_(false),
      tap_slop_(3) {
  HandleConfigMap(gesture_detector->config_map());
}

void DefaultGestureHandler::HandleConfigMap(const Value& config) {
  if (!config.IsMap()) return;
  const auto& map = config.GetMap();
  if (auto iter = map.find("tapSlop"); iter != map.end()) {
    if (iter->second.IsInt()) {
      tap_slop_ =
          page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetInt());
    } else if (iter->second.IsFloat()) {
      tap_slop_ =
          page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetFloat());
    } else if (iter->second.IsDouble()) {
      tap_slop_ =
          page_view_->ConvertFrom<kPixelTypeLogical>(iter->second.GetDouble());
    } else {
      FML_DLOG(ERROR) << "Unsupported type:" << iter->second.type()
                      << " for tapSlop";
    }
  }
}

void DefaultGestureHandler::OnHandle(
    const PointerEvent* pointer_event, float fling_delta_x, float fling_delta_y,
    bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  last_pointer_event_ = pointer_event;
  if (status_ >= GestureConstants::LYNX_STATE_FAIL) {
    OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
    return;
  }

  if (handle_by_simultaneous && extra_bundle != nullptr) {
    if (extra_bundle->IsNeedConsumedSimultaneousGesture()) {
      if (gesture_arena_member_ &&
          !page_view_->gesture_manager()->ShouldInterceptGesture()) {
        gesture_arena_member_->GestureScrollBy(
            extra_bundle->SimultaneousDeltaX(),
            extra_bundle->SimultaneousDeltaY());
      }
    }
    return;
  }

  if (pointer_event != nullptr) {
    PointerEvent::EventType type = pointer_event->type;
    if (type == PointerEvent::EventType::kDownEvent) {
      last_point_ = pointer_event->position;
      is_invoked_end_ = false;
      Begin();
      OnBegin(last_point_.x(), last_point_.y(), pointer_event);
    } else if (type == PointerEvent::EventType::kMoveEvent) {
      FloatPoint delta = last_point_ - pointer_event->position;
      if (delta.IsOrigin()) {
        return;
      }
      if (extra_bundle != nullptr &&
          extra_bundle->GestureDirection() ==
              GestureConstants::DIRECTION_UNDETERMINED) {
        extra_bundle->SetGestureDirection(
            std::fabs(delta.x()) > std::fabs(delta.y())
                ? GestureConstants::DIRECTION_HORIZONTAL
                : GestureConstants::DIRECTION_VERTICAL);
      }
      if (extra_bundle != nullptr &&
          extra_bundle->GestureDirection() !=
              GestureConstants::DIRECTION_UNDETERMINED) {
        if (extra_bundle->GestureDirection() ==
            GestureConstants::DIRECTION_HORIZONTAL) {
          delta.SetY(0);
        } else {
          delta.SetX(0);
        }
      }

      if (status_ == GestureConstants::LYNX_STATE_INIT) {
        OnBegin(last_point_.x(), last_point_.y(), pointer_event);
        Activate();
      } else {
        if (ShouldFail(delta.x(), delta.y(), extra_bundle)) {
          OnUpdate(delta.x(), delta.y(), pointer_event, extra_bundle);
          Fail();
          OnEnd(last_point_.x(), last_point_.y(), pointer_event);
        } else {
          Activate();
          OnUpdate(delta.x(), delta.y(), pointer_event, extra_bundle);
        }
      }
      last_point_ = pointer_event->position;
    } else if (type == PointerEvent::EventType::kUpEvent) {
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
    if (extra_bundle != nullptr &&
        extra_bundle->GestureDirection() !=
            GestureConstants::DIRECTION_UNDETERMINED) {
      if (extra_bundle->GestureDirection() ==
          GestureConstants::DIRECTION_HORIZONTAL) {
        fling_delta_y = 0;
      } else {
        fling_delta_x = 0;
      }
    }
    if (ShouldFail(fling_delta_x, fling_delta_y, extra_bundle)) {
      // consume last delta to arrive start or end
      auto member = gesture_arena_member_;
      if (member && !member->IsAtBorder(true) && !member->IsAtBorder(false)) {
        OnUpdate(fling_delta_x, fling_delta_y, nullptr, extra_bundle);
      }
      Fail();
      OnEnd(fling_delta_x, fling_delta_y, nullptr);
    } else {
      if (status_ == GestureConstants::LYNX_STATE_INIT) {
        OnBegin(last_point_.x(), last_point_.y(), pointer_event);
        Activate();
        return;
      }
      OnUpdate(fling_delta_x, fling_delta_y, nullptr, extra_bundle);
    }
  }
}

bool DefaultGestureHandler::ShouldFail(
    float delta_x, float delta_y,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  auto member = gesture_arena_member_;
  if (!member) {
    return true;
  }

  if (extra_bundle != nullptr) {
    if (extra_bundle->GestureDirection() ==
            GestureConstants::DIRECTION_HORIZONTAL &&
        member->GetScrollContainerDirection() !=
            GestureConstants::DIRECTION_HORIZONTAL) {
      return true;
    }
    if (extra_bundle->GestureDirection() ==
            GestureConstants::DIRECTION_VERTICAL &&
        member->GetScrollContainerDirection() !=
            GestureConstants::DIRECTION_VERTICAL) {
      return true;
    }
  }

  return !member->CanConsumeGesture(delta_x, delta_y);
}

void DefaultGestureHandler::Fail() {
  if (status_ != GestureConstants::LYNX_STATE_FAIL) {
    status_ = GestureConstants::LYNX_STATE_FAIL;
  }
  if (last_pointer_event_ == nullptr) {
    OnEnd(0, 0, nullptr);
  } else {
    OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
  }
}

void DefaultGestureHandler::End() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_END;
  }
  if (last_pointer_event_ == nullptr) {
    OnEnd(0, 0, nullptr);
  } else {
    OnEnd(last_point_.x(), last_point_.y(), last_pointer_event_);
  }
}

void DefaultGestureHandler::Reset() {
  BaseGestureHandler::Reset();
  last_point_ = {0, 0};
  is_invoked_begin_ = false;
  is_invoked_start_ = false;
  is_invoked_end_ = false;
}

Value DefaultGestureHandler::GenerateAdditionalEventParams(float delta_x,
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

void DefaultGestureHandler::OnBegin(float x, float y,
                                    const PointerEvent* pointer_event) {
  if (!IsOnBeginEnable() || is_invoked_begin_) {
    return;
  }
  is_invoked_begin_ = true;
  Value addition_params = GenerateAdditionalEventParams(x, y);
  SendGestureEvent(GestureConstants::ON_BEGIN, pointer_event, addition_params);
}

void DefaultGestureHandler::OnUpdate(
    float delta_x, float delta_y, const PointerEvent* pointer_event,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  auto member = gesture_arena_member_;
  if (member && !page_view_->gesture_manager()->ShouldInterceptGesture()) {
    member->GestureScrollBy(delta_x, delta_y);
  }
  if (extra_bundle != nullptr) {
    extra_bundle->SetIsNeedConsumedSimultaneousGesture(true);
    extra_bundle->SetSimultaneousDeltaX(delta_x);
    extra_bundle->SetSimultaneousDeltaY(delta_y);
  }

  if (std::abs(static_cast<int>(delta_x)) > tap_slop_ ||
      std::abs(static_cast<int>(delta_y)) > tap_slop_) {
    if (page_view_) {
      page_view_->OnGestureRecognizedWithSign(sign_);
    }
  }

  if (!IsOnUpdateEnable()) {
    return;
  }
  Value addition_params = GenerateAdditionalEventParams(delta_x, delta_y);
  SendGestureEvent(GestureConstants::ON_UPDATE, pointer_event, addition_params);
}

void DefaultGestureHandler::OnStart(float x, float y,
                                    const PointerEvent* pointer_event) {
  if (!IsOnStartEnable() || is_invoked_start_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_start_ = true;
  Value addition_params = GenerateAdditionalEventParams(x, y);
  SendGestureEvent(GestureConstants::ON_START, pointer_event, addition_params);
}

void DefaultGestureHandler::OnEnd(float x, float y,
                                  const PointerEvent* pointer_event) {
  if (!IsOnEndEnable() || is_invoked_end_ || !is_invoked_begin_) {
    return;
  }
  is_invoked_end_ = true;
  Value addition_params = GenerateAdditionalEventParams(x, y);
  SendGestureEvent(GestureConstants::ON_END, pointer_event, addition_params);
}

}  // namespace clay
