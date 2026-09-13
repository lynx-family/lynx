// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "third_party/jsoncpp/include/json/json.h"

#define private public
#define protected public

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/domain_agent/inspector_overlay_agent_ng.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class OverlayTestMessageSender : public MessageSender {
 public:
  struct Message {
    Json::Value value;
    std::thread::id thread_id;
  };

  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    EXPECT_EQ(type, "CDP");
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back({message, std::this_thread::get_id()});
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {
    Json::Value value;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(message, value, false));
    SendMessage(type, value);
  }

  std::vector<Message> Messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<Message> messages_;
};

class InspectorOverlayAgentTest : public ::testing::Test {
 public:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    mediator_->element_executor_ =
        std::make_shared<InspectorTasmExecutor>(mediator_, 1);
    tasm_thread_ = std::make_unique<fml::Thread>("overlay_test");
    mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    tasm_thread_->GetTaskRunner()->PostSyncTask(
        [this] { tasm_thread_id_ = std::this_thread::get_id(); });
    sender_ = std::make_shared<OverlayTestMessageSender>();
    // The constructor must retain the mediator, not reference this temporary.
    agent_ = std::make_unique<InspectorOverlayAgentNG>(
        std::shared_ptr<LynxDevToolMediator>(mediator_));
  }

  void TearDown() override {
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
    tasm_thread_->Join();
    mediator_->tasm_task_runner_ = nullptr;
  }

 protected:
  void Dispatch(const std::string& method, const Json::Value& params,
                bool omit_params = false) {
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(kRequestId);
    message["method"] = method;
    if (!omit_params) {
      message["params"] = params;
    }
    {
      auto responder = std::make_shared<CDPResponder>(sender_, kRequestId);
      agent_->CallMethod(responder, message);
    }
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
  }

  Json::Value OnlyResponse(bool on_tasm_thread = true) {
    auto messages = sender_->Messages();
    EXPECT_EQ(messages.size(), 1u);
    if (messages.empty()) {
      return Json::Value();
    }
    EXPECT_EQ(messages[0].value["id"].asInt64(), kRequestId);
    EXPECT_FALSE(messages[0].value.isMember("method"));
    if (on_tasm_thread) {
      EXPECT_EQ(messages[0].thread_id, tasm_thread_id_);
    }
    return messages[0].value;
  }

  Json::Value BuildHighlightParams(int node_id) {
    Json::Value params(Json::objectValue);
    params["nodeId"] = node_id;
    auto& color = params["highlightConfig"]["contentColor"];
    color["r"] = 255;
    color["g"] = 0;
    color["b"] = 0;
    color["a"] = 0.5;
    return params;
  }

  static constexpr int64_t kRequestId = 1LL << 40;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::thread::id tasm_thread_id_;
  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::unique_ptr<InspectorOverlayAgentNG> agent_;
  std::shared_ptr<OverlayTestMessageSender> sender_;
};

TEST_F(InspectorOverlayAgentTest, HighlightWithoutParamsReturnsInvalidParams) {
  Dispatch("Overlay.highlightNode", Json::Value(), true);
  auto response = OnlyResponse();
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
  EXPECT_EQ(response["error"]["message"], "Invalid nodeId: expected integer");
  EXPECT_FALSE(response.isMember("result"));
}

TEST_F(InspectorOverlayAgentTest, HideWithoutParamsReturnsSuccess) {
  Dispatch("Overlay.hideHighlight", Json::Value(), true);
  EXPECT_EQ(OnlyResponse()["result"], Json::Value(Json::objectValue));
}

TEST_F(InspectorOverlayAgentTest, HideWithNullParamsReturnsSuccess) {
  Dispatch("Overlay.hideHighlight", Json::Value());
  EXPECT_EQ(OnlyResponse()["result"], Json::Value(Json::objectValue));
}

TEST_F(InspectorOverlayAgentTest, HighlightRejectsNullParamsAndMissingNodeId) {
  for (const auto& params : {Json::Value(), Json::Value(Json::objectValue)}) {
    sender_ = std::make_shared<OverlayTestMessageSender>();
    Dispatch("Overlay.highlightNode", params);
    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"], "Invalid nodeId: expected integer");
    EXPECT_FALSE(response.isMember("result"));
  }
}

TEST_F(InspectorOverlayAgentTest, HighlightRejectsInvalidNodeIdValues) {
  const std::vector<Json::Value> values = {
      Json::Value(),
      Json::Value(true),
      Json::Value("1"),
      Json::Value(1.0),
      Json::Value(1.5),
      Json::Value(Json::arrayValue),
      Json::Value(Json::objectValue),
      Json::Value(static_cast<Json::Int64>(2147483648LL)),
      Json::Value(static_cast<Json::Int64>(-2147483649LL))};
  for (const auto& value : values) {
    SCOPED_TRACE(value.toStyledString());
    sender_ = std::make_shared<OverlayTestMessageSender>();
    Json::Value params(Json::objectValue);
    params["nodeId"] = value;
    Dispatch("Overlay.highlightNode", params);
    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"], "Invalid nodeId: expected integer");
    EXPECT_FALSE(response.isMember("result"));
  }
}

