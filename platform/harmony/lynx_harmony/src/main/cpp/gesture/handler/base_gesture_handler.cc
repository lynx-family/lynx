// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/base_gesture_handler.h"

#include "base/include/thread/timed_task.h"
#include "core/renderer/events/gesture.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/default_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/fling_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/longpress_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/native_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/pan_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/tap_gesture_handler.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {

BaseGestureHandler::BaseGestureHandler(
    int sign, LynxContext* lynx_context,
    std::shared_ptr<GestureDetector> gesture_detector,
    std::weak_ptr<GestureArenaMember> gesture_arena_member)
    : sign_(sign),
      status_(GestureConstants::LYNX_STATE_INIT),
      lynx_context_(std::move(lynx_context)),
      gesture_detector_(std::move(gesture_detector)),
      gesture_arena_member_(std::move(gesture_arena_member)) {
  enable_flags_ = {{GestureConstants::ON_TOUCHES_DOWN, false},
                   {GestureConstants::ON_TOUCHES_MOVE, false},
                   {GestureConstants::ON_TOUCHES_UP, false},
                   {GestureConstants::ON_TOUCHES_CANCEL, false},
                   {GestureConstants::ON_BEGIN, false},
                   {GestureConstants::ON_UPDATE, false},
                   {GestureConstants::ON_START, false},
                   {GestureConstants::ON_END, false}};
  HandleEnableGestureCallback(gesture_detector_.get()->gesture_callbacks());
}

GestureHandlerMap BaseGestureHandler::ConvertToGestureHandler(
    int sign, LynxContext* lynx_context,
    std::weak_ptr<GestureArenaMember> member,
    const GestureMap& gesture_detectors) {
  GestureHandlerMap gesture_handler_map;

  for (const auto& pair : gesture_detectors) {
    auto detector = pair.second;
    if (!detector) continue;

    if (detector->gesture_type() == GestureType::PAN) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<PanGestureHandler>(
                                      sign, lynx_context, detector, member));
    } else if (detector->gesture_type() == GestureType::TAP) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<TapGestureHandler>(
                                      sign, lynx_context, detector, member));
    } else if (detector->gesture_type() == GestureType::LONG_PRESS) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<LongPressGestureHandler>(
                                      sign, lynx_context, detector, member));
    } else if (detector->gesture_type() == GestureType::FLING) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<FlingGestureHandler>(
                                      sign, lynx_context, detector, member));
    } else if (detector->gesture_type() == GestureType::DEFAULT) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<DefaultGestureHandler>(
                                      sign, lynx_context, detector, member));
    } else if (detector->gesture_type() == GestureType::NATIVE) {
      gesture_handler_map.emplace(detector->gesture_id(),
                                  std::make_shared<NativeGestureHandler>(
                                      sign, lynx_context, detector, member));
    }
  }

  return gesture_handler_map;
}

void BaseGestureHandler::HandleEnableGestureCallback(
    const std::vector<GestureCallback>& callbacks) {
  if (callbacks.empty()) return;

  for (const auto& callback : callbacks) {
    if (enable_flags_.find(callback.name_.str()) != enable_flags_.end()) {
      enable_flags_[callback.name_.str()] = true;
    }
  }
}

void BaseGestureHandler::HandleMotionEvent(
    const ArkUI_UIInputEvent* input_event,
    std::shared_ptr<TouchEvent> lynx_touch_event, float delta_x,
    float delta_y) {
  OnHandle(input_event, lynx_touch_event, delta_x, delta_y);
}

bool BaseGestureHandler::IsEnd() const {
  return status_ == GestureConstants::LYNX_STATE_END;
}

bool BaseGestureHandler::IsActive() const {
  return status_ == GestureConstants::LYNX_STATE_ACTIVE;
}

int BaseGestureHandler::GetGestureStatus() const { return status_; }

