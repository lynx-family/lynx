// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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
#include "devtool/lynx_devtool/agent/domain_agent/inspector_layer_tree_agent_ng.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class LayerTreeTestMessageSender : public MessageSender {
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

class InspectorLayerTreeAgentTest : public ::testing::Test {
 public:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    executor_ = std::make_shared<InspectorTasmExecutor>(mediator_, 1);
    mediator_->element_executor_ = executor_;
    tasm_thread_ = std::make_unique<fml::Thread>("layer_tree_test");
    mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    tasm_thread_->GetTaskRunner()->PostSyncTask(
        [this] { tasm_thread_id_ = std::this_thread::get_id(); });

    response_sender_ = std::make_shared<LayerTreeTestMessageSender>();
    event_sender_ = std::make_shared<LayerTreeTestMessageSender>();
    listener_ = std::make_shared<LayerTreeTestMessageSender>();
    devtool_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    devtool_->message_sender_ = event_sender_;
    mediator_->devtool_wp_ = devtool_;
    mediator_->AddCDPEventListener("LayerTreeTest", listener_);
    agent_ = std::make_unique<InspectorLayerTreeAgentNG>(mediator_);
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
      auto responder =
          std::make_shared<CDPResponder>(response_sender_, kRequestId);
      agent_->CallMethod(responder, message);
    }
    tasm_thread_->GetTaskRunner()->PostSyncTask([] {});
  }

  Json::Value OnlyResponse() {
    auto messages = response_sender_->Messages();
    EXPECT_EQ(messages.size(), 1u);
    if (messages.empty()) {
      return Json::Value();
    }
    EXPECT_EQ(messages[0].value["id"].asInt64(), kRequestId);
    EXPECT_FALSE(messages[0].value.isMember("method"));
    return messages[0].value;
  }

  static constexpr int64_t kRequestId = 1LL << 40;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::thread::id tasm_thread_id_;
  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<InspectorTasmExecutor> executor_;
  std::shared_ptr<lynx::testing::LynxDevToolNGMock> devtool_;
  std::unique_ptr<InspectorLayerTreeAgentNG> agent_;
  std::shared_ptr<LayerTreeTestMessageSender> response_sender_;
  std::shared_ptr<LayerTreeTestMessageSender> event_sender_;
  std::shared_ptr<LayerTreeTestMessageSender> listener_;
};

TEST_F(InspectorLayerTreeAgentTest, EnableWithoutParamsRespondsBeforeEvents) {
  devtool_->message_sender_ = response_sender_;

  Dispatch("LayerTree.enable", Json::Value(), true);

  auto messages = response_sender_->Messages();
  ASSERT_EQ(messages.size(), 3u);
  EXPECT_EQ(messages[0].value["id"].asInt64(), kRequestId);
  EXPECT_EQ(messages[0].value["result"], Json::Value(Json::objectValue));
  EXPECT_EQ(messages[1].value["method"], "LayerTree.layerPainted");
  EXPECT_EQ(messages[2].value["method"], "LayerTree.layerTreeDidChange");
  EXPECT_FALSE(messages[1].value.isMember("id"));
  EXPECT_FALSE(messages[2].value.isMember("id"));
  for (const auto& message : messages) {
    EXPECT_EQ(message.thread_id, tasm_thread_id_);
  }
  EXPECT_TRUE(executor_->layer_tree_enabled_);
  auto events = listener_->Messages();
  ASSERT_EQ(events.size(), 2u);
  EXPECT_EQ(events[0].value, messages[1].value);
  EXPECT_EQ(events[1].value, messages[2].value);
}

TEST_F(InspectorLayerTreeAgentTest, DisableWithNullParamsStopsTreeEvents) {
  tasm_thread_->GetTaskRunner()->PostSyncTask(
      [this] { executor_->layer_tree_enabled_ = true; });

  Dispatch("LayerTree.disable", Json::Value());
  mediator_->SendLayerTreeDidChangeEvent();
  tasm_thread_->GetTaskRunner()->PostSyncTask([] {});

  EXPECT_EQ(OnlyResponse()["result"], Json::Value(Json::objectValue));
  auto messages = response_sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0].thread_id, tasm_thread_id_);
  EXPECT_FALSE(executor_->layer_tree_enabled_);
  EXPECT_TRUE(event_sender_->Messages().empty());
  EXPECT_TRUE(listener_->Messages().empty());
}

TEST_F(InspectorLayerTreeAgentTest, CompositingReasonsReturnsToRequestSender) {
  Json::Value params(Json::objectValue);
  params["layerId"] = "0";

  Dispatch("LayerTree.compositingReasons", params);

  Json::Value expected(Json::objectValue);
  expected["compositingReasons"] = Json::Value(Json::arrayValue);
  expected["compositingReasonsIds"] = Json::Value(Json::arrayValue);
  EXPECT_EQ(OnlyResponse()["result"], expected);
  auto messages = response_sender_->Messages();
  ASSERT_EQ(messages.size(), 1u);
  EXPECT_EQ(messages[0].thread_id, tasm_thread_id_);
  EXPECT_TRUE(event_sender_->Messages().empty());
  EXPECT_TRUE(listener_->Messages().empty());
}

TEST_F(InspectorLayerTreeAgentTest, UnknownMethodReturnsMethodNotFound) {
  Dispatch("LayerTree.unknown", Json::Value());

  auto response = OnlyResponse();
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::MethodNotFound));
  EXPECT_EQ(response["error"]["message"], "'LayerTree.unknown' wasn't found");
  EXPECT_FALSE(response.isMember("result"));
}

TEST_F(InspectorLayerTreeAgentTest, CompositingReasonsRejectsMissingParams) {
  Dispatch("LayerTree.compositingReasons", Json::Value(), true);

  auto response = OnlyResponse();
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(CDPErrorCode::InvalidParams));
  EXPECT_EQ(response["error"]["message"],
            "Invalid layerId: expected integer string");
}

TEST_F(InspectorLayerTreeAgentTest, CompositingReasonsRejectsInvalidLayerIds) {
  const std::vector<Json::Value> invalid_ids = {
      Json::Value(),
      Json::Value(Json::arrayValue),
      Json::Value(Json::objectValue),
      Json::Value(true),
      Json::Value(1),
      Json::Value(""),
      Json::Value("abc"),
      Json::Value("1suffix"),
      Json::Value("2147483648"),
      Json::Value("-2147483649"),
      Json::Value("999999999999999999999999999999")};
  for (const auto& layer_id : invalid_ids) {
    SCOPED_TRACE(layer_id.toStyledString());
    response_sender_ = std::make_shared<LayerTreeTestMessageSender>();
    Json::Value params(Json::objectValue);
    params["layerId"] = layer_id;
    Dispatch("LayerTree.compositingReasons", params);

    auto response = OnlyResponse();
    EXPECT_EQ(response["error"]["code"].asInt(),
              static_cast<int>(CDPErrorCode::InvalidParams));
    EXPECT_EQ(response["error"]["message"],
              "Invalid layerId: expected integer string");
    EXPECT_FALSE(response.isMember("result"));
  }
  EXPECT_TRUE(event_sender_->Messages().empty());
  EXPECT_TRUE(listener_->Messages().empty());
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
