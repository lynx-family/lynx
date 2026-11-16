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
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
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

class InputAgentPlatformFacadeMock
    : public lynx::testing::DevToolPlatformFacadeMock {
 public:
  struct KeyEventRecord {
    std::string type;
    std::string key;
    std::string text;
    int modifiers;
    bool auto_repeat;
  };

  void EmulateMouse(std::shared_ptr<MouseEvent> input) override {
    std::lock_guard<std::mutex> lock(mutex_);
    mouse_events_.push_back(*input);
  }

  std::vector<MouseEvent> MouseEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mouse_events_;
  }

  bool DispatchKeyEvent(const std::string& type, const std::string& key,
                        const std::string& text, int modifiers,
                        bool auto_repeat) override {
    std::lock_guard<std::mutex> lock(mutex_);
    key_events_.push_back({type, key, text, modifiers, auto_repeat});
    return true;
  }

  std::vector<KeyEventRecord> KeyEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return key_events_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<MouseEvent> mouse_events_;
  std::vector<KeyEventRecord> key_events_;
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
    platform_facade_ = std::make_shared<InputAgentPlatformFacadeMock>();
    devtool_mediator_->ui_task_runner_ = GetUIThread().GetTaskRunner();
    devtool_mediator_->ui_executor_ = ui_executor_;
    SetDevToolPlatformFacade(platform_facade_);
    agent_ = std::make_shared<InspectorInputAgent>(devtool_mediator_);
    message_sender_ = std::make_shared<InputAgentTestMessageSender>();
  }

  void TearDown() override {
    SetDevToolPlatformFacade(nullptr);
    GetUIThread().GetTaskRunner()->PostSyncTask([this]() {
      devtool_mediator_->ui_executor_.reset();
      ui_executor_.reset();
    });
    agent_.reset();
    platform_facade_.reset();
    message_sender_.reset();
    devtool_mediator_.reset();
  }

 protected:
  void SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& platform_facade) {
    devtool_mediator_->SetDevToolPlatformFacade(platform_facade);
    GetUIThread().GetTaskRunner()->PostSyncTask([]() {});
  }

  Json::Value BuildTapMessage(int64_t id, double x = 10, double y = 20) {
    Json::Value message(Json::ValueType::objectValue);
    message["id"] = id;
    message["method"] = "Input.synthesizeTapGesture";
    message["params"]["x"] = x;
    message["params"]["y"] = y;
    message["params"]["duration"] = 0;
    return message;
  }

  Json::Value BuildKeyMessage(int64_t id, const std::string& type = "keyDown") {
    Json::Value message(Json::objectValue);
    message["id"] = id;
    message["method"] = "Input.dispatchKeyEvent";
    message["params"]["type"] = type;
    message["params"]["key"] = "A";
    return message;
  }

  Json::Value BuildPinchMessage(int64_t id, double x = 10, double y = 20,
                                double scale_factor = 2) {
    Json::Value message(Json::ValueType::objectValue);
    message["id"] = id;
    message["method"] = "Input.synthesizePinchGesture";
    message["params"]["x"] = x;
    message["params"]["y"] = y;
    message["params"]["scaleFactor"] = scale_factor;
    return message;
  }

  Json::Value BuildScrollMessage(int64_t id) {
    Json::Value message(Json::objectValue);
    message["id"] = id;
    message["method"] = "Input.synthesizeScrollGesture";
    message["params"]["x"] = 100;
    message["params"]["y"] = 200;
    message["params"]["yDistance"] = -80;
    return message;
  }

  void EnableTwoTouchPoints() {
    input::PointerCapabilities capabilities;
    capabilities.default_source_type = input::PointerSourceType::kTouch;
    capabilities.supports_touch = true;
    capabilities.max_touch_points = 2;
    platform_facade_->mock_input_event_target_->SetCapabilities(capabilities);
  }

  void Dispatch(const Json::Value& message) {
    auto responder = std::make_shared<CDPResponder>(message_sender_,
                                                    message["id"].asInt64());
    agent_->CallMethod(responder, message);
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
  std::shared_ptr<InputAgentPlatformFacadeMock> platform_facade_;
  std::shared_ptr<InputAgentTestMessageSender> message_sender_;
};

