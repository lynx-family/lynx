// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_EVENT_JS_EVENT_LISTENER_H_
#define CORE_RUNTIME_JS_BINDINGS_EVENT_JS_EVENT_LISTENER_H_

#include <memory>

#include "base/include/value/base_value.h"
#include "core/event/event_listener.h"
#include "core/runtime/common/bindings/event/context_proxy.h"
#include "core/runtime/js/bindings/js_app.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {

class JSClosureEventListener : public event::EventListener {
 public:
  JSClosureEventListener(
      std::shared_ptr<App>, const piper::Value&,
      const EventListener::Options& options = EventListener::Options());
  ~JSClosureEventListener() override = default;

  void Invoke(fml::RefPtr<event::Event> event) override;

  bool Matches(EventListener* listener) override;

  piper::Value GetClosure();

 private:
  piper::Value ConvertEventToPiperValue(fml::RefPtr<event::Event> event);

  std::weak_ptr<App> native_app_;
  piper::Value closure_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_EVENT_JS_EVENT_LISTENER_H_
