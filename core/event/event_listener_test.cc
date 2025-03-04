// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event_listener_test.h"

#include "core/event/event_target.h"

namespace lynx {
namespace event {
namespace test {

MockEventListener::MockEventListener(EventListener::Type type,
                                     const std::string& content,
                                     const std::string& event_name,
                                     const std::string& erase_content,
                                     EventTarget* target)
    : EventListener(type),
      content_(content),
      event_name_(event_name),
      erase_content_(erase_content),
      target_(target) {}

void MockEventListener::Invoke(Event* event) {
  count_++;
  if (target_ != nullptr) {
    target_->RemoveEventListener(
        event_name_,
        std::make_unique<MockEventListener>(type(), erase_content_));
  }
}

bool MockEventListener::Matches(EventListener* listener) {
  if (type() != listener->type()) {
    return false;
  }
  return GetContent() ==
         static_cast<MockEventListener*>(listener)->GetContent();
}

}  // namespace test
}  // namespace event
}  // namespace lynx
