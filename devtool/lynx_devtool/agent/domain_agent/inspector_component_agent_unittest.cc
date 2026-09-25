// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_component_agent.h"

#include <memory>
#include <string>
#include <vector>

#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class ComponentTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    EXPECT_EQ(type, "CDP");
    messages_.push_back(message);
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {
    Json::Value value;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(message, value, false));
    SendMessage(type, value);
  }

  const std::vector<Json::Value>& Messages() const { return messages_; }

 private:
  std::vector<Json::Value> messages_;
};

class InspectorComponentAgentTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    MockReceiver::GetInstance().ResetAll();
    sender_ = std::make_shared<ComponentTestMessageSender>();
    agent_ = std::make_unique<InspectorComponentAgent>();
  }

  void Dispatch(int64_t id, const std::string& method,
                const Json::Value& params = Json::Value()) {
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(id);
    message["method"] = method;
    if (!params.isNull()) {
      message["params"] = params;
    }
    agent_->CallMethod(std::make_shared<CDPResponder>(sender_, id), message);
  }

  std::shared_ptr<ComponentTestMessageSender> sender_;
  std::unique_ptr<InspectorComponentAgent> agent_;
};

TEST_F(InspectorComponentAgentTest, UselessUpdateSendsEventThenSuccess) {
  Json::Value params(Json::objectValue);
  params["componentId"] = "comp-1";
  params["reason"] = "debug";
  Dispatch(1, "Component.uselessUpdate", params);

  const auto& messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  Json::Value event;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(MockReceiver::GetInstance().received_message_.second,
                           event, false));
  EXPECT_EQ(event["method"], "Component.uselessUpdate");
  EXPECT_EQ(event["params"], params);
  EXPECT_FALSE(event.isMember("id"));
  EXPECT_EQ(messages[0]["id"].asInt64(), 1);
  EXPECT_TRUE(messages[0]["result"].isObject());
}

TEST_F(InspectorComponentAgentTest, UnknownMethodReturnsMethodNotFound) {
  Dispatch(2, "Component.unknown");

  const auto& messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 2);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(messages[0]["error"]["message"],
            "'Component.unknown' wasn't found");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
