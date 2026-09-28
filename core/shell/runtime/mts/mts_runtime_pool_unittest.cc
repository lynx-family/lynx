// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/shell/runtime/mts/mts_runtime_pool.h"

#include <chrono>
#include <future>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
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
  auto quick_bundle = std::make_shared<lepus::QuickContextBundle>();
  quick_bundle->SetSource("function main() {}");
  std::shared_ptr<runtime::ContextBundle> context_bundle = quick_bundle;

  tasm::CompileOptions compile_options;
  tasm::PageConfig page_config;
  auto pool =
      MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType, false,
                             context_bundle, compile_options, &page_config);
  // FillPool is intentionally asynchronous. Preload uses the same single
  // normal-priority worker, so it must run after this fill task.
  pool->FillPool(2);
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
}

TEST(MTSRuntimePoolTest, PreloadRequiresAutoRefillDisabled) {
  auto pool =
      MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType, false);
  auto bytecode = GenerateBytecode("globalThis.preloaded = true;");
  ASSERT_FALSE(bytecode.empty());

  EXPECT_FALSE(pool->Preload("preload.js", std::move(bytecode), nullptr));
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
