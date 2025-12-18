// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_log_agent.h"

#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/testing/mock/lynx_devtool_mediator_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorLogAgentTest : public ::testing::Test {
 public:
  InspectorLogAgentTest() {}
  ~InspectorLogAgentTest() override {}
  void SetUp() override {
    devtool_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    const auto& mediator = devtool_->devtool_mediator_;
    agent_ = std::make_shared<InspectorLogAgent>(mediator);
    auto devtool_executor =
        std::make_shared<InspectorDefaultExecutor>(mediator);
    mediator->devtool_executor_ = devtool_executor;
    mediator->devtool_wp_ = devtool_;
    auto message_sender = std::make_shared<devtool::MessageSenderMock>();
    devtool_->message_sender_ = message_sender;
  }

 private:
  std::shared_ptr<InspectorLogAgent> agent_;
  std::shared_ptr<lynx::testing::LynxDevToolNGMock> devtool_;
};

TEST_F(InspectorLogAgentTest, CallMethod) {
  Json::Value msg1(Json::ValueType::objectValue);
  msg1["id"] = 1;
  msg1["method"] = "Log.enable";
  msg1["sessionId"] = "Main";
  std::string expected1 =
      "{\n   \"error\" : {\n      \"code\" : -32601,\n      \"message\" : "
      "\"Not implemented: Log.enable\"\n   },\n   \"id\" : 1\n}\n";
  agent_->CallMethod(devtool_->message_sender_, msg1);
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.second, expected1);

  MockReceiver::GetInstance().received_message_ = {"", ""};
  Json::Value msg2(Json::ValueType::objectValue);
  msg2["id"] = 2;
  msg2["method"] = "Log.enable";
  std::string expected2 = "{\n   \"id\" : 2,\n   \"result\" : {}\n}\n";
  agent_->CallMethod(devtool_->message_sender_, msg2);
  sleep(1);
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.second, expected2);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
