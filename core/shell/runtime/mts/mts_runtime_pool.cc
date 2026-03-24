// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/mts/mts_runtime_pool.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"

namespace lynx {
namespace shell {

std::shared_ptr<MTSRuntimePool> MTSRuntimePool::Create(
    runtime::ContextType context_type, bool disable_tracing_gc) {
  return std::shared_ptr<MTSRuntimePool>(
      new MTSRuntimePool(context_type, disable_tracing_gc));
}

std::shared_ptr<MTSRuntimePool> MTSRuntimePool::Create(
    runtime::ContextType context_type, bool disable_tracing_gc,
    const std::shared_ptr<runtime::ContextBundle>& context_bundle,
    const tasm::CompileOptions& compile_options,
    tasm::PageConfig* page_configs) {
  return std::shared_ptr<MTSRuntimePool>(
      new MTSRuntimePool(context_type, disable_tracing_gc, context_bundle,
                         compile_options, page_configs));
}

void MTSRuntimePool::FillPool(int32_t count) {
  if (count <= 0) {
    return;
  }
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [count, weak_pool =
                  std::weak_ptr<MTSRuntimePool>(shared_from_this())]() mutable {
        auto pool = weak_pool.lock();
        if (pool) {
          pool->AddMTSRuntimeSafely(count);
        }
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

void MTSRuntimePool::AddMTSRuntimeSafely(int32_t count) {
  decltype(mts_runtimes_) temp_mts_runtimes;
  uint32_t mode = tasm::performance::MemoryMonitor::ScriptingEngineMode();
  for (; count > 0; --count) {
    std::shared_ptr<runtime::MTSRuntime> mts_runtime =
        runtime::MTSRuntime::CreateContext(context_type_, disable_tracing_gc_,
                                           mode);
    if (!mts_runtime) {
      continue;
    }
    if (context_bundle_) {
      mts_runtime->SetSdkVersion(target_sdk_version_);
      mts_runtime->Initialize();
      if (context_type_ == runtime::ContextType::VMContextType) {
        // For lepus context, kTemplateAssembler needs to maintain a placeholder
        // to ensure the function index remains unchanged; otherwise, the
        // context cannot run correctly. It will be reset to the pointer of tasm
        // on runtime.
        mts_runtime->SetGlobalData(BASE_STATIC_STRING(tasm::kTemplateAssembler),
                                   lepus::Value());
      }
      tasm::Renderer::RegisterBuiltin(mts_runtime.get(), arch_option_);
      mts_runtime->RegisterLynx(enable_signal_api_);
      // if context_bundle_ exists, should call DeSerialize. And if DeSerialize
      // fails, just return.
      if (!mts_runtime->DeSerialize(*context_bundle_, false, nullptr)) {
        return;
      }
      // Try Execute
      if (enable_mts_pre_execute_) {
        mts_runtime->TryExecute();
      }
    }
    temp_mts_runtimes.emplace_back(std::move(mts_runtime));
  }

  // lock and insert
  std::lock_guard<std::mutex> lock{mtx_};
  for (auto& r : temp_mts_runtimes) {
    mts_runtimes_.emplace_back(std::move(r));
  }
}

std::shared_ptr<runtime::MTSRuntime> MTSRuntimePool::TakeMTSRuntimeSafely() {
  std::shared_ptr<runtime::MTSRuntime> mts_runtime = nullptr;
  {
    // lock to take context safely
    std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
    if (!lock.owns_lock() || mts_runtimes_.empty()) {
      return nullptr;
    }
    mts_runtime.swap(mts_runtimes_.back());
    mts_runtimes_.pop_back();
  }

  // generate a new context
  if (enable_auto_generate_) {
    FillPool(1);
  }

  return mts_runtime;
}

void MTSRuntimePool::SetEnableAutoGenerate(bool enable) {
  enable_auto_generate_ = enable;
}

}  // namespace shell
}  // namespace lynx
