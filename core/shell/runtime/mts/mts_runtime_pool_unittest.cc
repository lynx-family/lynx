// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/shell/runtime/mts/mts_runtime_pool.h"

#include <chrono>
#include <future>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

namespace {

std::vector<uint8_t> GenerateBytecode(const char* source) {
  lepus::QuickContext compiler_context;
  if (!lepus::BytecodeGenerator::GenerateBytecode(&compiler_context, source,
                                                  "2.0")
           .empty()) {
    return {};
  }
  size_t bytecode_size = 0;
  uint8_t* bytecode = LEPUS_WriteObject(
      compiler_context.context(), &bytecode_size,
      compiler_context.GetTopLevelFunction(), LEPUS_WRITE_OBJ_BYTECODE);
  if (!bytecode) {
    return {};
  }
  std::vector<uint8_t> result(bytecode, bytecode + bytecode_size);
  if (!compiler_context.GetGCFlag()) {
    lepus_free(compiler_context.context(), bytecode);
  }
  return result;
}

template <typename Task>
void OnReporter(Task&& task) {
  fml::AutoResetWaitableEvent done;
  tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [&task, &done] {
        task();
        done.Signal();
      });
  done.Wait();
}

void WaitForNormalWorker() {
  std::promise<void> completion;
  auto future = completion.get_future();
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [&completion] { completion.set_value(); },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
  ASSERT_EQ(std::future_status::ready,
            future.wait_for(std::chrono::seconds(5)));
}

}  // namespace

TEST(MTSRuntimePoolTest, PreserveExactContextType) {
  auto vm_pool =
      MTSRuntimePool::Create(runtime::ContextType::VMContextType, false);
  ASSERT_EQ(runtime::ContextType::VMContextType, vm_pool->context_type_);

  auto quick_pool =
      MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType, false);
  ASSERT_EQ(runtime::ContextType::LepusNGContextType,
            quick_pool->context_type_);
}

TEST(MTSRuntimePoolTest, EnsureMTSRuntimePoolKeepsLegacyVmAndQuickSemantics) {
  tasm::LynxTemplateBundle vm_bundle;
  vm_bundle.is_lepusng_binary_ = false;
  vm_bundle.context_type_ = runtime::ContextType::VMContextType;
  vm_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::VMContextType);
  vm_bundle.EnsureMTSRuntimePool();
  ASSERT_TRUE(vm_bundle.mts_runtime_pool_ != nullptr);
  ASSERT_EQ(runtime::ContextType::VMContextType,
            vm_bundle.mts_runtime_pool_->context_type_);

  tasm::LynxTemplateBundle quick_bundle;
  quick_bundle.is_lepusng_binary_ = true;
  quick_bundle.context_type_ = runtime::ContextType::LepusNGContextType;
  quick_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::LepusNGContextType);
  quick_bundle.EnsureMTSRuntimePool();
  ASSERT_TRUE(quick_bundle.mts_runtime_pool_ != nullptr);
  ASSERT_EQ(runtime::ContextType::LepusNGContextType,
            quick_bundle.mts_runtime_pool_->context_type_);
}

TEST(MTSRuntimePoolTest, PrepareLepusContextRejectsRtsPools) {
  tasm::LynxTemplateBundle rts_bundle;
  rts_bundle.context_type_ = runtime::ContextType::RTSContextType;
  rts_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::RTSContextType);

  EXPECT_FALSE(rts_bundle.PrepareLepusContext(5));
  EXPECT_FALSE(rts_bundle.EnableUseContextPool());
  EXPECT_EQ(nullptr, rts_bundle.mts_runtime_pool_);

  tasm::LynxTemplateBundle rts_native_bundle;
  rts_native_bundle.context_type_ = runtime::ContextType::RTSNativeContextType;
  rts_native_bundle.context_bundle_ = runtime::ContextBundle::Create(
      runtime::ContextType::RTSNativeContextType);
  EXPECT_EQ(nullptr, rts_native_bundle.mts_runtime_pool_);

  EXPECT_FALSE(rts_native_bundle.PrepareLepusContext(5));
  EXPECT_FALSE(rts_native_bundle.EnableUseContextPool());
}

