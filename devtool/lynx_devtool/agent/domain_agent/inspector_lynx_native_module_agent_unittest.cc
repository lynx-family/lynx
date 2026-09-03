// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/jsoncpp/include/json/value.h"
#define private public
#define protected public

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_native_module_agent.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorLynxNativeModuleAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    MockReceiver::GetInstance().ResetAll();
    mediator_ = std::make_shared<LynxDevToolMediator>();
    devtool_thread_ = std::make_unique<fml::Thread>("devtools");
    mediator_->default_task_runner_ = devtool_thread_->GetTaskRunner();
    agent_ = std::make_shared<InspectorLynxNativeModuleAgent>(mediator_);
    sender_ = std::make_shared<MessageSenderMock>();
  }

  Json::Value Dispatch(const std::string& method, int64_t id) {
    MockReceiver::GetInstance().ResetAll();
    Json::Value message(Json::ValueType::objectValue);
    message["id"] = static_cast<Json::Int64>(id);
    message["method"] = method;
    agent_->CallMethod(sender_, message);
    FlushDevToolTasks();

    Json::Value response;
    Json::Reader reader;
    EXPECT_EQ(MockReceiver::GetInstance().received_message_.first, "CDP");
    EXPECT_TRUE(reader.parse(
        MockReceiver::GetInstance().received_message_.second, response, false));
    return response;
  }

  void FlushDevToolTasks() {
    std::promise<void> completed;
    auto future = completed.get_future();
    mediator_->default_task_runner_->PostTask(
        [&completed]() { completed.set_value(); });
    ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
  }

  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorLynxNativeModuleAgent> agent_;
  std::shared_ptr<MessageSender> sender_;
  std::unique_ptr<fml::Thread> devtool_thread_;
};

TEST_F(InspectorLynxNativeModuleAgentTest, DispatchesEnableAndDisable) {
  Json::Value response = Dispatch("LynxNativeModule.enable", 1);

  EXPECT_EQ(response["id"].asInt64(), 1);
  EXPECT_TRUE(response["result"].isObject());

  response = Dispatch("LynxNativeModule.disable", 2);

  EXPECT_EQ(response["id"].asInt64(), 2);
  EXPECT_TRUE(response["result"].isObject());
}

TEST_F(InspectorLynxNativeModuleAgentTest, DispatchesGetRecords) {
  Json::Value response = Dispatch("LynxNativeModule.getRecords", 7);

  EXPECT_EQ(response["id"].asInt64(), 7);
  EXPECT_EQ(response["result"]["latestSequence"].asInt64(), 0);
  const Json::Value& records = response["result"]["records"];
  EXPECT_TRUE(records.isArray());
  EXPECT_TRUE(records.empty());
}

TEST_F(InspectorLynxNativeModuleAgentTest, RejectsUnknownMethod) {
  Json::Value response = Dispatch("LynxNativeModule.unknown", 9);

  EXPECT_EQ(response["id"].asInt64(), 9);
  EXPECT_EQ(response["error"]["code"].asInt(), kInspectorErrorCode);
  EXPECT_EQ(response["error"]["message"].asString(),
            "Not implemented: LynxNativeModule.unknown");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
