// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/bts/lynx_bts_runtime_proxy_impl.h"

#include <string>
#include <utility>

#include "base/include/value/base_value.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/shell/common/shell_trace_event_def.h"
#include "core/shell/runtime/bts/bts_runtime_mediator.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace shell {

void LynxBTSRuntimeProxyImpl::CallJSFunction(
    std::string module_id, std::string method_id,
    std::unique_ptr<pub::Value> params) {
  CallJSFunction(module_id, method_id, [params = std::move(params)]() mutable {
    return std::move(params);
  });
}

void LynxBTSRuntimeProxyImpl::CallJSApiCallbackWithValue(
    int32_t callback_id, std::unique_ptr<pub::Value> params) {
  CallJSApiCallbackWithValue(
      callback_id,
      [params = std::move(params)]() mutable { return std::move(params); });
}

void LynxBTSRuntimeProxyImpl::CallJSIntersectionObserver(
    int32_t observer_id, int32_t callback_id,
    std::unique_ptr<pub::Value> params) {
  CallJSIntersectionObserver(
      observer_id, callback_id,
      [params = std::move(params)]() mutable { return std::move(params); });
}

void LynxBTSRuntimeProxyImpl::CallJSFunction(std::string module_id,
                                             std::string method_id,
                                             ParamsGetter getter) {
  if (!actor_) {
    return;
  }
  auto enqueue_info = tasm::performance::JSBlockingMonitor::MarkJSTaskEnqueue();

  [[maybe_unused]] uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CALL_JS_FUNCTION_PROXY,
              [flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
                ctx.event()->add_debug_annotations(kTaskName,
                                                   kJSTaskCallJSFunction);
              });
  actor_->Act([enqueue_info, module_id = std::move(module_id),
               method_id = std::move(method_id), getter = std::move(getter),
               is_runtime_standalone_mode = is_runtime_standalone_mode_,
               flow_id](auto& runtime) mutable {
    (void)flow_id;  // Explicitly reference `flow_id` to suppress the compiler
                    // warning.
    auto task = [&runtime, enqueue_info, module_id = std::move(module_id),
                 method_id = std::move(method_id), getter = std::move(getter),
                 flow_id] {
      auto js_runtime = runtime->GetJSRuntime();
      if (js_runtime == nullptr) {
        LOGE(
            "try call js module before js context is ready! "
            "module:"
            << module_id << " method:" << method_id << &runtime);
        return;
      }
      runtime->GetDelegate()->AddJSBlockingTime(enqueue_info.enqueue_time);
      auto params = getter();
      if (params == nullptr) {
        LOGE(
            "try call js module args is nullptr! "
            "module:"
            << module_id << " method:" << method_id << &runtime);
        return;
      }
      piper::Scope scope(*js_runtime);

      // Timing
      tasm::timing::LongTaskMonitor::Scope long_task_scope(
          runtime->GetPageOptions(), tasm::timing::kJSFuncTask,
          tasm::timing::kTaskNameJSProxyCallJSFunction);
      std::string first_arg_str;
      if (params->Length() > 0) {
        first_arg_str = params->GetValueAtIndex(0)->str();
      }
      (void)flow_id;  // Explicitly reference `flow_id` to suppress the compiler
                      // warning.
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, RUNTIME_ACTOR_CALL_JS_FUNCTION,
          [&](lynx::perfetto::EventContext ctx) {
            ctx.event()->add_debug_annotations("module_name", module_id);
            ctx.event()->add_debug_annotations("method_name", method_id);
            if (params->Length() > 0) {
              ctx.event()->add_debug_annotations("arg0", first_arg_str);
            }
            ctx.event()->add_terminating_flow_ids(flow_id);
          });

      tasm::timing::LongTaskTiming* timing =
          tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
      if (timing != nullptr) {
        if (params->Length() > 0) {
          timing->task_info_ =
              base::AppendString(module_id, ".", method_id, ".", first_arg_str);
        } else {
          timing->task_info_ = base::AppendString(module_id, ".", method_id);
        }
      }

      // Convert to piper value
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                        CALL_JS_FUNCTION_CONVERT_VALUE_TO_PIPER_ARRAY);
      auto piper_data =
          pub::ValueUtils::ConvertValueToPiperValue(*js_runtime, *params.get());
      auto piper_obj = piper_data.asObject(*js_runtime);
      if (!piper_obj) {
        js_runtime->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
            "CallJSFunction fail! Reason: pub value to piper value failed."));
        return;
      }
      auto piper_array = (*piper_obj).asArray(*js_runtime);
      if (!piper_array) {
        js_runtime->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
            "CallJSFunction fail! Reason: pub value to piper value failed."));
        return;
      }
      TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

      // Call
      TRACE_EVENT(LYNX_TRACE_CATEGORY, CALL_JS_FUNCTION_FIRE);
      runtime->CallFunction(module_id, method_id, *piper_array);
    };

    // In LynxBackgroundRuntime standalone, we don't need to postpone
    // GlobalEvent to LoadApp via cache. User can decide the time to
    // run FE code and sending GlobalEvent, so it is user's responsibility
    // to ensure that events arrive after FE code execution.
    if (is_runtime_standalone_mode) {
      if (runtime->IsRuntimeReady()) {
        runtime->OnErrorOccurred(base::LynxError(base::LynxError(
            error::E_BTS_RUNTIME_ERROR,
            "call sendGlobalEvent on invalid state, will be ignored",
            base::LynxErrorLevel::Error)));
      } else {
        task();
      }
    } else {
      runtime->call(std::move((task)));
    }
  });
}

