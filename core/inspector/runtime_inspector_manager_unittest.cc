// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/inspector/runtime_inspector_manager.h"

#include "core/inspector/runtime_inspector_manager_unittest.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace testing {
class RuntimeInspectorManagerTest : public ::testing::Test {
 public:
  RuntimeInspectorManagerTest() {}
  ~RuntimeInspectorManagerTest() override {}
  void SetUp() override {}
};

TEST_F(RuntimeInspectorManagerTest, BuildInspectorUrl) {
  auto manager = new MockRuntimeInspectorManager(1);

  EXPECT_TRUE(manager->BuildInspectorUrl("/app-service.js") ==
              "file://view1/app-service.js");
  EXPECT_TRUE(manager->BuildInspectorUrl("app-service.js") ==
              "file://view1/app-service.js");
  EXPECT_TRUE(manager->BuildInspectorUrl("/test.js") == "file://view1/test.js");
  EXPECT_TRUE(manager->BuildInspectorUrl("test.js") == "file://view1/test.js");
  EXPECT_TRUE(manager->BuildInspectorUrl("/lynx_core.js") ==
              "file://shared/lynx_core.js");
  EXPECT_TRUE(manager->BuildInspectorUrl("lynx_core.js") ==
              "file://shared/lynx_core.js");
}
}  // namespace testing
}  // namespace piper
}  // namespace lynx