TEST_F(InspectorInputAgentTest, InsertTextReturnsSuccess) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 7;
  message["method"] = "Input.insertText";
  message["params"]["text"] = "hello";

  Dispatch(message);
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));

  const Json::Value response = LastResponse();
  EXPECT_EQ(response["id"].asInt64(), 7);
  EXPECT_TRUE(response["result"].isObject());
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventUsesOptionalDefaults) {
  Dispatch(BuildKeyMessage(100));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->KeyEvents();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].type, "keyDown");
  EXPECT_EQ(events[0].key, "A");
  EXPECT_TRUE(events[0].text.empty());
  EXPECT_EQ(events[0].modifiers, 0);
  EXPECT_FALSE(events[0].auto_repeat);
  EXPECT_EQ(LastResponse()["id"].asInt64(), 100);
  EXPECT_TRUE(LastResponse()["result"].isObject());
  EXPECT_TRUE(LastResponse()["result"].empty());
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventForwardsTypesAndRepeat) {
  const std::vector<std::string> types = {"keyDown", "rawKeyDown", "keyUp"};
  for (size_t index = 0; index < types.size(); ++index) {
    auto message = BuildKeyMessage(101 + index, types[index]);
    message["params"]["text"] = "ignored text";
    message["params"]["modifiers"] = 0;
    message["params"]["autoRepeat"] = true;
    message["params"]["commands"] = Json::Value(Json::arrayValue);
    Dispatch(message);
    ASSERT_TRUE(message_sender_->WaitForMessageCount(index + 1));
    EXPECT_EQ(LastResponse()["id"].asInt64(), 101 + index);
    EXPECT_TRUE(LastResponse()["result"].isObject());
  }
  const auto events = platform_facade_->KeyEvents();
  ASSERT_EQ(events.size(), types.size());
  for (size_t index = 0; index < types.size(); ++index) {
    EXPECT_EQ(events[index].type, types[index]);
    EXPECT_EQ(events[index].text, "ignored text");
    EXPECT_TRUE(events[index].auto_repeat);
  }
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventRejectsInvalidParams) {
  std::vector<Json::Value> invalid_params;
  const auto valid_params = BuildKeyMessage(110)["params"];
  auto add_invalid = [&](const char* field, const Json::Value& value) {
    auto params = valid_params;
    params[field] = value;
    invalid_params.push_back(std::move(params));
  };
  invalid_params.emplace_back(Json::nullValue);
  invalid_params.emplace_back(Json::arrayValue);
  add_invalid("type", Json::Value());
  add_invalid("type", 42);
  add_invalid("type", "keyPress");
  add_invalid("key", Json::Value());
  add_invalid("key", 42);
  add_invalid("key", "");
  add_invalid("key", std::string("A\0B", 3));
  add_invalid("text", Json::Value());
  add_invalid("text", 42);
  add_invalid("modifiers", Json::Value());
  add_invalid("modifiers", true);
  add_invalid("modifiers", "0");
  add_invalid("modifiers", 0.5);
  add_invalid("modifiers", -1);
  add_invalid("modifiers", 16);
  add_invalid("modifiers", Json::Value(Json::UInt64{1} << 40));
  add_invalid("autoRepeat", Json::Value());
  add_invalid("autoRepeat", 1);
  add_invalid("autoRepeat", "true");
  add_invalid("commands", Json::Value());
  add_invalid("commands", "selectAll");
  Json::Value commands(Json::arrayValue);
  commands.append(42);
  add_invalid("commands", commands);
  auto missing_type = valid_params;
  missing_type.removeMember("type");
  invalid_params.push_back(std::move(missing_type));
  auto missing_key = valid_params;
  missing_key.removeMember("key");
  invalid_params.push_back(std::move(missing_key));

  for (size_t index = 0; index < invalid_params.size(); ++index) {
    SCOPED_TRACE(index);
    auto message = BuildKeyMessage(110 + index);
    message["params"] = invalid_params[index];
    Dispatch(message);
    ASSERT_TRUE(message_sender_->WaitForMessageCount(index + 1));
    EXPECT_EQ(LastResponse()["id"].asInt64(), 110 + index);
    EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
  }
  EXPECT_TRUE(platform_facade_->KeyEvents().empty());
  EXPECT_EQ(message_sender_->Messages().size(), invalid_params.size());
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventRejectsUnsupportedFeatures) {
  std::vector<Json::Value> messages;
  auto character = BuildKeyMessage(150, "char");
  character["params"].removeMember("key");
  character["params"]["text"] = "A";
  messages.push_back(std::move(character));
  auto modified = BuildKeyMessage(151);
  modified["params"]["modifiers"] = 8;
  messages.push_back(std::move(modified));
  auto commands = BuildKeyMessage(152);
  commands["params"]["commands"] = Json::Value(Json::arrayValue);
  commands["params"]["commands"].append("selectAll");
  messages.push_back(std::move(commands));

  for (size_t index = 0; index < messages.size(); ++index) {
    Dispatch(messages[index]);
    ASSERT_TRUE(message_sender_->WaitForMessageCount(index + 1));
    EXPECT_EQ(LastResponse()["id"], messages[index]["id"]);
    EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::ServerError));
  }
  EXPECT_TRUE(platform_facade_->KeyEvents().empty());
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventWithoutFacadeReturnsError) {
  SetDevToolPlatformFacade(nullptr);
  Dispatch(BuildKeyMessage(160));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input target is unavailable");
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventUnsupportedFacadeReturnsError) {
  SetDevToolPlatformFacade(
      std::make_shared<lynx::testing::DevToolPlatformFacadeMock>());
  Dispatch(BuildKeyMessage(161));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.dispatchKeyEvent");
}