void LynxBTSRuntimeProxyImpl::CallJSApiCallbackWithValue(int32_t callback_id,
                                                         ParamsGetter getter) {
  if (!actor_) {
    return;
  }
  auto enqueue_info = tasm::performance::JSBlockingMonitor::MarkJSTaskEnqueue();
  actor_->Act([enqueue_info, callback_id,
               getter = std::move(getter)](auto& runtime) {
    runtime->GetDelegate()->AddJSBlockingTime(enqueue_info.enqueue_time);
    auto js_runtime = runtime->GetJSRuntime();
    if (js_runtime == nullptr) {
      LOGR(
          "try CallJSApiCallbackWithValue before js context is ready "
          "callback_id:"
          << callback_id << &runtime);
      return;
    }
    auto params = getter();
    if (params == nullptr) {
      LOGR(
          "try CallJSApiCallbackWithValue params is nullptr."
          "callback_id:"
          << callback_id << &runtime);
      return;
    }
    piper::Scope scope(*js_runtime);
    auto piper_data =
        pub::ValueUtils::ConvertValueToPiperValue(*js_runtime, *params.get());
    runtime->CallJSApiCallbackWithValue(piper::ApiCallBack(callback_id),
                                        std::move(piper_data));
  });
}

void LynxBTSRuntimeProxyImpl::CallJSIntersectionObserver(int32_t observer_id,
                                                         int32_t callback_id,
                                                         ParamsGetter getter) {
  if (!actor_) {
    return;
  }
  [[maybe_unused]] uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CALL_JS_INTERSECTION_OBSERVER_PROXY,
              [flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
                ctx.event()->add_debug_annotations(
                    kTaskName, kJSTaskCallJSIntersectionObserver);
              });
  auto enqueue_info = tasm::performance::JSBlockingMonitor::MarkJSTaskEnqueue();

  actor_->Act([enqueue_info, observer_id, callback_id,
               getter = std::move(getter), flow_id](auto& runtime) {
    auto js_runtime = runtime->GetJSRuntime();
    if (js_runtime == nullptr) {
      LOGE(
          "try CallJSIntersectionObserver before js context is ready "
          "observer_id:"
          << observer_id << " callback_id:" << callback_id << &runtime);
      return;
    }
    runtime->GetDelegate()->AddJSBlockingTime(enqueue_info.enqueue_time);
    (void)flow_id;  // Explicitly reference `flow_id` to suppress the compiler
                    // warning.
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CALL_JS_INTERSECTION_OBSERVER,
                [flow_id](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_terminating_flow_ids(flow_id);
                });
    auto params = getter();
    if (params == nullptr) {
      LOGE(
          "try CallJSIntersectionObserver params is nullptr! "
          "observer_id:"
          << observer_id << " callback_id:" << callback_id << &runtime);
      return;
    }
    piper::Scope scope(*js_runtime);
    auto piper_data =
        pub::ValueUtils::ConvertValueToPiperValue(*js_runtime, *params.get());

    runtime->CallIntersectionObserver(observer_id, callback_id,
                                      std::move(piper_data));
  });
}

void LynxBTSRuntimeProxyImpl::EvaluateScript(const std::string& url,
                                             std::string script,
                                             int32_t callback_id) {
  if (!actor_) {
    return;
  }
  [[maybe_unused]] uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, EVALUATE_SCRIPT_PROXY,
              [flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
                ctx.event()->add_debug_annotations("task_name",
                                                   kJSTaskEvaluateScript);
              });
  auto enqueue_info = tasm::performance::JSBlockingMonitor::MarkJSTaskEnqueue();
  actor_->Act([enqueue_info, url, script = std::move(script), callback_id,
               flow_id](auto& runtime) {
    runtime->GetDelegate()->AddJSBlockingTime(enqueue_info.enqueue_time);
    (void)flow_id;  // Explicitly reference `flow_id` to suppress the compiler
                    // warning.
    TRACE_EVENT(LYNX_TRACE_CATEGORY, EVALUATE_SCRIPT,
                [flow_id](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_terminating_flow_ids(flow_id);
                });

    runtime->EvaluateScript(url, std::move(script),
                            piper::ApiCallBack(callback_id));
  });
}

void LynxBTSRuntimeProxyImpl::RejectDynamicComponentLoad(
    const std::string& url, int32_t callback_id, int32_t err_code,
    const std::string& err_msg) {
  if (!actor_) {
    return;
  }
  actor_->Act([url, callback_id, err_code, err_msg](auto& runtime) {
    runtime->CallJSApiCallbackWithValue(
        piper::ApiCallBack(callback_id),
        tasm::lazy_bundle::ConstructErrorMessageForBTS(url, err_code, err_msg));
  });
}

void LynxBTSRuntimeProxyImpl::AddLifecycleListener(
    std::unique_ptr<runtime::RuntimeLifecycleListenerDelegate> delegate) {
  [[maybe_unused]] uint64_t flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ADD_LIFE_CYCLE_LISTENER_PROXY,
              [flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(flow_id);
                ctx.event()->add_debug_annotations(kTaskName,
                                                   kJSTaskEvaluateScript);
              });
  if (!actor_) {
    return;
  }
  auto enqueue_info = tasm::performance::JSBlockingMonitor::MarkJSTaskEnqueue();
  actor_->Act([enqueue_info, delegate = std::move(delegate),
               flow_id](auto& runtime) mutable {
    runtime->GetDelegate()->AddJSBlockingTime(enqueue_info.enqueue_time);
    (void)flow_id;  // Explicitly reference `flow_id` to suppress the compiler
                    // warning.
    TRACE_EVENT(LYNX_TRACE_CATEGORY, ADD_LIFE_CYCLE_LISTENER,
                [flow_id](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_terminating_flow_ids(flow_id);
                });
    runtime->AddLifecycleListener(std::move(delegate));
  });
}

}  // namespace shell
}  // namespace lynx
