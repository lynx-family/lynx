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
    : public ::lynx::testing::DevToolPlatformFacadeMock {
 public:
  void StopScreenCast() override { ++stop_screencast_count_; }
  void OnAckReceived() override { ++ack_count_; }

  void PageReload(bool ignore_cache, const std::string& template_binary,
                  const std::string& reload_url, bool from_template_fragments,
                  int32_t template_size) override {
    reload_ignore_cache_ = ignore_cache;
    reload_template_binary_ = template_binary;
    reload_url_ = reload_url;
    reload_from_template_fragments_ = from_template_fragments;
    reload_template_size_ = template_size;
  }

  void Navigate(const std::string& url) override { navigated_url_ = url; }

  int stop_screencast_count_ = 0;
  int ack_count_ = 0;
  bool reload_ignore_cache_ = false;
  bool reload_from_template_fragments_ = false;
  int32_t reload_template_size_ = 0;
  std::string reload_template_binary_;
  std::string reload_url_;
  std::string navigated_url_;
};

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

  void ExpectError(const std::string& method, int64_t id, Json::Value params,
                   CDPErrorCode code, const std::string& error_message) {
    const auto messages = Dispatch(method, id, std::move(params));
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0]["error"]["code"].asInt(), static_cast<int>(code));
    EXPECT_EQ(messages[0]["error"]["message"], error_message);
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

  Json::Value params(Json::objectValue);
  params["format"] = "png";
  params["quality"] = 80;
  params["maxWidth"] = 720;
  params["maxHeight"] = 1280;
  params["everyNthFrame"] = 2;
  messages = Dispatch("Page.startScreencast", 6, params);
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["result"], Json::Value(Json::objectValue));
  ASSERT_EQ(facade_->screen_cast_requests_.size(), 1u);
  EXPECT_EQ(facade_->screen_cast_requests_[0].format_, "png");

  EXPECT_EQ(Dispatch("Page.stopScreencast", 7).size(), 1u);
  EXPECT_EQ(facade_->stop_screencast_count_, 1);

  EXPECT_EQ(Dispatch("Page.screencastFrameAck", 8).size(), 1u);
  EXPECT_EQ(facade_->ack_count_, 1);
}

TEST_F(InspectorPageAgentTest, DispatchesReloadAndNavigation) {
  Json::Value reload_params(Json::objectValue);
  reload_params["ignoreCache"] = true;
  reload_params["pageData"] = "template";
  reload_params["url"] = "https://example.com/reload";
  reload_params["fromPageDataFragments"] = true;
  reload_params["pageDataLength"] = 8;
  EXPECT_EQ(Dispatch("Page.reload", 9, reload_params).size(), 1u);
  EXPECT_TRUE(facade_->reload_ignore_cache_);
  EXPECT_EQ(facade_->reload_template_binary_, "template");
  EXPECT_EQ(facade_->reload_url_, "https://example.com/reload");
  EXPECT_TRUE(facade_->reload_from_template_fragments_);
  EXPECT_EQ(facade_->reload_template_size_, 8);

  Json::Value navigate_params(Json::objectValue);
  navigate_params["url"] = "https://example.com/page";
  auto messages = Dispatch("Page.navigate", 10, navigate_params);
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["result"]["frameId"], "");
  EXPECT_FALSE(messages[0]["result"].isMember("loaderId"));
  EXPECT_EQ(facade_->navigated_url_, "https://example.com/page");

  navigate_params["url"] = "about:blank";
  messages = Dispatch("Page.navigate", 11, navigate_params);
  ASSERT_EQ(messages.size(), 2u);
  EXPECT_EQ(messages[0]["id"].asInt64(), 11);
  EXPECT_EQ(messages[1]["method"], "Page.frameNavigated");
}

TEST_F(InspectorPageAgentTest, RejectsInvalidCommandParams) {
  Json::Value params(Json::objectValue);
  params["format"] = "webp";
  ExpectError("Page.startScreencast", 12, params, CDPErrorCode::InvalidParams,
              "Invalid format: expected jpeg or png");

  params = Json::Value(Json::objectValue);
  params["quality"] = 101;
  ExpectError("Page.startScreencast", 13, params, CDPErrorCode::InvalidParams,
              "Invalid quality: expected integer from 0 to 100");

  params = Json::Value(Json::objectValue);
  params["maxWidth"] = -1;
  ExpectError("Page.startScreencast", 14, params, CDPErrorCode::InvalidParams,
              "Invalid dimensions: expected non-negative integers");

  params = Json::Value(Json::objectValue);
  params["maxHeight"] = "1280";
  ExpectError("Page.startScreencast", 15, params, CDPErrorCode::InvalidParams,
              "Invalid screencast parameter type");

  params = Json::Value(Json::objectValue);
  params["everyNthFrame"] = 0;
  ExpectError("Page.startScreencast", 16, params, CDPErrorCode::InvalidParams,
              "Invalid everyNthFrame: expected positive integer");

  params = Json::Value(Json::objectValue);
  params["mode"] = "invalid";
  ExpectError("Page.startScreencast", 17, params, CDPErrorCode::InvalidParams,
              "Invalid mode: expected fullscreen or lynxview");

  params = Json::Value(Json::objectValue);
  params["ignoreCache"] = "false";
  ExpectError("Page.reload", 18, params, CDPErrorCode::InvalidParams,
              "Invalid reload parameters");

  params = Json::Value(Json::objectValue);
  params["pageDataLength"] = -1;
  ExpectError("Page.reload", 19, params, CDPErrorCode::InvalidParams,
              "Invalid reload parameters");

  params = Json::Value(Json::objectValue);
  params["pageData"] = 1;
  ExpectError("Page.reload", 20, params, CDPErrorCode::InvalidParams,
              "Invalid reload parameters");

  ExpectError("Page.navigate", 21, Json::Value(), CDPErrorCode::InvalidParams,
              "Invalid url: expected non-empty string");

  params = Json::Value(Json::objectValue);
  params["url"] = 1;
  ExpectError("Page.navigate", 22, params, CDPErrorCode::InvalidParams,
              "Invalid url: expected non-empty string");
}

TEST_F(InspectorPageAgentTest, ReportsUnavailablePageTarget) {
  ui_executor_->devtool_platform_facade_.reset();

  ExpectError("Page.startScreencast", 23, Json::Value(),
              CDPErrorCode::ServerError, "Page target is unavailable");
  ExpectError("Page.stopScreencast", 24, Json::Value(),
              CDPErrorCode::ServerError, "Page target is unavailable");
  ExpectError("Page.screencastFrameAck", 25, Json::Value(),
              CDPErrorCode::ServerError, "Page target is unavailable");
  ExpectError("Page.reload", 26, Json::Value(), CDPErrorCode::ServerError,
              "Page target is unavailable");

  Json::Value params(Json::objectValue);
  params["url"] = "https://example.com/page";
  ExpectError("Page.navigate", 27, params, CDPErrorCode::ServerError,
              "Page target is unavailable");
}

TEST_F(InspectorPageAgentTest, RejectsUnknownMethod) {
  const auto messages = Dispatch("Page.unknown", 28);

  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0]["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(messages[0]["error"]["message"], "'Page.unknown' wasn't found");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