TEST_F(InspectorInputAgentTest,
       DispatchKeyEventWithoutUITaskRunnerRepliesOnce) {
  devtool_mediator_->ui_task_runner_ = nullptr;
  Dispatch(BuildKeyMessage(162));
  devtool_mediator_->ui_task_runner_ = GetUIThread().GetTaskRunner();
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["id"].asInt64(), 162);
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_TRUE(platform_facade_->KeyEvents().empty());
  EXPECT_EQ(message_sender_->Messages().size(), 1u);
}

TEST_F(InspectorInputAgentTest, DispatchKeyEventWithoutUIExecutorRepliesOnce) {
  devtool_mediator_->ui_executor_ = nullptr;
  Dispatch(BuildKeyMessage(163));
  devtool_mediator_->ui_executor_ = ui_executor_;
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["id"].asInt64(), 163);
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_TRUE(platform_facade_->KeyEvents().empty());
  EXPECT_EQ(message_sender_->Messages().size(), 1u);
}

TEST_F(InspectorInputAgentTest, DispatchMouseEventForwardsPointerInput) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 35;
  message["method"] = "Input.dispatchMouseEvent";
  message["params"]["type"] = "mouseMoved";
  message["params"]["x"] = 195;
  message["params"]["y"] = 756;
  message["params"]["buttons"] = 1;
  message["params"]["modifiers"] = 8;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->MouseEvents();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].type_, "mouseMoved");
  EXPECT_EQ(events[0].x_, 195);
  EXPECT_EQ(events[0].y_, 756);
  EXPECT_EQ(events[0].button_, "left");
  EXPECT_EQ(events[0].modifiers_, 8);
  EXPECT_EQ(LastResponse()["id"].asInt64(), 35);
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest, DispatchMouseWheelUsesDispatchDirection) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 37;
  message["method"] = "Input.dispatchMouseEvent";
  message["params"]["type"] = "mouseWheel";
  message["params"]["x"] = 10;
  message["params"]["y"] = 20;
  message["params"]["deltaX"] = 1.5;
  message["params"]["deltaY"] = -2.5;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->MouseEvents();
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].button_, "none");
  EXPECT_FLOAT_EQ(events[0].delta_x_, 1.5f);
  EXPECT_FLOAT_EQ(events[0].delta_y_, -2.5f);
  EXPECT_EQ(LastResponse()["id"].asInt64(), 37);
}

TEST_F(InspectorInputAgentTest, EmulateTouchFromMouseEventReturnsSuccess) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 36;
  message["method"] = "Input.emulateTouchFromMouseEvent";
  message["params"]["button"] = "left";
  message["params"]["clickCount"] = 1;
  message["params"]["deltaX"] = 0;
  message["params"]["deltaY"] = 0;
  message["params"]["modifiers"] = 0;
  message["params"]["type"] = "mousePressed";
  message["params"]["x"] = 10;
  message["params"]["y"] = 20;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest, UnknownMethodReturnsMethodNotFound) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 36;
  message["method"] = "Input.unknown";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "'Input.unknown' wasn't found");
}

