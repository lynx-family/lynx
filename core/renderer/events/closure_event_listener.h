// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_EVENTS_CLOSURE_EVENT_LISTENER_H_
#define CORE_RENDERER_EVENTS_CLOSURE_EVENT_LISTENER_H_

#include <memory>

#include "base/include/closure.h"
#include "core/event/event_listener.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace event {
using ClosureListener = base::MoveOnlyClosure<void, lepus::Value>;

class ClosureEventListener : public event::EventListener {
 public:
  explicit ClosureEventListener(ClosureListener&& closure);
  ~ClosureEventListener() override = default;

  void Invoke(event::Event* event) override;

  bool Matches(EventListener* listener) override;

 private:
  ClosureListener closure_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_RENDERER_EVENTS_CLOSURE_EVENT_LISTENER_H_
