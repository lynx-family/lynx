// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/common/resource/response_promise_unittest.h"

#include <memory>

#include "core/runtime/bindings/common/resource/response_promise.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace test {
TEST_F(ResponsePromiseTest, TestResponsePromiseAddCallback) {
  ResponsePromise<int32_t> response_promise;
  response_promise.AddCallback([](int32_t value) { EXPECT_EQ(value, 42); });
  response_promise.SetValue(42);
}

TEST_F(ResponsePromiseTest, TestResponsePromiseWait) {
  ResponsePromise<int32_t> response_promise;
  // set value after 500ms;
  std::thread setter_thread([&response_promise]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    response_promise.SetValue(42);
  });
  auto result = response_promise.Wait(0);

  setter_thread.join();
  ASSERT_FALSE(result.has_value());
}

TEST_F(ResponsePromiseTest, TestResponsePromiseWait2) {
  ResponsePromise<int32_t> response_promise;
  // set value after 500ms;
  std::thread setter_thread([&response_promise]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    response_promise.SetValue(42);
  });
  auto result = response_promise.Wait(1);

  setter_thread.join();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 42);
}

TEST_F(ResponsePromiseTest, TestResponsePromiseWaitDouble) {
  ResponsePromise<int32_t> response_promise;

  // set value after 500ms;
  std::thread setter_thread([&response_promise]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    response_promise.SetValue(42);
  });

  auto start = std::chrono::steady_clock::now();
  auto result =
      response_promise.Wait(1.0);  // Waiting for 1s, make sure that the setter
                                   // thread is already triggered.
  auto end = std::chrono::steady_clock::now();

  setter_thread.join();

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 42);

  auto duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  EXPECT_GE(duration_ms, 400);
  EXPECT_LE(duration_ms, 600);
}

TEST_F(ResponsePromiseTest, TestResponsePromiseAddCallbackAfterSetValue) {
  ResponsePromise<int32_t> response_promise;
  response_promise.SetValue(42);
  response_promise.AddCallback([](int32_t value) { EXPECT_EQ(value, 42); });
}

}  // namespace test
}  // namespace runtime
}  // namespace lynx
