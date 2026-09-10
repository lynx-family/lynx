// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/system_info_agent.h"

#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

// SystemInfoGetInfo assembles and sends the response synchronously, so the
// test sender can capture the last message without any waiting.
class SystemInfoTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    type_ = type;
    message_ = message.toStyledString();
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {
    type_ = type;
    message_ = message;
  }

  Json::Value Response() const {
    Json::Value response;
    Json::Reader reader;
    EXPECT_EQ(type_, "CDP");
    EXPECT_TRUE(reader.parse(message_, response, false));
    return response;
  }

 private:
  std::string type_;
  std::string message_;
};

class SystemInfoAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sender_ = std::make_shared<SystemInfoTestMessageSender>();
    agent_ = std::make_shared<SystemInfoAgent>();
  }

  Json::Value Dispatch(const std::string& method) {
    Json::Value message(Json::ValueType::objectValue);
    message["id"] = 11;
    message["method"] = method;
    auto responder = std::make_shared<CDPResponder>(sender_, 11);
    agent_->CallMethod(responder, message);
    return sender_->Response();
  }

  std::shared_ptr<SystemInfoAgent> agent_;
  std::shared_ptr<SystemInfoTestMessageSender> sender_;
};

TEST_F(SystemInfoAgentTest, GetInfoReturnsResult) {
  Json::Value response = Dispatch("SystemInfo.getInfo");

  EXPECT_EQ(response["id"].asInt64(), 11);
  ASSERT_TRUE(response["result"].isObject());
  // GetSystemModelName defaults to "" in the platform facade mock; the platform
  // field is compiled in and always present.
  EXPECT_TRUE(response["result"].isMember("modelName"));
  EXPECT_TRUE(response["result"].isMember("platform"));
  EXPECT_FALSE(response.isMember("error"));
}

TEST_F(SystemInfoAgentTest, UnknownMethodReturnsMethodNotFound) {
  Json::Value response = Dispatch("SystemInfo.unknown");

  EXPECT_EQ(response["id"].asInt64(), 11);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"].asString(),
            "'SystemInfo.unknown' wasn't found");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
