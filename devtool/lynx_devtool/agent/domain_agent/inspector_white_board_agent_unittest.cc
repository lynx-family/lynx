// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "third_party/jsoncpp/include/json/json.h"

#define private public
#define protected public

#include "base/include/fml/thread.h"
#include "core/shared_data/lynx_white_board.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_white_board_agent.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/lynx_devtool/shared_data/white_board_inspector_impl.h"
#include "devtool/lynx_devtool/shared_data/white_board_inspector_runtime_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class WhiteBoardTestMessageSender : public MessageSender {
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

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
  }

  std::vector<Json::Value> Messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<Json::Value> messages_;
};

class InspectorWhiteBoardAgentTest : public ::testing::Test {
 public:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    tasm_executor_ = std::make_shared<InspectorTasmExecutor>(mediator_, 1);
    js_debugger_ =
        std::make_shared<InspectorJavaScriptDebuggerImpl>(mediator_, 1);
    inspector_ = std::make_shared<WhiteBoardInspectorImpl>();
    white_board_ = std::make_shared<tasm::WhiteBoard>();
    inspector_->SetWhiteBoard(white_board_);
    tasm_thread_ = std::make_unique<fml::Thread>("white_board_test_tasm");
    js_thread_ = std::make_unique<fml::Thread>("white_board_test_js");
    sender_ = std::make_shared<WhiteBoardTestMessageSender>();
    agent_ = std::make_unique<InspectorWhiteBoardAgent>(mediator_);
  }

  void TearDown() override {
    FlushThreads();
    mediator_->element_executor_.reset();
    mediator_->js_debugger_.reset();
    mediator_->tasm_task_runner_ = nullptr;
    mediator_->js_task_runner_ = nullptr;
  }

 protected:
  void ConfigureTasmPath() {
    mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    mediator_->element_executor_ = tasm_executor_;
    auto delegate = tasm_executor_->GetWhiteBoardInspectorDelegate();
    delegate->SetInspector(inspector_);
  }

  void ConfigureJSPath() {
    mediator_->tasm_task_runner_ = nullptr;
    mediator_->js_task_runner_ = js_thread_->GetTaskRunner();
    mediator_->js_debugger_ = js_debugger_;
    js_debugger_->white_board_inspector_delegate_ =
        std::make_shared<WhiteBoardInspectorRuntimeDelegate>(js_debugger_, 1);
    js_debugger_->white_board_inspector_delegate_->SetInspector(inspector_);
  }

  Json::Value Dispatch(const std::string& method,
                       const Json::Value& params = Json::Value()) {
    sender_->Clear();
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(++request_id_);
    message["method"] = method;
    message["params"] = params;
    {
      auto responder = std::make_shared<CDPResponder>(sender_, request_id_);
      agent_->CallMethod(responder, message);
    }
    FlushThreads();

    auto messages = sender_->Messages();
    EXPECT_EQ(messages.size(), 1u);
    if (messages.empty()) {
      return Json::Value();
    }
    EXPECT_EQ(messages[0]["id"].asInt64(), request_id_);
    return messages[0];
  }

  void FlushThreads() {
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
    js_thread_->GetTaskRunner()->PostSyncTask([] {});
  }

  void ExpectSuccess(const std::string& method,
                     const Json::Value& params = Json::Value()) {
    const Json::Value response = Dispatch(method, params);
    EXPECT_TRUE(response["result"].isObject());
    EXPECT_FALSE(response.isMember("error"));
  }

  void RunCommandSequence() {
    ExpectSuccess("WhiteBoard.enable");

    Json::Value set_params(Json::objectValue);
    set_params["key"] = "key";
    set_params["value"] = "\"value\"";
    ExpectSuccess("WhiteBoard.setSharedData", set_params);

    const Json::Value get_response = Dispatch("WhiteBoard.getSharedData");
    ASSERT_TRUE(get_response["result"]["entries"].isArray());
    ASSERT_EQ(get_response["result"]["entries"].size(), 1u);
    EXPECT_EQ(get_response["result"]["entries"][0]["key"], "key");
    EXPECT_EQ(get_response["result"]["entries"][0]["value"], "\"value\"");

    Json::Value remove_params(Json::objectValue);
    remove_params["key"] = "key";
    ExpectSuccess("WhiteBoard.removeSharedData", remove_params);

    set_params["key"] = "key-to-clear";
    ExpectSuccess("WhiteBoard.setSharedData", set_params);
    ExpectSuccess("WhiteBoard.clear");
    ExpectSuccess("WhiteBoard.disable");
  }

  int64_t request_id_ = 0;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::unique_ptr<fml::Thread> js_thread_;
  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorTasmExecutor> tasm_executor_;
  std::shared_ptr<InspectorJavaScriptDebuggerImpl> js_debugger_;
  std::shared_ptr<WhiteBoardInspectorImpl> inspector_;
  std::shared_ptr<tasm::WhiteBoard> white_board_;
  std::shared_ptr<WhiteBoardTestMessageSender> sender_;
  std::unique_ptr<InspectorWhiteBoardAgent> agent_;
};

TEST_F(InspectorWhiteBoardAgentTest, TasmPathHandlesAllCommands) {
  ConfigureTasmPath();
  RunCommandSequence();
}

TEST_F(InspectorWhiteBoardAgentTest, TasmPathReportsMissingDelegate) {
  ConfigureTasmPath();
  tasm_executor_->white_board_inspector_delegate_.reset();

  const Json::Value response = Dispatch("WhiteBoard.enable");
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"],
            "InspectorTasmExecutor::WhiteBoardEnable, "
            "white_board_inspector_delegate_ is null");
}

TEST_F(InspectorWhiteBoardAgentTest, JSPathHandlesAllCommands) {
  ConfigureJSPath();
  RunCommandSequence();
}

TEST_F(InspectorWhiteBoardAgentTest, JSPathReportsMissingDelegate) {
  ConfigureJSPath();
  js_debugger_->white_board_inspector_delegate_.reset();

  const Json::Value response = Dispatch("WhiteBoard.enable");
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"],
            "InspectorJavaScriptDebuggerImpl::WhiteBoardEnable, "
            "white_board_inspector_delegate_ is null");
}

TEST_F(InspectorWhiteBoardAgentTest, UnknownMethodReturnsMethodNotFound) {
  const Json::Value response = Dispatch("WhiteBoard.unknown");
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"], "'WhiteBoard.unknown' wasn't found");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
