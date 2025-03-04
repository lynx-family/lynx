// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/lynx_module_timing.h"

#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace piper {

NativeModuleInfoCollector::NativeModuleInfoCollector(
    const std::shared_ptr<ModuleDelegate>& delegate,
    const std::string& module_name, const std::string& method_name,
    const std::string& method_first_arg_name)
    : delegate_(delegate) {
  timing_.module_name_ = module_name;
  timing_.method_name_ = method_name;
  timing_.method_first_arg_name_ = method_first_arg_name;
  // TODO: - @limeng.amer
  //  "bridge.call" is supported in the first stage, and other methods will be
  //  added later. eg:
  //  LynxIntersectionObserverModule、LynxUIMethodModule、LynxSetModule...
  enable_ = (module_name == "bridge" && method_name == "call" &&
             !method_first_arg_name.empty() &&
             tasm::LynxEnv::GetInstance().EnableJSBTiming());
}

void NativeModuleInfoCollector::EndCallFunc(uint64_t start_time) {
  if (!enable_) {
    return;
  }
  timing_.jsb_func_call_start_ = start_time;
  timing_.jsb_func_call_end_ = base::CurrentSystemTimeMilliseconds();
  timing_.jsb_func_call_ = timing_.jsb_func_call_end_ - start_time;
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_call_end",
      [this](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg",
                                           timing_.method_first_arg_name_);
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(timing_.jsb_func_call_end_));
        ctx.event()->add_debug_annotations(
            "jsb_func_call.duration", std::to_string(timing_.jsb_func_call_));
      });
}

void NativeModuleInfoCollector::EndFuncParamsConvert(uint64_t start_time) {
  if (!enable_) {
    return;
  }
  uint64_t end = base::CurrentSystemTimeMilliseconds();
  timing_.jsb_func_convert_params_ = end - start_time;
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_convert_params_end",
      [this, end](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg",
                                           timing_.method_first_arg_name_);
        ctx.event()->add_debug_annotations("timestamp", std::to_string(end));
        ctx.event()->add_debug_annotations(
            "jsb_func_convert_params.duration",
            std::to_string(timing_.jsb_func_convert_params_));
      });
}

void NativeModuleInfoCollector::EndPlatformMethodInvoke(uint64_t start_time) {
  if (!enable_) {
    return;
  }
  uint64_t end = base::CurrentSystemTimeMilliseconds();
  timing_.jsb_func_platform_method_ = end - start_time;
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_func_platform_method_end",
      [this, end](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg",
                                           timing_.method_first_arg_name_);
        ctx.event()->add_debug_annotations("timestamp", std::to_string(end));
        ctx.event()->add_debug_annotations(
            "jsb_func_platform_method.duration",
            std::to_string(timing_.jsb_func_platform_method_));
      });
}

void NativeModuleInfoCollector::CallbackThreadSwitchStart() {
  if (!enable_) {
    return;
  }
  timing_.jsb_callback_thread_switch_start_ =
      base::CurrentSystemTimeMilliseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_thread_switch_start",
      [this](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg",
                                           timing_.method_first_arg_name_);
        ctx.event()->add_debug_annotations(
            "timestamp",
            std::to_string(timing_.jsb_callback_thread_switch_start_));
      });
}

void NativeModuleInfoCollector::EndCallbackInvoke(uint64_t convert_params_time,
                                                  uint64_t invoke_start) {
  if (!enable_) {
    return;
  }
  uint64_t end = base::CurrentSystemTimeMilliseconds();
  timing_.jsb_callback_convert_params_ = convert_params_time;
  timing_.jsb_callback_invoke_ = end - invoke_start;
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_invoke_end",
      [this, end](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg",
                                           timing_.method_first_arg_name_);
        ctx.event()->add_debug_annotations("timestamp", std::to_string(end));
        ctx.event()->add_debug_annotations(
            "jsb_callback_invoke.duration",
            std::to_string(timing_.jsb_callback_invoke_));
      });
}

void NativeModuleInfoCollector::EndCallCallback(uint64_t switch_end_time,
                                                uint64_t start_time) {
  if (!enable_) {
    return;
  }
  timing_.jsb_callback_thread_switch_end_ = switch_end_time;
  timing_.jsb_callback_call_start_ = start_time;
  timing_.jsb_callback_call_end_ = base::CurrentSystemTimeMilliseconds();
  timing_.jsb_callback_call_ = timing_.jsb_callback_call_end_ - start_time;
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JSB, "JSBTiming::jsb_callback_call_end",
      [this](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("first_arg",
                                           timing_.method_first_arg_name_);
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(timing_.jsb_callback_call_end_));
        ctx.event()->add_debug_annotations(
            "jsb_callback_call.duration",
            std::to_string(timing_.jsb_callback_call_));
      });
}

void NativeModuleInfoCollector::OnErrorOccurred(
    NativeModuleStatusCode status_code) {
  if (!enable_ && timing_.status_code_ != NativeModuleStatusCode::SUCCESS) {
    return;
  }
  timing_.status_code_ = status_code;
}

uint64_t NativeModuleInfoCollector::GetFuncCallStart() const {
  return timing_.jsb_func_call_start_;
}

uint64_t NativeModuleInfoCollector::GetCallbackThreadSwitchStart() const {
  return timing_.jsb_callback_thread_switch_start_;
}

uint64_t NativeModuleInfoCollector::GetCallbackInvokeDuration() const {
  return timing_.jsb_callback_invoke_;
}

void NativeModuleInfoCollector::SetNetworkRequestInfo(
    const NetworkRequestInfo& info) {
  timing_.network_request_info_ = info;
}

NetworkRequestInfo NativeModuleInfoCollector::GetNetworkRequestInfo() const {
  return timing_.network_request_info_;
}

std::string NativeModuleInfoCollector::GetFirstArg() {
  return timing_.method_first_arg_name_;
}

// ModuleCallback & LynxModule
// ModuleCallback and LynxModule jointly hold NativeModuleInfoCollector.
// NativeModuleInfoCollector will destruct When both a and b are released.
NativeModuleInfoCollector::~NativeModuleInfoCollector() {
  if (!enable_) {
    return;
  }
  auto delegate = delegate_.lock();
  if (delegate == nullptr) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY_JSB, "JSBTiming::Flush",
              [this](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations(
                    "first_arg", timing_.method_first_arg_name_);
              });
  // Calculate timing data
  timing_.jsb_callback_thread_switch_waiting_ =
      timing_.jsb_callback_thread_switch_start_ - timing_.jsb_func_call_end_;
  timing_.jsb_callback_thread_switch_ =
      timing_.jsb_callback_thread_switch_end_ -
      timing_.jsb_callback_thread_switch_start_;
  if (timing_.jsb_func_call_end_ >= timing_.jsb_callback_call_end_) {
    timing_.jsb_call_ = timing_.jsb_func_call_;
  } else {
    timing_.jsb_call_ =
        timing_.jsb_callback_call_end_ - timing_.jsb_func_call_start_;
  }
  // flush data
  delegate->FlushJSBTiming(std::move(timing_));
}
}  // namespace piper
}  // namespace lynx
