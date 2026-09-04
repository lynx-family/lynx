// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "devtool/base_devtool/native/global_message_channel.h"
#include "devtool/base_devtool/native/global_message_dispatcher.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_base_agent.h"
#include "devtool/base_devtool/native/test/mock_devtool.h"
#include "devtool/base_devtool/native/test/mock_message_handler.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace testing {

class BaseDevToolTest : public ::testing::Test {
 public:
  BaseDevToolTest() = default;
  ~BaseDevToolTest() override {}

  void SetUp() override {
    mock_devtool_ = std::make_shared<devtool::MockDevTool>();
  }

 private:
  std::shared_ptr<devtool::MockDevTool> mock_devtool_;
};

class TestDevToolMessageDispatcher : public devtool::DevToolMessageDispatcher {
 public:
  std::shared_ptr<devtool::MessageSender> GetSender() const override {
    return sender_;
  }

 private:
  std::shared_ptr<devtool::MessageSender> sender_ =
      std::make_shared<devtool::MessageSenderMock>();
};

class CapturingMessageSender : public devtool::MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    messages_.emplace_back(type, msg);
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    Json::Value value;
    Json::Reader reader;
    if (!reader.parse(msg, value, false)) {
      value = msg;
    }
    messages_.emplace_back(type, std::move(value));
  }

  const std::vector<std::pair<std::string, Json::Value>>& Messages() const {
    return messages_;
  }

 private:
  std::vector<std::pair<std::string, Json::Value>> messages_;
};

Json::Value ParseLastResponse() {
  Json::Value response;
  Json::Reader reader;
  EXPECT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, response,
      false));
  return response;
}

TEST_F(BaseDevToolTest, BaseDevToolAttachAndDetach) {
  std::string url = "www.mock.js";
  mock_devtool_->Attach(url);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().url_, url);
  mock_devtool_->Detach();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().url_, "");
}

TEST_F(BaseDevToolTest, BaseDevToolRegister) {
  auto agent = std::make_unique<devtool::MockBaseAgent>();
  mock_devtool_->RegisterAgent("MockAgent", std::move(agent));
  const std::string jsonResponse = R"(
    {
      "id": 1,
      "result": "empty"
    }
  )";
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", jsonResponse);
  // Because we don't implement Method in agent, it should return error.
  EXPECT_TRUE(
      devtool::MockReceiver::GetInstance().received_message_.second.find(
          "error") != std::string::npos);

  const std::string correctResponse = R"({
      "id": 1,
      "method": "MockAgent.test"
    })";
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", correctResponse);
  EXPECT_TRUE(devtool::MockReceiver::GetInstance().received_json_.find(
                  "method") != std::string::npos);
}

TEST_F(BaseDevToolTest, DispatchCDPMessageAllowsNullParams) {
  auto agent = std::make_unique<devtool::MockBaseAgent>();
  mock_devtool_->RegisterAgent("MockAgent", std::move(agent));
  devtool::MockReceiver::GetInstance().ResetAll();

  const std::string message = R"({
      "id": 1,
      "method": "MockAgent.test",
      "params": null
    })";
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", message);

  EXPECT_FALSE(devtool::MockReceiver::GetInstance().received_json_.empty());
  EXPECT_TRUE(
      devtool::MockReceiver::GetInstance().received_message_.second.empty());
}

TEST_F(BaseDevToolTest, DispatchCDPMessageRejectsNonObjectRoot) {
  auto agent = std::make_unique<devtool::MockBaseAgent>();
  mock_devtool_->RegisterAgent("MockAgent", std::move(agent));
  devtool::MockReceiver::GetInstance().ResetAll();

  // A valid JSON root that is not an object (here an array). JsonCpp asserts on
  // isMember()/operator[] for non-object, non-null values, so without the
  // isObject() guard this would abort before reaching envelope validation.
  const std::string message = R"([1, 2, 3])";
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", message);

  // It must produce an InvalidRequest (-32600) error response with a null id
  // instead of crashing.
  const std::string& response =
      devtool::MockReceiver::GetInstance().received_message_.second;
  EXPECT_NE(response.find("error"), std::string::npos);
  EXPECT_NE(response.find("-32600"), std::string::npos);
  EXPECT_NE(response.find("null"), std::string::npos);
  EXPECT_NE(response.find("Message must be a JSON object"), std::string::npos);
}

