// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/jsi/quickjs/quickjs_runtime_wrapper.h"

#include <mutex>
#include <utility>

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/cutils.h"
#ifdef __cplusplus
}
#endif

#include "base/trace/native/trace_controller.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/jsi/quickjs/quickjs_context_wrapper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_helper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_host_function.h"
#include "core/runtime/js/jsi/quickjs/quickjs_host_object.h"
#include "core/runtime/js/runtime_constant.h"

namespace lynx {
namespace runtime {
namespace js {
using detail::QuickjsHostFunctionProxy;
using detail::QuickjsHostObjectProxy;

LEPUSClassID QuickjsRuntimeInstance::s_function_id_ = 0;
LEPUSClassID QuickjsRuntimeInstance::s_object_id_ = 0;

QuickjsRuntimeInstance::QuickjsRuntimeInstance()
#if ENABLE_TRACE_PERFETTO
    : creation_time_as_unique_(fml::TimePoint::Now()),
      notification_callback_(std::make_unique<base::NotificationCallback>(
          base::NotificationCallback::CallbackList{
              {LYNX_ON_TRACE_BEGIN_NOTIFICATION,
               [this](const std::string& tag, intptr_t data) {
                 if (this->rt_) {
                   LEPUS_SetGCInfoThreshold(
                       this->rt_,
                       lynx::runtime::kMemoryReportDeltaThresholdInTrace);
                 }
               }},
              {kBTSReportMemoryInfo,
               [this](const std::string& tag, intptr_t data) {
                 if (this->rt_) {
                   LEPUS_ReportGCInfo(this->rt_);
                 }
               }},
              {kBTSTakeVMSnapshot,
               [this](const std::string& tag, intptr_t data) {
                 intptr_t* payload = reinterpret_cast<intptr_t*>(data);
                 js::JSIContext* ctx =
                     reinterpret_cast<js::JSIContext*>(payload[0]);
                 if (ctx &&
                     ctx->getVM().get() == static_cast<VMInstance*>(this)) {
                   const char* identifier =
                       reinterpret_cast<const char*>(payload[1]);
                   const bool initial = static_cast<bool>(payload[2]);
                   if (this->initial_snapshot_captured_ && initial) {
                     LOGI("Initial snapshot already captured of '" << identifier
                                                                   << "'");
                     return;
                   }
                   auto qjs_ctx = static_cast<QuickjsContextWrapper*>(ctx);
                   if (detail::QuickjsHelper::TakeHeapSnapshot(
                           qjs_ctx->getContext(), identifier)) {
                     if (initial) {
                       this->initial_snapshot_captured_ = true;
                     }
                   }
                 }
               }}}))
#endif
{
}

QuickjsRuntimeInstance::~QuickjsRuntimeInstance() {
  LOGE("LYNX free quickjs runtime start");
  if (rt_) {
    LEPUS_SetGCObserver(rt_, nullptr);
    LEPUS_FreeRuntime(rt_);
  }
  GetFunctionIdContainer().erase(rt_);
  GetObjectIdContainer().erase(rt_);

  rt_ = nullptr;
#if ENABLE_TRACE_PERFETTO
  ReportMemoryForTrace();
#endif

  LOGI("LYNX free quickjs runtime end. " << this << " LEPUSRuntime: " << rt_);
}

LepusIdContainer& QuickjsRuntimeInstance::GetObjectIdContainer() {
  static thread_local LepusIdContainer sObjectIdContainer;
  return sObjectIdContainer;
}

LepusIdContainer& QuickjsRuntimeInstance::GetFunctionIdContainer() {
  static thread_local LepusIdContainer sFunctionIdContainer;
  return sFunctionIdContainer;
}

void QuickjsRuntimeInstance::InitQuickjsRuntime(bool is_sync,
                                                uint32_t runtime_mode) {
  LEPUSRuntime* rt;
  rt = LEPUS_NewRuntimeWithMode(runtime_mode);
  if (!rt) {
    LOGE("init quickjs runtime failed!");
    return;
  }
  if (tasm::LynxEnv::GetInstance().IsDisableTracingGC()) {
    LEPUS_SetRuntimeInfo(rt, "Lynx_JS_RC");
  } else {
    LEPUS_SetRuntimeInfo(rt, "Lynx_JS");
  }
  rt_ = rt;

  LEPUS_SetGCObserver(rt_, static_cast<GCObserver*>(this));

#if ENABLE_TRACE_PERFETTO
  if (trace::TraceController::Instance()->IsTracingStarted()) {
    LEPUS_SetGCInfoThreshold(rt_,
                             lynx::runtime::kMemoryReportDeltaThresholdInTrace);
  }
#endif

  static std::once_flag s_init_id_flag;
  static LEPUSClassDef s_function_class_def;
  static LEPUSClassExoticMethods s_exotic_method;
  static LEPUSClassDef s_object_class_def;
  std::call_once(s_init_id_flag, [] {
    LEPUS_NewClassID(&s_function_id_);
    LEPUS_NewClassID(&s_object_id_);
    // init function class def
    s_function_class_def.class_name = "LynxFunctionDef";
    s_function_class_def.finalizer = QuickjsHostFunctionProxy::hostFinalizer;
    s_function_class_def.call = QuickjsHostFunctionProxy::FunctionCallback;

    // init exotic method
    s_exotic_method.get_own_property = QuickjsHostObjectProxy::getOwnProperty;
    s_exotic_method.get_own_property_names =
        QuickjsHostObjectProxy::getPropertyNames;
    s_exotic_method.get_property = QuickjsHostObjectProxy::getProperty;
    s_exotic_method.set_property = QuickjsHostObjectProxy::setProperty;

    // init object class def
    s_object_class_def.class_name = "LynxObjectClassDef";
    s_object_class_def.finalizer = QuickjsHostObjectProxy::hostFinalizer;
    s_object_class_def.exotic = &s_exotic_method;
  });

  LEPUS_NewClass(rt_, s_function_id_, &s_function_class_def);
  LEPUS_NewClass(rt_, s_object_id_, &s_object_class_def);

  if (is_sync) {
    AddToIdContainer();
  }
#if LYNX_ENABLE_FROZEN_MODE
  // Due to the fact that QuickJS’s GC traverses all objects in a Stop The World
  // fashion to try to free the circular objects, it causes extra time
  // consumption once QuickJS triggers GC. Currently, the default GC threshold
  // of QuickJS is 256 bytes, and even a slight change in the JS Framework may
  // cause changes in the GC timing. The impact is that in performance
  // degradation tests, some indicators may experience significant fluctuations
  // due to changes in the GC timing. To avoid the GC from fluctuating the
  // performance, when LYNX_ENABLE_FROZEN_MODE is enabled, set the GC of the JS
  // QuickJSRuntime to INT_MAX. In the long run, a reasonable GC threshold needs
  // to be set for QuickJS.
  LEPUS_SetGCThreshold(rt_, INT_MAX);
#endif
  LOGI("lynx InitQuickjsRuntime success");
}

void QuickjsRuntimeInstance::OnGC(std::string mem_info) {
#if ENABLE_TRACE_PERFETTO
  ReportMemoryForTrace();
#endif
  for (auto* observer : obs_set_ptr_) {
    observer->OnRuntimeGC({{kRawRuntimeMemoryInfo, mem_info}});
  }
}

#if ENABLE_TRACE_PERFETTO
void QuickjsRuntimeInstance::ReportMemoryForTrace() {
  if (!trace::TraceController::Instance()->IsTracingStarted()) {
    return;
  }

  // When Lynx Trace is enabled, memory data is reported directly on the JS
  // thread. Otherwise, multithreading may cause the time sequence of events to
  // be disordered, hindering automated analysis.
  auto now = fml::TimePoint::Now();
  if (rt_ && ((now - last_trace_event_time_).ToMilliseconds() < 16)) {
    return;
  }
  last_trace_event_time_ = now;
  std::string track_name =
      "bts_vm_acc_" +
      std::to_string(creation_time_as_unique_.ToEpochDelta().ToNanoseconds());

  auto usage = detail::QuickjsHelper::GetMemoryUsage(rt_);
  TRACE_COUNTER(LYNX_TRACE_CATEGORY, track_name.c_str(), usage.heap_size,
                kRawRuntimeBaseMemoryInfo, usage.base_size,
                kRawRuntimePageRssMemoryInfo, usage.page_rss_size, "ptr",
                static_cast<VMInstance*>(this));
}
#endif

void QuickjsRuntimeInstance::AddObserver(JSIObserver* obs) {
  if (!obs) {
    return;
  }
  obs_set_ptr_.emplace(obs);
}

void QuickjsRuntimeInstance::RemoveObserver(JSIObserver* obs) {
  if (!obs) {
    return;
  }
  obs_set_ptr_.erase(obs);
}

std::string QuickjsRuntimeInstance::GetDebugDescription() const {
  return detail::QuickjsHelper::GetDebugDescription(rt_);
}

void QuickjsRuntimeInstance::AddToIdContainer() {
  GetFunctionIdContainer().insert({rt_, s_function_id_});
  GetObjectIdContainer().insert({rt_, s_object_id_});
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
