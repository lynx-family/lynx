// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public

#include "core/runtime/vm/lepus/tasks/lepus_callback_manager.h"

#include "core/renderer/dom/air/testing/air_lepus_context_mock.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace air {
namespace testing {

class LepusCallbackManagerTest : public ::testing::Test {
 public:
  LepusCallbackManagerTest() {}
  ~LepusCallbackManagerTest() override {}

  std::unique_ptr<tasm::LepusCallbackManager> lepus_callback_manager;
  AirMockLepusContext context;

  void SetUp() override {
    lepus_callback_manager = std::make_unique<tasm::LepusCallbackManager>();
  }
};

TEST_F(LepusCallbackManagerTest, CacheTask) {
  int64_t id_1 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task1"));
  int64_t id_2 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task2"));
  int64_t id_3 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task3"));
  ASSERT_TRUE(id_1 == 1);
  ASSERT_TRUE(id_2 == 2);
  ASSERT_TRUE(id_3 == 3);
}

TEST_F(LepusCallbackManagerTest, InvokeTask) {
  int64_t id_1 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task1"));
  EXPECT_TRUE(id_1 == 1);
  lepus_callback_manager->InvokeTask(id_1, lepus::Value(1));
  EXPECT_TRUE(lepus_callback_manager->task_map_.empty());
  int64_t id_2 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task2"));
  EXPECT_TRUE(id_2 == 2);
  EXPECT_TRUE(!lepus_callback_manager->task_map_.empty());
}

}  // namespace testing
}  // namespace air
}  // namespace lynx
