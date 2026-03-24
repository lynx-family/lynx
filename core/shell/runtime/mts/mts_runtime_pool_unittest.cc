// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/shell/runtime/mts/mts_runtime_pool.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

TEST(MTSRuntimePoolTest, PreserveExactContextType) {
  auto vm_pool =
      MTSRuntimePool::Create(runtime::ContextType::VMContextType, false);
  ASSERT_EQ(runtime::ContextType::VMContextType, vm_pool->context_type_);

  auto quick_pool =
      MTSRuntimePool::Create(runtime::ContextType::LepusNGContextType, false);
  ASSERT_EQ(runtime::ContextType::LepusNGContextType,
            quick_pool->context_type_);
}

TEST(MTSRuntimePoolTest, PrepareVMByConfigsKeepsLegacyVmAndQuickSemantics) {
  tasm::LynxTemplateBundle vm_bundle;
  vm_bundle.is_lepusng_binary_ = false;
  vm_bundle.context_type_ = runtime::ContextType::VMContextType;
  vm_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::VMContextType);
  vm_bundle.PrepareVMByConfigs();
  ASSERT_TRUE(vm_bundle.mts_runtime_pool_ != nullptr);
  ASSERT_EQ(runtime::ContextType::VMContextType,
            vm_bundle.mts_runtime_pool_->context_type_);

  tasm::LynxTemplateBundle quick_bundle;
  quick_bundle.is_lepusng_binary_ = true;
  quick_bundle.context_type_ = runtime::ContextType::LepusNGContextType;
  quick_bundle.context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::LepusNGContextType);
  quick_bundle.PrepareVMByConfigs();
  ASSERT_TRUE(quick_bundle.mts_runtime_pool_ != nullptr);
  ASSERT_EQ(runtime::ContextType::LepusNGContextType,
            quick_bundle.mts_runtime_pool_->context_type_);
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
