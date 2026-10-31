// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_ui_tree_agent.h"

#include <memory>
#include <string>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class UITreePlatformFacadeMock
    : public ::lynx::testing::DevToolPlatformFacadeMock {
 public:
  std::string GetLynxUITree() override { return tree_; }

  std::string GetUINodeInfo(int id) override {
    last_node_id_ = id;
    return node_info_;
  }

  int SetUIStyle(int id, std::string name, std::string content) override {
    last_node_id_ = id;
    last_style_name_ = std::move(name);
    last_style_content_ = std::move(content);
    return set_style_result_;
  }

  int last_node_id_ = -1;
  std::string tree_ = R"({"name":"page","id":1})";
  std::string node_info_ = R"({"id":2,"view":{"name":"MockView"}})";
  std::string last_style_name_;
  std::string last_style_content_;
  int set_style_result_ = 0;
};

class InspectorUITreeAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    ui_executor_ = std::make_shared<InspectorUIExecutor>(mediator_);
    facade_ = std::make_shared<UITreePlatformFacadeMock>();
    ui_executor_->SetDevToolPlatformFacade(facade_);
    mediator_->ui_executor_ = ui_executor_;
    ui_thread_ = std::make_unique<fml::Thread>("uitree_test");
    mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    mediator_->default_task_runner_ = ui_thread_->GetTaskRunner();
    agent_ = std::make_unique<InspectorUITreeAgent>(mediator_);
    sender_ = std::make_shared<MessageSenderMock>();
  }

  void TearDown() override {
    ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    ui_thread_->Join();
    mediator_->ui_task_runner_ = nullptr;
    mediator_->default_task_runner_ = nullptr;
  }

  Json::Value Dispatch(const std::string& method, int64_t id,
                       Json::Value params = Json::Value()) {
    MockReceiver::GetInstance().ResetAll();
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(id);
    message["method"] = method;
    if (!params.isNull()) {
      message["params"] = std::move(params);
    }
    agent_->CallMethod(std::make_shared<CDPResponder>(sender_, id), message);
    if (mediator_->ui_task_runner_ != nullptr) {
      ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    }

    Json::Value response;
    Json::Reader reader;
    EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
    EXPECT_TRUE(reader.parse(
        MockReceiver::GetInstance().received_message_.second, response, false));
    return response;
  }

  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<UITreePlatformFacadeMock> facade_;
  std::unique_ptr<InspectorUITreeAgent> agent_;
  std::shared_ptr<MessageSender> sender_;
  std::unique_ptr<fml::Thread> ui_thread_;
};

TEST_F(InspectorUITreeAgentTest, DispatchesAllMethods) {
  Json::Value enable_params(Json::objectValue);
  enable_params["useCompression"] = true;
  enable_params["compressionThreshold"] = 4096;
  EXPECT_EQ(Dispatch("UITree.enable", 1, enable_params)["result"],
            Json::Value(Json::objectValue));
  EXPECT_TRUE(ui_executor_->uitree_enabled_);

  Json::Value response = Dispatch("UITree.getLynxUITree", 2);
  EXPECT_EQ(response["result"]["root"]["name"], "page");

  Json::Value node_params(Json::objectValue);
  node_params["UINodeId"] = 2;
  response = Dispatch("UITree.getUIInfoForNode", 3, node_params);
  EXPECT_EQ(response["result"]["view"]["name"], "MockView");

  Json::Value style_params(Json::objectValue);
  style_params["UINodeId"] = 2;
  style_params["styleName"] = "visible";
  style_params["styleContent"] = "false";
  EXPECT_EQ(Dispatch("UITree.setUIStyle", 4, style_params)["result"],
            Json::Value(Json::objectValue));
  EXPECT_EQ(facade_->last_style_name_, "visible");

  EXPECT_EQ(Dispatch("UITree.disable", 5)["result"],
            Json::Value(Json::objectValue));
  EXPECT_FALSE(ui_executor_->uitree_enabled_);
}

TEST_F(InspectorUITreeAgentTest, RejectsInvalidParams) {
  Json::Value enable_params(Json::objectValue);
  enable_params["useCompression"] = "yes";
  Json::Value response = Dispatch("UITree.enable", 6, enable_params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));

  enable_params = Json::Value(Json::objectValue);
  enable_params["compressionThreshold"] = -1;
  response = Dispatch("UITree.enable", 7, enable_params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));

  Dispatch("UITree.enable", 8);
  response = Dispatch("UITree.getUIInfoForNode", 9);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));

  response = Dispatch("UITree.setUIStyle", 10);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorUITreeAgentTest, ReportsCommandErrors) {
  Json::Value response = Dispatch("UITree.getLynxUITree", 11);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"], "UITree is not enabled");

  Dispatch("UITree.enable", 12);
  facade_->set_style_result_ = -1;
  Json::Value style_params(Json::objectValue);
  style_params["UINodeId"] = 2;
  style_params["styleName"] = "visible";
  style_params["styleContent"] = "false";
  response = Dispatch("UITree.setUIStyle", 13, style_params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"], "Failed to set UI style");
}

TEST_F(InspectorUITreeAgentTest, ReportsInvalidPlatformData) {
  Dispatch("UITree.enable", 14);
  facade_->tree_ = "not json";
  Json::Value response = Dispatch("UITree.getLynxUITree", 15);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InternalError));
  EXPECT_EQ(response["error"]["message"], "Invalid UITree data");

  facade_->node_info_ = "not json";
  Json::Value params(Json::objectValue);
  params["UINodeId"] = 2;
  response = Dispatch("UITree.getUIInfoForNode", 16, params);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InternalError));
  EXPECT_EQ(response["error"]["message"], "Invalid UI node data");
}

TEST_F(InspectorUITreeAgentTest, ReportsUnavailableTargets) {
  Dispatch("UITree.enable", 17);
  ui_executor_->SetDevToolPlatformFacade(nullptr);
  Json::Value response = Dispatch("UITree.getLynxUITree", 18);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"], "UITree target is unavailable");

  ui_executor_->SetDevToolPlatformFacade(facade_);
  mediator_->default_task_runner_ = nullptr;
  response = Dispatch("UITree.getLynxUITree", 19);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"], "UITree target is unavailable");
}

TEST_F(InspectorUITreeAgentTest, RejectsUnknownMethod) {
  Json::Value response = Dispatch("UITree.unknown", 20);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"], "'UITree.unknown' wasn't found");
}

TEST_F(InspectorUITreeAgentTest, ReportsUnavailableUIThread) {
  mediator_->ui_task_runner_ = nullptr;

  Json::Value response = Dispatch("UITree.enable", 21);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"], "UITree target is unavailable");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
