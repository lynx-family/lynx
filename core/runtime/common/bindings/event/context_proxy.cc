// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/bindings/event/context_proxy.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/event/event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/common/js_call_native_frequency_monitor.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace runtime {

ContextProxy::ContextProxy(Delegate& delegate, Type origin_type,
                           Type target_type)
    : delegate_(delegate),
      origin_type_(origin_type),
      target_type_(target_type),
      event_listener_(nullptr),
      dispatch_event_frequency_monitor_(nullptr) {
  auto& env = tasm::LynxEnv::GetInstance();
  if (env.EnableJSCallNativeFrequencyMonitor()) {
    dispatch_event_frequency_monitor_ =
        std::make_unique<JsCallNativeFrequencyMonitor>(
            env.GetJSCallNativeFrequencyMonitorThresholdCommon());
  }
}

ContextProxy::~ContextProxy() = default;

std::string ContextProxy::ConvertContextTypeToString(ContextProxy::Type type) {
  if (type == ContextProxy::Type::kJSContext) {
    return kJSContext;
  } else if (type == ContextProxy::Type::kCoreContext) {
    return kCoreContext;
  } else if (type == ContextProxy::Type::kUIContext) {
    return kUIContext;
  } else if (type == ContextProxy::Type::kDevTool) {
    return kDevTool;
  } else if (type == ContextProxy::Type::kNative) {
    return kNative;
  } else if (type == ContextProxy::Type::kEngine) {
    return kEngine;
  }
  return kUnknown;
}

ContextProxy::Type ContextProxy::ConvertStringToContextType(
    const std::string& type_str) {
  if (type_str == kJSContext) {
    return ContextProxy::Type::kJSContext;
  } else if (type_str == kCoreContext) {
    return ContextProxy::Type::kCoreContext;
  } else if (type_str == kUIContext) {
    return ContextProxy::Type::kUIContext;
  } else if (type_str == kDevTool) {
    return ContextProxy::Type::kDevTool;
  } else if (type_str == kNative) {
    return ContextProxy::Type::kNative;
  } else if (type_str == kEngine) {
    return ContextProxy::Type::kEngine;
  }
  return ContextProxy::Type::kUnknown;
}

void ContextProxy::PostMessage(const lepus::Value& message) {
  auto event = fml::MakeRefCounted<MessageEvent>(
      origin_type_, target_type_,
      std::make_unique<pub::ValueImplLepus>(message));
  DispatchEvent(std::move(event));
}

void ContextProxy::SetListenerBeforePublishEvent(
    std::unique_ptr<event::EventListener> listener) {
  event_listener_.swap(listener);
}

event::EventListener* ContextProxy::GetListenerBeforePublishEvent() {
  return event_listener_.get();
}

event::DispatchEventResult ContextProxy::DispatchEvent(
    fml::RefPtr<event::Event> event) {
  if (event->event_type() != event::Event::EventType::kMessageEvent) {
    return {event::EventCancelType::kNotCanceled, false};
  }
  auto message_event = fml::static_ref_ptr_cast<runtime::MessageEvent>(event);
  if (dispatch_event_frequency_monitor_) {
    const std::string target_str = message_event->GetTargetString();
    const std::string origin_str = message_event->GetOriginString();
    const std::string& type_str = message_event->type();
    std::string method_name = target_str;
    method_name.reserve(method_name.size() + origin_str.size() +
                        type_str.size() + 2);
    method_name.push_back('#');
    method_name += origin_str;
    method_name.push_back('#');
    method_name += type_str;
    auto error_opt = dispatch_event_frequency_monitor_->Record(
        "ContextProxy::DispatchEvent", method_name);
    if (error_opt) {
      LOGW("ContextProxy::DispatchEvent called too frequently. target:"
           << target_str << " origin:" << origin_str << " type:" << type_str);
      ReportError(std::move(*error_opt));
    }
  }
  if (message_event->GetTargetType() == origin_type_) {
    bool consumed = false;
    if (event_listener_ != nullptr) {
      event_listener_->Invoke(event);
      consumed = true;
    }
    consumed |= EventTarget::DispatchEvent(message_event).consumed;
    return {event::EventCancelType::kNotCanceled, consumed};
  }
  return delegate_.DispatchMessageEvent(
      runtime::MessageEvent::ShallowCopy(*message_event));
}

}  // namespace runtime
}  // namespace lynx
