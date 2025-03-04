// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/common/platform_call_back_manager.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class PlatformCallBackManagerTest : public ::testing::Test {
 public:
  PlatformCallBackManagerTest() {}
  ~PlatformCallBackManagerTest() override {}

  void SetUp() override {}
};

class PlatformCallBackTest : public lynx::shell::PlatformCallBack {
 public:
  void InvokeWithValue(const lepus::Value& value) override{};

  ~PlatformCallBackTest() override{};
};

TEST_F(PlatformCallBackManagerTest, TEST_CreatePlatformCallBack) {
  std::shared_ptr<PlatformCallBackManager> callback_manager =
      std::make_shared<PlatformCallBackManager>();
  std::unique_ptr<lynx::shell::PlatformCallBack> callback =
      std::make_unique<PlatformCallBackTest>();
  auto holder =
      callback_manager->CreatePlatformCallBackHolder(std::move(callback));
  ASSERT_TRUE(callback_manager->HasCallBack(holder->id()));
}

TEST_F(PlatformCallBackManagerTest, TEST_ErasePlatformCallBack) {
  std::shared_ptr<PlatformCallBackManager> callback_manager =
      std::make_shared<PlatformCallBackManager>();
  std::unique_ptr<lynx::shell::PlatformCallBack> callback =
      std::make_unique<PlatformCallBackTest>();
  auto holder =
      callback_manager->CreatePlatformCallBackHolder(std::move(callback));
  ASSERT_TRUE(callback_manager->HasCallBack(holder->id()));
  callback_manager->EraseCallBack(holder);
  ASSERT_FALSE(callback_manager->HasCallBack(holder->id()));
}
}  // namespace testing
}  // namespace shell
}  // namespace lynx
