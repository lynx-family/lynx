// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_TOUCH_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_TOUCH_EVENT_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/value_wrapper/value_impl_lepus.h"
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

  int64_t TimeStamp() const { return time_stamp_; }

  void SetTimeStamp(int64_t time_stamp) { time_stamp_ = time_stamp; }

  bool IsMultiTouch() const { return is_multi_touch_; }

  const lepus::Value& UITouchMap() const { return target_touch_map_; }

 private:
  float target_point_[2] = {0};
  float page_point_[2] = {0};
  float client_point_[2] = {0};
  int64_t time_stamp_{0};
  bool is_multi_touch_{false};
  lepus::Value target_touch_map_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_TOUCH_EVENT_H_
