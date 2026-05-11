// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"

#include <sys/wait.h>

#include <cstddef>
#include <memory>

#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/value.h"

namespace lynx {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class InspectorUIExecutorTest : public ::testing::Test {
 public:
  InspectorUIExecutorTest() = default;
  ~InspectorUIExecutorTest() override {}

  void SetUp() override {
    lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    devtool::MockReceiver::GetInstance().ResetAll();
    devtool_mediator_ = std::make_shared<lynx::devtool::LynxDevToolMediator>();
    devtools_ng_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    message_sender_ = std::make_shared<devtool::MessageSenderMock>();
    devtools_ng_->message_sender_ = message_sender_;
    devtool_mediator_->devtool_wp_ = devtools_ng_;
    ui_executor_ =
        std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
  }

 private:
  std::shared_ptr<devtool::InspectorUIExecutor> ui_executor_;
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<fml::Thread> ui_thread_;
};

TEST_F(InspectorUIExecutorTest, PageReloadTest) {
  LOGI("InspectorUIExecutorTest PageReloadTest start");

  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  ui_executor_->SetDevToolPlatformFacade(facade);
  EXPECT_EQ(ui_executor_->devtool_platform_facade_.get(), facade.get());
  {
    // test empty value
    Json::Value message;
    message["id"] = 21;

    ui_executor_->PageReload(message_sender_, message);

    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 21,\n   \"result\" : {}\n}\n");
  }

  {
    // test partial value
    Json::Value message;
    message["id"] = 22;
    Json::Value params;
    params["ignoreCache"] = false;
    message["params"] = params;
    ui_executor_->PageReload(message_sender_, message);

    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 22,\n   \"result\" : {}\n}\n");
  }

  {
    // test normal value
    Json::Value message;
    message["id"] = 23;
    Json::Value params;
    params["ignoreCache"] = true;
    params["pageData"] = "test_template_binary_data";
    params["fromPageDataFragments"] = true;
    params["pageDataLength"] = 2048;
    params["url"] = "http://test.example.com/reload";
    message["params"] = params;
    ui_executor_->PageReload(message_sender_, message);

    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 23,\n   \"result\" : {}\n}\n");
  }
}

TEST_F(InspectorUIExecutorTest, StartScreencastTest) {
  LOGI("InspectorUIExecutorTest StartScreencastTest start");

  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade =
      std::make_shared<testing::DevToolPlatformFacadeMock>();
  ui_executor_->SetDevToolPlatformFacade(facade);
  EXPECT_EQ(ui_executor_->devtool_platform_facade_.get(), facade.get());

  {
    Json::Value message;
    message["id"] = 31;
    ui_executor_->StartScreencast(message_sender_, message);
    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 31,\n   \"result\" : {}\n}\n");
  }

  {
    Json::Value message;
    message["id"] = 32;
    Json::Value params;
    params["format"] = "jpeg";
    params["quality"] = 80;
    message["params"] = params;
    ui_executor_->StartScreencast(message_sender_, message);
    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 32,\n   \"result\" : {}\n}\n");
  }

  {
    Json::Value message;
    message["id"] = 33;
    Json::Value params;
    params["format"] = "png";
    params["quality"] = 100;
    params["maxWidth"] = 720;
    params["maxHeight"] = 1280;
    params["everyNthFrame"] = 2;
    params["mode"] = "lynxview";
    message["params"] = params;
    ui_executor_->StartScreencast(message_sender_, message);
    EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
              "{\n   \"id\" : 33,\n   \"result\" : {}\n}\n");
  }
}

}  // namespace testing
}  // namespace lynx
