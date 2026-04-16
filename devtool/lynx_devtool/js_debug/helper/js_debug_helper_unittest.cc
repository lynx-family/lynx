// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"

#include "core/runtime/mts_context.h"
#include "devtool/js_inspect/inspector_const.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class JSDebugHelperTest : public ::testing::Test {
 public:
  JSDebugHelperTest() {}
  ~JSDebugHelperTest() override {}
  void SetUp() override {}
};

TEST_F(JSDebugHelperTest, IsDebugAvailable) {
  EXPECT_FALSE(JSDebugHelper::GetInstance()->IsJSDebugAvailable());
  EXPECT_FALSE(JSDebugHelper::GetInstance()->IsLepusDebugAvailable());
}

TEST_F(JSDebugHelperTest, CreateRuntimeInspectorManager) {
  EXPECT_EQ(
      JSDebugHelper::GetInstance()->CreateRuntimeInspectorManager(kKeyEngineV8),
      nullptr);
  EXPECT_EQ(JSDebugHelper::GetInstance()->CreateRuntimeInspectorManager(
                kKeyEngineQuickjs),
            nullptr);
}

TEST_F(JSDebugHelperTest, CreateLepusInspectorManager) {
  EXPECT_EQ(JSDebugHelper::GetInstance()->CreateLepusInspectorManager(
                runtime::ContextType::LepusNGContextType),
            nullptr);
}

TEST_F(JSDebugHelperTest, MakeRuntime) {
  EXPECT_EQ(JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineV8), nullptr);
  EXPECT_EQ(JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineQuickjs),
            nullptr);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
