// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/mts/mts_runtime_pool.h"

#include <algorithm>

#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/devtool_wrapper/devtool_pool.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"

namespace lynx {
namespace shell {

std::shared_ptr<MTSRuntimePool> MTSRuntimePool::Create(
    runtime::ContextType context_type, bool disable_tracing_gc) {
  return std::shared_ptr<MTSRuntimePool>(
      new MTSRuntimePool(context_type, disable_tracing_gc));
}

std::shared_ptr<MTSRuntimePool> MTSRuntimePool::Create(
    runtime::ContextType context_type, const std::string& template_url,
    bool disable_tracing_gc,
    const std::shared_ptr<runtime::ContextBundle>& context_bundle,
    const tasm::CompileOptions& compile_options,
    tasm::PageConfig* page_configs) {
  return std::shared_ptr<MTSRuntimePool>(
      new MTSRuntimePool(context_type, template_url, disable_tracing_gc,
                         context_bundle, compile_options, page_configs));
}

MTSRuntimePool::~MTSRuntimePool() {
  is_destroying_.store(true, std::memory_order_release);
  {
    // ReportPoolState posts its update while holding this mutex. Crossing the
    // mutex here guarantees that destruction is queued after every update.
    std::lock_guard<std::mutex> lock{mtx_};
  }
  if (!is_global_pool_) {
    tasm::performance::GlobalMemoryMonitor::GetInstance()
        .OnMTSRuntimePoolDestroy(pool_instance_id_);
  }
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

bool MTSRuntimePool::Preload(std::string url, std::vector<uint8_t> bytecode,
                             base::MoveOnlyClosure<> callback) {
  if (context_type_ != runtime::ContextType::LepusNGContextType ||
      url.empty() || bytecode.empty() || enable_auto_generate_ ||
      is_destroying_.load(std::memory_order_acquire)) {
    LOGE("MTSRuntimePool preload failed");
    return false;
  }

  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [url = std::move(url), bytecode = std::move(bytecode),
       callback = std::move(callback),
       weak_pool =
           std::weak_ptr<MTSRuntimePool>(shared_from_this())]() mutable {
        auto pool = weak_pool.lock();
        if (!pool || pool->is_destroying_.load(std::memory_order_acquire)) {
          LOGE("MTSRuntimePool preload failed");
          return;
        }

        bool success = true;
        {
          std::lock_guard<std::mutex> lock{pool->mtx_};
          if (pool->mts_runtimes_.empty()) {
            success = false;
          }
          for (const auto& runtime : pool->mts_runtimes_) {
            lepus::Value result;
            if (!runtime ||
                !runtime->EvalBinary(bytecode.data(), bytecode.size(), result,
                                     url.c_str())) {
              success = false;
              break;
            }
          }
        }
        pool->ReportPoolState();

        if (!success) {
          LOGE("MTSRuntimePool preload failed");
        } else if (callback) {
          callback();
        }
      },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  return true;
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
  {
    std::lock_guard<std::mutex> lock{mtx_};
    for (auto& r : temp_mts_runtimes) {
      mts_runtimes_.emplace_back(std::move(r));
    }
  }
  ReportPoolState();
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
  ReportPoolState();

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

void MTSRuntimePool::InitReportPoolState() {
  static std::atomic<int32_t> id{0};
  pool_instance_id_ = id.fetch_add(1, std::memory_order_relaxed);
  created_at_ms_ = tasm::performance::MemoryNowMs();
#if ENABLE_TRACE_PERFETTO
  report_pool_state_ = std::make_unique<base::NotificationCallback>(
      LYNX_ON_TRACE_BEGIN_NOTIFICATION,
      [&](const std::string& tag, intptr_t data) { ReportPoolState(); });
#endif
}

void MTSRuntimePool::ReportPoolState() {
  if (is_destroying_.load(std::memory_order_acquire)) {
    return;
  }
  std::lock_guard<std::mutex> lock{mtx_};
  if (is_destroying_.load(std::memory_order_acquire)) {
    return;
  }
  int64_t heap_bytes = 0;
  for (const auto& runtime : mts_runtimes_) {
    if (runtime) {
      heap_bytes += std::max<int64_t>(0, runtime->GetCurrentHeapSizeBytes());
    }
  }
  if (!is_global_pool_) {
    tasm::performance::GlobalMemoryMonitor::GetInstance()
        .OnMTSRuntimePoolUpdate(pool_instance_id_, template_url_, context_type_,
                                created_at_ms_, mts_runtimes_.size(),
                                heap_bytes);
  }
#if ENABLE_TRACE_PERFETTO
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, MTS_VM_POOL_STATE_EVENT,
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations(
                            "pool_instance_id",
                            std::to_string(pool_instance_id_));
                        int index = 0;
                        for (const auto& runtime : mts_runtimes_) {
                          ctx.event()->add_debug_annotations(
                              std::string("id_") + std::to_string(index++),
                              runtime->GetMTSContext()->GetDebugDescription());
                        }
                      });
#endif
}

}  // namespace shell
}  // namespace lynx