TEST_F(BaseDevToolTest, CDPResponderSendsSuccess) {
  auto sender = std::make_shared<CapturingMessageSender>();
  {
    auto responder = std::make_shared<devtool::CDPResponder>(sender, 7);
    Json::Value result(Json::objectValue);
    result["value"] = "ok";
    responder->SendSuccess(std::move(result));
  }

  ASSERT_EQ(sender->Messages().size(), 1u);
  EXPECT_EQ(sender->Messages()[0].first, "CDP");
  EXPECT_EQ(sender->Messages()[0].second["id"].asInt64(), 7);
  EXPECT_EQ(sender->Messages()[0].second["result"]["value"].asString(), "ok");
}

TEST_F(BaseDevToolTest, CDPResponderDestructorSendsFallbackSuccess) {
  auto sender = std::make_shared<CapturingMessageSender>();
  { auto responder = std::make_shared<devtool::CDPResponder>(sender, 8); }

  ASSERT_EQ(sender->Messages().size(), 1u);
  EXPECT_EQ(sender->Messages()[0].second["id"].asInt64(), 8);
  EXPECT_TRUE(sender->Messages()[0].second["result"].isObject());
}

TEST_F(BaseDevToolTest, CDPResponderSendsExplicitError) {
  auto sender = std::make_shared<CapturingMessageSender>();
  auto responder = std::make_shared<devtool::CDPResponder>(sender, 9);
  responder->SendError(devtool::CDPErrorCode::InvalidParams,
                       "Expected object params");

  ASSERT_EQ(sender->Messages().size(), 1u);
  const Json::Value& response = sender->Messages()[0].second;
  EXPECT_EQ(response["id"].asInt64(), 9);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(devtool::CDPErrorCode::InvalidParams));
  EXPECT_EQ(response["error"]["message"].asString(), "Expected object params");
}

TEST_F(BaseDevToolTest, CDPResponderUsesErrorTypeForEmptyMessage) {
  auto sender = std::make_shared<CapturingMessageSender>();
  auto responder = std::make_shared<devtool::CDPResponder>(sender, 10);
  responder->SendError(devtool::CDPErrorCode::InternalError);

  ASSERT_EQ(sender->Messages().size(), 1u);
  EXPECT_EQ(sender->Messages()[0].second["error"]["message"].asString(),
            "Internal error");
}

TEST_F(BaseDevToolTest, CDPResponderSendsAtMostOnce) {
  auto sender = std::make_shared<CapturingMessageSender>();
  {
    auto responder = std::make_shared<devtool::CDPResponder>(sender, 11);
    responder->SendSuccess();
    responder->SendError(devtool::CDPErrorCode::ServerError,
                         "Must not be sent");
  }

  ASSERT_EQ(sender->Messages().size(), 1u);
  EXPECT_TRUE(sender->Messages()[0].second.isMember("result"));
  EXPECT_FALSE(sender->Messages()[0].second.isMember("error"));
}

TEST_F(BaseDevToolTest, CDPResponderRetrieveSenderSuppressesFallback) {
  auto sender = std::make_shared<CapturingMessageSender>();
  std::shared_ptr<devtool::MessageSender> retrieved;
  {
    auto responder = std::make_shared<devtool::CDPResponder>(sender, 12);
    retrieved = responder->RetrieveSender();
  }

  EXPECT_EQ(retrieved, sender);
  EXPECT_TRUE(sender->Messages().empty());
}

TEST_F(BaseDevToolTest, DispatchCDPMessageRejectsMissingId) {
  devtool::MockReceiver::GetInstance().ResetAll();
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", R"({"method":"MockAgent.test"})");

  const Json::Value response = ParseLastResponse();
  EXPECT_TRUE(response["id"].isNull());
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(devtool::CDPErrorCode::InvalidRequest));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Message must have integer 'id' property");
  EXPECT_EQ(response["error"]["message"].asString(),
            "Message must have integer 'id' property");
}

TEST_F(BaseDevToolTest, DispatchCDPMessageRejectsNonIntegerId) {
  devtool::MockReceiver::GetInstance().ResetAll();
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP",
                                 R"({"id":"1","method":"MockAgent.test"})");

  const Json::Value response = ParseLastResponse();
  EXPECT_TRUE(response["id"].isNull());
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(devtool::CDPErrorCode::InvalidRequest));
}

TEST_F(BaseDevToolTest, DispatchCDPMessageRejectsMissingMethod) {
  devtool::MockReceiver::GetInstance().ResetAll();
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", R"({"id":13})");

  const Json::Value response = ParseLastResponse();
  EXPECT_EQ(response["id"].asInt64(), 13);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(devtool::CDPErrorCode::InvalidRequest));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Message must have string 'method' property");
}

