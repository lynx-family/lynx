// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_wrapper_utils.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

namespace {

constexpr int32_t kWidth = 1080;
constexpr int32_t kHeight = 1920;

class RecordingMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.emplace_back(type, message);
    condition_.notify_all();
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {}

  bool WaitForMessageCount(size_t count, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_.wait_for(
        lock, timeout, [this, count] { return messages_.size() >= count; });
  }

  Json::Value MessageAt(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.at(index).second;
  }

  size_t MessageCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.size();
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<std::pair<std::string, Json::Value>> messages_;
};

class UIMethodPaintingContext : public tasm::MockPaintingContext {
 public:
  using InvokeCallback =
      std::function<void(int32_t code, const pub::Value& data)>;

  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const InvokeCallback& callback) override {
    invoked_id = id;
    invoked_method = method;
    invoked_params = pub::ValueUtils::ConvertValueToLepusValue(params);
    ++invoke_count;
    if (respond_automatically) {
      callback(response_code, PubLepusValue(response_data));
    } else {
      pending_callback = callback;
    }
  }

  void Respond() {
    if (!pending_callback) {
      return;
    }
    auto callback = std::move(pending_callback);
    callback(response_code, PubLepusValue(response_data));
  }

  int32_t response_code = tasm::LynxGetUIResult::SUCCESS;
  lepus::Value response_data;
  bool respond_automatically = true;
  int invoke_count = 0;
  int64_t invoked_id = -1;
  std::string invoked_method;
  lepus::Value invoked_params;
  InvokeCallback pending_callback;
};

Json::Value Request(int64_t id, const std::string& method) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = static_cast<Json::Int64>(id);
  message["method"] = method;
  return message;
}

}  // namespace

class InspectorLynxUIMethodTest : public ::testing::Test {
 public:
  void SetUp() override {
    tasm::LynxEnvConfig lynx_env_config(kWidth, kHeight, 1.f, 1.f);
    tasm_delegate_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto painting_context = std::make_unique<UIMethodPaintingContext>();
    painting_context_ = painting_context.get();
    manager_ = std::make_unique<tasm::ElementManager>(
        std::move(painting_context), tasm_delegate_.get(), lynx_env_config);

    mediator_ = std::make_shared<LynxDevToolMediator>();
    executor_ = std::make_shared<InspectorTasmExecutor>(mediator_, nullptr, 1);
    tasm_thread_ = std::make_unique<fml::Thread>("ui-component-tasm");
    mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    mediator_->element_executor_ = executor_;
    sender_ = std::make_shared<RecordingMessageSender>();
    agent_ = std::make_shared<InspectorLynxAgentNG>(mediator_);

    page_ = manager_->CreateFiberPage("page", 0);
    manager_->SetFiberPageElement(page_);
    ElementInspector::InitForInspector(std::make_tuple(page_.get()));
    target_ = manager_->CreateFiberElement("view");
    ElementInspector::InitForInspector(std::make_tuple(target_.get()));
    page_->InsertNode(target_);
    executor_->element_root_ = page_.get();
    backend_node_id_ = ElementInspector::NodeId(target_.get());
  }

  void TearDown() override {
    agent_.reset();
    mediator_->element_executor_.reset();
    executor_.reset();
    target_ = nullptr;
    page_ = nullptr;
    manager_.reset();
    mediator_.reset();
    tasm_thread_.reset();
  }

 protected:
  void FlushTasmTasks() {
    std::promise<void> promise;
    auto future = promise.get_future();
    ASSERT_TRUE(mediator_->RunOnTASMThread(
        [&promise]() { promise.set_value(); }, true));
    ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
  }

  Json::Value Call(Json::Value message) {
    const size_t expected_count = sender_->MessageCount() + 1;
    agent_->CallMethod(sender_, message);
    FlushTasmTasks();
    EXPECT_TRUE(
        sender_->WaitForMessageCount(expected_count, std::chrono::seconds(1)));
    return sender_->MessageAt(expected_count - 1);
  }

  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorTasmExecutor> executor_;
  std::shared_ptr<InspectorLynxAgentNG> agent_;
  std::shared_ptr<RecordingMessageSender> sender_;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_delegate_;
  std::unique_ptr<tasm::ElementManager> manager_;
  std::unique_ptr<fml::Thread> tasm_thread_;
  UIMethodPaintingContext* painting_context_ = nullptr;
  fml::RefPtr<tasm::PageElement> page_;
  fml::RefPtr<tasm::Element> target_;
  int32_t backend_node_id_ = 0;
};

