// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_EVENTS_CLOSURE_EVENT_LISTENER_H_
#define CORE_RENDERER_EVENTS_CLOSURE_EVENT_LISTENER_H_

#include <memory>

#include "base/include/closure.h"
#include "base/include/value/base_value.h"
#include "core/event/event_listener.h"

namespace lynx {
namespace event {
using ClosureListener = base::MoveOnlyClosure<void, lepus::Value>;

class ClosureEventListener : public event::EventListener {
 public:
  enum class ClosureType {
    kNone,
    kJS,
    kCore,
    kClient,
  };

  explicit ClosureEventListener(
      ClosureListener&& closure,
      const EventListener::Options& options = EventListener::Options(),
      ClosureType closure_type = ClosureType::kNone,
      const lepus::Value& lepus_object = lepus::Value());
  ~ClosureEventListener() override = default;

  void Invoke(event::Event* event) override;

  bool Matches(EventListener* listener) override;

  ClosureType closure_type() { return closure_type_; }

  lepus::Value& lepus_object() { return lepus_object_; }

 private:
  ClosureListener closure_;
  ClosureType closure_type_{ClosureType::kNone};
  lepus::Value lepus_object_{};
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_RENDERER_EVENTS_CLOSURE_EVENT_LISTENER_H_
