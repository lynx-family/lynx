// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_UNITTEST_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_UNITTEST_H_

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
class PipelineLifecycleTest : public testing::Test {
 public:
  PipelineLifecycleTest() = default;
  ~PipelineLifecycleTest() override = default;
  void SetUp() override{};
  void TearDown() override{};
};
}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_UNITTEST_H_
