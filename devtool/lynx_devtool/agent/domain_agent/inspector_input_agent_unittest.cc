// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_input_agent.h"

#include <chrono>
#include <memory>
#include <thread>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorInputAgentTest : public ::testing::Test {
 public:
  InspectorInputAgentTest() = default;
  ~InspectorInputAgentTest() override = default;

  void SetUp() override {
    MockReceiver::GetInstance().ResetAll();
    devtool_mediator_ = std::make_shared<LynxDevToolMediator>();
    ui_executor_ = std::make_shared<InspectorUIExecutor>(devtool_mediator_);
    ui_executor_->SetDevToolPlatformFacade(
        std::make_shared<lynx::testing::DevToolPlatformFacadeMock>());
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    devtool_mediator_->ui_executor_ = ui_executor_;
    agent_ = std::make_shared<InspectorInputAgent>(devtool_mediator_);
    message_sender_ = std::make_shared<MessageSenderMock>();
  }

 protected:
  void WaitForUIThread() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::shared_ptr<InspectorInputAgent> agent_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<MessageSenderMock> message_sender_;
  std::unique_ptr<fml::Thread> ui_thread_;
};

TEST_F(InspectorInputAgentTest, InsertTextReturnsSuccess) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 7;
  message["method"] = "Input.insertText";
  message["params"]["text"] = "hello";

  agent_->CallMethod(message_sender_, message);
  WaitForUIThread();

  EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
  EXPECT_EQ(MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 7,\n   \"result\" : {}\n}\n");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
