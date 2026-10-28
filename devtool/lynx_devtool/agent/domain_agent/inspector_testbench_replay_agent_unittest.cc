// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_replay_agent.h"

#include <memory>
#include <string>
#include <vector>

#include "core/services/replay/replay_controller.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class ReplayTestMessageSender : public MessageSender {
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

class InspectorTestBenchReplayAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sender_ = std::make_shared<ReplayTestMessageSender>();
    agent_ = std::make_unique<InspectorTestBenchReplayAgent>();
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

  std::shared_ptr<ReplayTestMessageSender> sender_;
  std::unique_ptr<InspectorTestBenchReplayAgent> agent_;
};

TEST_F(InspectorTestBenchReplayAgentTest, UnknownMethodReturnsMethodNotFound) {
  Dispatch(1, "Replay.unknown");

  const auto& messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 1);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(messages[0]["error"]["message"], "Not implemented: Replay.unknown");
}

TEST_F(InspectorTestBenchReplayAgentTest, DisabledReplayReturnsMethodNotFound) {
  if (lynx::tasm::replay::ReplayController::Enable()) {
    GTEST_SKIP() << "TestBench replay is enabled in this build";
  }
  Dispatch(2, "Replay.start");

  const auto& messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 2);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(messages[0]["error"]["message"], "Not implemented: Replay.start");
}

TEST_F(InspectorTestBenchReplayAgentTest,
       InvalidEndParamsReturnInvalidParamsWhenReplayEnabled) {
  if (!lynx::tasm::replay::ReplayController::Enable()) {
    GTEST_SKIP() << "TestBench replay is not enabled in this build";
  }
  Dispatch(3, "Replay.end", Json::Value(Json::objectValue));

  const auto& messages = sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 3);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
  EXPECT_EQ(messages[0]["error"]["message"],
            "Invalid file path: expected string");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
