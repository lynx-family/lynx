// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <sys/wait.h>

#include "third_party/jsoncpp/include/json/value.h"
#define private public
#define protected public

#include <memory>
#include <thread>

#include "base/include/log/logging.h"
#include "core/renderer/dom/fiber/block_element.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/replay/replay_controller.h"
#include "core/services/replay/testbench_test_replay.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace testing {

// Notice: If you find some case is not stable, Please check that the thread is
// same as mediator using
class DevToolMediatorTest : public ::testing::Test {
 public:
  DevToolMediatorTest() = default;
  ~DevToolMediatorTest() override {}

  void SetUp() override {
    base::UIThread::Init();
    devtool::MockReceiver::GetInstance().ResetAll();
    devtool_mediator_ = std::make_shared<lynx::devtool::LynxDevToolMediator>();
    devtools_ng_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    message_sender_ = std::make_shared<devtool::MessageSenderMock>();
    devtools_ng_->message_sender_ = message_sender_;
    devtool_mediator_->devtool_wp_ = devtools_ng_;
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    tasm_thread_ = std::make_unique<fml::Thread>("tasm");
    devtool_thread_ = std::make_unique<fml::Thread>("devtools");
    devtool_mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    devtool::LynxGlobalDevToolMediator::GetInstance().ui_task_runner_ =
        ui_thread_->GetTaskRunner();
    devtool_mediator_->devtool_executor_ =
        std::make_shared<devtool::InspectorDefaultExecutor>(devtool_mediator_);
    devtool_mediator_->ui_executor_ =
        std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
    devtool_mediator_->element_executor_ =
        std::make_shared<devtool::InspectorTasmExecutor>(devtool_mediator_);
    facade_ = std::make_shared<testing::DevToolPlatformFacadeMock>();
    devtool_mediator_->devtool_executor_->SetDevToolPlatformFacade(facade_);
    devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade_);
    devtool_mediator_->default_task_runner_ = devtool_thread_->GetTaskRunner();
  }

 private:
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::unique_ptr<fml::Thread> devtool_thread_;
};

TEST_F(DevToolMediatorTest, InspectorEnableCase) {
  LOGI("InspectorEnableCase start");
  Json::Value param;
  devtool_mediator_->InspectorEnable(message_sender_, param);
  devtool_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, InspectorDetachedCase) {
  LOGI("InspectorDetachedCase start");
  Json::Value param;
  devtool_mediator_->InspectorDetached(message_sender_, param);
  devtool_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"method\" : \"Inspector.detached\",\n   \"params\" : {\n   "
            "   \"reason\" : \"\"\n   }\n}\n");
}

TEST_F(DevToolMediatorTest, RecordStartCase) {
  EXPECT_TRUE(lynx::tasm::recorder::RecorderController::Enable());
  Json::Value param;
  devtool::LynxGlobalDevToolMediator::GetInstance().RecordingStart(
      message_sender_, param);
  ui_thread_->Join();
  EXPECT_TRUE(lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
                  .IsRecordingProcess());
}

TEST_F(DevToolMediatorTest, DISABLED_RecordEndCase) {
  EXPECT_TRUE(lynx::tasm::recorder::RecorderController::Enable());
  Json::Value param;
  devtool::LynxGlobalDevToolMediator::GetInstance().RecordingEnd(
      message_sender_, param);
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().thread_.Join();
  EXPECT_FALSE(lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
                   .IsRecordingProcess());
}

TEST_F(DevToolMediatorTest, ReplayStartCase) {
  EXPECT_TRUE(lynx::tasm::replay::ReplayController::Enable());
  Json::Value param;
  devtool::LynxGlobalDevToolMediator::GetInstance().ReplayStart(message_sender_,
                                                                param);
  ui_thread_->Join();
  EXPECT_TRUE(lynx::tasm::replay::TestBenchTestReplay::GetInstance().IsStart());
}

TEST_F(DevToolMediatorTest, ReplayEndCase) {
  EXPECT_TRUE(lynx::tasm::replay::ReplayController::Enable());
  Json::Value param;
  devtool::LynxGlobalDevToolMediator::GetInstance().ReplayEnd(message_sender_,
                                                              param);
  ui_thread_->Join();
  // TODO: Here are some concerns: why wasn't the state set to false during the
  // specific implementation?
}

