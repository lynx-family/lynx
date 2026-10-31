// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_agent.h"

#include <memory>
#include <string>
#include <vector>

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorTestMessageSender : public MessageSender {
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

class InspectorAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sender_ = std::make_shared<InspectorTestMessageSender>();
    agent_ = std::make_unique<InspectorAgent>(nullptr);
  }

  void Dispatch(int64_t id, const std::string& method) {
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(id);
    message["method"] = method;
    agent_->CallMethod(std::make_shared<CDPResponder>(sender_, id), message);
  }

  std::shared_ptr<InspectorTestMessageSender> sender_;
  std::unique_ptr<InspectorAgent> agent_;
};

TEST_F(InspectorAgentTest, EnableReturnsSuccess) {
  Dispatch(1, "Inspector.enable");

  const auto messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 1);
  EXPECT_TRUE(messages[0]["result"].isObject());
  EXPECT_FALSE(messages[0].isMember("error"));
}

TEST_F(InspectorAgentTest, DetachedMethodIsRegistered) {
  auto iter = agent_->functions_map_.find("Inspector.detached");
  ASSERT_NE(iter, agent_->functions_map_.end());
  EXPECT_TRUE(iter->second == &InspectorAgent::Detached);
}

TEST_F(InspectorAgentTest, UnknownMethodReturnsMethodNotFound) {
  Dispatch(3, "Inspector.unknown");

  const auto messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 3);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(messages[0]["error"]["message"],
            "'Inspector.unknown' wasn't found");
  EXPECT_FALSE(messages[0].isMember("result"));
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
