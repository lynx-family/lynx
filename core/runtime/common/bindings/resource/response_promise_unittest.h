// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_BINDINGS_RESOURCE_RESPONSE_PROMISE_UNITTEST_H_
#define CORE_RUNTIME_COMMON_BINDINGS_RESOURCE_RESPONSE_PROMISE_UNITTEST_H_

#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx {
namespace runtime {
namespace test {
class ResponsePromiseTest : public testing::Test {
 public:
  ResponsePromiseTest() = default;
  ~ResponsePromiseTest() override = default;
  void SetUp() override{};
  void TearDown() override{};
};
}  // namespace test
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_BINDINGS_RESOURCE_RESPONSE_PROMISE_UNITTEST_H_
