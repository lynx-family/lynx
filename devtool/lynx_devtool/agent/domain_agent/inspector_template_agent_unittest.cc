// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "third_party/jsoncpp/include/json/json.h"

#define private public
#define protected public

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_template_agent.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class TemplateTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    EXPECT_EQ(type, "CDP");
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(message);
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {
    Json::Value value;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(message, value, false));
    SendMessage(type, value);
  }

  std::vector<Json::Value> Messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<Json::Value> messages_;
};

class InspectorTemplateAgentTest : public ::testing::Test {
 public:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    ui_executor_ = std::make_shared<InspectorUIExecutor>(mediator_);
    element_executor_ = std::make_shared<InspectorTasmExecutor>(mediator_, 1);
    mediator_->ui_executor_ = ui_executor_;
    mediator_->element_executor_ = element_executor_;
    ui_thread_ = std::make_unique<fml::Thread>("template_test_ui");
    tasm_thread_ = std::make_unique<fml::Thread>("template_test_tasm");
    mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    facade_ = std::make_shared<lynx::testing::DevToolPlatformFacadeMock>();
    mediator_->SetDevToolPlatformFacade(facade_);
    sender_ = std::make_shared<TemplateTestMessageSender>();
    // The constructor must retain the mediator, not reference this temporary.
    agent_ = std::make_unique<InspectorTemplateAgent>(
        std::shared_ptr<LynxDevToolMediator>(mediator_));
  }

  void TearDown() override {
    ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
    mediator_->SetDevToolPlatformFacade(nullptr);
    mediator_->ui_executor_.reset();
    mediator_->element_executor_.reset();
    mediator_->ui_task_runner_ = nullptr;
    mediator_->tasm_task_runner_ = nullptr;
  }

 protected:
  void Dispatch(const std::string& method, const Json::Value& params,
                bool omit_params = false) {
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(kRequestId);
    message["method"] = method;
    if (!omit_params) {
      message["params"] = params;
    }
    {
      auto responder = std::make_shared<CDPResponder>(sender_, kRequestId);
      agent_->CallMethod(responder, message);
    }
    // Drain both executor threads so the queued response is delivered.
    ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
  }

  Json::Value OnlyResponse() {
    auto messages = sender_->Messages();
    EXPECT_EQ(messages.size(), 1u);
    if (messages.empty()) {
      return Json::Value();
    }
    EXPECT_EQ(messages[0]["id"].asInt64(), kRequestId);
    EXPECT_FALSE(messages[0].isMember("method"));
    return messages[0];
  }

  static constexpr int64_t kRequestId = 1LL << 40;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<InspectorTasmExecutor> element_executor_;
  std::shared_ptr<lynx::testing::DevToolPlatformFacadeMock> facade_;
  std::unique_ptr<InspectorTemplateAgent> agent_;
  std::shared_ptr<TemplateTestMessageSender> sender_;
};

TEST_F(InspectorTemplateAgentTest, ConfigInfoReturnsEmptyStringResult) {
  Dispatch("Template.templateConfigInfo", Json::Value(), true);
  auto response = OnlyResponse();
  EXPECT_EQ(response["result"], Json::Value(""));
  EXPECT_FALSE(response.isMember("error"));
}

TEST_F(InspectorTemplateAgentTest, TemplateDataReturnsResultObject) {
  // The mock facade returns no template value, so the result is an empty
  // object rather than carrying a content field.
  Dispatch("Template.templateData", Json::Value(Json::objectValue));
  auto response = OnlyResponse();
  EXPECT_EQ(response["result"], Json::Value(Json::objectValue));
  EXPECT_FALSE(response.isMember("error"));
}

TEST_F(InspectorTemplateAgentTest, GetTemplateJsReturnsData) {
  Json::Value params(Json::objectValue);
  params["offset"] = 0;
  params["size"] = 16;
  Dispatch("Template.getTemplateJs", params);
  auto response = OnlyResponse();
  EXPECT_TRUE(response["result"].isMember("data"));
  EXPECT_EQ(response["result"]["data"], Json::Value(""));
  EXPECT_FALSE(response.isMember("error"));
}

TEST_F(InspectorTemplateAgentTest,
       GetTemplateJsMissingParamsReturnsInvalidParams) {
  const std::vector<Json::Value> params_list = {
      Json::Value(Json::objectValue), [] {
        Json::Value only_offset(Json::objectValue);
        only_offset["offset"] = 0;
        return only_offset;
      }()};
  for (const auto& params : params_list) {
    SCOPED_TRACE(params.toStyledString());
    sender_ = std::make_shared<TemplateTestMessageSender>();
    Dispatch("Template.getTemplateJs", params);
    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"],
              "Params must have offset and size properties");
    EXPECT_FALSE(response.isMember("result"));
  }
}

TEST_F(InspectorTemplateAgentTest, TemplateApiInfoReturnsUseDefault) {
  // The executor was built without a TemplateAssembler, so useDefault is false.
  Dispatch("Template.templateApi", Json::Value(Json::objectValue));
  auto response = OnlyResponse();
  EXPECT_EQ(response["result"]["useDefault"], Json::Value(false));
  EXPECT_FALSE(response.isMember("error"));
}

TEST_F(InspectorTemplateAgentTest, UnknownMethodReturnsMethodNotFound) {
  Dispatch("Template.unknown", Json::Value());
  auto response = OnlyResponse();
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"], "'Template.unknown' wasn't found");
  EXPECT_FALSE(response.isMember("result"));
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
