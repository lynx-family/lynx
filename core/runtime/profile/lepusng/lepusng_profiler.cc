// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/lepusng/lepusng_profiler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/runtime/lepusng/jsvalue_helper.h"

#if ENABLE_TRACE_PERFETTO
#ifdef __cplusplus
extern "C" {
#endif
extern void StartCpuProfiler(LEPUSContext *ctx);
extern LEPUSValue StopCpuProfiler(LEPUSContext *ctx);
extern void SetCpuProfilerInterval(LEPUSContext *ctx, uint64_t);
void QJSDebuggerInitialize(LEPUSContext *ctx);
void QJSDebuggerFree(LEPUSContext *ctx);
#ifdef __cplusplus
}
#endif

namespace lynx {
namespace runtime {
namespace profile {

LepusNGProfiler::LepusNGProfiler(std::shared_ptr<runtime::MTSRuntime> context) {
  if (context->IsLepusNGContext()) {
    weak_context_ = context;
  }
}

LepusNGProfiler::~LepusNGProfiler() {
  task_runner_ = nullptr;
  weak_context_.reset();
}

void LepusNGProfiler::StartProfiling(bool is_create) {
  auto task = [weak_context = weak_context_] {
    auto context = weak_context.lock();
    if (context != nullptr &&
        runtime::MTSRuntime::ToQuickContext(context.get())->context()) {
      StartCpuProfiler(
          runtime::MTSRuntime::ToQuickContext(context.get())->context());
    }
  };
  RuntimeProfiler::StartProfiling(std::move(task), is_create);
}

std::unique_ptr<RuntimeProfile> LepusNGProfiler::StopProfiling(
    bool is_destory) {
  std::string runtime_profile = "";
  auto task = [&runtime_profile, weak_context = weak_context_] {
    auto context = weak_context.lock();
    if (context != nullptr &&
        runtime::MTSRuntime::ToQuickContext(context.get())->context()) {
      auto quick_context = runtime::MTSRuntime::ToQuickContext(context.get());
      auto result = StopCpuProfiler(quick_context->context());
      runtime_profile = lepus::LEPUSValueHelper::ToStdString(
          quick_context->context(), result);
      // Only free value when not in GC mode
      if (!LEPUS_IsGCMode(quick_context->context())) {
        LEPUS_FreeValue(quick_context->context(), result);
      }
      QJSDebuggerFree(quick_context->context());
    }
  };

  RuntimeProfiler::StopProfiling(std::move(task), is_destory);

  if (!runtime_profile.empty()) {
    return std::make_unique<RuntimeProfile>(runtime_profile, track_id_);
  }
  return nullptr;
}

void LepusNGProfiler::SetupProfiling(int32_t sampling_interval) {
  auto task = [weak_context = weak_context_, sampling_interval] {
    auto context = weak_context.lock();
    if (context != nullptr &&
        runtime::MTSRuntime::ToQuickContext(context.get())->context()) {
      QJSDebuggerInitialize(
          runtime::MTSRuntime::ToQuickContext(context.get())->context());
      SetCpuProfilerInterval(
          runtime::MTSRuntime::ToQuickContext(context.get())->context(),
          sampling_interval);
    }
  };

  RuntimeProfiler::SetupProfiling(std::move(task));
}

trace::RuntimeProfilerType LepusNGProfiler::GetType() {
  return trace::RuntimeProfilerType::quickjs;
}

}  // namespace profile
}  // namespace runtime
}  // namespace lynx

#endif