TEST_F(DevToolMediatorTest, IOReadCase) {
  Json::Value param;
  param["id"] = 1;
  param["params"]["handle"] = "1";
  param["params"]["size"] = 1024;
  devtool::LynxGlobalDevToolMediator::GetInstance().IORead(message_sender_,
                                                           param);
  devtool_thread_->Join();

  sleep(1);

  Json::Value res;
  Json::Reader reader;
  bool is_valid_json = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid_json);
  EXPECT_EQ(res["id"], 1);
  EXPECT_TRUE(res["result"]["base64Encoded"].asBool());
  EXPECT_TRUE(res["result"]["eof"].asBool());
}

TEST_F(DevToolMediatorTest, IOReadInvalidStreamHandleCase) {
  Json::Value param;
  param["id"] = 1;
  param["params"]["handle"] = "a";
  devtool::LynxGlobalDevToolMediator::GetInstance().IORead(message_sender_,
                                                           param);
  devtool_thread_->Join();

  sleep(1);

  Json::Value res;
  Json::Reader reader;
  bool is_valid_json = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid_json);
  EXPECT_EQ(res["error"]["code"].asInt(), devtool::kInspectorErrorCode);
  EXPECT_EQ(res["error"]["message"].asString(), "Get invalid stream handle");
  EXPECT_EQ(res["id"], 1);
}

TEST_F(DevToolMediatorTest, IOCloseCase) {
  Json::Value param;
  param["id"] = 1;
  param["params"]["handle"] = "1";
  param["params"]["size"] = 1024;
  devtool::LynxGlobalDevToolMediator::GetInstance().IOClose(message_sender_,
                                                            param);
  devtool_thread_->Join();

  sleep(1);

  Json::Value res;
  Json::Reader reader;
  bool is_valid_json = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid_json);
  EXPECT_EQ(res["id"], 1);
}

TEST_F(DevToolMediatorTest, IOCloseInvalidStreamHandleCase) {
  Json::Value param;
  param["id"] = 1;
  param["params"]["handle"] = "a";
  devtool::LynxGlobalDevToolMediator::GetInstance().IOClose(message_sender_,
                                                            param);
  devtool_thread_->Join();

  sleep(1);

  Json::Value res;
  Json::Reader reader;
  bool is_valid_json = reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res);
  EXPECT_TRUE(is_valid_json);
  EXPECT_EQ(res["error"]["code"].asInt(), devtool::kInspectorErrorCode);
  EXPECT_EQ(res["error"]["message"].asString(), "Get invalid stream handle");
  EXPECT_EQ(res["id"], 1);
}

TEST_F(DevToolMediatorTest, LogEnable) {
  Json::Value param;
  devtool_mediator_->LogEnable(message_sender_, param);
  devtool_thread_->Join();
  EXPECT_TRUE(
      devtool_mediator_->devtool_executor_->console_msg_manager_->enable_);
}
TEST_F(DevToolMediatorTest, LynxGetPropertyElementNull) {
  Json::Value param;
  devtool_mediator_->LynxGetProperties(message_sender_, param);
  tasm_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {\n      \"properties\" : "
            "\"\"\n   }\n}\n");
}

TEST_F(DevToolMediatorTest, LynxGetDataElementNull) {
  Json::Value param;
  devtool_mediator_->LynxGetData(message_sender_, param);
  tasm_thread_->Join();
  EXPECT_EQ(
      devtool::MockReceiver::GetInstance().received_message_.second,
      "{\n   \"id\" : 0,\n   \"result\" : {\n      \"data\" : \"\"\n   }\n}\n");
}

TEST_F(DevToolMediatorTest, LynxGetComponentIdElementNull) {
  Json::Value param;
  devtool_mediator_->LynxGetComponentId(message_sender_, param);
  tasm_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {\n      \"componentId\" : "
            "-1\n   }\n}\n");
}

TEST_F(DevToolMediatorTest, LynxSetTraceMode) {
  Json::Value request = Json::Value(Json::ValueType::objectValue);
  Json::Value param = Json::Value(Json::ValueType::objectValue);
  param["enableTraceMode"] = true;
  request["params"] = param;
  devtool_mediator_->LynxSetTraceMode(message_sender_, request);
  devtool_thread_->Join();
  EXPECT_TRUE(facade_->devtools_switch_.find("enable_dom_tree") !=
              facade_->devtools_switch_.end());
  EXPECT_TRUE(facade_->devtools_switch_.find("enable_preview_screen_shot") !=
              facade_->devtools_switch_.end());
  EXPECT_TRUE(facade_->devtools_switch_.find("enable_quickjs_debug") !=
              facade_->devtools_switch_.end());
  EXPECT_TRUE(facade_->devtools_switch_.find("enable_v8") !=
              facade_->devtools_switch_.end());
  EXPECT_FALSE(facade_->devtools_switch_["enable_dom_tree"]);
  EXPECT_FALSE(facade_->devtools_switch_["enable_preview_screen_shot"]);
  EXPECT_FALSE(facade_->devtools_switch_["enable_quickjs_debug"]);
  EXPECT_FALSE(facade_->devtools_switch_["enable_v8"]);
}