TEST(MTSRuntimePoolTest, PrepareLepusContextKeepsLegacyExplicitPools) {
  tasm::LynxTemplateBundle vm_bundle;
  vm_bundle.context_type_ = runtime::ContextType::VMContextType;
  vm_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::VMContextType);
  vm_bundle.EnsureMTSRuntimePool();
  ASSERT_TRUE(vm_bundle.mts_runtime_pool_ != nullptr);

  EXPECT_TRUE(vm_bundle.PrepareLepusContext(1));
  EXPECT_TRUE(vm_bundle.EnableUseContextPool());

  tasm::LynxTemplateBundle quick_bundle;
  quick_bundle.context_type_ = runtime::ContextType::LepusNGContextType;
  quick_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::LepusNGContextType);
  quick_bundle.EnsureMTSRuntimePool();
  ASSERT_TRUE(quick_bundle.mts_runtime_pool_ != nullptr);

  EXPECT_TRUE(quick_bundle.PrepareLepusContext(1));
  EXPECT_TRUE(quick_bundle.EnableUseContextPool());
}

TEST(MTSRuntimePoolTest, PoolCreationDoesNotEnablePool) {
  tasm::LynxTemplateBundle bundle;
  bundle.context_type_ = runtime::ContextType::LepusNGContextType;
  bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::LepusNGContextType);
  bundle.page_configs_ = std::make_shared<tasm::PageConfig>();
  bundle.page_configs_->SetEnableUseContextPool(tasm::TernaryBool::FALSE_VALUE);

  bundle.EnsureMTSRuntimePool();
  ASSERT_NE(nullptr, bundle.mts_runtime_pool_);
  auto pool = bundle.mts_runtime_pool_;
  EXPECT_TRUE(pool->mts_runtimes_.empty());
  EXPECT_FALSE(bundle.EnableUseContextPool());

  EXPECT_FALSE(bundle.PrepareLepusContextByConfigs(0));
  bundle.SetEnableVMAutoGenerate(false);
  EXPECT_FALSE(bundle.mts_runtime_pool_->enable_auto_generate_);

  ASSERT_TRUE(bundle.PrepareLepusContext(1));
  EXPECT_EQ(pool, bundle.mts_runtime_pool_);
  EXPECT_TRUE(bundle.EnableUseContextPool());
  EXPECT_FALSE(bundle.mts_runtime_pool_->enable_auto_generate_);
}

TEST(MTSRuntimePoolTest, PreloadWaitsForFillAndLoadsCurrentRuntimes) {
  tasm::performance::MemoryMonitor::ForceEnableForTesting(
      tasm::performance::MemoryMonitor::ForceEnableMode::kCurrentProcess);
  auto quick_bundle = std::make_shared<lepus::QuickContextBundle>();
  quick_bundle->SetSource("function main() {}");
  std::shared_ptr<runtime::ContextBundle> context_bundle = quick_bundle;

  tasm::CompileOptions compile_options;
  tasm::PageConfig page_config;
  auto pool = MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType,
                                     "app/preload", false, context_bundle,
                                     compile_options, &page_config);
  // FillPool is intentionally asynchronous. Wait with a task on the same
  // worker, then poison the reporter cache so only Preload can repair it.
  pool->FillPool(2);
  WaitForNormalWorker();
  ASSERT_EQ(2U, pool->mts_runtimes_.size());
  OnReporter([&] {
    auto& global = tasm::performance::GlobalMemoryMonitor::GetInstance();
    ASSERT_EQ(global.mts_runtime_pool_state_.count(pool->pool_instance_id_),
              1u);
    global.mts_runtime_pool_state_.at(pool->pool_instance_id_).heap_bytes = -1;
  });
  auto bytecode = GenerateBytecode(
      "globalThis.preloaded = (globalThis.preloaded || 0) + 1;");
  ASSERT_FALSE(bytecode.empty());
  pool->SetEnableAutoGenerate(false);

  std::promise<bool> completion;
  auto future = completion.get_future();
  ASSERT_TRUE(pool->Preload("preload.js", std::move(bytecode),
                            [&completion]() { completion.set_value(true); }));
  EXPECT_FALSE(pool->enable_auto_generate_);
  ASSERT_EQ(std::future_status::ready,
            future.wait_for(std::chrono::seconds(5)));
  EXPECT_TRUE(future.get());
  EXPECT_EQ(2U, pool->mts_runtimes_.size());
  int64_t expected_heap = 0;
  for (const auto& runtime : pool->mts_runtimes_) {
    const auto heap_bytes = runtime->GetCurrentHeapSizeBytes();
    EXPECT_GE(heap_bytes, 0);
    expected_heap += heap_bytes;
  }
  OnReporter([&] {
    const auto& state =
        tasm::performance::GlobalMemoryMonitor::GetInstance()
            .mts_runtime_pool_state_.at(pool->pool_instance_id_);
    EXPECT_EQ(state.heap_bytes, expected_heap);
    EXPECT_EQ(state.runtime_count, 2u);
  });
}

