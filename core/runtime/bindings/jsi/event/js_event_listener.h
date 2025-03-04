// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_EVENT_JS_EVENT_LISTENER_H_
#define CORE_RUNTIME_BINDINGS_JSI_EVENT_JS_EVENT_LISTENER_H_

#include <memory>

#include "core/event/event_listener.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace piper {

class JSClosureEventListener : public event::EventListener {
 public:
  JSClosureEventListener(std::shared_ptr<Runtime>, std::shared_ptr<App>,
                         const piper::Value&);
  virtual ~JSClosureEventListener() override = default;

  virtual void Invoke(event::Event* event) override;

  virtual bool Matches(EventListener* listener) override;

  piper::Value GetClosure();

 private:
  piper::Value ConvertEventToPiperValue(event::Event* event);

  std::weak_ptr<Runtime> rt_;
  std::weak_ptr<App> native_app_;
  piper::Value closure_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_EVENT_JS_EVENT_LISTENER_H_
