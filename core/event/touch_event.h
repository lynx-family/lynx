// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_TOUCH_EVENT_H_
#define CORE_EVENT_TOUCH_EVENT_H_

#include <string>

#include "core/event/event.h"

namespace lynx {
namespace event {

class TouchEvent : public Event {
 public:
  TouchEvent(const std::string& event_name, float x, float y, float page_x,
             float page_y);
  float x() const { return x_; }
  float y() const { return y_; }
  float page_x() const { return page_x_; }
  float page_y() const { return page_y_; }
  int64_t identifier() const { return identifier_; }

 private:
  float x_;
  float y_;
  float page_x_;
  float page_y_;
  int64_t identifier_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_TOUCH_EVENT_H_
