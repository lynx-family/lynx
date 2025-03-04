// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_runtime_proxy_impl.h"

#include <string>
#include <utility>

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace shell {

void LynxRuntimeProxyImpl::CallJSFunction(std::string module_id,
                                          std::string method_id,
                                          std::unique_ptr<pub::Value> params) {
  if (!actor_) {
    return;
  }
  actor_->Act([module_id = std::move(module_id),
               method_id = std::move(method_id), params = std::move(params),
               is_runtime_standalone_mode = is_runtime_standalone_mode_,
               instance_id = actor_->GetInstanceId()](auto& runtime) mutable {
    auto task = [&runtime, module_id = std::move(module_id),
                 method_id = std::move(method_id), params = std::move(params),
                 instance_id] {
      auto js_runtime = runtime->GetJSRuntime();
      if (js_runtime == nullptr) {
        LOGE("try call js module before js context is ready! module:"
             << module_id << " method:" << method_id << &runtime);
        return;
      }
      piper::Scope scope(*js_runtime);

      // Timing
      tasm::timing::LongTaskMonitor::Scope long_task_scope(
          instance_id, tasm::timing::kJSFuncTask,
          tasm::timing::kTaskNameJSProxyCallJSFunction);
      std::string first_arg_str;
      if (params->Length() > 0) {
        first_arg_str = params->GetValueAtIndex(0)->str();
      }

      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, "CallJSFunction",
          [&](lynx::perfetto::EventContext ctx) {
            ctx.event()->add_debug_annotations("module_name", module_id);
            ctx.event()->add_debug_annotations("method_name", method_id);
            if (params->Length() > 0) {
              ctx.event()->add_debug_annotations("first_arg", first_arg_str);
            }
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
                        "CallJSFunction:ConvertValueToPiperArray");
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
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallJSFunction:Fire");
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

void LynxRuntimeProxyImpl::CallJSApiCallbackWithValue(
    int32_t callback_id, std::unique_ptr<pub::Value> params) {
  if (!actor_) {
    return;
  }
  actor_->Act([callback_id, params = std::move(params)](auto& runtime) {
    auto js_runtime = runtime->GetJSRuntime();
    if (js_runtime == nullptr) {
      LOGR(
          "try CallJSApiCallbackWithValue before js context is ready! "
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

void LynxRuntimeProxyImpl::CallJSIntersectionObserver(
    int32_t observer_id, int32_t callback_id,
    std::unique_ptr<pub::Value> params) {
  if (!actor_) {
    return;
  }
  actor_->Act([observer_id, callback_id,
               params = std::move(params)](auto& runtime) {
    auto js_runtime = runtime->GetJSRuntime();
    if (js_runtime == nullptr) {
      LOGE(
          "try CallJSIntersectionObserver before js context is ready! "
          "observer_id:"
          << observer_id << " callback_id:" << callback_id << &runtime);
      return;
    }
    piper::Scope scope(*js_runtime);
    auto piper_data =
        pub::ValueUtils::ConvertValueToPiperValue(*js_runtime, *params.get());

    runtime->CallIntersectionObserver(
        (int32_t)observer_id, (int32_t)callback_id, std::move(piper_data));
  });
}

void LynxRuntimeProxyImpl::EvaluateScript(const std::string& url,
                                          std::string script,
                                          int32_t callback_id) {
  if (!actor_) {
    return;
  }
  actor_->Act([url, script = std::move(script), callback_id](auto& runtime) {
    runtime->EvaluateScript(url, std::move(script),
                            piper::ApiCallBack(callback_id));
  });
}

void LynxRuntimeProxyImpl::RejectDynamicComponentLoad(
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

}  // namespace shell
}  // namespace lynx