TEST_F(InspectorInputAgentTest, InsertTextWithoutFacadeReturnsServerError) {
  SetDevToolPlatformFacade(nullptr);
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 38;
  message["method"] = "Input.insertText";
  message["params"]["text"] = "hello";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input target is unavailable");
}

TEST_F(InspectorInputAgentTest,
       InsertTextWithNonStringTextReturnsInvalidParams) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 39;
  message["method"] = "Input.insertText";
  message["params"]["text"] = 42;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: expected string text");
}

TEST_F(InspectorInputAgentTest,
       EmulateTouchFromMouseEventWithoutFacadeReturnsServerError) {
  SetDevToolPlatformFacade(nullptr);
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 40;
  message["method"] = "Input.emulateTouchFromMouseEvent";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input target is unavailable");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureUsesDefaultTouchSource) {
  Dispatch(BuildTapMessage(8));

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

  Dispatch(message);

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

  Dispatch(BuildTapMessage(10));

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

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].source_type, input::PointerSourceType::kTouch);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureHonorsTapCount) {
  Json::Value message = BuildTapMessage(12);
  message["params"]["tapCount"] = 2;

  Dispatch(message);

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

  Dispatch(first);
  Dispatch(second);

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

TEST_F(InspectorInputAgentTest,
       SynthesizeTapGestureUsesNewTargetAfterFacadeChanges) {
  auto original_target = platform_facade_->mock_input_event_target_;
  Dispatch(BuildTapMessage(33, 10, 20));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));

  platform_facade_ = std::make_shared<InputAgentPlatformFacadeMock>();
  SetDevToolPlatformFacade(platform_facade_);
  Dispatch(BuildTapMessage(34, 30, 40));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(2));
  EXPECT_EQ(original_target->Events().size(), 2u);
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_FLOAT_EQ(events[0].pointers[0].x, 30.f);
  EXPECT_FLOAT_EQ(events[0].pointers[0].y, 40.f);
}

TEST_F(InspectorInputAgentTest, SynthesizeTapWaitsForProcessingResult) {
  platform_facade_->mock_input_event_target_->SetProcessingResult(false);

  Dispatch(BuildTapMessage(22));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizeTapGesture failed");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapReportsInjectionFailure) {
  platform_facade_->mock_input_event_target_->SetInjectionResult(false);

  Dispatch(BuildTapMessage(23));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizeTapGesture failed");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsUnsupportedSource) {
  Json::Value message = BuildTapMessage(24);
  message["params"]["gestureSourceType"] = "mouse";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizeTapGesture source mouse");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsInvalidSource) {
  Json::Value message = BuildTapMessage(25);
  message["params"]["gestureSourceType"] = "pen";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(
      LastResponse()["error"]["message"].asString(),
      "Invalid params: expected gestureSourceType default, touch, or mouse");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsMissingParams) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 42;
  message["method"] = "Input.synthesizeTapGesture";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: expected finite numeric x and y");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureAllowsZeroTapCount) {
  Json::Value message = BuildTapMessage(26);
  message["params"]["tapCount"] = 0;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest,
       SynthesizeTapGestureRejectsNonFiniteCoordinate) {
  Json::Value message =
      BuildTapMessage(27, std::numeric_limits<double>::infinity(), 20);

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: expected finite numeric x and y");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsNegativeDuration) {
  Json::Value message = BuildTapMessage(28);
  message["params"]["duration"] = -1;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: duration must be a non-negative integer");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsRealValuedDuration) {
  Json::Value message = BuildTapMessage(29);
  message["params"]["duration"] = 1.0;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: duration must be a non-negative integer");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsExcessiveDuration) {
  Json::Value message = BuildTapMessage(29);
  message["params"]["duration"] = 10001;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: tap sequence duration exceeds 10000 ms");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsExcessiveTapCount) {
  Json::Value message = BuildTapMessage(30);
  message["params"]["tapCount"] = 201;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: tapCount exceeds 200");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest, SynthesizePinchGestureEmitsTwoPointerSequence) {
  EnableTwoTouchPoints();
  Json::Value message = BuildPinchMessage(40);
  message["params"]["relativeSpeed"] = 100000;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 5u);
  EXPECT_EQ(events[0].type, input::PointerEventType::kDown);
  ASSERT_EQ(events[0].pointers.size(), 1u);
  EXPECT_EQ(events[1].type, input::PointerEventType::kDown);
  ASSERT_EQ(events[1].pointers.size(), 2u);
  EXPECT_EQ(events[2].type, input::PointerEventType::kMove);
  ASSERT_EQ(events[2].pointers.size(), 2u);
  EXPECT_EQ(events[3].type, input::PointerEventType::kUp);
  ASSERT_EQ(events[3].pointers.size(), 2u);
  EXPECT_EQ(events[4].type, input::PointerEventType::kUp);
  ASSERT_EQ(events[4].pointers.size(), 1u);
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest,
       SynthesizePinchGestureUsesDefaultRelativeSpeed) {
  EnableTwoTouchPoints();

  Dispatch(BuildPinchMessage(41));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_GE(events.size(), 5u);
  EXPECT_GE(events.back().timestamp_us - events.front().timestamp_us, 125000);
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest,
       SynthesizePinchGestureRejectsSinglePointerTarget) {
  Dispatch(BuildPinchMessage(42));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizePinchGesture requires two touch "
            "points");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizePinchGestureAllowsScaleOne) {
  EnableTwoTouchPoints();
  Dispatch(BuildPinchMessage(43, 10, 20, 1));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest, SynthesizePinchGestureRejectsInvalidScale) {
  Json::Value message = BuildPinchMessage(44);
  message["params"]["scaleFactor"] = 0;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: scaleFactor must be a finite positive number");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest,
       SynthesizePinchGestureRejectsInvalidRelativeSpeed) {
  Json::Value message = BuildPinchMessage(45);
  message["params"]["relativeSpeed"] = 0;

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Invalid params: relativeSpeed must be a positive integer");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
}

