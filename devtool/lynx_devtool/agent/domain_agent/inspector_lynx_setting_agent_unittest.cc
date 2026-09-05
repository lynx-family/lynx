// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_setting_agent.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/testing/mock/global_devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class LynxSettingTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    SetMessage(type, message.toStyledString());
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {
    SetMessage(type, message);
  }

  bool WaitForMessage() {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_.wait_for(lock, std::chrono::seconds(1),
                               [this] { return received_; });
  }

  Json::Value Response() {
    std::lock_guard<std::mutex> lock(mutex_);
    Json::Value response;
    Json::Reader reader;
    EXPECT_EQ(type_, "CDP");
    EXPECT_TRUE(reader.parse(message_, response, false));
    return response;
  }

 private:
  void SetMessage(const std::string& type, const std::string& message) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      type_ = type;
      message_ = message;
      received_ = true;
    }
    condition_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::string type_;
  std::string message_;
  bool received_ = false;
};

class InspectorLynxSettingAgentTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    sender_ = std::make_shared<LynxSettingTestMessageSender>();
    agent_ = std::make_shared<InspectorLynxSettingAgent>();
    lynx::testing::GlobalDevToolPlatformFacadeMock::LynxSettingResultJson() =
        "{}";
    lynx::testing::GlobalDevToolPlatformFacadeMock::LynxSettingErrorMessage()
        .clear();
    lynx::testing::GlobalDevToolPlatformFacadeMock::LastLynxSettingRequest() =
        {};
  }

  Json::Value Dispatch(const std::string& method,
                       const Json::Value& params = Json::Value()) {
    Json::Value message(Json::ValueType::objectValue);
    message["id"] = 7;
    message["method"] = method;
    if (!params.isNull()) {
      message["params"] = params;
    }
    auto responder = std::make_shared<CDPResponder>(sender_, 7);
    agent_->CallMethod(responder, message);
    EXPECT_TRUE(sender_->WaitForMessage());
    return sender_->Response();
  }

 protected:
  std::shared_ptr<InspectorLynxSettingAgent> agent_;
  std::shared_ptr<LynxSettingTestMessageSender> sender_;
};

TEST_F(InspectorLynxSettingAgentTest, ReturnsPlatformResult) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::LynxSettingResultJson() =
      "{\"values\":{\"key\":\"value\"}}";

  Json::Value response = Dispatch("LynxSetting.getValues");

  EXPECT_EQ(response["id"].asInt64(), 7);
  EXPECT_EQ(response["result"]["values"]["key"].asString(), "value");
  EXPECT_EQ(
      lynx::testing::GlobalDevToolPlatformFacadeMock::LastLynxSettingRequest()
          .method,
      "LynxSetting.getValues");
}

TEST_F(InspectorLynxSettingAgentTest, ForwardsStringKeyAndMockValue) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "feature_key";
  params["value"] = "enabled";

  Json::Value response = Dispatch("LynxSetting.setMockValue", params);

  EXPECT_TRUE(response["result"].isObject());
  const auto& request =
      lynx::testing::GlobalDevToolPlatformFacadeMock::LastLynxSettingRequest();
  EXPECT_EQ(request.key, "feature_key");
  EXPECT_EQ(request.value, "enabled");
}

TEST_F(InspectorLynxSettingAgentTest, RejectsMissingKey) {
  Json::Value response =
      Dispatch("LynxSetting.getValue", Json::Value(Json::objectValue));

  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::kInvalidParams));
  EXPECT_EQ(response["error"]["message"].asString(), "expected string key");
}

TEST_F(InspectorLynxSettingAgentTest, RejectsNonStringMockValue) {
  Json::Value params(Json::ValueType::objectValue);
  params["key"] = "feature_key";
  params["value"] = true;

  Json::Value response = Dispatch("LynxSetting.setMockValue", params);

  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::kInvalidParams));
  EXPECT_EQ(response["error"]["message"].asString(),
            "expected string mock value");
}

TEST_F(InspectorLynxSettingAgentTest, ForwardsPlatformError) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::LynxSettingErrorMessage() =
      "Failed to persist mock value";

  Json::Value response = Dispatch("LynxSetting.clearMockValues");

  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::kServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Failed to persist mock value");
}

TEST_F(InspectorLynxSettingAgentTest, RejectsInvalidPlatformJson) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::LynxSettingResultJson() =
      "not json";

  Json::Value response = Dispatch("LynxSetting.getFetchInfo");

  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::kInternalError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Invalid LynxSetting result JSON");
}

TEST_F(InspectorLynxSettingAgentTest, RejectsUnknownMethod) {
  Json::Value response = Dispatch("LynxSetting.unknown");

  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::kMethodNotFound));
  EXPECT_EQ(response["error"]["message"].asString(),
            "'LynxSetting.unknown' wasn't found");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
