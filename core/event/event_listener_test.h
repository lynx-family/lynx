// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "core/event/event_listener.h"

#ifndef CORE_EVENT_EVENT_LISTENER_TEST_H_
#define CORE_EVENT_EVENT_LISTENER_TEST_H_

namespace lynx {
namespace event {

class EventTarget;

namespace test {

class MockEventListener : public EventListener {
 public:
  MockEventListener(EventListener::Type, const std::string&,
                    const std::string& event_name = "",
                    const std::string& erase_content = "",
                    EventTarget* target = nullptr);

  ~MockEventListener() override = default;

  virtual void Invoke(Event* event) override;
  virtual bool Matches(EventListener* listener) override;

  int32_t GetCount() { return count_; }
  std::string GetContent() { return content_; }

 private:
  int32_t count_{0};
  std::string content_;

  std::string event_name_;
  std::string erase_content_;
  EventTarget* target_{nullptr};
};

}  // namespace test
}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_LISTENER_TEST_H_
