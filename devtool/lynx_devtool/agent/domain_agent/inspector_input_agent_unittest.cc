// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_input_agent.h"

#include <chrono>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace devtool {
namespace testing {

class InputAgentTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    OnMessage(type, msg);
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    Json::Value value;
    Json::Reader reader;
    if (!reader.parse(msg, value, false)) {
      value = msg;
    }
    OnMessage(type, value);
  }

  bool WaitForMessageCount(size_t count, std::chrono::milliseconds timeout =
                                             std::chrono::milliseconds(1000)) {
    std::unique_lock<std::mutex> lock(mutex_);
    return condition_.wait_for(
        lock, timeout, [this, count]() { return messages_.size() >= count; });
  }

  std::vector<std::pair<std::string, Json::Value>> Messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
  }

 private:
  void OnMessage(const std::string& type, const Json::Value& message) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      messages_.emplace_back(type, message);
    }
    condition_.notify_all();
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<std::pair<std::string, Json::Value>> messages_;
};

class InspectorInputAgentTest : public ::testing::Test {
 public:
  InspectorInputAgentTest() = default;
  ~InspectorInputAgentTest() override = default;

  static fml::Thread& GetUIThread() {
    static fml::Thread thread("input_agent_ui");
    return thread;
  }

  void SetUp() override {
    devtool_mediator_ = std::make_shared<LynxDevToolMediator>();
    ui_executor_ = std::make_shared<InspectorUIExecutor>(devtool_mediator_);
    platform_facade_ =
        std::make_shared<lynx::testing::DevToolPlatformFacadeMock>();
    ui_executor_->SetDevToolPlatformFacade(platform_facade_);
    devtool_mediator_->ui_task_runner_ = GetUIThread().GetTaskRunner();
    devtool_mediator_->ui_executor_ = ui_executor_;
    agent_ = std::make_shared<InspectorInputAgent>(devtool_mediator_);
    message_sender_ = std::make_shared<InputAgentTestMessageSender>();
  }

  void TearDown() override {
    GetUIThread().GetTaskRunner()->PostSyncTask([this]() {
      ui_executor_->SetDevToolPlatformFacade(nullptr);
      devtool_mediator_->ui_executor_.reset();
      ui_executor_.reset();
    });
    agent_.reset();
    platform_facade_.reset();
    message_sender_.reset();
    devtool_mediator_.reset();
  }

 protected:
  Json::Value BuildTapMessage(int64_t id, double x = 10, double y = 20) {
    Json::Value message(Json::ValueType::objectValue);
    message["id"] = id;
    message["method"] = "Input.synthesizeTapGesture";
    message["params"]["x"] = x;
    message["params"]["y"] = y;
    message["params"]["duration"] = 0;
    return message;
  }

  Json::Value LastResponse() {
    const auto messages = message_sender_->Messages();
    EXPECT_FALSE(messages.empty());
    EXPECT_EQ(messages.back().first, "CDP");
    return messages.back().second;
  }

  std::shared_ptr<InspectorInputAgent> agent_;
  std::shared_ptr<LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<InspectorUIExecutor> ui_executor_;
  std::shared_ptr<lynx::testing::DevToolPlatformFacadeMock> platform_facade_;
  std::shared_ptr<InputAgentTestMessageSender> message_sender_;
};

