// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

#include <utility>

#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/handler/default_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/fling_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/longpress_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/native_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/tap_gesture_handler.h"

namespace clay {

BaseGestureHandler::BaseGestureHandler(
    int sign, PageView* page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : sign_(sign),
      status_(GestureConstants::LYNX_STATE_INIT),
      gesture_detector_(std::move(gesture_detector)),
      gesture_arena_member_(std::move(gesture_arena_member)),
      page_view_(page_view) {
  enable_flags_ = {{GestureConstants::ON_TOUCHES_DOWN, false},
                   {GestureConstants::ON_TOUCHES_MOVE, false},
                   {GestureConstants::ON_TOUCHES_UP, false},
                   {GestureConstants::ON_TOUCHES_CANCEL, false},
                   {GestureConstants::ON_BEGIN, false},
                   {GestureConstants::ON_UPDATE, false},
                   {GestureConstants::ON_START, false},
                   {GestureConstants::ON_END, false}};
  HandleEnableGestureCallback(gesture_detector_->GestureCallbackNames());
}

GestureHandlerMap BaseGestureHandler::ConvertToGestureHandler(
    int sign, PageView* page_view, fml::WeakPtr<GestureArenaMember> member,
    const GestureMap& gesture_detectors) {
  GestureHandlerMap gesture_handler_map;

  for (const auto& pair : gesture_detectors) {
    auto detector = pair.second;
    if (!detector) continue;

    if (detector->gesture_type() == GestureHandlerType::Pan) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<PanGestureHandler>(
                                      sign, page_view, detector, member));
    } else if (detector->gesture_type() == GestureHandlerType::Tap) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<TapGestureHandler>(
                                      sign, page_view, detector, member));
    } else if (detector->gesture_type() == GestureHandlerType::LongPress) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<LongPressGestureHandler>(
                                      sign, page_view, detector, member));
    } else if (detector->gesture_type() == GestureHandlerType::Fling) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<FlingGestureHandler>(
                                      sign, page_view, detector, member));
    } else if (detector->gesture_type() == GestureHandlerType::Default) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<DefaultGestureHandler>(
                                      sign, page_view, detector, member));
    } else if (detector->gesture_type() == GestureHandlerType::Native) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<NativeGestureHandler>(
                                      sign, page_view, detector, member));
    }
  }

  return gesture_handler_map;
}

void BaseGestureHandler::HandleEnableGestureCallback(
    const std::vector<std::string>& callback_names) {
  if (callback_names.empty()) return;

  for (const auto& callback_name : callback_names) {
    if (enable_flags_.find(callback_name) != enable_flags_.end()) {
      enable_flags_[callback_name] = true;
    }
  }
}

void BaseGestureHandler::HandleMotionEvent(
    const PointerEvent* pointer_event, float delta_x, float delta_y,
    bool handle_by_simultaneous,
    const std::shared_ptr<GestureExtraBundle>& extra_bundle) {
  OnHandle(pointer_event, delta_x, delta_y, handle_by_simultaneous,
           extra_bundle);
}

bool BaseGestureHandler::IsEnd() const {
  return status_ == GestureConstants::LYNX_STATE_END;
}

bool BaseGestureHandler::IsActive() const {
  return status_ == GestureConstants::LYNX_STATE_ACTIVE;
}

int BaseGestureHandler::GetGestureStatus() const { return status_; }

void BaseGestureHandler::SendGestureEvent(const std::string& event_name,
                                          const PointerEvent* pointer_event,
                                          Value& additional_params) {
  if (!gesture_detector_) return;
  page_view_->HandleGestureEvent(sign_, gesture_detector_->gesture_id(),
                                 event_name, pointer_event, additional_params);
}

bool BaseGestureHandler::IsOnBeginEnable() const {
  return enable_flags_.at(GestureConstants::ON_BEGIN);
}

bool BaseGestureHandler::IsOnUpdateEnable() const {
  return enable_flags_.at(GestureConstants::ON_UPDATE);
}

bool BaseGestureHandler::IsOnStartEnable() const {
  return enable_flags_.at(GestureConstants::ON_START);
}

bool BaseGestureHandler::IsOnEndEnable() const {
  return enable_flags_.at(GestureConstants::ON_END);
}

void BaseGestureHandler::Activate() {
  status_ = GestureConstants::LYNX_STATE_ACTIVE;
}

void BaseGestureHandler::Reset() {
  status_ = GestureConstants::LYNX_STATE_INIT;
}

void BaseGestureHandler::Fail() {
  if (status_ != GestureConstants::LYNX_STATE_END) {
    status_ = GestureConstants::LYNX_STATE_FAIL;
  }
}

void BaseGestureHandler::Begin() {
  status_ = GestureConstants::LYNX_STATE_BEGIN;
}

void BaseGestureHandler::Ignore() {
  status_ = GestureConstants::LYNX_STATE_UNDETERMINED;
}

void BaseGestureHandler::End() { status_ = GestureConstants::LYNX_STATE_END; }

void BaseGestureHandler::OnTouchesDown(const PointerEvent* pointer_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_DOWN)) {
    Value empty;
    SendGestureEvent(GestureConstants::ON_TOUCHES_DOWN, pointer_event, empty);
  }
}

void BaseGestureHandler::OnTouchesMove(const PointerEvent* pointer_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_MOVE)) {
    Value empty;
    SendGestureEvent(GestureConstants::ON_TOUCHES_MOVE, pointer_event, empty);
  }
}

void BaseGestureHandler::OnTouchesUp(const PointerEvent* pointer_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_UP)) {
    Value empty;
    SendGestureEvent(GestureConstants::ON_TOUCHES_UP, pointer_event, empty);
  }
}

void BaseGestureHandler::OnTouchesCancel(const PointerEvent* pointer_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_CANCEL)) {
    Value empty;
    SendGestureEvent(GestureConstants::ON_TOUCHES_CANCEL, pointer_event, empty);
  }
}

std::shared_ptr<GestureDetector> BaseGestureHandler::GetGestureDetector()
    const {
  return gesture_detector_;
}

}  // namespace clay