TEST(MTSRuntimePoolTest, PreloadRequiresAutoRefillDisabled) {
  auto pool =
      MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType, false);
  auto bytecode = GenerateBytecode("globalThis.preloaded = true;");
  ASSERT_FALSE(bytecode.empty());

  EXPECT_FALSE(pool->Preload("preload.js", std::move(bytecode), nullptr));
}

TEST(MTSRuntimePoolTest,
     BundleLocalPoolReportsLifecycleAndGlobalPoolIsIgnored) {
  tasm::performance::MemoryMonitor::ForceEnableForTesting(
      tasm::performance::MemoryMonitor::ForceEnableMode::kCurrentProcess);
  auto global_pool =
      MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType, false);
  global_pool->ReportPoolState();
  const auto global_pool_id = global_pool->pool_instance_id_;
  OnReporter([&] {
    EXPECT_EQ(tasm::performance::GlobalMemoryMonitor::GetInstance()
                  .mts_runtime_pool_state_.count(global_pool_id),
              0u);
  });

  auto quick_bundle = std::make_shared<lepus::QuickContextBundle>();
  quick_bundle->SetSource("function main() {}");
  std::shared_ptr<runtime::ContextBundle> context_bundle = quick_bundle;
  tasm::CompileOptions compile_options;
  tasm::PageConfig page_config;
  auto pool = MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType,
                                     "app/local-pool", false, context_bundle,
                                     compile_options, &page_config);
  const auto pool_id = pool->pool_instance_id_;
  EXPECT_NE(pool_id, global_pool_id);
  OnReporter([&] {
    const auto& state = tasm::performance::GlobalMemoryMonitor::GetInstance()
                            .mts_runtime_pool_state_.at(pool_id);
    EXPECT_EQ(state.template_url, "app/local-pool");
    EXPECT_EQ(state.context_type,
              static_cast<int32_t>(runtime::ContextType::LepusNGContextType));
    EXPECT_EQ(state.runtime_count, 0u);
    EXPECT_EQ(state.heap_bytes, 0);
  });

  pool->SetEnableAutoGenerate(false);
  pool->FillPoolSync(1);
  ASSERT_EQ(pool->mts_runtimes_.size(), 1u);
  const auto heap_bytes =
      pool->mts_runtimes_.front()->GetCurrentHeapSizeBytes();
  OnReporter([&] {
    const auto& state = tasm::performance::GlobalMemoryMonitor::GetInstance()
                            .mts_runtime_pool_state_.at(pool_id);
    EXPECT_EQ(state.runtime_count, 1u);
    EXPECT_EQ(state.heap_bytes, heap_bytes);
  });

  ASSERT_NE(pool->TakeMTSRuntimeSafely(), nullptr);
  OnReporter([&] {
    const auto& state = tasm::performance::GlobalMemoryMonitor::GetInstance()
                            .mts_runtime_pool_state_.at(pool_id);
    EXPECT_EQ(state.runtime_count, 0u);
    EXPECT_EQ(state.heap_bytes, 0);
  });

  pool.reset();
  OnReporter([&] {
    EXPECT_EQ(tasm::performance::GlobalMemoryMonitor::GetInstance()
                  .mts_runtime_pool_state_.count(pool_id),
              0u);
  });
}

TEST(MTSRuntimePoolTest, QuickContextPoolTest) {
  // Some tasks of QuickContextPool will be executed in background threads. In
  // order to prevent affecting the stability of the unit test, the background
  // thread needs to be terminated in advance.
  base::TaskRunnerManufactor::GetConcurrentLoop(
      base::ConcurrentTaskType::NORMAL_PRIORITY)
      .Terminate();

  auto& pool = tasm::LynxGlobalPool::GetInstance().GetQuickContextPool();
  constexpr int32_t kSize = 5;
  pool.FillPool(kSize);

  // should have a size of 5
  ASSERT_EQ(kSize, pool.mts_runtimes_.size());

  // should obtain a lepusNG context
  auto mts_runtime = pool.TakeMTSRuntimeSafely();
  ASSERT_TRUE(mts_runtime != nullptr);
  ASSERT_TRUE(mts_runtime->IsLepusNGContext());

  // size should grow again to 5
  ASSERT_EQ(kSize, pool.mts_runtimes_.size());
}

}  // namespace shell
}  // namespace lynx