TEST_F(InspectorInputAgentTest,
       SynthesizePinchGestureRejectsUnsupportedSource) {
  Json::Value message = BuildPinchMessage(46);
  message["params"]["gestureSourceType"] = "mouse";

  Dispatch(message);

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizePinchGesture source mouse");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizePinchReportsInjectionFailure) {
  EnableTwoTouchPoints();
  platform_facade_->mock_input_event_target_->SetInjectionResult(false);

  Dispatch(BuildPinchMessage(47));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizePinchGesture failed");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizeScrollGestureUsesTouchDefaults) {
  Dispatch(BuildScrollMessage(50));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_GE(events.size(), 4u);
  EXPECT_EQ(events.front().type, input::PointerEventType::kDown);
  EXPECT_EQ(events.back().type, input::PointerEventType::kUp);
  EXPECT_FLOAT_EQ(events.back().pointers[0].x, 100);
  EXPECT_FLOAT_EQ(events.back().pointers[0].y, 120);
  // 80 logical pixels at the default 800 px/s plus 100 ms to stop.
  EXPECT_GE(events.back().timestamp_us - events.front().timestamp_us, 200000);
  for (const auto& event : events) {
    EXPECT_EQ(event.source_type, input::PointerSourceType::kTouch);
    EXPECT_EQ(event.pointers.size(), 1u);
  }
  EXPECT_TRUE(LastResponse()["result"].isObject());
  EXPECT_EQ(LastResponse()["id"].asInt64(), 50);
}

TEST_F(InspectorInputAgentTest, SynthesizeScrollGestureHonorsOptionsAndRepeat) {
  auto message = BuildScrollMessage(51);
  message["params"]["gestureSourceType"] = "touch";
  message["params"]["xDistance"] = 40;
  message["params"]["xOverscroll"] = 10;
  message["params"]["yOverscroll"] = -20;
  message["params"]["speed"] = 100000;
  message["params"]["preventFling"] = false;
  message["params"]["repeatCount"] = 1;
  message["params"]["repeatDelayMs"] = 100;
  Dispatch(message);
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  const auto events = platform_facade_->mock_input_event_target_->Events();
  ASSERT_EQ(events.size(), 8u);
  EXPECT_FLOAT_EQ(events[1].pointers[0].x, 140);
  EXPECT_FLOAT_EQ(events[1].pointers[0].y, 120);
  EXPECT_FLOAT_EQ(events[2].pointers[0].x, 130);
  EXPECT_FLOAT_EQ(events[2].pointers[0].y, 140);
  EXPECT_EQ(events[3].type, input::PointerEventType::kUp);
  EXPECT_EQ(events[3].timestamp_us, events[2].timestamp_us);
  EXPECT_EQ(events[4].type, input::PointerEventType::kDown);
  EXPECT_FLOAT_EQ(events[4].pointers[0].y, 200);
  EXPECT_GE(events[4].timestamp_us - events[3].timestamp_us, 100000);
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest, SynthesizeScrollGestureAllowsNoDisplacement) {
  auto message = BuildScrollMessage(52);
  message["params"].removeMember("yDistance");
  Dispatch(message);
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
  EXPECT_TRUE(LastResponse()["result"].isObject());
}

