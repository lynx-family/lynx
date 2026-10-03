// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_performance_agent.h"

#include <memory>
#include <string>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorPerformanceAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    ui_executor_ = std::make_shared<InspectorUIExecutor>(mediator_);
    mediator_->ui_executor_ = ui_executor_;
    ui_thread_ = std::make_unique<fml::Thread>("performance_test");
    mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    agent_ = std::make_unique<InspectorPerformanceAgent>(mediator_);
    sender_ = std::make_shared<MessageSenderMock>();
  }

  void TearDown() override {
    ui_thread_->GetTaskRunner()->PostSyncTask([] {});
    ui_thread_->Join();
    mediator_->ui_task_runner_ = nullptr;
  }

  Json::Value Dispatch(const std::string& method, int64_t id) {
    MockReceiver::GetInstance().ResetAll();
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(id);
    message["method"] = method;
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
  std::unique_ptr<InspectorPerformanceAgent> agent_;
  std::shared_ptr<MessageSender> sender_;
  std::unique_ptr<fml::Thread> ui_thread_;
};

TEST_F(InspectorPerformanceAgentTest, DispatchesEnableAndDisable) {
  Json::Value response = Dispatch("Performance.enable", 1);
  EXPECT_EQ(response["id"].asInt64(), 1);
  EXPECT_EQ(response["result"], Json::Value(Json::objectValue));
  EXPECT_TRUE(ui_executor_->performance_ready_);

  response = Dispatch("Performance.disable", 2);
  EXPECT_EQ(response["id"].asInt64(), 2);
  EXPECT_EQ(response["result"], Json::Value(Json::objectValue));
  EXPECT_FALSE(ui_executor_->performance_ready_);
}

TEST_F(InspectorPerformanceAgentTest, QueriesWithoutShellReturnEmptyResult) {
  for (const auto& method : {"Performance.getAllTimingInfo",
                             "Performance.getAllPerformanceEntries"}) {
    Json::Value response = Dispatch(method, 3);
    EXPECT_EQ(response["id"].asInt64(), 3);
    EXPECT_EQ(response["result"], Json::Value(Json::objectValue));
  }
}

TEST_F(InspectorPerformanceAgentTest, RejectsUnknownMethod) {
  Json::Value response = Dispatch("Performance.unknown", 4);

  EXPECT_EQ(response["id"].asInt64(), 4);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"].asString(),
            "'Performance.unknown' wasn't found");
}

TEST_F(InspectorPerformanceAgentTest, ReportsUnavailableUIThread) {
  mediator_->ui_task_runner_ = nullptr;

  Json::Value response = Dispatch("Performance.enable", 5);

  EXPECT_EQ(response["id"].asInt64(), 5);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Performance target is unavailable");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
