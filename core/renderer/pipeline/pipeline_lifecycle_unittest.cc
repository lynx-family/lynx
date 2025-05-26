// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/pipeline/pipeline_lifecycle_unittest.h"

#define private public
#define protected public

#include "core/renderer/pipeline/pipeline_lifecycle.h"

namespace lynx {
namespace tasm {
namespace test {
TEST_F(PipelineLifecycleTest, TestCanAdvanceTo01) {
  PipelineLifecycle lifecycle;
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo02) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo03) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInStyleResolve));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo04) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterStyleResolve));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo05) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInPerformLayout));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo06) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterPerformLayout));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo07) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kUIOpFlush));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}

TEST_F(PipelineLifecycleTest, TestCanAdvanceTo08) {
  PipelineLifecycle lifecycle;
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInactive));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(lifecycle.AdvanceTo(LifecycleState::kStopped));

  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUninitialized));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInactive));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterStyleResolve));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kUIOpFlush));
  EXPECT_FALSE(lifecycle.CanAdvanceTo(LifecycleState::kStopped));
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