TEST_F(InspectorInputAgentTest, SynthesizeScrollGestureRejectsInvalidParams) {
  const std::pair<std::string, Json::Value> invalid_params[] = {
      {"x", Json::Value()},
      {"y", std::numeric_limits<double>::infinity()},
      {"xDistance", "10"},
      {"yDistance", std::numeric_limits<double>::quiet_NaN()},
      {"xOverscroll", std::numeric_limits<double>::infinity()},
      {"yOverscroll", Json::Value()},
      {"speed", 0},
      {"speed", -1},
      {"speed", 800.0},
      {"preventFling", "true"},
      {"repeatCount", -1},
      {"repeatCount", 1.5},
      {"repeatDelayMs", -1},
      {"repeatDelayMs", 0.0},
      {"gestureSourceType", "pen"},
      {"interactionMarkerName", 1},
      {"yDistance", 1e30},
      {"repeatCount", std::numeric_limits<int>::max()},
  };
  size_t count = 0;
  for (const auto& param : invalid_params) {
    SCOPED_TRACE(param.first + ": " + param.second.toStyledString());
    auto message = BuildScrollMessage(53);
    message["params"][param.first] = param.second;
    Dispatch(message);
    ASSERT_TRUE(message_sender_->WaitForMessageCount(++count));
    EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
  }
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
}

TEST_F(InspectorInputAgentTest, SynthesizeScrollGestureRejectsMouseTargets) {
  input::PointerCapabilities capabilities;
  capabilities.default_source_type = input::PointerSourceType::kMouse;
  capabilities.supports_mouse = true;
  capabilities.supports_touch = true;
  platform_facade_->mock_input_event_target_->SetCapabilities(capabilities);
  Dispatch(BuildScrollMessage(54));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));

  auto message = BuildScrollMessage(55);
  message["params"]["gestureSourceType"] = "mouse";
  Dispatch(message);
  ASSERT_TRUE(message_sender_->WaitForMessageCount(2));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizeScrollGesture source mouse");
  EXPECT_TRUE(platform_facade_->mock_input_event_target_->Events().empty());
}

TEST_F(InspectorInputAgentTest, SynthesizeScrollGestureRejectsMissingTargets) {
  platform_facade_->ResetInputEventTarget();
  Dispatch(BuildScrollMessage(56));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizeScrollGesture");
  SetDevToolPlatformFacade(nullptr);
  Dispatch(BuildScrollMessage(57));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(2));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input target is unavailable");
}

TEST_F(InspectorInputAgentTest,
       SynthesizeScrollGestureReportsInjectionFailure) {
  platform_facade_->mock_input_event_target_->SetInjectionResult(false);
  auto message = BuildScrollMessage(58);
  message["params"]["repeatCount"] = 1;
  Dispatch(message);
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizeScrollGesture failed");
  EXPECT_EQ(platform_facade_->mock_input_event_target_->Events().size(), 1u);
}

TEST_F(InspectorInputAgentTest,
       SynthesizeScrollGestureReportsProcessingFailure) {
  platform_facade_->mock_input_event_target_->SetProcessingResult(false);
  Dispatch(BuildScrollMessage(59));
  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input.synthesizeScrollGesture failed");
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsMissingTarget) {
  SetDevToolPlatformFacade(nullptr);

  Dispatch(BuildTapMessage(31));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Input target is unavailable");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

TEST_F(InspectorInputAgentTest, SynthesizeTapGestureRejectsNullInputTarget) {
  auto platform_facade =
      std::make_shared<lynx::testing::DevToolPlatformFacadeMock>();
  platform_facade->ResetInputEventTarget();
  SetDevToolPlatformFacade(platform_facade);

  Dispatch(BuildTapMessage(32));

  ASSERT_TRUE(message_sender_->WaitForMessageCount(1));
  EXPECT_EQ(LastResponse()["error"]["message"].asString(),
            "Not implemented: Input.synthesizeTapGesture");
  EXPECT_EQ(LastResponse()["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
