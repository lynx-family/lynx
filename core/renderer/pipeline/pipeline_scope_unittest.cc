// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/pipeline/pipeline_scope.h"

#include <memory>

#include "core/renderer/pipeline/pipeline_scope_unittest.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"

namespace lynx {
namespace tasm {
namespace test {
PipelineScopeTest::PipelineScopeTest() {
  static constexpr int32_t kWidth = 1024;
  static constexpr int32_t kHeight = 768;
  static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
  static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
  auto lynx_env_config =
      LynxEnvConfig(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                    kDefaultPhysicalPixelsPerLayoutUnit);
  delegate_ = std::make_unique<MockTasmDelegate>();
  auto manager =
      std::make_unique<ElementManager>(std::make_unique<MockPaintingContext>(),
                                       delegate_.get(), lynx_env_config);
  tasm_ = std::make_unique<lynx::tasm::TemplateAssembler>(
      *delegate_.get(), std::move(manager), delegate_.get(), 0, true);
}

TEST_F(PipelineScopeTest, TestPipelineContextConstructor01) {
  auto options = std::make_shared<PipelineOptions>();

  {
    PipelineScope scope(tasm_.get(), options);
    auto* context = tasm_->GetCurrentPipelineContext();
    EXPECT_EQ(context->GetVersion().GetMajor(), 0);
    EXPECT_EQ(context->GetVersion().GetMinor(), 1);
    EXPECT_EQ(options->HeldByContext(), true);
  }

  auto* context = tasm_->GetCurrentPipelineContext();
  EXPECT_EQ(context, nullptr);
  EXPECT_EQ(options->HeldByContext(), false);
}

TEST_F(PipelineScopeTest, TestPipelineContextConstructor02) {
  auto options = std::make_shared<PipelineOptions>();

  {
    PipelineScope scope(tasm_.get(), options, true);
    auto* context = tasm_->GetCurrentPipelineContext();
    EXPECT_EQ(context->GetVersion().GetMajor(), 1);
    EXPECT_EQ(context->GetVersion().GetMinor(), 0);
    EXPECT_EQ(options->HeldByContext(), true);
  }

  auto* context = tasm_->GetCurrentPipelineContext();
  EXPECT_EQ(context, nullptr);
  EXPECT_EQ(options->HeldByContext(), false);
}

TEST_F(PipelineScopeTest, TestPipelineContextConstructor03) {
  auto options = std::make_shared<PipelineOptions>();

  PipelineScope scope(tasm_.get(), options, true);
  auto* context = tasm_->GetCurrentPipelineContext();
  EXPECT_EQ(context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(context->GetVersion().GetMinor(), 0);
  EXPECT_EQ(options->HeldByContext(), true);

  scope.Exit();

  auto* current_context = tasm_->GetCurrentPipelineContext();
  EXPECT_EQ(current_context, nullptr);
  EXPECT_EQ(options->HeldByContext(), false);
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
