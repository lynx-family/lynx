// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_ui_operation_queue.h"

#include "base/include/debug/lynx_assert.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class LynxUIOperationQueueTest : public ::testing::Test {
 protected:
  LynxUIOperationQueueTest() = default;
  ~LynxUIOperationQueueTest() override = default;
};

TEST_F(LynxUIOperationQueueTest, FlushOnce) {
  int32_t result = 0;
  LynxUIOperationQueue queue;

  int32_t expect = 10;
  for (int32_t i = 0; i < expect; ++i) {
    queue.EnqueueUIOperation([&result] { ++result; });
  }

  queue.Flush();

  ASSERT_EQ(result, expect);
}

TEST_F(LynxUIOperationQueueTest, FlushRepeatedly) {
  int32_t result = 0;
  LynxUIOperationQueue queue;

  queue.Flush();

  ASSERT_EQ(result, 0);

  int32_t expect = 10;
  for (int32_t i = 0; i < expect; ++i) {
    queue.EnqueueUIOperation([&result] { ++result; });
  }

  queue.Flush();

  ASSERT_EQ(result, expect);

  queue.Flush();

  ASSERT_EQ(result, expect);
}

TEST_F(LynxUIOperationQueueTest, EnqueueUIOperationRepeatedly) {
  int32_t result = 0;
  LynxUIOperationQueue queue;

  int32_t expect = 0;
  for (int32_t i = 0; i < expect; ++i) {
    queue.EnqueueUIOperation([&result] { ++result; });
    queue.Flush();
    ASSERT_EQ(result, ++expect);
  }
}

TEST_F(LynxUIOperationQueueTest, CheckException) {
  static const constexpr int32_t expect_error_code = 777;
  static const std::string expect_error_message = "MEGA";

  int32_t error_code = 0;
  std::string error_message;

  LynxUIOperationQueue queue;
  queue.EnqueueUIOperation([]() {
    base::ErrorStorage::GetInstance().SetError(
        base::LynxError(expect_error_code, expect_error_message));
  });

  ASSERT_EQ(error_code, 0);
  ASSERT_TRUE(error_message.empty());

  queue.SetErrorCallback(
      [&error_code, &error_message](const base::LynxError& error) mutable {
        error_code = error.error_code_;
        error_message = error.error_message_;
      });
  queue.Flush();

  queue.EnqueueUIOperation([]() {
    base::ErrorStorage::GetInstance().SetError(
        base::LynxError(expect_error_code, expect_error_message));
  });
  queue.Flush();
  ASSERT_EQ(error_code, expect_error_code);
  ASSERT_EQ(error_message, expect_error_message);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
