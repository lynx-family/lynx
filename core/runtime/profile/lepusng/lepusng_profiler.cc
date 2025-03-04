// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/lepusng/lepusng_profiler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"

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
namespace profile {

LepusNGProfiler::LepusNGProfiler(std::shared_ptr<lepus::Context> context)
    : weak_context_(context) {}

LepusNGProfiler::~LepusNGProfiler() {
  task_runner_ = nullptr;
  weak_context_.reset();
}

void LepusNGProfiler::StartProfiling(bool is_create) {
  auto task = [weak_context = weak_context_] {
    auto context = weak_context.lock();
    if (context != nullptr && context->context()) {
      StartCpuProfiler(context->context());
    }
  };
  RuntimeProfiler::StartProfiling(std::move(task), is_create);
}

std::unique_ptr<RuntimeProfile> LepusNGProfiler::StopProfiling(
    bool is_destory) {
  std::string runtime_profile = "";
  auto task = [&runtime_profile, weak_context = weak_context_] {
    auto context = weak_context.lock();
    if (context != nullptr && context->context()) {
      auto result = StopCpuProfiler(context->context());
      runtime_profile =
          lepus::LEPUSValueHelper::ToStdString(context->context(), result);
      LEPUS_FreeValue(context->context(), result);
      QJSDebuggerFree(context->context());
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
    if (context != nullptr && context->context()) {
      QJSDebuggerInitialize(context->context());
      SetCpuProfilerInterval(context->context(), sampling_interval);
    }
  };

  RuntimeProfiler::SetupProfiling(std::move(task));
}

trace::RuntimeProfilerType LepusNGProfiler::GetType() {
  return trace::RuntimeProfilerType::quickjs;
}

}  // namespace profile
}  // namespace lynx

#endif
