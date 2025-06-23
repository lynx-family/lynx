// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/harmony/performance_controller_harmony.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/services/timing_handler/timing_handler.h"
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
      DECLARE_NAPI_FUNCTION("markPaintEndTimingIfNeeded",
                            MarkPaintEndTimingIfNeeded),
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

napi_value PerformanceControllerHarmonyJSWrapper::Constructor(
    napi_env env, napi_callback_info info) {
  /**
   * constructor(ref: Object, func:Function);
   * 0 - ref: Object `PerformanceController`
   * 1 - func:Function
   */
  size_t argc = 2;
  napi_value args[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  // C++ take the ownership of the instance.
  PerformanceControllerHarmonyJSWrapper* perf_controller =
      new PerformanceControllerHarmonyJSWrapper(env);
  // 0 - ref: Object
  napi_create_reference(env, args[0], 1, &perf_controller->js_impl_strong_ref_);
  // 1 - func:Function
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
  PerformanceControllerHarmonyJSWrapper* js_wrapper;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&js_wrapper));
  if (!js_wrapper) {
    return nullptr;
  }
  // 0 - timestamp: number
  uint64_t timestamp_us = base::NapiUtil::ConvertToBigUInt64(env, argv[0]);
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

napi_value PerformanceControllerHarmonyJSWrapper::MarkPaintEndTimingIfNeeded(
    napi_env env, napi_callback_info info) {
  /**
   * markPaintEndTimingIfNeeded(): void;
   */
  size_t argc = 0;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  PerformanceControllerHarmonyJSWrapper* js_wrapper;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&js_wrapper));
  if (!js_wrapper) {
    return nullptr;
  }
  auto nativeActorPtr = js_wrapper->actor_.lock();
  if (!nativeActorPtr) {
    return nullptr;
  }
  lynx::tasm::timing::TimestampUs timestamp_us =
      lynx::base::CurrentSystemTimeMicroseconds();
  nativeActorPtr->ActAsync([timestamp_us](auto& controller) {
    controller->GetTimingHandler().SetPaintEndTimingIfNeeded(timestamp_us);
  });
  return nullptr;
}

void PerformanceControllerHarmonyJSWrapper::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry_map) {
  auto shared_this = shared_from_this();
  base::UIThread::GetRunner()->PostTask(
      [shared_this, lepus_entry_map = pub::ValueUtils::ConvertValueToLepusValue(
                        *entry_map)]() mutable {
        if (!shared_this->js_impl_strong_ref_ ||
            !shared_this->js_on_performance_event_func_ref_) {
          return;
        }
        base::NapiHandleScope scope(shared_this->env_);
        size_t argc = 1;
        napi_value argv[argc];
        // arg[0] - PerformanceEvent
        argv[0] = base::NapiConvertHelper::CreateNapiValue(shared_this->env_,
                                                           lepus_entry_map);
        // Call JS Method :
        // PerformanceController.onPerformanceEvent(entry: PerformanceEntry):
        // void
        base::NapiUtil::InvokeJsMethod(
            shared_this->env_, shared_this->js_impl_strong_ref_,
            shared_this->js_on_performance_event_func_ref_, argc, argv);
      });
}

// Run on report thread.
PerformanceControllerHarmonyJSWrapper::
    ~PerformanceControllerHarmonyJSWrapper() {
  base::UIThread::GetRunner()->PostSyncTask(
      [env = env_, js_impl_ref = js_impl_strong_ref_,
       js_on_performance_event_func_ref =
           js_on_performance_event_func_ref_]() mutable {
        base::NapiHandleScope scope(env);
        napi_value js_object =
            base::NapiUtil::GetReferenceNapiValue(env, js_impl_ref);
        if (js_object) {
          napi_remove_wrap(env, js_object, nullptr);
        }
        napi_delete_reference(env, js_impl_ref);
        if (js_on_performance_event_func_ref) {
          napi_delete_reference(env, js_on_performance_event_func_ref);
        }
      });
  env_ = nullptr;
  js_on_performance_event_func_ref_ = nullptr;
  js_impl_strong_ref_ = nullptr;
}
}  // namespace performance
}  // namespace tasm
}  // namespace lynx
