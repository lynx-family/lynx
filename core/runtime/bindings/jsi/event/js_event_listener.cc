// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/event/js_event_listener.h"

#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx {
namespace piper {

App* JSClosureEventListener::GetApp() {
  if (unsafe_weak_app_) {
    return unsafe_weak_app_.Lock();
  }
  auto lock_app = native_app_.lock();
  if (lock_app) {
    return lock_app.get();
  }
  return nullptr;
}

JSClosureEventListener::JSClosureEventListener(
    std::weak_ptr<App> app, UnsafeWeakPtr<App> unsafe_weak_app,
    const piper::Value& closure)
    : event::EventListener(event::EventListener::Type::kJSClosureEventListener),
      native_app_(app),
      unsafe_weak_app_(unsafe_weak_app) {
  auto* app_ptr = GetApp();
  if (!app_ptr) {
    return;
  }
  auto rt = app_ptr->GetRuntime();
  if (rt == nullptr) {
    return;
  }
  closure_ = piper::Value(*rt, closure);
}

void JSClosureEventListener::Invoke(event::Event* event) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CALL_JS_CLOSURE_EVENT,
              [event](lynx::perfetto::EventContext ctx) {
                auto type = event ? event->type() : "null";
                ctx.event()->add_debug_annotations("type", type);
              });
  if (!closure_.isObject()) {
    return;
  }

  auto* app = GetApp();
  if (!app || app->IsDestroying()) {
    return;
  }
  auto rt = app->GetRuntime();
  if (!rt) {
    return;
  }
  auto page_options = app ? app->GetPageOptions() : tasm::PageOptions();
  tasm::timing::LongTaskMonitor::Scope long_task_scope(
      page_options, tasm::timing::kJSFuncTask,
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

  std::shared_ptr<Runtime> other_rt = nullptr;
  std::shared_ptr<Runtime> rt = nullptr;
  auto other_native_app = other->GetApp();
  if (other_native_app) {
    other_rt = other_native_app->GetRuntime();
  }
  auto native_app = GetApp();
  if (native_app) {
    rt = native_app->GetRuntime();
  }

  if (!rt || rt != other_rt) {
    return false;
  }

  return piper::Value::strictEquals(*rt, closure_, other->closure_);
}

piper::Value JSClosureEventListener::GetClosure() {
  auto native_app = GetApp();
  std::shared_ptr<Runtime> rt = nullptr;
  if (native_app && !native_app->IsDestroying()) {
    rt = native_app->GetRuntime();
  }
  if (rt == nullptr) {
    return piper::Value::undefined();
  }
  return piper::Value(*rt, closure_);
}

piper::Value JSClosureEventListener::ConvertEventToPiperValue(
    event::Event* event) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              CLOSURE_EVENT_LISTENER_CONVERT_TO_PIPER_VALUE);
  auto app = GetApp();
  std::shared_ptr<Runtime> rt = nullptr;
  if (app && !app->IsDestroying()) {
    rt = app->GetRuntime();
  }
  if (rt == nullptr || event == nullptr || app == nullptr) {
    return piper::Value::undefined();
  }
  piper::Object obj(*rt);
  if (event->event_type() == event::Event::EventType::kMessageEvent) {
    runtime::MessageEvent* message_event =
        static_cast<runtime::MessageEvent*>(event);
    obj.setProperty(*rt, runtime::kType,
                    piper::String::createFromUtf8(*rt, message_event->type()));
    obj.setProperty(
        *rt, runtime::kData,
        pub::ValueUtils::ConvertValueToPiperValue(
            *rt, *message_event->message(), app->jsi_object_wrapper_manager()));
    obj.setProperty(
        *rt, runtime::kOrigin,
        piper::String::createFromUtf8(*rt, message_event->GetOriginString()));
  }

  return piper::Value(*rt, obj);
}

}  // namespace piper
}  // namespace lynx
