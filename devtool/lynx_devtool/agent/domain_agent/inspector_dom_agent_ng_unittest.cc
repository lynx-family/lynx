// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_dom_agent_ng.h"

#include <future>
#include <memory>

#include "base/include/fml/thread.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/reader.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace devtool {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class FocusRecordingPlatformFacade
    : public lynx::testing::DevToolPlatformFacadeMock {
 public:
  void Focus(int node_id) override { focused_node_id_ = node_id; }

  int focused_node_id_ = -1;
};

class InspectorDOMAgentNGTest : public ::testing::Test {
 public:
  InspectorDOMAgentNGTest() = default;
  ~InspectorDOMAgentNGTest() override = default;

  void SetUp() override {
    lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator_ = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager_ = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<lynx::tasm::MockPaintingContext>(),
        tasm_mediator_.get(), lynx_env_config);
    MockReceiver::GetInstance().ResetAll();
    devtool_mediator_ = std::make_shared<LynxDevToolMediator>();
    message_sender_ = std::make_shared<MessageSenderMock>();
    element_executor_ =
        std::make_shared<InspectorTasmExecutor>(devtool_mediator_, nullptr, 1);
    ui_executor_ = std::make_shared<InspectorUIExecutor>(devtool_mediator_);
    facade_ = std::make_shared<FocusRecordingPlatformFacade>();
    ui_executor_->SetDevToolPlatformFacade(facade_);
    tasm_thread_ = std::make_unique<fml::Thread>("tasm");
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    devtool_mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    devtool_mediator_->element_executor_ = element_executor_;
    devtool_mediator_->ui_executor_ = ui_executor_;
    agent_ = std::make_shared<InspectorDOMAgentNG>(devtool_mediator_);
  }

 protected:
  void FlushTasmTasks() {
    std::promise<void> promise;
    auto future = promise.get_future();
    devtool_mediator_->RunOnTASMThread([&promise]() { promise.set_value(); },
                                       true);
    future.wait();
  }

  void FlushUITasks() {
    std::promise<void> promise;
    auto future = promise.get_future();
    devtool_mediator_->RunOnUIThread([&promise]() { promise.set_value(); },
                                     true);
    future.wait();
  }

  Json::Value ParseLastResponse() {
    Json::Value response;
    Json::Reader reader;
    EXPECT_TRUE(reader.parse(
        MockReceiver::GetInstance().received_message_.second, response));
    return response;
  }

  std::shared_ptr<InspectorDOMAgentNG> agent_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<InspectorTasmExecutor> element_executor_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<MessageSenderMock> message_sender_;
  std::shared_ptr<FocusRecordingPlatformFacade> facade_;
  std::unique_ptr<lynx::tasm::ElementManager> manager_;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator_;
};

TEST_F(InspectorDOMAgentNGTest, FocusReturnsSuccessAndFocusesNode) {
  auto input = manager_->CreateFiberElement("input");
  ElementInspector::InitForInspector(std::make_tuple(input.get()));
  input->MarkCanBeLayoutOnly(false);
  element_executor_->element_root_ = input.get();
  int node_id = ElementInspector::NodeId(input.get());

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 11;
  message["method"] = "DOM.focus";
  message["params"]["nodeId"] = node_id;

  agent_->CallMethod(message_sender_, message);
  FlushTasmTasks();
  FlushUITasks();

  Json::Value response = ParseLastResponse();
  EXPECT_EQ(response["id"], 11);
  EXPECT_TRUE(response["error"].isNull());
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_EQ(facade_->focused_node_id_, node_id);
}

TEST_F(InspectorDOMAgentNGTest, FocusMissingNodeReturnsError) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 12;
  message["method"] = "DOM.focus";
  message["params"]["nodeId"] = 99999;

  agent_->CallMethod(message_sender_, message);
  FlushTasmTasks();

  Json::Value response = ParseLastResponse();
  EXPECT_EQ(response["id"], 12);
  EXPECT_EQ(response["error"]["code"], -32000);
  EXPECT_EQ(response["error"]["message"], "Element not found.");
  EXPECT_TRUE(response["result"].isNull());
  EXPECT_EQ(facade_->focused_node_id_, -1);
}

TEST_F(InspectorDOMAgentNGTest, FocusMissingNodeIdReturnsInvalidParams) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 13;
  message["method"] = "DOM.focus";
  message["params"] = Json::Value(Json::ValueType::objectValue);

  agent_->CallMethod(message_sender_, message);
  FlushTasmTasks();

  Json::Value response = ParseLastResponse();
  EXPECT_EQ(response["id"], 13);
  EXPECT_EQ(response["error"]["code"], -32602);
  EXPECT_EQ(response["error"]["message"],
            "DOM.focus requires a numeric nodeId parameter.");
  EXPECT_TRUE(response["result"].isNull());
  EXPECT_EQ(facade_->focused_node_id_, -1);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
