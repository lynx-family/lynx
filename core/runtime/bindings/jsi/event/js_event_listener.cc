// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/event/js_event_listener.h"

#include "core/base/lynx_trace_categories.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/common/utils.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace piper {

JSClosureEventListener::JSClosureEventListener(std::shared_ptr<Runtime> rt,
                                               std::shared_ptr<App> app,
                                               const piper::Value& closure)
    : event::EventListener(event::EventListener::Type::kJSClosureEventListener),
      rt_(rt),
      native_app_(app) {
  if (rt == nullptr) {
    return;
  }
  closure_ = piper::Value(*rt, closure);
}

void JSClosureEventListener::Invoke(event::Event* event) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSClosureEvent",
              [event](lynx::perfetto::EventContext ctx) {
                auto type = event ? event->type() : "null";
                ctx.event()->add_debug_annotations("type", type);
              });
  auto rt = rt_.lock();
  if (rt == nullptr) {
    return;
  }
  if (!closure_.isObject()) {
    return;
  }
  int32_t instance_id = static_cast<int32_t>(rt->getRuntimeId());
  tasm::timing::LongTaskMonitor::Scope long_task_scope(
      instance_id, tasm::timing::kJSFuncTask,
      tasm::timing::kTaskNameJSEventListenerInvoke,
      event ? event->type() : "null");
  piper::Scope scope(*rt);

  auto func = closure_.getObject(*rt).asFunction(*rt);
  if (!func) {
    return;
  }

  const piper::Value args[1] = {ConvertEventToPiperValue(event)};
  size_t count = 1;
  func->call(*rt, args, count);
}

bool JSClosureEventListener::Matches(EventListener* listener) {
  if (listener->type() != type()) {
    return false;
  }
  auto* other = static_cast<JSClosureEventListener*>(listener);

  auto rt = rt_.lock();
  auto other_rt = other->rt_.lock();

  if (rt.get() != other_rt.get()) {
    return false;
  }

  return piper::Value::strictEquals(*rt, closure_, other->closure_);
}

piper::Value JSClosureEventListener::GetClosure() {
  auto rt = rt_.lock();
  if (rt == nullptr) {
    return piper::Value::undefined();
  }
  return piper::Value(*rt, closure_);
}

piper::Value JSClosureEventListener::ConvertEventToPiperValue(
    event::Event* event) {
  auto rt = rt_.lock();
  auto app = native_app_.lock();
  if (rt == nullptr || event == nullptr || app == nullptr) {
    return piper::Value::undefined();
  }
  piper::Object obj(*rt);
  if (event->event_type() == event::Event::EventType::kMessageEvent) {
    runtime::MessageEvent* message_event =
        static_cast<runtime::MessageEvent*>(event);
    obj.setProperty(*rt, runtime::kType,
                    piper::String::createFromUtf8(*rt, message_event->type()));
    obj.setProperty(*rt, runtime::kData,
                    *valueFromLepus(*rt, message_event->message(),
                                    app->jsi_object_wrapper_manager().get()));
    obj.setProperty(
        *rt, runtime::kOrigin,
        piper::String::createFromUtf8(*rt, message_event->GetOriginString()));
  }

  return piper::Value(*rt, obj);
}

}  // namespace piper
}  // namespace lynx
