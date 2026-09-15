// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent.h"

#include <utility>
#include <vector>

#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {
namespace {

class AgentMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    EXPECT_EQ(type, "CDP");
    messages.push_back(msg);
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    Json::Value parsed;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(msg, parsed));
    SendMessage(type, parsed);
  }

  std::vector<Json::Value> messages;
};

class AgentMessageDispatcher : public DevToolMessageDispatcher {
 public:
  std::shared_ptr<MessageSender> GetSender() const override { return sender; }
  std::shared_ptr<AgentMessageSender> sender =
      std::make_shared<AgentMessageSender>();
};

}  // namespace

class InspectorLynxAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto agent = std::make_unique<InspectorLynxAgent>(dispatcher_.GetSender());
    agent_ = agent.get();
    dispatcher_.RegisterAgent("LynxAgent", std::move(agent));
  }

  void Dispatch(const Json::Value& params,
                const std::string& method = "LynxAgent.sendMessage") {
    Json::Value request(Json::objectValue);
    request["id"] = Json::Int64(4294967297LL);
    request["method"] = method;
    request["params"] = params;
    dispatcher_.DispatchMessage(response_sender_, "CDP",
                                request.toStyledString());
  }

  void ExpectError(CDPErrorCode code) {
    ASSERT_EQ(response_sender_->messages.size(), 1u);
    const auto& response = response_sender_->messages.front();
    EXPECT_EQ(response["id"].asInt64(), 4294967297LL);
    EXPECT_EQ(response["error"]["code"].asInt(), static_cast<int>(code));
    EXPECT_FALSE(response.isMember("result"));
  }

  AgentMessageDispatcher dispatcher_;
  InspectorLynxAgent* agent_ = nullptr;
  std::shared_ptr<AgentMessageSender> response_sender_ =
      std::make_shared<AgentMessageSender>();
};

TEST_F(InspectorLynxAgentTest, DeliversMessageAndAcknowledgesAcceptance) {
  // Treat messages as opaque strings, including quotes, newlines and Unicode.
  const std::string payload = "{\"text\":\"hello\"}\n\xf0\x9f\x98\x80";
  std::string received;
  agent_->SetMessageHandler([&](const std::string& message) {
    received = message;
    return true;
  });
  Json::Value params(Json::objectValue);
  params["message"] = payload;
  Dispatch(params);

  EXPECT_EQ(received, payload);
  ASSERT_EQ(response_sender_->messages.size(), 1u);
  const auto& response = response_sender_->messages.front();
  EXPECT_EQ(response["id"].asInt64(), 4294967297LL);
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_TRUE(response["result"].empty());
  EXPECT_FALSE(response.isMember("error"));
  EXPECT_TRUE(dispatcher_.sender->messages.empty());
}

TEST_F(InspectorLynxAgentTest, PublishesUnsolicitedEventOnGlobalSender) {
  agent_->SendMessageReceived("ready");

  ASSERT_EQ(dispatcher_.sender->messages.size(), 1u);
  const auto& event = dispatcher_.sender->messages.front();
  EXPECT_EQ(event["method"].asString(), "LynxAgent.messageReceived");
  EXPECT_EQ(event["params"]["message"].asString(), "ready");
  EXPECT_FALSE(event.isMember("id"));
  EXPECT_FALSE(event.isMember("result"));
  EXPECT_TRUE(response_sender_->messages.empty());
}

TEST_F(InspectorLynxAgentTest, RejectsMissingAndNonStringMessage) {
  int calls = 0;
  agent_->SetMessageHandler([&](const std::string&) {
    ++calls;
    return true;
  });
  Dispatch(Json::Value());
  ExpectError(CDPErrorCode::InvalidParams);
  response_sender_->messages.clear();
  Dispatch(Json::Value(Json::objectValue));
  ExpectError(CDPErrorCode::InvalidParams);
  response_sender_->messages.clear();
  Json::Value params(Json::objectValue);
  params["message"] = 42;
  Dispatch(params);
  ExpectError(CDPErrorCode::InvalidParams);
  EXPECT_EQ(calls, 0);
}

TEST_F(InspectorLynxAgentTest, RejectsUnknownMethod) {
  Dispatch(Json::Value(), "LynxAgent.unknown");
  ExpectError(CDPErrorCode::MethodNotFound);
}

TEST_F(InspectorLynxAgentTest, ReportsUnavailableAndRejectedRuntime) {
  Json::Value params(Json::objectValue);
  params["message"] = "hello";
  Dispatch(params);
  ExpectError(CDPErrorCode::ServerError);
  response_sender_->messages.clear();
  agent_->SetMessageHandler([](const std::string&) { return false; });
  Dispatch(params);
  ExpectError(CDPErrorCode::ServerError);
}

TEST_F(InspectorLynxAgentTest, HandlerCanPublishAndUnbindItself) {
  agent_->SetMessageHandler([&](const std::string& message) {
    agent_->SendMessageReceived(message);
    agent_->SetMessageHandler({});
    return true;
  });
  Json::Value params(Json::objectValue);
  params["message"] = "";
  Dispatch(params);
  ASSERT_EQ(response_sender_->messages.size(), 1u);
  EXPECT_TRUE(response_sender_->messages.front()["result"].isObject());
  ASSERT_EQ(dispatcher_.sender->messages.size(), 1u);
  EXPECT_EQ(
      dispatcher_.sender->messages.front()["params"]["message"].asString(), "");

  response_sender_->messages.clear();
  Dispatch(params);
  ExpectError(CDPErrorCode::ServerError);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
