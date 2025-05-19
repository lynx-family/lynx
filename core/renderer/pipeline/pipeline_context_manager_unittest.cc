// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/pipeline/pipeline_context_manager_unittest.h"

#include <memory>

#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/pipeline/pipeline_context_manager.h"
#include "core/renderer/pipeline/pipeline_option.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST_F(PipelineContextManagerTest, TestPipelineContextManagerCreate) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(false);
  auto context = manager->CreateAndUpdateCurrentPipelineContext(options);
  EXPECT_EQ(context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);
  context = manager->CreateAndUpdateCurrentPipelineContext(options, true);
  EXPECT_EQ(context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);
}

TEST_F(PipelineContextManagerTest,
       TestPipelineContextManagerGetCurrentContext) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(false);
  auto context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto current_context = manager->GetCurrentPipelineContext();
  EXPECT_EQ(context, current_context);
  EXPECT_EQ(context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);

  context->RequestResolve(true);
  EXPECT_TRUE(context->IsResolveRequested());
  context->RequestResolve(false);
  EXPECT_FALSE(context->IsResolveRequested());
  context->RequestLayout(true);
  EXPECT_TRUE(context->IsLayoutRequested());
  context->RequestLayout(false);
  EXPECT_FALSE(context->IsLayoutRequested());
  context->RequestFlushUIOperation(true);
  EXPECT_TRUE(context->IsFlushUIOperationRequested());
  context->RequestFlushUIOperation(false);
  EXPECT_FALSE(context->IsFlushUIOperationRequested());
  context->MarkReload(true);
  EXPECT_TRUE(context->IsReload());
  context->MarkReload(false);
  EXPECT_FALSE(context->IsReload());

  auto next_context =
      manager->CreateAndUpdateCurrentPipelineContext(options, false);
  EXPECT_NE(next_context, current_context);
  EXPECT_EQ(next_context, manager->GetCurrentPipelineContext());
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextManagerTest,
       TestPipelineContextManagerGetContextByVersion) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(false);
  std::vector<const PipelineContext*> contexts{};
  for (int i = 0; i < 10; i++) {
    auto context =
        manager->CreateAndUpdateCurrentPipelineContext(options, i % 2 == 0);
    contexts.push_back(context);
  }
  for (int i = 0; i < 10; i++) {
    auto context =
        manager->GetPipelineContextByVersion(contexts[i]->GetVersion());
    EXPECT_EQ(context, contexts[i]);
    EXPECT_EQ(context->GetVersion().GetMajor(),
              contexts[i]->GetVersion().GetMajor());
    EXPECT_EQ(context->GetVersion().GetMinor(),
              contexts[i]->GetVersion().GetMinor());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
