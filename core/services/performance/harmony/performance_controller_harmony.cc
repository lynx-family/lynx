// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/harmony/performance_controller_harmony.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/services/trace/service_trace_event_def.h"
#include "core/value_wrapper/harmony/value_impl_napi.h"

namespace lynx {
namespace tasm {
namespace performance {

napi_value PerformanceControllerHarmonyJSWrapper::Init(napi_env env,
                                                       napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("setTiming", SetTiming),
      DECLARE_NAPI_FUNCTION("markTiming", MarkTiming),
      DECLARE_NAPI_FUNCTION("destroy", Destroy),
  };
#undef DECLARE_NAPI_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "PerformanceController", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "PerformanceController", cons);
  return exports;
}

// Run on UI Thread
napi_value PerformanceControllerHarmonyJSWrapper::Constructor(
    napi_env env, napi_callback_info info) {
  /**
   * constructor(ref: Object, onPerformanceEvent: Function);
   * 0 - ref: Object `PerformanceController`
   * 1 - onPerformanceEvent: Function
   */
  size_t argc = 2;
  napi_value args[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  // C++ take the ownership of the instance.
  PerformanceControllerHarmonyJSWrapper* perf_controller =
      new PerformanceControllerHarmonyJSWrapper(env);
  // 0 - ref: js PerformanceController
  napi_create_reference(env, args[0], 1, &perf_controller->js_impl_strong_ref_);
  // 1 - func: PerformanceController.onPerformanceEvent
  napi_valuetype js_func_type;
  napi_typeof(env, args[1], &js_func_type);
  if (js_func_type != napi_null) {
    napi_create_reference(env, args[1], 0,
                          &perf_controller->js_on_performance_event_func_ref_);
  }
  // as `js_this.native = perf_controller`
  napi_wrap(
      env, js_this, perf_controller, [](napi_env, void* data, void*) {},
      nullptr, nullptr);
  return js_this;
}

// Run on UI Thread
napi_value PerformanceControllerHarmonyJSWrapper::SetTiming(
    napi_env env, napi_callback_info info) {
  /**
   * setTiming(timestamp: number, timingKey: string, pipelineID: string): void;
   * 0 - timestamp: number
   * 1 - timingKey: string
   * 2 - pipelineID: string
   */
  size_t argc = 3;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  PerformanceControllerHarmonyJSWrapper* js_wrapper = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&js_wrapper));
  if (!js_wrapper) {
    return nullptr;
  }
  // 0 - timestamp: number
  uint64_t timestamp_us = base::NapiUtil::ConvertToInt64(env, argv[0]);
  // 1 - timingKey: string
  std::string timing_key = base::NapiUtil::ConvertToShortString(env, argv[1]);
  // 2 - pipelineID: string
  std::string pipeline_id = base::NapiUtil::ConvertToString(env, argv[2]);

  auto nativeActorPtr = js_wrapper->actor_.lock();
  if (!nativeActorPtr) {
    return nullptr;
  }
  nativeActorPtr->ActAsync([timing_key = std::move(timing_key), timestamp_us,
                            pipeline_id = std::move(pipeline_id)](
                               auto& controller) mutable {
    controller->GetTimingHandler().SetTiming(
        timing_key, static_cast<lynx::tasm::timing::TimestampUs>(timestamp_us),
        pipeline_id);
  });
  return nullptr;
}

PerformanceControllerHarmonyJSWrapper::PerformanceControllerHarmonyJSWrapper(
    napi_env env)
    : env_(env) {
  LOGI("new PerformanceControllerHarmonyJSWrapper, this:" << this);
};

PerformanceControllerHarmonyJSWrapper::
    ~PerformanceControllerHarmonyJSWrapper() {
  LOGI("PerformanceControllerHarmonyJSWrapper::~, this:" << this);
}

// Run on UI Thread
napi_value PerformanceControllerHarmonyJSWrapper::MarkTiming(
    napi_env env, napi_callback_info info) {
  /**
   * markTiming(timingKey: string, pipelineID: string): void;
   * 0 - timingKey: string
   * 1 - pipelineID: string
   */
  size_t argc = 2;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  PerformanceControllerHarmonyJSWrapper* js_wrapper = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&js_wrapper));
  if (!js_wrapper) {
    return nullptr;
  }
  // 0 - timingKey: string
  std::string timing_key = base::NapiUtil::ConvertToShortString(env, argv[0]);
  // 1 - pipelineID: string
  std::string pipeline_id = base::NapiUtil::ConvertToString(env, argv[1]);

  auto nativeActorPtr = js_wrapper->actor_.lock();
  if (!nativeActorPtr) {
    return nullptr;
  }
  lynx::tasm::timing::TimestampUs timestamp_us =
      lynx::base::CurrentSystemTimeMicroseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, TIMING_MARK + timing_key,
      [&pipeline_id, &timing_key, timestamp_us,
       instance_id =
           nativeActorPtr->GetInstanceId()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("timing_key", timing_key);
        ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(timestamp_us));
        ctx.event()->add_debug_annotations("instance_id",
                                           std::to_string(instance_id));
      });
  nativeActorPtr->ActAsync([timing_key = std::move(timing_key), timestamp_us,
                            pipeline_id = std::move(pipeline_id)](
                               auto& controller) mutable {
    controller->GetTimingHandler().SetTiming(
        timing_key, static_cast<lynx::tasm::timing::TimestampUs>(timestamp_us),
        pipeline_id);
  });
  return nullptr;
}

