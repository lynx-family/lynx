// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/pipeline/pipeline_context_unittest.h"

#include <memory>
#include <vector>

#include "base/include/fml/hash_combine.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/pipeline/pipeline_option.h"
#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {
namespace test {
PipelineContextTest::PipelineContextTest()
    : options_(std::make_shared<PipelineOptions>()) {}

TEST_F(PipelineContextTest, TestPipelineContextConstructor01) {
  auto version = PipelineVersion(1, 2);
  auto context = std::make_unique<PipelineContext>(version);
  EXPECT_EQ(context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextTest, TestPipelineContextConstructor02) {
  auto version = PipelineVersion(1, 2);
  auto context = std::make_shared<PipelineContext>(version);
  auto next_context = PipelineContext::Create(context->GetVersion(), true);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 2);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextTest, TestPipelineContextConstructor03) {
  auto version = PipelineVersion(1, 2);
  auto context = std::make_shared<PipelineContext>(version);
  auto next_context = PipelineContext::Create(context->GetVersion(), false);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 3);
}

TEST_F(PipelineContextTest, TestPipelineContextCreate01) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), false);
  EXPECT_EQ(context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);
}

TEST_F(PipelineContextTest, TestPipelineContextCreate02) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), false);
  auto next_context = PipelineContext::Create(context->GetVersion(), true);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 1);
}

TEST_F(PipelineContextTest, TestPipelineContextCreate03) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), false);
  auto next_context = PipelineContext::Create(context->GetVersion(), false);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextTest, TestPipelineContextResolve) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsResolveRequested());
  context->RequestResolve(true);
  EXPECT_TRUE(context->IsResolveRequested());
  context->RequestResolve(false);
  EXPECT_FALSE(context->IsResolveRequested());
}

TEST_F(PipelineContextTest, TestPipelineContextLayout) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsLayoutRequested());
  context->RequestLayout(true);
  EXPECT_TRUE(context->IsLayoutRequested());
  context->RequestLayout(false);
  EXPECT_FALSE(context->IsLayoutRequested());
}

TEST_F(PipelineContextTest, TestPipelineContextFlush) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsFlushUIOperationRequested());
  context->RequestFlushUIOperation(true);
  EXPECT_TRUE(context->IsFlushUIOperationRequested());
  context->RequestFlushUIOperation(false);
  EXPECT_FALSE(context->IsFlushUIOperationRequested());
}

TEST_F(PipelineContextTest, TestPipelineContextReload) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsReload());
  context->MarkReload(true);
  EXPECT_TRUE(context->IsReload());
  context->MarkReload(false);
  EXPECT_FALSE(context->IsReload());
}

TEST_F(PipelineContextTest, TestPipelineContextGetHash) {
  std::vector<std::unique_ptr<PipelineContext>> contexts{};
  for (int i = 0; i < 10; i++) {
    auto context =
        PipelineContext::Create(PipelineVersion::Create(), i % 2 == 0);
    EXPECT_NE(context, nullptr);
    contexts.push_back(std::move(context));
  }
  for (int i = 0; i < 10; i++) {
    EXPECT_NE(contexts[i], nullptr);
    auto seed = fml::HashCombine();
    fml::HashCombineSeed(seed, contexts[i].get(),
                         contexts[i]->GetVersion().GetMajor(),
                         contexts[i]->GetVersion().GetMinor());
    EXPECT_EQ(seed, contexts[i]->GetHash());
  }
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