TEST_F(InspectorInputAgentTest, InsertTextReturnsSuccess) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 7;
  message["method"] = "Input.insertText";
  message["params"]["text"] = "hello";

  agent_->CallMethod(message_sender_, message);
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));

  const Json::Value response = LastResponse();
  EXPECT_EQ(response["id"].asInt64(), 7);
  EXPECT_TRUE(response["result"].isObject());
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureUsesDefaultTouchSource) {
  agent_->CallMethod(message_sender_, BuildTapMessage(8));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].source_type, input::PointerSourceType::kTouch);
  EXPECT_EQ(events[0].type, input::PointerEventType::kDown);
  EXPECT_EQ(events[1].type, input::PointerEventType::kUp);
  EXPECT_EQ(LastResponse()["id"].asInt64(), 8);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureUsesDefaultDuration) {
  Json::Value message = BuildTapMessage(9);
  message["params"].removeMember("duration");

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_GE(events[1].timestamp_us - events[0].timestamp_us, 50000);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureUsesDefaultMouseSource) {
  input::PointerCapabilities capabilities;
  capabilities.default_source_type = input::PointerSourceType::kMouse;
  capabilities.supports_mouse = true;
  platform_facade_->mock_input_event_target_->SetCapabilities(capabilities);

  agent_->CallMethod(message_sender_, BuildTapMessage(10));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].source_type, input::PointerSourceType::kMouse);
  EXPECT_EQ(events[0].type, input::PointerEventType::kDown);
  EXPECT_EQ(events[1].type, input::PointerEventType::kUp);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureSupportsExplicitTouch) {
  Json::Value message = BuildTapMessage(11);
  message["params"]["gestureSourceType"] = "touch";

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].source_type, input::PointerSourceType::kTouch);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureHonorsTapCount) {
  Json::Value message = BuildTapMessage(12);
  message["params"]["tapCount"] = 2;

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 4u);
  EXPECT_EQ(events[0].click_count, 1);
  EXPECT_EQ(events[1].click_count, 1);
  EXPECT_EQ(events[2].click_count, 1);
  EXPECT_EQ(events[3].click_count, 1);
  EXPECT_NE(events[0].action_pointer_id, events[2].action_pointer_id);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGesturesAreQueued) {
  Json::Value first = BuildTapMessage(20, 10, 20);
  first["params"]["duration"] = 50;
  Json::Value second = BuildTapMessage(21, 30, 20);

  agent_->CallMethod(message_sender_, first);
  agent_->CallMethod(message_sender_, second);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(2));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 4u);
  EXPECT_EQ(events[0].type, input::PointerEventType::kDown);
  EXPECT_EQ(events[1].type, input::PointerEventType::kUp);
  EXPECT_EQ(events[2].type, input::PointerEventType::kDown);
  EXPECT_EQ(events[3].type, input::PointerEventType::kUp);
  EXPECT_FLOAT_EQ(events[0].pointers[0].x, 10.f);
  EXPECT_FLOAT_EQ(events[2].pointers[0].x, 30.f);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapWaitsForProcessingResult) {
  platform_facade_->mock_input_event_target_->SetProcessingResult(false);

  agent_->CallMethod(message_sender_, BuildTapMessage(22));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizeTapGesture failed");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapReportsInjectionFailure) {
  platform_facade_->mock_input_event_target_->SetInjectionResult(false);

  agent_->CallMethod(message_sender_, BuildTapMessage(23));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizeTapGesture failed");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsUnsupportedSource) {
  Json::Value message = BuildTapMessage(24);
  message["params"]["gestureSourceType"] = "mouse";

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizeTapGesture source mouse");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsInvalidSource) {
  Json::Value message = BuildTapMessage(25);
  message["params"]["gestureSourceType"] = "pen";

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(
      LastResponse()["error"]["message"].asString(),
      "Invalid params: expected gestureSourceType default, touch, or mouse");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureAllowsZeroTapCount) {
  Json::Value message = BuildTapMessage(26);
  message["params"]["tapCount"] = 0;

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest,
       SynthesizeTapGestureRejectsNonFiniteCoordinate) {
  Json::Value message =
      BuildTapMessage(27, std::numeric_limits<double>::infinity(), 20);

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: expected finite numeric x and y");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsNegativeDuration) {
  Json::Value message = BuildTapMessage(28);
  message["params"]["duration"] = -1;

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: duration must be a non-negative integer");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsExcessiveDuration) {
  Json::Value message = BuildTapMessage(29);
  message["params"]["duration"] = 10001;

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: tap sequence duration exceeds 10000 ms");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsExcessiveTapCount) {
  Json::Value message = BuildTapMessage(30);
  message["params"]["tapCount"] = 201;

  agent_->CallMethod(message_sender_, message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: tapCount exceeds 200");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsMissingTarget) {
  ui_executor_->SetDevToolPlatformFacade(nullptr);

  agent_->CallMethod(message_sender_, BuildTapMessage(31));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input target is unavailable");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsNullInputTarget) {
  auto platform_facade =
      std::make_shared<lynx::testing::DevToolPlatformFacadeMock>();
  platform_facade->ResetInputEventTarget();
  ui_executor_->SetDevToolPlatformFacade(platform_facade);

  agent_->CallMethod(message_sender_, BuildTapMessage(32));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizeTapGesture");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