TEST_F(InspectorLynxUIMethodTest,
       ForwardsExistingMethodAndParamsWithoutAliases) {
  auto request = Request(1, "Lynx.invokeUIMethod");
  request["params"]["backendNodeId"] = backend_node_id_;
  request["params"]["methodName"] = "seek";
  request["params"]["params"]["frame"] = 12.5;
  painting_context_->response_data = lepus::Value("done");
  auto response = Call(request);
  EXPECT_EQ(painting_context_->invoked_id, target_->impl_id());
  EXPECT_EQ(painting_context_->invoked_method, "seek");
  EXPECT_DOUBLE_EQ(
      painting_context_->invoked_params.Table()->GetValue("frame").Number(),
      12.5);
  EXPECT_EQ(response["result"]["code"], 0);
  EXPECT_EQ(response["result"]["data"], "done");
  request["params"]["methodName"] = "play";
  Call(request);
  EXPECT_EQ(painting_context_->invoked_method, "play");
}

TEST_F(InspectorLynxUIMethodTest,
       PreservesTypedReadOnlyStateAndNullCallbackData) {
  auto state = lepus::Dictionary::Create();
  state->SetValue("state", lepus::Value("ready"));
  state->SetValue("frameIndex", lepus::Value());
  state->SetValue("text", lepus::Value("quoted \"value\"\n"));
  painting_context_->response_data = lepus::Value(state);
  auto request = Request(2, "Lynx.invokeUIMethod");
  request["params"]["backendNodeId"] = backend_node_id_;
  request["params"]["methodName"] = "getComponentState";
  auto response = Call(request);
  EXPECT_EQ(response["result"]["data"]["state"], "ready");
  EXPECT_TRUE(response["result"]["data"]["frameIndex"].isNull());
  EXPECT_EQ(response["result"]["data"]["text"].asString(),
            "quoted \"value\"\n");
  EXPECT_TRUE(painting_context_->invoked_params.IsTable());
  painting_context_->response_data = lepus::Value();
  EXPECT_TRUE(Call(request)["result"]["data"].isNull());
}

TEST_F(InspectorLynxUIMethodTest, KeepsNativeErrorCodesAndData) {
  auto request = Request(3, "Lynx.invokeUIMethod");
  request["params"]["backendNodeId"] = backend_node_id_;
  request["params"]["methodName"] = "unknown";
  painting_context_->response_code = tasm::LynxGetUIResult::METHOD_NOT_FOUND;
  painting_context_->response_data = lepus::Value("unsupported method");
  auto response = Call(request);
  EXPECT_EQ(response["result"]["code"],
            tasm::LynxGetUIResult::METHOD_NOT_FOUND);
  EXPECT_EQ(response["result"]["data"], "unsupported method");
}

TEST_F(InspectorLynxUIMethodTest,
       RejectsInvalidRequestAndMissingNodeBeforePlatform) {
  auto request = Request(4, "Lynx.invokeUIMethod");
  EXPECT_EQ(Call(request)["error"]["code"], -32602);
  request["params"]["backendNodeId"] = backend_node_id_;
  request["params"]["methodName"] = "";
  EXPECT_EQ(Call(request)["error"]["code"], -32602);
  request["params"]["methodName"] = "play";
  request["params"]["params"] = 2;
  EXPECT_EQ(Call(request)["error"]["code"], -32602);
  request["params"].removeMember("params");
  request["params"]["backendNodeId"] = -1;
  EXPECT_EQ(Call(request)["error"]["code"], -32602);
  request["params"]["backendNodeId"] = 999999;
  EXPECT_EQ(Call(request)["error"]["message"], "Node not found");
  EXPECT_EQ(painting_context_->invoke_count, 0);
}

TEST_F(InspectorLynxUIMethodTest, WaitsForAsynchronousNativeCallback) {
  painting_context_->respond_automatically = false;
  painting_context_->response_data = lepus::Value(true);
  auto request = Request(5, "Lynx.invokeUIMethod");
  request["params"]["backendNodeId"] = backend_node_id_;
  request["params"]["methodName"] = "pause";
  agent_->CallMethod(sender_, request);
  FlushTasmTasks();
  EXPECT_EQ(sender_->MessageCount(), 0U);
  ASSERT_TRUE(mediator_->RunOnTASMThread(
      [context = painting_context_]() { context->Respond(); }));
  FlushTasmTasks();
  ASSERT_TRUE(sender_->WaitForMessageCount(1, std::chrono::seconds(1)));
  EXPECT_EQ(sender_->MessageCount(), 1U);
  EXPECT_TRUE(sender_->MessageAt(0)["result"]["data"].asBool());
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
