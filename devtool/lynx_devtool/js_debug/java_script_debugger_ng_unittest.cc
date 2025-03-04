// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/js_debug/java_script_debugger_ng.h"

#include "devtool/base_devtool/native/test/message_sender_mock.h"
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

    mediator->devtool_wp_ = devtool;
    debugger_->devtool_mediator_wp_ = mediator;
    debugger_->SendResponse(message);
    EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
    EXPECT_EQ(MockReceiver::GetInstance().received_message_.second, message);
  }

  MockReceiver::GetInstance().received_message_ = {"", ""};
  debugger_->SendResponse(message);
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "");
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.second, "");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
