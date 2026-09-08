// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/js_debug/java_script_debugger_ng.h"

#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/testing/mock/lynx_devtool_mediator_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class JavaScriptDebuggerNGMock : public JavaScriptDebuggerNG {
 public:
  JavaScriptDebuggerNGMock(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
      : JavaScriptDebuggerNG(devtool_mediator) {
    attached_ = true;
  }
  ~JavaScriptDebuggerNGMock() override = default;

  void DispatchMessage(const std::string& message,
                       const std::string& session_id = "") override {}
  void RunOnTargetThread(base::closure&& closure,
                         bool run_now = true) override {}
};

class JavaScriptDebuggerNGTest : public ::testing::Test {
 public:
  JavaScriptDebuggerNGTest() {}
  ~JavaScriptDebuggerNGTest() override {}
  void SetUp() override {
    debugger_ = std::make_shared<JavaScriptDebuggerNGMock>(nullptr);
  }

 private:
  std::shared_ptr<JavaScriptDebuggerNGMock> debugger_;
};

TEST_F(JavaScriptDebuggerNGTest, SendResponse) {
  std::string message =
      "{\n   \"method\" : \"test\",\n   \"params\" : {\n      \"result\" : "
      "\"test SendResponse\"\n   }\n}\n";
  {
    auto devtool = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    auto mediator = std::make_shared<lynx::testing::LynxDevToolMediatorMock>();

    mediator->devtool_wp_ = devtool;
    debugger_->devtool_mediator_wp_ = mediator;
    auto message_sender = std::make_shared<devtool::MessageSenderMock>();
    devtool->message_sender_ = message_sender;

    debugger_->SendResponse(message);
    sleep(1);
    EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
    EXPECT_EQ(MockReceiver::GetInstance().received_message_.second, message);
  }

  MockReceiver::GetInstance().received_message_ = {"", ""};
  debugger_->SendResponse(message);
  sleep(1);
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "");
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.second, "");
}

TEST_F(JavaScriptDebuggerNGTest,
       ReportsUnavailableBackgroundRuntimeForRequests) {
  auto debugger = std::make_shared<InspectorJavaScriptDebuggerImpl>(nullptr, 1);
  const std::vector<std::string> requests = {
      R"({"id":1,"method":"Runtime.evaluate","params":{"expression":"Runtime.enable"}})",
      R"({"id":2,"method":"Debugger.enable"})",
      R"({"id":3,"method":"Profiler.start"})",
      R"({"id":4,"method":"HeapProfiler.takeHeapSnapshot"})",
  };

  for (size_t index = 0; index < requests.size(); ++index) {
    debugger->DispatchMessage(requests[index]);
    ASSERT_FALSE(debugger->message_buf_.empty());

    Json::Value response;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(debugger->message_buf_.front(), response, false));
    debugger->message_buf_.pop();
    EXPECT_EQ(response["id"].asInt(), static_cast<int>(index + 1));
    EXPECT_EQ(response["error"]["code"].asInt(), -32000);
    EXPECT_EQ(response["error"]["message"].asString(),
              "Background JavaScript runtime is unavailable.");
    EXPECT_FALSE(debugger->runtime_enable_needed_);
  }

  EXPECT_TRUE(debugger->message_buf_.empty());
}

TEST_F(JavaScriptDebuggerNGTest, LatchesExactRuntimeEnableWithoutResponding) {
  auto debugger = std::make_shared<InspectorJavaScriptDebuggerImpl>(nullptr, 1);

  debugger->DispatchMessage(R"({"method":"Debugger.enable"})");
  EXPECT_TRUE(debugger->message_buf_.empty());
  EXPECT_FALSE(debugger->runtime_enable_needed_);

  debugger->DispatchMessage(R"({"id":5,"method":"Runtime.enable"})");
  EXPECT_TRUE(debugger->runtime_enable_needed_);
  EXPECT_TRUE(debugger->message_buf_.empty());
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
