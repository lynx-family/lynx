// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/global.h"

#include <memory>

#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace test {

class GlobalTest : public ::testing::Test {
 protected:
  void SetUp() override {
    runtime_ = base::UnsafeOwningPtr<Runtime>(
        ::testing::utils::makeJSRuntime().release());
  }

  base::UnsafeOwningPtr<Runtime> runtime_;
  std::shared_ptr<ConsoleMessagePostMan> post_man_;
  tasm::PageOptions page_options_;
};

TEST_F(GlobalTest, InstallsSharedHostObjects) {
  Global global;
  global.Init(*runtime_, post_man_, page_options_, true);

  ASSERT_TRUE(runtime_);
  auto runtime_global = runtime_->global();
  EXPECT_TRUE(
      runtime_global.getProperty(*runtime_, "nativeConsole")->isObject());
  EXPECT_TRUE(runtime_global.getProperty(*runtime_, "SystemInfo")->isObject());
  EXPECT_TRUE(runtime_global.getProperty(*runtime_, "LynxJSBI")->isObject());
  EXPECT_TRUE(
      runtime_global.getProperty(*runtime_, "TextCodecHelper")->isObject());
}

TEST_F(GlobalTest, SkipsSharedHostObjectsForPageRealm) {
  Global global;
  global.Init(*runtime_, post_man_, page_options_, false);

  auto runtime_global = runtime_->global();
  EXPECT_TRUE(
      runtime_global.getProperty(*runtime_, "nativeConsole")->isObject());
  EXPECT_TRUE(
      runtime_global.getProperty(*runtime_, "SystemInfo")->isUndefined());
  EXPECT_TRUE(runtime_global.getProperty(*runtime_, "LynxJSBI")->isUndefined());
  EXPECT_TRUE(
      runtime_global.getProperty(*runtime_, "TextCodecHelper")->isUndefined());
}

TEST_F(GlobalTest, EnsuresConsoleWithExplicitRuntime) {
  Global global;
  global.Init(*runtime_, post_man_, page_options_, true);

  global.EnsureConsole(*runtime_, post_man_, page_options_);
  EXPECT_TRUE(runtime_->global().getProperty(*runtime_, "console")->isObject());
}

}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
