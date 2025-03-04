// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_EVENT_CONTEXT_PROXY_TEST_H_
#define CORE_RUNTIME_BINDINGS_COMMON_EVENT_CONTEXT_PROXY_TEST_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "core/runtime/bindings/common/event/context_proxy.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace test {

class MockContextProxyDelegate : public ContextProxy::Delegate {
 public:
  MockContextProxyDelegate();
  virtual event::DispatchEventResult DispatchMessageEvent(
      MessageEvent event) override;
  std::vector<MessageEvent> event_vec_;

  std::unordered_map<ContextProxy::Type, std::unique_ptr<ContextProxy>>
      proxy_map_in_js_;
  std::unordered_map<ContextProxy::Type, std::unique_ptr<ContextProxy>>
      proxy_map_in_lepus_;
};

class ContextProxyTest : public ::testing::Test {
 public:
  ContextProxyTest() = default;
  ~ContextProxyTest() override = default;

  void SetUp() override {}

  void TearDown() override {}

  MockContextProxyDelegate delegate_;
};

}  // namespace test
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_EVENT_CONTEXT_PROXY_TEST_H_
