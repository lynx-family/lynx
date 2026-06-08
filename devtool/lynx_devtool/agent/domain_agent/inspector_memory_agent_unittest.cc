// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_memory_agent.h"

#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <string>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/testing/mock/global_devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class TestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    OnMessage(type, msg.toStyledString());
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    OnMessage(type, msg);
  }

  void ResetReceivedMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    received_message_ = {"", ""};
    has_received_message_ = false;
  }

  std::pair<std::string, std::string> GetReceivedMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    return received_message_;
  }

  bool WaitForMessage(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    return message_cv_.wait_for(lock, timeout,
                                [this]() { return has_received_message_; });
  }

 private:
  void OnMessage(const std::string& type, const std::string& msg) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      received_message_ = std::make_pair(type, msg);
      has_received_message_ = true;
    }
    message_cv_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable message_cv_;
  std::pair<std::string, std::string> received_message_;
  bool has_received_message_ = false;
};

class InspectorMemoryAgentTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    sender_ = std::make_shared<TestMessageSender>();
    agent_ = std::make_shared<InspectorMemoryAgent>();
    sender_->ResetReceivedMessage();
    lynx::testing::GlobalDevToolPlatformFacadeMock::MemoryUsageResultJson() =
        "{}";
    lynx::testing::GlobalDevToolPlatformFacadeMock::MemoryUsageErrorMessage() =
        "";
    lynx::testing::GlobalDevToolPlatformFacadeMock::LastMemoryUsageTimeoutMs() =
        -1;
  }

  Json::Value ReadLastMessage() {
    Json::Value value;
    Json::Reader reader;
    const auto message = sender_->GetReceivedMessage();
    EXPECT_TRUE(reader.parse(message.second, value, false));
    return value;
  }

  bool WaitForLastMessage() {
    return sender_->WaitForMessage(std::chrono::milliseconds(1000));
  }

 protected:
  std::shared_ptr<InspectorMemoryAgent> agent_;
  std::shared_ptr<TestMessageSender> sender_;
};

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageReturnsPlatformResult) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::MemoryUsageResultJson() =
      "{\"collectionStatus\":\"completed\",\"totalBytes\":1024,"
      "\"instances\":[]}";

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 1;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"]["timeoutMs"] = 20;
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  EXPECT_EQ(sender_->GetReceivedMessage().first, "CDP");
  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 1);
  EXPECT_EQ(lynx::testing::GlobalDevToolPlatformFacadeMock::
                LastMemoryUsageTimeoutMs(),
            20);
  EXPECT_EQ(response["result"]["collectionStatus"].asString(), "completed");
  EXPECT_EQ(response["result"]["totalBytes"].asInt64(), 1024);
  EXPECT_TRUE(response["result"]["instances"].isArray());
}

TEST_F(InspectorMemoryAgentTest, WaitForLastMessageAcceptsEmptyPayload) {
  sender_->SendMessage("CDP", std::string());

  ASSERT_TRUE(WaitForLastMessage());
  const auto message = sender_->GetReceivedMessage();
  EXPECT_EQ(message.first, "CDP");
  EXPECT_TRUE(message.second.empty());
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageRejectsInvalidTimeout) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 2;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"]["timeoutMs"] = "fast";
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 2);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid timeoutMs: expected integer milliseconds");
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageUsesDefaultTimeout) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 7;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"]["timeoutMs"] = 0;
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 7);
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_EQ(lynx::testing::GlobalDevToolPlatformFacadeMock::
                LastMemoryUsageTimeoutMs(),
            0);
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageRejectsNegativeTimeout) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 10;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"]["timeoutMs"] = -1;
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 10);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid timeoutMs: expected non-negative integer milliseconds");
  EXPECT_EQ(lynx::testing::GlobalDevToolPlatformFacadeMock::
                LastMemoryUsageTimeoutMs(),
            -1);
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageRejectsTooLargeTimeout) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 8;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"]["timeoutMs"] = 300001;
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 8);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid timeoutMs: expected value <= 300000");
  EXPECT_EQ(lynx::testing::GlobalDevToolPlatformFacadeMock::
                LastMemoryUsageTimeoutMs(),
            -1);
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageRejectsUint64Timeout) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 9;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"]["timeoutMs"] = Json::Value(
      static_cast<Json::UInt64>(std::numeric_limits<int64_t>::max()) + 1);
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 9);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid timeoutMs: expected value <= 300000");
  EXPECT_EQ(lynx::testing::GlobalDevToolPlatformFacadeMock::
                LastMemoryUsageTimeoutMs(),
            -1);
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageRejectsInvalidParams) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 3;
  message["method"] = "Memory.getAllMemoryUsage";
  message["params"] = 1;
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 3);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid params: expected object");
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageForwardsPlatformError) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::MemoryUsageErrorMessage() =
      "platform query failed";

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 4;
  message["method"] = "Memory.getAllMemoryUsage";
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 4);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(), "platform query failed");
}

TEST_F(InspectorMemoryAgentTest, GetAllMemoryUsageRejectsInvalidPlatformJson) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::MemoryUsageResultJson() =
      "not json";

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 5;
  message["method"] = "Memory.getAllMemoryUsage";
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 5);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid memory usage result JSON");
}

TEST_F(InspectorMemoryAgentTest, UnknownMemoryMethodReturnsNotImplemented) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 6;
  message["method"] = "Memory.unknown";
  agent_->CallMethod(sender_, message);
  ASSERT_TRUE(WaitForLastMessage());

  Json::Value response = ReadLastMessage();
  EXPECT_EQ(response["id"].asInt64(), 6);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Not implemented: Memory.unknown");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
