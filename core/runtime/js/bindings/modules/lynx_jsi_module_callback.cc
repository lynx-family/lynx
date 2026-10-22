// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/module_interceptor.h"
#include "core/runtime/js/template_delegate.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/recorder/record.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace runtime {
namespace js {
// BINARY_KEEP_SOURCE_FILE
ModuleCallbackFunctionHolder::ModuleCallbackFunctionHolder(Function&& func)
    : function_(std::move(func)) {}

ModuleCallback::ModuleCallback(int64_t callback_id)
    : LynxModuleCallback(callback_id) {
  group_interceptor_ = GroupInterceptor::Current();
}

std::shared_ptr<ModuleCallback> ModuleCallback::CloneForMockDelivery(
    int64_t id) const {
  auto callback = std::make_shared<ModuleCallback>(id);
  callback->SetModuleName(module_name_);
  callback->SetMethodName(method_name_);
  callback->SetNativeModuleInvocationContext(invocation_context_);
  callback->group_interceptor_ = group_interceptor_;
  callback->interception_argument_index_ = interception_argument_index_;
  callback->SetCallbackFlowId(CallbackFlowId());
  callback->timing_collector_ = timing_collector_;
  callback->SetFirstArg(FirstArg());
  callback->SetRecordID(record_id_);
  return callback;
}

void ModuleCallback::Invoke(Runtime* runtime,
                            ModuleCallbackFunctionHolder* holder) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, NATIVE_MODULE_CALLBACK,
              [this](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_terminating_flow_ids(CallbackFlowId());
                ctx.event()->add_debug_annotations("module_name", module_name_);
                ctx.event()->add_debug_annotations("method_name", method_name_);
              });
  if ((!args_ || !args_->IsArray()) && !custom_args_converter_) {
    LOGW("NativeModule: Callback's args is invalid.");
    return;
  }
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, PUB_VALUE_TO_JS_VALUE);
  uint64_t convert_params_start = base::CurrentSystemTimeMilliseconds();
  if (custom_args_converter_) {
    args_ = custom_args_converter_(runtime, this);
  }
  if (!args_ || !args_->IsArray()) {
    LOGW("NativeModule: Callback's args is invalid.");
  }
  if (!args_ || !args_->IsArray()) return;
  if (group_interceptor_) {
    group_interceptor_->BeforeCallback(interception_argument_index_, args_);
  }
  size_t size = static_cast<size_t>(args_->Length());
  Value values[size];
  args_->ForeachArray([&values, runtime](int64_t index, const pub::Value& val) {
    values[index] = pub::ValueUtils::ConvertValueToPiperValue(*runtime, val);
  });
  lepus::Value observer_result =
      invocation_context_ ? pub::ValueUtils::ConvertValueToLepusValue(*args_)
                          : lepus::Value();
  // Directly destroy `args_` to avoid issues caused by the unstable destruction
  // order of `shared_ptr`, which can lead to `args_` being destroyed by other
  // threads.
  args_.reset();
  uint64_t convert_params_end = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
  RECORD_OPTIONAL(size != 0, NativeModuleCallback, log_context_,
                  module_name_.c_str(), method_name_.c_str(), values[0],
                  runtime, callback_id(), record_id_);

  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, MODULE_INVOKE_CALLBACK);
  uint64_t invoke_js_callback_start = base::CurrentSystemTimeMilliseconds();
  holder->function_.call(*runtime, values, size);

  if (invocation_context_) {
    invocation_context_->OnCallback(std::move(observer_result));
  }

  if (timing_collector_ != nullptr) {
    timing_collector_->EndCallbackInvoke(
        (convert_params_end - convert_params_start), invoke_js_callback_start);
    if (group_interceptor_ && notify_callback_invoked_) {
      group_interceptor_->OnCallbackInvoked(timing_collector_, this);
    }
  }
}

void ModuleCallback::ReportLynxErrors(runtime::TemplateDelegate* delegate) {
  if (delegate) {
    for (auto& error : errors_) {
      delegate->OnErrorOccurred(std::move(error));
    }
  }
  errors_.clear();
}

void ModuleCallback::SetRecordID(int64_t record_id) { record_id_ = record_id; }

void ModuleCallback::SetArgs(std::unique_ptr<pub::Value> args) {
  args_ = std::move(args);
}

void ModuleCallback::SetArgsConverter(
    std::function<std::unique_ptr<pub::Value>(Runtime* rt,
                                              ModuleCallback* callback)>
        converter) {
  custom_args_converter_ = std::move(converter);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
