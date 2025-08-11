// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_TOUCH_EVENT_H_
#define CORE_EVENT_TOUCH_EVENT_H_

#include <string>

#include "core/event/event.h"

#define EVENT_TOUCH_START "touchstart"
#define EVENT_TOUCH_MOVE "touchmove"
#define EVENT_TOUCH_END "touchend"
#define EVENT_TOUCH_CANCEL "touchcancel"
#define EVENT_TAP "tap"
#define EVENT_LONG_PRESS "longpress"

namespace lynx {
namespace event {

class TouchEvent : public Event {
 public:
  // Indicates whether a long press event is triggered and consumed.
  static bool long_press_consumed_;

  TouchEvent(const std::string& event_name, float x = 0, float y = 0,
             float page_x = 0, float page_y = 0, float client_x = 0,
             float client_y = 0, float time_stamp = 0);
  TouchEvent(const std::string& event_name, const lepus::Value& targets_touches,
             float time_stamp = 0);

  float x() const { return x_; }
  float y() const { return y_; }
  float page_x() const { return page_x_; }
  float page_y() const { return page_y_; }
  float client_x() const { return client_x_; }
  float client_y() const { return client_y_; }
  int64_t identifier() const { return identifier_; }
  bool is_multi_touch() const { return is_multi_touch_; }

  void HandleEventCustomDetail() override;

  bool HandleEventConflictAndParam() override;

  lepus::Value& GetCurrentTouches();

 private:
  float x_;
  float y_;
  float page_x_;
  float page_y_;
  float client_x_;
  float client_y_;
  int64_t identifier_;
  // Indicates whether the touch event contains multi-finger information.
  bool is_multi_touch_{false};
  // Save the touched object and its touch information.
  lepus::Value targets_touches_{};
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_TOUCH_EVENT_H_
