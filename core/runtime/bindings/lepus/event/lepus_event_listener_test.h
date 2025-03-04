// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_EVENT_LEPUS_EVENT_LISTENER_TEST_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_EVENT_LEPUS_EVENT_LISTENER_TEST_H_

#include "core/runtime/bindings/jsi/event/js_event_listener.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class LepusClosureEventListenerTest : public ::testing::Test {
 public:
  LepusClosureEventListenerTest() = default;
  ~LepusClosureEventListenerTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_EVENT_LEPUS_EVENT_LISTENER_TEST_H_
