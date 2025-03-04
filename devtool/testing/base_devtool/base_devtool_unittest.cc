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
