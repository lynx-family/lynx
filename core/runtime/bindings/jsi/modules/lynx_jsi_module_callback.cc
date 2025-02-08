// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/runtime/bindings/jsi/modules/module_interceptor.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "core/value_wrapper/value_impl_lepus.h"
#if ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/native_module_recorder.h"
#endif

namespace lynx {
namespace piper {

// BINARY_KEEP_SOURCE_FILE
ModuleCallbackFunctionHolder::ModuleCallbackFunctionHolder(Function&& func)
    : function_(std::move(func)) {}

ModuleCallback::ModuleCallback(int64_t callback_id)
    : LynxModuleCallback(callback_id) {}

void ModuleCallback::Invoke(Runtime* runtime,
                            ModuleCallbackFunctionHolder* holder) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "InvokeCallback",
              [this](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_terminating_flow_ids(CallbackFlowId());
                ctx.event()->add_debug_annotations("module_name", module_name_);
                ctx.event()->add_debug_annotations("method_name", method_name_);
              });
  if ((!args_ || !args_->IsArray()) && !custom_args_converter_) {
    LOGW("callback's args is invalid.");
    return;
  }
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JSB, "PubValueToJSValue");
  uint64_t convert_params_start = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_convert_params_start",
      [convert_params_start,
       timing_collector = timing_collector_](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(convert_params_start));
        if (timing_collector != nullptr) {
          ctx.event()->add_flow_ids(timing_collector->FlowId());
        }
      });
  if (custom_args_converter_) {
    args_ = custom_args_converter_(runtime, this);
  }
  if (!args_ || !args_->IsArray()) {
    LOGW("callback's args is invalid.");
  }
  size_t size = static_cast<size_t>(args_->Length());
  piper::Value values[size];
  args_->ForeachArray([&values, runtime](int64_t index, const pub::Value& val) {
    values[index] = pub::ValueUtils::ConvertValueToPiperValue(*runtime, val);
  });
  uint64_t convert_params_end = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_convert_params_end",
      [convert_params_start, convert_params_end,
       timing_collector = timing_collector_](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(convert_params_end));
        ctx.event()->add_debug_annotations(
            "jsb_callback_convert_params",
            std::to_string(convert_params_end - convert_params_start));
        if (timing_collector != nullptr) {
          ctx.event()->add_flow_ids(timing_collector->FlowId());
        }
      });
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JSB);
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::NativeModuleRecorder::GetInstance().RecordCallback(
      module_name_.c_str(), method_name_.c_str(), values[0], runtime,
      callback_id(), record_id_);
#endif  // ENABLE_TESTBENCH_RECORDER

  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "InvokeCallback");
  uint64_t invoke_js_callback_start = base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_invoke_start",
      [do_invoke_start_timestamp = base::CurrentSystemTimeMilliseconds(),
       timing_collector = timing_collector_](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(do_invoke_start_timestamp));
        if (timing_collector != nullptr) {
          ctx.event()->add_flow_ids(timing_collector->FlowId());
        }
      });
  holder->function_.call(*runtime, values, size);

  if (timing_collector_ != nullptr) {
    timing_collector_->EndCallbackInvoke(
        (convert_params_end - convert_params_start), invoke_js_callback_start);
    if (group_interceptor_) {
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

#if ENABLE_TESTBENCH_RECORDER
void ModuleCallback::SetRecordID(int64_t record_id) { record_id_ = record_id; }
#endif

void ModuleCallback::SetArgs(std::unique_ptr<pub::Value> args) {
  args_ = std::move(args);
}

void ModuleCallback::SetArgsConverter(
    std::function<std::unique_ptr<pub::Value>(piper::Runtime* rt,
                                              ModuleCallback* callback)>
        converter) {
  custom_args_converter_ = std::move(converter);
}

}  // namespace piper
}  // namespace lynx
