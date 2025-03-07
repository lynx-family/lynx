// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public
#include <memory>
#include <thread>

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

// TODO(YUCHI): Add testcases for global handler and global slot
// The ut for js inspect are missing.

}  // namespace testing
}  // namespace lynx
