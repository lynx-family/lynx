// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/mts/mts_runtime_pool.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/devtool_wrapper/devtool_pool.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
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

MTSRuntimePool::~MTSRuntimePool() {
  is_destroying_.store(true, std::memory_order_release);
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, MTS_VM_POOL_STATE_EVENT,
                      "pool_instance_id", pool_instance_id_, "destroyed", "1");
}

void MTSRuntimePool::FillPool(int32_t count) {
  if (count <= 0 || is_destroying_.load(std::memory_order_acquire)) {
    return;
  }
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [count, weak_pool =
                  std::weak_ptr<MTSRuntimePool>(shared_from_this())]() mutable {
        auto pool = weak_pool.lock();
        if (!pool || pool->is_destroying_.load(std::memory_order_acquire)) {
          return;
        }
        pool->AddMTSRuntimeSafely(count);
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

void MTSRuntimePool::FillPoolSync(int32_t count) {
  if (count <= 0 || is_destroying_.load(std::memory_order_acquire)) {
    return;
  }
  AddMTSRuntimeSafely(count);
}

void MTSRuntimePool::AddMTSRuntimeSafely(int32_t count) {
  if (is_destroying_.load(std::memory_order_acquire)) {
    return;
  }
  decltype(mts_runtimes_) temp_mts_runtimes;
  uint32_t mode = tasm::performance::MemoryMonitor::ScriptingEngineMode(true);
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
      tasm::Renderer::RegisterBuiltin(mts_runtime.get(), arch_option_,
                                      enable_element_api_new_registration_);
      mts_runtime->RegisterLynx(enable_signal_api_);

      if (devtool_pool_ != nullptr &&
          context_type_ == runtime::ContextType::LepusNGContextType &&
          enable_mts_pre_execute_) {
        devtool_pool_->CreateDevTool();
        auto lepus_observer = devtool_pool_->OnMTSRuntimeCreated();
        // Currently, only support debugging the main entry.
        mts_runtime->InitInspector(lepus_observer, LEPUS_DEFAULT_CONTEXT_NAME);
        static const std::string file_name = "file:///main-thread.js";
        mts_runtime->SetDebugInfoURL(debug_info_url_, file_name);
      }

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

#if ENABLE_TRACE_PERFETTO
  ReportPoolState();
#endif
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
  if (enable_auto_generate_ &&
      !is_destroying_.load(std::memory_order_acquire)) {
    FillPool(1);
  }

  return mts_runtime;
}

void MTSRuntimePool::SetEnableAutoGenerate(bool enable) {
  enable_auto_generate_ = enable;
}

#if ENABLE_TRACE_PERFETTO
void MTSRuntimePool::InitReportPoolState() {
  static std::atomic<int32_t> id{0};
  pool_instance_id_ = id++;
  report_pool_state_ = std::make_unique<base::NotificationCallback>(
      LYNX_ON_TRACE_BEGIN_NOTIFICATION,
      [&](const std::string& tag, intptr_t data) { ReportPoolState(); });
}

void MTSRuntimePool::ReportPoolState() {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, MTS_VM_POOL_STATE_EVENT,
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations(
                            "pool_instance_id",
                            std::to_string(pool_instance_id_));
                        int index = 0;
                        for (auto& vm_inst : mts_runtimes_) {
                          ctx.event()->add_debug_annotations(
                              std::string("id_") + std::to_string(index++),
                              vm_inst->GetMTSContext()->GetDebugDescription());
                        }
                      });
}
#endif

}  // namespace shell
}  // namespace lynx
