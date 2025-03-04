// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/runtime/vm/lepus/quick_context_pool.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {

TEST(QuickContextPoolTest, QuickContextPoolTest) {
  // Some tasks of QuickContextPool will be executed in background threads. In
  // order to prevent affecting the stability of the unit test, the background
  // thread needs to be terminated in advance.
  base::TaskRunnerManufactor::GetNormalPriorityLoop().Terminate();

  auto& context_pool =
      tasm::LynxGlobalPool::GetInstance().GetQuickContextPool();
  constexpr int32_t kSize = 5;
  context_pool.FillPool(kSize);

  // should have a size of 5
  ASSERT_EQ(kSize, context_pool.contexts_.size());

  // should obtain a lepusNG context
  auto context = context_pool.TakeContextSafely();
  ASSERT_TRUE(context != nullptr);
  ASSERT_TRUE(context->IsLepusNGContext());

  // size should grow again to 5
  ASSERT_EQ(kSize, context_pool.contexts_.size());
}

}  // namespace lepus
}  // namespace lynx
