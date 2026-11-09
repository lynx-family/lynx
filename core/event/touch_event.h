// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_TOUCH_EVENT_H_
#define CORE_EVENT_TOUCH_EVENT_H_

#include <cstdint>
#include <string>
#include <utility>

#include "core/event/event.h"
#include "core/public/event/touch_event_data.h"

#define EVENT_TOUCH_START "touchstart"
#define EVENT_TOUCH_MOVE "touchmove"
#define EVENT_TOUCH_END "touchend"
#define EVENT_TOUCH_CANCEL "touchcancel"
#define EVENT_TAP "tap"
#define EVENT_CLICK "click"
#define EVENT_LONG_PRESS "longpress"

namespace lynx {
namespace tasm {

// Carries either scalar coordinates or a touch table keyed by target ID.
struct EventInfo {
  explicit EventInfo(const lepus::Value& params, int64_t timestamp = 0)
      : params(params), is_multi_finger(true), timestamp(timestamp) {}
  EventInfo(int tag, float x, float y, float client_x, float client_y,
            float page_x, float page_y, int64_t timestamp = 0,
            event::TouchEventTargetPoints current_target_points = {})
      : tag(tag),
        x(x),
        y(y),
        client_x(client_x),
        client_y(client_y),
        page_x(page_x),
        page_y(page_y),
        timestamp(timestamp),
        current_target_points(std::move(current_target_points)) {}
  EventInfo(const EventInfo& info) = delete;
  EventInfo& operator=(const EventInfo& info) = delete;
  EventInfo(EventInfo&& info) noexcept = default;
  EventInfo& operator=(EventInfo&& info) noexcept = default;

  lepus::Value params;
  bool is_multi_finger{false};
  int32_t tag{0};
  float x{0};
  float y{0};
  float client_x{0};
  float client_y{0};
  float page_x{0};
  float page_y{0};
  int64_t timestamp{0};
  event::TouchEventTargetPoints current_target_points;
};

}  // namespace tasm
namespace event {

// Owned by a page and accessed only during engine event dispatch.
struct TouchEventState {
  lepus::Value current_touches{lepus::CArray::Create()};
  bool long_press_consumed{false};
};

class TouchEvent : public Event {
 public:
  TouchEvent(const std::string& event_name, float x = 0, float y = 0,
             float page_x = 0, float page_y = 0, float client_x = 0,
             float client_y = 0, int64_t time_stamp = 0,
             TouchEventTargetPoints current_target_points = {});
  TouchEvent(const std::string& event_name, const lepus::Value& targets_touches,
             int64_t time_stamp = 0);

  float x() const { return x_; }
  float y() const { return y_; }
  float page_x() const { return page_x_; }
  float page_y() const { return page_y_; }
  float client_x() const { return client_x_; }
  float client_y() const { return client_y_; }
  int64_t identifier() const { return identifier_; }
  bool is_multi_touch() const { return is_multi_touch_; }

  void HandleEventBaseDetail(bool is_core_event = false) override;

  void HandleEventCustomDetail() override;

  bool HandleEventConflictAndParam() override;

 private:
  friend class EventTarget;

  void SetLongPressConsumed(bool consumed);
  TouchEventState* GetTouchEventState();
  void RestoreInputPaths();

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
  TouchEventTargetPoints current_target_points_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_TOUCH_EVENT_H_
