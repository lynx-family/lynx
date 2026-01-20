// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/context_pool.h"

#include <algorithm>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/lepus/context.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"

namespace lynx {
namespace lepus {

std::shared_ptr<LynxContextPool> LynxContextPool::Create(
    bool is_lepus_ng, bool disable_tracing_gc) {
  return std::shared_ptr<LynxContextPool>(
      new LynxContextPool(is_lepus_ng, disable_tracing_gc));
}

std::shared_ptr<LynxContextPool> LynxContextPool::Create(
    bool is_lepus_ng, bool disable_tracing_gc,
    const std::shared_ptr<ContextBundle>& context_bundle,
    const tasm::CompileOptions& compile_options,
    tasm::PageConfig* page_configs) {
  return std::shared_ptr<LynxContextPool>(
      new LynxContextPool(is_lepus_ng, disable_tracing_gc, context_bundle,
                          compile_options, page_configs));
}

void LynxContextPool::FillPool(int32_t count) {
  if (count <= 0) {
    return;
  }
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [count, weak_pool = std::weak_ptr<LynxContextPool>(
                  shared_from_this())]() mutable {
        auto context_pool = weak_pool.lock();
        if (context_pool) {
          context_pool->AddContextSafely(count);
        }
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

void LynxContextPool::AddContextSafely(int32_t count) {
  // build contexts without lock
  decltype(contexts_) temp_contexts;
  uint32_t mode = tasm::performance::MemoryMonitor::ScriptingEngineMode();
  for (; count > 0; --count) {
    std::shared_ptr<Context> context =
        Context::CreateContext(is_lepus_ng_, disable_tracing_gc_, mode);
    if (context_bundle_) {
      context->SetSdkVersion(target_sdk_version_);
      context->Initialize();
      if (!is_lepus_ng_) {
        // For lepus context, kTemplateAssembler needs to maintain a placeholder
        // to ensure the function index remains unchanged; otherwise, the
        // context cannot run correctly. It will be reset to the pointer of tasm
        // on runtime.
        context->SetGlobalData(BASE_STATIC_STRING(tasm::kTemplateAssembler),
                               lepus::Value());
      }
      tasm::Renderer::RegisterBuiltin(context.get(), arch_option_);
      context->RegisterLynx(enable_signal_api_);
      // if context_bundle_ exists, should call DeSerialize. And if DeSerialize
      // fails, just return.
      if (!context->DeSerialize(*context_bundle_, false, nullptr)) {
        return;
      }
      // Try Execute
      if (enable_mts_pre_execute_) {
        context->TryExecute();
      }
    }
    temp_contexts.emplace_back(std::move(context));
  }

  // lock and insert
  std::lock_guard<std::mutex> lock{mtx_};
  for (auto& c : temp_contexts) {
    contexts_.emplace_back(std::move(c));
  }
}

std::shared_ptr<lepus::Context> LynxContextPool::TakeContextSafely() {
  std::shared_ptr<lepus::Context> context = nullptr;
  {
    // lock to take context safely
    std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
    if (!lock.owns_lock() || contexts_.empty()) {
      return nullptr;
    }
    context.swap(contexts_.back());
    contexts_.pop_back();
  }

  // generate a new context
  if (enable_auto_generate_) {
    FillPool(1);
  }

  return context;
}

void LynxContextPool::SetEnableAutoGenerate(bool enable) {
  enable_auto_generate_ = enable;
}

}  // namespace lepus
}  // namespace lynx
