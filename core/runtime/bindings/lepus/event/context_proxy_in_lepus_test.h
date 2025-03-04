// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_EVENT_CONTEXT_PROXY_IN_LEPUS_TEST_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_EVENT_CONTEXT_PROXY_IN_LEPUS_TEST_H_

#include "core/runtime/bindings/lepus/event/context_proxy_in_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

class ContextProxyInLepusTest : public ::testing::Test {
 public:
  ContextProxyInLepusTest() = default;
  ~ContextProxyInLepusTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

}  // namespace test
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_EVENT_CONTEXT_PROXY_IN_LEPUS_TEST_H_