TEST_F(BaseDevToolTest, DispatchCDPMessageRejectsNonStringMethod) {
  devtool::MockReceiver::GetInstance().ResetAll();
  mock_devtool_->DispatchMessage(std::make_shared<devtool::MessageSenderMock>(),
                                 "CDP", R"({"id":14,"method":42})");

  const Json::Value response = ParseLastResponse();
  EXPECT_EQ(response["id"].asInt64(), 14);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(devtool::CDPErrorCode::InvalidRequest));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Message must have string 'method' property");
}

TEST_F(BaseDevToolTest, DispatchCDPMessageRejectsNonObjectParams) {
  devtool::MockReceiver::GetInstance().ResetAll();
  mock_devtool_->DispatchMessage(
      std::make_shared<devtool::MessageSenderMock>(), "CDP",
      R"({"id":15,"method":"MockAgent.test","params":[]})");

  const Json::Value response = ParseLastResponse();
  EXPECT_EQ(response["id"].asInt64(), 15);
  EXPECT_EQ(response["error"]["code"].asInt(),
            static_cast<int>(devtool::CDPErrorCode::InvalidRequest));
  EXPECT_EQ(response["error"]["message"].asString(),
            "Params must be an object or null");
}

TEST_F(BaseDevToolTest, BaseDevToolRegisterMultipleThread) {
  // This test case only verifies if multiple threads can register and
  // dispatch messages simultaneously without crashing
  for (int i = 0; i < 1000; ++i) {
    devtool::MockReceiver::GetInstance().ResetAll();
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
    int thread_ready = 0;

    // Thread 1: register agent
    std::thread agent_thread([&]() {
      {
        std::unique_lock<std::mutex> lock(mutex);
        thread_ready++;
        cv.notify_all();  // Wake up all waiting threads
        std::cout << "agent_thread check ready:" << i
                  << std::endl;  // Debugging print
        cv.wait(lock, [&] { return ready; });
      }
      std::cout << "agent_thread register agent:" << i
                << std::endl;  // Debugging print
      auto agent = std::make_unique<devtool::MockBaseAgent>();
      mock_devtool_->RegisterAgent("MockAgent", std::move(agent));
    });

    // Thread 2: register handler
    std::thread handler_thread([&]() {
      {
        std::unique_lock<std::mutex> lock(mutex);
        thread_ready++;
        cv.notify_all();  // Wake up all waiting threads
        std::cout << "handler_thread check ready:" << i
                  << std::endl;  // Debugging print
        cv.wait(lock, [&] { return ready; });
      }
      std::cout << "handler_thread register handler:" << i
                << std::endl;  // Debugging print
      auto handler = std::make_unique<devtool::MockMessageHandler>();
      mock_devtool_->RegisterMessageHandler("TestHandler", std::move(handler));
    });

    // Thread 3: dispatch message
    std::thread dispatch_thread([&]() {
      {
        std::unique_lock<std::mutex> lock(mutex);
        thread_ready++;
        cv.notify_all();  // Wake up all waiting threads
        std::cout << "dispatch_thread check ready:" << i
                  << std::endl;  // Debugging print
        cv.wait(lock, [&] { return ready; });
      }
      std::cout << "dispatch_thread dispatch message:" << i
                << std::endl;  // Debugging print
      const std::string cdpMsg = R"({
      "id": 1,
      "method": "MockAgent.test"
    })";
      mock_devtool_->DispatchMessage(
          std::make_shared<devtool::MessageSenderMock>(), "CDP", cdpMsg);

      // Dispatch message to handler
      const std::string handlerMsg = R"({
          "id": 1,
          "params": "I am a test message"
    })";
      mock_devtool_->DispatchMessage(
          std::make_shared<devtool::MessageSenderMock>(), "TestHandler",
          handlerMsg);
    });

    // Wait for all threads to be ready
    {
      std::unique_lock<std::mutex> lock(mutex);
      std::cout << "Main thread check ready:" << std::endl;  // Debugging print
      cv.wait(lock, [&] { return thread_ready == 3; });
      std::cout
          << "starting all threads (agent, handler, dispatch) - iteration: "
          << i << std::endl;
      ready = true;
      cv.notify_all();  // Wake up all waiting threads
    }
    // Wait for all threads to complete
    agent_thread.join();
    handler_thread.join();
    dispatch_thread.join();
    std::cout << "all threads completed: iteration " << i << std::endl;
  }
}

