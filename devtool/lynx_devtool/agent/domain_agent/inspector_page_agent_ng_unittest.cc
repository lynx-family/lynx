// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_page_agent_ng.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class PageTestMessageSender : public MessageSender {
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

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
  }

 private:
  mutable std::mutex mutex_;
  std::vector<Json::Value> messages_;
};

class PagePlatformFacadeMock
    : public ::lynx::testing::DevToolPlatformFacadeMock {};

class InspectorPageAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    ui_executor_ = std::make_shared<InspectorUIExecutor>(mediator_);
    element_executor_ = std::make_shared<InspectorTasmExecutor>(mediator_, 1);
    facade_ = std::make_shared<PagePlatformFacadeMock>();
    ui_executor_->SetDevToolPlatformFacade(facade_);
    mediator_->ui_executor_ = ui_executor_;
    mediator_->element_executor_ = element_executor_;

    ui_thread_ = std::make_unique<fml::Thread>("page_test_ui");
    tasm_thread_ = std::make_unique<fml::Thread>("page_test_tasm");
    mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();

    sender_ = std::make_shared<PageTestMessageSender>();
    devtools_ng_ = std::make_shared<::lynx::testing::LynxDevToolNGMock>();
    devtools_ng_->message_sender_ = sender_;
    devtools_ng_->devtool_mediator_ = mediator_;
    mediator_->devtool_wp_ = devtools_ng_;
    agent_ = std::make_unique<InspectorPageAgentNG>(mediator_);
  }

  void TearDown() override {
    ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
    mediator_->ui_task_runner_ = nullptr;
    mediator_->tasm_task_runner_ = nullptr;
  }

  std::vector<Json::Value> Dispatch(const std::string& method, int64_t id,
                                    Json::Value params = Json::Value()) {
    sender_->Clear();
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(id);
    message["method"] = method;
    if (!params.isNull()) {
      message["params"] = std::move(params);
    }
    agent_->CallMethod(std::make_shared<CDPResponder>(sender_, id), message);
    ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
    return sender_->Messages();
  }

  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<InspectorTasmExecutor> element_executor_;
  std::shared_ptr<PagePlatformFacadeMock> facade_;
  std::shared_ptr<PageTestMessageSender> sender_;
  std::shared_ptr<::lynx::testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<InspectorPageAgentNG> agent_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::unique_ptr<fml::Thread> tasm_thread_;
};

TEST_F(InspectorPageAgentTest, EnableSendsWelcomeEventBeforeResponse) {
  const auto messages = Dispatch("Page.enable", 1);

  ASSERT_EQ(messages.size(), 2u);
  EXPECT_EQ(messages[0]["method"], "Log.entryAdded");
  EXPECT_FALSE(messages[0].isMember("id"));
  EXPECT_EQ(messages[1]["id"].asInt64(), 1);
  EXPECT_EQ(messages[1]["result"], Json::Value(Json::objectValue));
}

TEST_F(InspectorPageAgentTest, DispatchesQueriesAndScreencastCommands) {
  auto messages = Dispatch("Page.canEmulate", 2);
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_TRUE(messages[0]["result"]["result"].asBool());

  messages = Dispatch("Page.canScreencast", 3);
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_TRUE(messages[0]["result"]["result"].asBool());

  messages = Dispatch("Page.getResourceTree", 4);
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["result"]["frameTree"]["frame"]["url"],
            "file:///Lynx.html");

  messages = Dispatch("Page.getResourceContent", 5);
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["result"]["content"], "");
  EXPECT_FALSE(messages[0]["result"]["base64Encoded"].asBool());
}

TEST_F(InspectorPageAgentTest, RejectsUnknownMethod) {
  const auto messages = Dispatch("Page.unknown", 12);

  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(messages[0]["error"]["message"], "Not implemented: Page.unknown");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