TEST_F(DevToolMediatorTest, GetLynxVersion) {
  Json::Value request = Json::Value(Json::ValueType::objectValue);
  devtool_mediator_->LynxGetVersion(message_sender_, request);
  devtool_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : \"1.1.5\"\n}\n");
}

TEST_F(DevToolMediatorTest, GetRectToWindow) {
  Json::Value request = Json::Value(Json::ValueType::objectValue);
  devtool_mediator_->LynxGetRectToWindow(message_sender_, request);
  ui_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {\n      \"height\" : 1.0,\n   "
            "   \"left\" : 1.0,\n      \"top\" : 1.0,\n      \"width\" : 1.0\n "
            "  }\n}\n");
}

TEST_F(DevToolMediatorTest, LynxTransferData) {
  Json::Value request = Json::Value(Json::ValueType::objectValue);
  devtool_mediator_->LynxTransferData(message_sender_, request);
  ui_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second, "");
}

TEST_F(DevToolMediatorTest, LynxGetViewLocationOnScreen) {
  Json::Value request = Json::Value(Json::ValueType::objectValue);
  devtool_mediator_->LynxGetViewLocationOnScreen(message_sender_, request);
  ui_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {\n      \"x\" : 1,\n      "
            "\"y\" : 1\n   }\n}\n");
}

TEST_F(DevToolMediatorTest, LynxSendEventToVM) {
  Json::Value request = Json::Value(Json::ValueType::objectValue);
  devtool_mediator_->LynxSendEventToVM(message_sender_, request);
  ui_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, LogDisable) {
  Json::Value param;
  devtool_mediator_->LogDisable(message_sender_, param);
  devtool_thread_->Join();
  EXPECT_FALSE(
      devtool_mediator_->devtool_executor_->console_msg_manager_->enable_);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, LogClear) {
  Json::Value param;
  devtool_mediator_->LogClear(message_sender_, param);
  devtool_thread_->Join();
  EXPECT_TRUE(devtool_mediator_->devtool_executor_->console_msg_manager_
                  ->log_messages_.empty());
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 0,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, LogEntryAdded) {
  lynx::piper::ConsoleMessage param("test", 2, 0);
  devtool_mediator_->LogClear(message_sender_, Json::Value());
  devtool_mediator_->LogEnable(message_sender_, Json::Value());
  devtool_mediator_->SendLogEntryAddedEvent(std::move(param));
  devtool_thread_->Join();
  EXPECT_FALSE(devtool_mediator_->devtool_executor_->console_msg_manager_
                   ->log_messages_.empty());
  EXPECT_TRUE(
      devtool::MockReceiver::GetInstance().received_message_.second.find(
          "Log.entryAdded") != std::string::npos);
}

TEST_F(DevToolMediatorTest, StartMemoryTracing) {
  Json::Value param;
  param["id"] = 1;
  lynx::devtool::LynxGlobalDevToolMediator::GetInstance().MemoryStartTracing(
      message_sender_, param);
  devtool_thread_->Join();
  sleep(1);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 1,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, StopMemoryTracing) {
  Json::Value param;
  param["id"] = 1;
  lynx::devtool::LynxGlobalDevToolMediator::GetInstance().MemoryStopTracing(
      message_sender_, param);
  devtool_thread_->Join();
  sleep(1);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 1,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, HighlightTest) {
  Json::Value param1(Json::objectValue);
  param1["id"] = 1;
  devtool_mediator_->HighlightNode(message_sender_, param1);
  tasm_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 1,\n   \"result\" : {}\n}\n");

  Json::Value param2;
  param2["id"] = 1;
  devtool_mediator_->HideHighlight(message_sender_, param2);
  tasm_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 1,\n   \"result\" : {}\n}\n");
}

TEST_F(DevToolMediatorTest, GetAllTimingInfoTest) {
  Json::Value param;
  param["id"] = 1;
  devtool_mediator_->getAllTimingInfo(message_sender_, param);
  ui_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 1\n}\n");
}

}  // namespace testing
}  // namespace lynx
