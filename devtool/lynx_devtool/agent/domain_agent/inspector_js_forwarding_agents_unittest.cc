// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_debugger_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_heap_profiler_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_profiler_agent.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_runtime_agent.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class CountingMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string&, const Json::Value&) override {
    ++message_count_;
  }

  void SendMessage(const std::string&, const std::string&) override {
    ++message_count_;
  }

  int message_count_{0};
};

template <typename Agent>
void ExpectResponderReleased(
    const std::shared_ptr<LynxDevToolMediator>& mediator,
    const std::string& method) {
  Agent agent(mediator);
  auto sender = std::make_shared<CountingMessageSender>();
  {
    auto responder = std::make_shared<CDPResponder>(sender, 1);
    Json::Value message(Json::objectValue);
    message["id"] = 1;
    message["method"] = method;
    agent.CallMethod(responder, message);
  }
  EXPECT_EQ(sender->message_count_, 0);
}

TEST(InspectorJSForwardingAgentsTest, ReleaseResponderBeforeForwarding) {
  auto mediator = std::make_shared<LynxDevToolMediator>();

  ExpectResponderReleased<InspectorDebuggerAgent>(mediator, "Debugger.enable");
  ExpectResponderReleased<InspectorRuntimeAgent>(mediator, "Runtime.enable");
  ExpectResponderReleased<InspectorHeapProfilerAgent>(
      mediator, "HeapProfiler.takeHeapSnapshot");
  ExpectResponderReleased<InspectorProfilerAgent>(mediator, "Profiler.enable");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