TEST_F(InspectorOverlayAgentTest, HighlightRejectsInvalidHighlightConfig) {
  const std::vector<Json::Value> values = {Json::Value(), Json::Value(true),
                                           Json::Value("config"),
                                           Json::Value(Json::arrayValue)};
  for (const auto& value : values) {
    SCOPED_TRACE(value.toStyledString());
    sender_ = std::make_shared<OverlayTestMessageSender>();
    Json::Value params(Json::objectValue);
    params["nodeId"] = 1;
    params["highlightConfig"] = value;
    Dispatch("Overlay.highlightNode", params);
    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"],
              "Invalid highlightConfig: expected object");
    EXPECT_FALSE(response.isMember("result"));
  }
}

TEST_F(InspectorOverlayAgentTest, HighlightRejectsInvalidContentColorObject) {
  const std::vector<Json::Value> values = {Json::Value(), Json::Value(true),
                                           Json::Value("red"),
                                           Json::Value(Json::arrayValue)};
  for (const auto& value : values) {
    SCOPED_TRACE(value.toStyledString());
    sender_ = std::make_shared<OverlayTestMessageSender>();
    Json::Value params(Json::objectValue);
    params["nodeId"] = 1;
    params["highlightConfig"]["contentColor"] = value;
    Dispatch("Overlay.highlightNode", params);
    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"],
              "Invalid contentColor: expected object");
    EXPECT_FALSE(response.isMember("result"));
  }
}

TEST_F(InspectorOverlayAgentTest, HighlightRejectsInvalidRgbComponents) {
  const std::vector<Json::Value> values = {
      Json::Value(),
      Json::Value(true),
      Json::Value("1"),
      Json::Value(1.0),
      Json::Value(-1),
      Json::Value(256),
      Json::Value(static_cast<Json::Int64>(2147483648LL))};
  for (const auto& value : values) {
    SCOPED_TRACE(value.toStyledString());
    for (const auto* component : {"r", "g", "b"}) {
      SCOPED_TRACE(component);
      sender_ = std::make_shared<OverlayTestMessageSender>();
      Json::Value params = BuildHighlightParams(1);
      params["highlightConfig"]["contentColor"][component] = value;
      Dispatch("Overlay.highlightNode", params);
      auto response = OnlyResponse();
      EXPECT_EQ(response["error"]["code"].asInt(),
                static_cast<int>(CDPErrorCode::InvalidParams));
      EXPECT_EQ(
          response["error"]["message"],
          "Invalid contentColor: expected r, g, and b integers in [0, 255]");
      EXPECT_FALSE(response.isMember("result"));
    }
  }
}

TEST_F(InspectorOverlayAgentTest, HighlightRejectsInvalidAlpha) {
  const std::vector<Json::Value> values = {
      Json::Value(),
      Json::Value(true),
      Json::Value("0.5"),
      Json::Value(Json::arrayValue),
      Json::Value(-0.1),
      Json::Value(1.1),
      Json::Value(std::numeric_limits<double>::infinity()),
      Json::Value(std::numeric_limits<double>::quiet_NaN())};
  for (const auto& value : values) {
    SCOPED_TRACE(value.toStyledString());
    sender_ = std::make_shared<OverlayTestMessageSender>();
    Json::Value params = BuildHighlightParams(1);
    params["highlightConfig"]["contentColor"]["a"] = value;
    Dispatch("Overlay.highlightNode", params);
    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"],
              "Invalid contentColor.a: expected finite number in [0, 1]");
    EXPECT_FALSE(response.isMember("result"));
  }
}

TEST_F(InspectorOverlayAgentTest, MissingNodeReturnsTopLevelServerError) {
  Json::Value params = BuildHighlightParams(99999);
  Dispatch("Overlay.highlightNode", params);

  auto response = OnlyResponse();
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::ServerError));
  EXPECT_EQ(response["error"]["message"], "Node is not an Element");
  EXPECT_FALSE(response.isMember("result"));
}

TEST_F(InspectorOverlayAgentTest, UnknownMethodReturnsMethodNotFound) {
  Dispatch("Overlay.unknown", Json::Value());

  auto response = OnlyResponse(false);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"], "'Overlay.unknown' wasn't found");
  EXPECT_FALSE(response.isMember("result"));
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