TEST_F(BaseDevToolTest, BaseDevToolStatusCheck) {
  std::thread t([=]() {
    devtool::DevToolStatus::GetInstance().SetStatus(
        devtool::DevToolStatus::DevToolStatusKey::kDevToolStatusKeyIsConnected,
        "connect");
  });
  t.join();
  auto result = devtool::DevToolStatus::GetInstance().GetStatus(
      devtool::DevToolStatus::DevToolStatusKey::kDevToolStatusKeyIsConnected,
      "default");
  EXPECT_EQ(result, "connect");
}

TEST_F(BaseDevToolTest, BaseDevToolCompress) {
  auto agent = std::make_unique<devtool::MockBaseAgent>();
  const std::string jsonResponse = R"(
{
  "id": 1,
  "result": {
    "root": {
      "nodeId": 1,
      "backendNodeId": 2,
      "nodeType": 9,
      "nodeName": "#document",
      "localName": "",
      "nodeValue": "",
      "childNodeCount": 2,
      "children": [
        {
          "nodeId": 3,
          "parentId": 1,
          "backendNodeId": 4,
          "nodeType": 10,
          "nodeName": "html",
          "localName": "",
          "nodeValue": "",
          "publicId": "",
          "systemId": ""
        },
        {
          "nodeId": 5,
          "parentId": 1,
          "backendNodeId": 6,
          "nodeType": 1,
          "nodeName": "HTML",
          "localName": "html",
          "nodeValue": "",
          "childNodeCount": 2,
          "attributes": [],
          "children": [
            {
              "nodeId": 7,
              "parentId": 5,
              "backendNodeId": 8,
              "nodeType": 1,
              "nodeName": "HEAD",
              "localName": "head",
              "nodeValue": "",
              "childNodeCount": 2,
              "attributes": []
            },
            {
              "nodeId": 9,
              "parentId": 5,
              "backendNodeId": 10,
              "nodeType": 1,
              "nodeName": "BODY",
              "localName": "body",
              "nodeValue": "",
              "childNodeCount": 0,
              "attributes": []
            }
          ]
        }
      ]
    }
  }
}
)";

  Json::Value result(Json::ValueType::objectValue);
  agent->CompressData("", jsonResponse, result, "test");
  EXPECT_NE(result.get("test", ""), jsonResponse);
  EXPECT_TRUE(result.get("compress", false) == true);
}

TEST_F(BaseDevToolTest, GlobalMessageDispatcherGetSender) {
  auto global_dispatcher = devtool::GlobalMessageDispatcher::Create();
  ASSERT_NE(global_dispatcher, nullptr);

  auto sender = global_dispatcher->GetSender();
  ASSERT_NE(sender, nullptr);

  Json::Value test_message;
  test_message["id"] = 1;
  test_message["method"] = "Test.method";
  test_message["params"] = Json::Value(Json::ValueType::objectValue);

  EXPECT_NO_THROW(sender->SendMessage("CDP", test_message));

  std::string test_string_msg = R"({"id": 2, "method": "Test.stringMethod"})";
  EXPECT_NO_THROW(sender->SendMessage("CDP", test_string_msg));

  EXPECT_NO_THROW(sender->SendOKResponse(123));

  EXPECT_NO_THROW(sender->SendErrorResponse(456, "Test error message"));

  auto sender2 = global_dispatcher->GetSender();
  ASSERT_NE(sender2, nullptr);
  EXPECT_EQ(sender.get(), sender2.get());
}

TEST_F(BaseDevToolTest, DevToolMessageDispatcherUnregisterMessageHandler) {
  TestDevToolMessageDispatcher dispatcher;

  devtool::MockReceiver::GetInstance().ResetAll();
  auto handler = std::make_unique<devtool::MockMessageHandler>();
  dispatcher.RegisterMessageHandler("TestHandler", std::move(handler));

  const std::string handler_msg = R"({
      "id": 1,
      "params": "I am a test message"
    })";
  dispatcher.DispatchMessage(dispatcher.GetSender(), "TestHandler",
                             handler_msg);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.first,
            "TestHandler");
  EXPECT_TRUE(
      devtool::MockReceiver::GetInstance().received_message_.second.find(
          "MockMessageHandler") != std::string::npos);

  devtool::MockReceiver::GetInstance().ResetAll();
  dispatcher.UnregisterMessageHandler("TestHandler");
  EXPECT_EQ(dispatcher.handler_map_.find("TestHandler"),
            dispatcher.handler_map_.end());

  dispatcher.DispatchMessage(dispatcher.GetSender(), "TestHandler",
                             handler_msg);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.first, "");
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second, "");
}

// TODO(YUCHI): Add testcases for global handler and global slot
// The ut for js inspect are missing.

}  // namespace testing
}  // namespace lynx