void BaseGestureHandler::SendGestureEvent(const std::string& event_name,
                                          const lepus::Value& event_params) {
  if (!gesture_detector_) return;

  GestureEvent gestureEvent = {sign_, gesture_detector_->gesture_id(),
                               event_name, event_params};
  lynx_context_->HandleGestureEvent(gestureEvent);
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

const lepus::Value BaseGestureHandler::GetEventParamsFromTouchEvent(
    std::shared_ptr<TouchEvent> touch_event) const {
  auto params = lepus::Dictionary::Create();

  if (touch_event) {
    int64_t time_stamp = std::chrono::system_clock::now().time_since_epoch() /
                         std::chrono::milliseconds(1);
    params->SetValue("timestamp", time_stamp);
    params->SetValue("type", touch_event->Name());
    const lepus::Value& map = touch_event->UITouchMap();
    float client_x, client_y, page_x, page_y, x, y;
    for (auto it : *map.Table()) {
      if (it.second.Array()->size() >= 1) {
        const auto& event_info = it.second.Array()->get(0).Array();
        client_x = event_info->get(1).Number();
        client_y = event_info->get(2).Number();
        page_x = event_info->get(3).Number();
        page_y = event_info->get(4).Number();
        x = event_info->get(5).Number();
        y = event_info->get(6).Number();
      }
    }

    params->SetValue("x", x);
    params->SetValue("y", y);

    params->SetValue("pageX", page_x);
    params->SetValue("pageY", page_y);

    params->SetValue("clientX", client_x);
    params->SetValue("clientY", client_y);
  }
  return lepus::Value(std::move(params));
}

int BaseGestureHandler::Px2vp(float px_value) const {
  float scaled_density = lynx_context_->ScaledDensity();
  return static_cast<int>(px_value / scaled_density);
}

int BaseGestureHandler::Dip2Px(float dp_value) const {
  float scaled_density = lynx_context_->ScaledDensity();
  return static_cast<int>(dp_value * scaled_density);
}

void BaseGestureHandler::Activate() {
  status_ = GestureConstants::LYNX_STATE_ACTIVE;
}

void BaseGestureHandler::Reset() {
  status_ = GestureConstants::LYNX_STATE_INIT;
}

void BaseGestureHandler::Fail() { status_ = GestureConstants::LYNX_STATE_FAIL; }

void BaseGestureHandler::Begin() {
  status_ = GestureConstants::LYNX_STATE_BEGIN;
}

void BaseGestureHandler::Ignore() {
  status_ = GestureConstants::LYNX_STATE_UNDETERMINED;
}

void BaseGestureHandler::End() { status_ = GestureConstants::LYNX_STATE_END; }

void BaseGestureHandler::OnTouchesDown(
    std::shared_ptr<TouchEvent> touch_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_DOWN)) {
    SendGestureEvent(GestureConstants::ON_TOUCHES_DOWN,
                     GetEventParamsFromTouchEvent(touch_event));
  }
}

void BaseGestureHandler::OnTouchesMove(
    std::shared_ptr<TouchEvent> touch_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_MOVE)) {
    SendGestureEvent(GestureConstants::ON_TOUCHES_MOVE,
                     GetEventParamsFromTouchEvent(touch_event));
  }
}

void BaseGestureHandler::OnTouchesUp(std::shared_ptr<TouchEvent> touch_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_UP)) {
    SendGestureEvent(GestureConstants::ON_TOUCHES_UP,
                     GetEventParamsFromTouchEvent(touch_event));
  }
}

void BaseGestureHandler::OnTouchesCancel(
    std::shared_ptr<TouchEvent> touch_event) {
  if (enable_flags_.at(GestureConstants::ON_TOUCHES_CANCEL)) {
    SendGestureEvent(GestureConstants::ON_TOUCHES_CANCEL,
                     GetEventParamsFromTouchEvent(touch_event));
  }
}

std::shared_ptr<GestureDetector> BaseGestureHandler::GetGestureDetector()
    const {
  return gesture_detector_;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