// Run on UI Thread
void PerformanceControllerHarmonyJSWrapper::OnPerformanceEvent(
    lepus::Value lepus_entry_map) {
  if (!js_impl_strong_ref_ || !js_on_performance_event_func_ref_ || !env_) {
    return;
  }
  base::NapiHandleScope scope(env_);
  size_t argc = 1;
  napi_value argv[argc];
  // arg[0] - PerformanceEvent
  argv[0] = base::NapiConvertHelper::CreateNapiValue(env_, lepus_entry_map);
  // Call JS Method :
  // PerformanceController.onPerformanceEvent(entry: PerformanceEntry):
  // void
  base::NapiUtil::InvokeJsMethod(env_, js_impl_strong_ref_,
                                 js_on_performance_event_func_ref_, argc, argv);
}

// Run on UI Thread
napi_value PerformanceControllerHarmonyJSWrapper::Destroy(
    napi_env env, napi_callback_info info) {
  LOGI("static PerformanceControllerHarmonyJSWrapper::Destroy");
  size_t argc = 0;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  PerformanceControllerHarmonyJSWrapper* js_wrapper = nullptr;
  // remove js_this
  napi_status status =
      napi_remove_wrap(env, js_this, reinterpret_cast<void**>(&js_wrapper));
  if (js_wrapper) {
    js_wrapper->Destroy();
    js_wrapper = nullptr;
  }
  NAPI_THROW_IF_FAILED_NULL(
      env, status,
      "PerformanceControllerHarmonyJSWrapper napi_remove_wrap failed!");
  return nullptr;
}

// Run on UI Thread
void PerformanceControllerHarmonyJSWrapper::Destroy() {
  if (!env_) {
    return;
  }
  LOGI("PerformanceControllerHarmonyJSWrapper::Destroy(), this:" << this);
  base::NapiHandleScope scope(env_);
  if (js_impl_strong_ref_) {
    // get js_this of PerformanceController
    napi_value js_object =
        base::NapiUtil::GetReferenceNapiValue(env_, js_impl_strong_ref_);
    if (js_object) {
      // as `js_this.native = null`
      napi_remove_wrap(env_, js_object, nullptr);
    }
    // remove js_this of PerformanceController
    napi_delete_reference(env_, js_impl_strong_ref_);
    js_impl_strong_ref_ = nullptr;
  }
  if (js_on_performance_event_func_ref_) {
    // remove ref of PerformanceController.onPerformanceEvent
    napi_delete_reference(env_, js_on_performance_event_func_ref_);
    js_on_performance_event_func_ref_ = nullptr;
  }
  env_ = nullptr;
}

// Run on Report Thread
void PerformanceControllerHarmony::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry_map) {
  if (!js_wrapper_) {
    return;
  }
  base::UIThread::GetRunner()->PostTask(
      [js_wrapper = js_wrapper_,
       lepus_entry_map =
           pub::ValueUtils::ConvertValueToLepusValue(*entry_map)]() mutable {
        js_wrapper->OnPerformanceEvent(std::move(lepus_entry_map));
      });
}

PerformanceControllerHarmony::PerformanceControllerHarmony(
    std::shared_ptr<PerformanceControllerHarmonyJSWrapper> wrapper)
    : js_wrapper_(std::move(wrapper)) {
  LOGI("new PerformanceControllerHarmony, this:" << this);
};

// Run on Report Thread
PerformanceControllerHarmony::~PerformanceControllerHarmony() {
  LOGI("PerformanceControllerHarmony::~, this:" << this);
  if (!js_wrapper_) {
    return;
  }
  base::UIThread::GetRunner()->PostTask(
      [js_wrapper = js_wrapper_]() { js_wrapper->Destroy(); });
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
