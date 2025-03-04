// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_EVENT_MESSAGE_EVENT_TEST_H_
#define CORE_RUNTIME_BINDINGS_COMMON_EVENT_MESSAGE_EVENT_TEST_H_

#include "core/runtime/bindings/common/event/message_event.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace test {

class MessageEventTest : public ::testing::Test {
 public:
  MessageEventTest() = default;
  ~MessageEventTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

}  // namespace test
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_EVENT_MESSAGE_EVENT_TEST_H_
