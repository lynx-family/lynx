// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_EVENT_EVENT_OBJECT_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_EVENT_EVENT_OBJECT_H_

#include <string>

#include "base/include/value/ref_counted_class.h"
#include "core/event/event.h"

namespace lynx {
namespace tasm {

class EventObject : public lepus::RefCounted {
 public:
  lepus::RefType GetRefType() const override {
    return lepus::RefType::kEventObject;
  };

  EventObject(const std::string& type, int64_t time_stamp,
              event::Event::EventType event_type, event::Event::Bubbles bubbles,
              event::Event::Cancelable cancelable,
              event::Event::ComposedMode composed_mode,
              event::Event::PhaseType phase_type,
              const lepus::Value& event_detail)
      : event_(event::Event(type, time_stamp, event_type, bubbles, cancelable,
                            composed_mode, phase_type)) {
    event_.set_detail(event_detail);
  }

  event::Event& GetEvent() { return event_; }

 private:
  event::Event event_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_EVENT_EVENT_OBJECT_H_
