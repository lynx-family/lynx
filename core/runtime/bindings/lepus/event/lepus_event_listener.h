// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_EVENT_LEPUS_EVENT_LISTENER_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_EVENT_LEPUS_EVENT_LISTENER_H_

#include "core/event/event.h"
#include "core/event/event_listener.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class LepusClosureEventListener : public event::EventListener {
 public:
  LepusClosureEventListener(lepus::Context* context, lepus::Value closure);
  virtual ~LepusClosureEventListener() override = default;

  virtual void Invoke(event::Event* event) override;

  virtual bool Matches(EventListener* listener) override;

 private:
  lepus::Value ConvertEventToLepusValue(event::Event* event);

  lepus::Context* context_;
  lepus::Value closure_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_EVENT_LEPUS_EVENT_LISTENER_H_
