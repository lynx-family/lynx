// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_TOUCH_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_TOUCH_EVENT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/event/touch_event_data.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/lynx_event.h"

namespace lynx {
namespace tasm {
namespace harmony {

class TouchEvent : public LynxEvent {
 public:
  static constexpr const char* const START = "touchstart";
  static constexpr const char* const MOVE = "touchmove";
  static constexpr const char* const UP = "touchend";
  static constexpr const char* const CANCEL = "touchcancel";
  static constexpr const char* const TAP = "tap";
  static constexpr const char* const CLICK = "click";
  static constexpr const char* const LONGPRESS = "longpress";

  TouchEvent(int id, std::string name)
      : LynxEvent(id, std::move(name), LynxEventType::kTouch) {}

  TouchEvent(std::string name, lepus::Value target_touch_map)
      : LynxEvent(-1, std::move(name), LynxEventType::kTouch),
        is_multi_touch_(true),
        target_touch_map_(std::move(target_touch_map)) {}

  void SetTargetPoint(float target_point[2]);

  void GetTargetPoint(float target_point[2]) const;

  void SetPagePoint(float page_point[2]);

  void GetPagePoint(float page_point[2]) const;

  void SetClientPoint(float client_point[2]);

  void GetClientPoint(float client_point[2]) const;

  void SetCurrentTargetPoints(
      lynx::event::TouchEventTargetPoints current_target_points) {
    current_target_points_ = std::move(current_target_points);
  }

  const lynx::event::TouchEventTargetPoints& CurrentTargetPoints() const {
    return current_target_points_;
  }

  int64_t TimeStamp() const { return time_stamp_; }

  void SetTimeStamp(int64_t time_stamp) { time_stamp_ = time_stamp; }

  bool IsMultiTouch() const { return is_multi_touch_; }

  const lepus::Value& UITouchMap() const { return target_touch_map_; }

  lepus::Value EventParams() const;

  void SetTarget(std::weak_ptr<EventTarget> target) {
    target_ = std::move(target);
  }

  std::weak_ptr<EventTarget> Target() const { return target_; }

 private:
  float target_point_[2] = {0};
  float page_point_[2] = {0};
  float client_point_[2] = {0};
  int64_t time_stamp_{0};
  bool is_multi_touch_{false};
  lepus::Value target_touch_map_;
  lynx::event::TouchEventTargetPoints current_target_points_;
  std::weak_ptr<EventTarget> target_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_TOUCH_EVENT_H_
