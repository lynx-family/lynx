// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <sys/wait.h>

#include "third_party/jsoncpp/include/json/value.h"
#define private public
#define protected public

#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/renderer/dom/fiber/block_element.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "core/services/replay/replay_controller.h"
#include "core/services/replay/testbench_test_replay.h"
#include "devtool/base_devtool/native/test/message_sender_mock.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/testing/mock/cdp_event_listener_sender_mock.h"
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
    devtools_ng_->devtool_mediator_ = devtool_mediator_;
    devtool_mediator_->devtool_wp_ = devtools_ng_;
    ui_thread_ = std::make_unique<fml::Thread>("ui");
    tasm_thread_ = std::make_unique<fml::Thread>("tasm");
    devtool_thread_ = std::make_unique<fml::Thread>("devtools");
    cdp_event_listener_thread_ =
        std::make_unique<fml::Thread>("cdp_event_listener");
    devtool_mediator_->tasm_task_runner_ = tasm_thread_->GetTaskRunner();
    devtool_mediator_->ui_task_runner_ = ui_thread_->GetTaskRunner();
    devtool_mediator_->cdp_event_listener_runner_ =
        cdp_event_listener_thread_->GetTaskRunner();
    devtool::LynxGlobalDevToolMediator::GetInstance().ui_task_runner_ =
        ui_thread_->GetTaskRunner();
    devtool_mediator_->devtool_executor_ =
        std::make_shared<devtool::InspectorDefaultExecutor>(devtool_mediator_);
    devtool_mediator_->ui_executor_ =
        std::make_shared<devtool::InspectorUIExecutor>(devtool_mediator_);
    devtool_mediator_->element_executor_ =
        std::make_shared<devtool::InspectorTasmExecutor>(devtool_mediator_, 1);
    facade_ = std::make_shared<testing::DevToolPlatformFacadeMock>();
    devtool_mediator_->devtool_executor_->SetDevToolPlatformFacade(facade_);
    devtool_mediator_->ui_executor_->SetDevToolPlatformFacade(facade_);
    devtool_mediator_->default_task_runner_ = devtool_thread_->GetTaskRunner();
  }

  // Drain all tasks queued on the tasm runner without terminating its message
  // loop (unlike tasm_thread_->Join(), which can only be called once).
  void FlushTasmTasks() {
    std::promise<void> p;
    auto f = p.get_future();
    devtool_mediator_->tasm_task_runner_->PostTask([&p]() { p.set_value(); });
    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
  }

  void FlushDevToolTasks() {
    std::promise<void> p;
    auto f = p.get_future();
    devtool_mediator_->default_task_runner_->PostTask(
        [&p]() { p.set_value(); });
    ASSERT_EQ(f.wait_for(std::chrono::seconds(5)), std::future_status::ready);
  }

 private:
  std::shared_ptr<devtool::LynxDevToolMediator> devtool_mediator_;
  std::shared_ptr<devtool::MessageSender> message_sender_;
  std::shared_ptr<testing::DevToolPlatformFacadeMock> facade_;
  std::shared_ptr<testing::LynxDevToolNGMock> devtools_ng_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::unique_ptr<fml::Thread> tasm_thread_;
  std::unique_ptr<fml::Thread> devtool_thread_;
  std::unique_ptr<fml::Thread> cdp_event_listener_thread_;
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

TEST_F(DevToolMediatorTest, GetBoxModelsRunsOnUIExecutorCase) {
  facade_->box_model_response_ = std::vector<double>(34, 12);

  std::vector<devtool::InspectorBoxModelQuery> queries(2);
  queries[0].layout_object.id = 101;
  queries[0].transform_node.id = 201;
  queries[1].layout_object.id = 102;
  queries[1].transform_node.id = 202;

  std::promise<std::vector<std::vector<double>>> promise;
  auto future = promise.get_future();
  devtool_mediator_->GetBoxModels(
      queries, [&promise](std::vector<std::vector<double>> box_models) {
        promise.set_value(std::move(box_models));
      });

  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  std::vector<std::vector<double>> box_models = future.get();
  ASSERT_EQ(box_models.size(), 2U);
  EXPECT_EQ(box_models[0], facade_->box_model_response_);
  EXPECT_EQ(box_models[1], facade_->box_model_response_);
  ASSERT_EQ(facade_->box_model_queries_.size(), 2U);
  EXPECT_EQ(facade_->box_model_queries_[0].layout_object.id, 101);
  EXPECT_EQ(facade_->box_model_queries_[1].transform_node.id, 202);
}

TEST_F(DevToolMediatorTest, GetBoxModelRunsOnUIExecutorCase) {
  facade_->box_model_response_ = std::vector<double>(34, 9);

  devtool::InspectorBoxModelQuery query;
  query.layout_object.id = 301;
  query.transform_node.id = 401;

  std::promise<std::vector<double>> promise;
  auto future = promise.get_future();
  devtool_mediator_->GetBoxModel(query,
                                 [&promise](std::vector<double> box_model) {
                                   promise.set_value(std::move(box_model));
                                 });

  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_EQ(future.get(), facade_->box_model_response_);
  ASSERT_EQ(facade_->box_model_queries_.size(), 1U);
  EXPECT_EQ(facade_->box_model_queries_[0].layout_object.id, 301);
}

TEST_F(DevToolMediatorTest, GetBoxModelRunsOnTasmThreadCase) {
  facade_->box_model_response_ = std::vector<double>(34, 9);

  devtool::InspectorBoxModelQuery query;
  query.layout_object.id = 303;
  query.transform_node.id = 403;

  auto promise = std::make_shared<std::promise<bool>>();
  auto future = promise->get_future();
  auto tasm_runner = devtool_mediator_->tasm_task_runner_;
  devtool_mediator_->GetBoxModel(
      query, [tasm_runner, promise](std::vector<double>) {
        promise->set_value(tasm_runner->RunsTasksOnCurrentThread());
      });

  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());
}

TEST_F(DevToolMediatorTest, GetBoxModelsRunsOnTasmThreadCase) {
  facade_->box_model_response_ = std::vector<double>(34, 12);

  std::vector<devtool::InspectorBoxModelQuery> queries(2);
  queries[0].layout_object.id = 101;
  queries[0].transform_node.id = 201;
  queries[1].layout_object.id = 102;
  queries[1].transform_node.id = 202;

  auto promise = std::make_shared<std::promise<bool>>();
  auto future = promise->get_future();
  auto tasm_runner = devtool_mediator_->tasm_task_runner_;
  devtool_mediator_->GetBoxModels(
      queries, [tasm_runner, promise](std::vector<std::vector<double>>) {
        promise->set_value(tasm_runner->RunsTasksOnCurrentThread());
      });

  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());
}

TEST_F(DevToolMediatorTest, GetLayoutTreeRunsOnTasmThreadCase) {
  auto promise = std::make_shared<std::promise<bool>>();
  auto future = promise->get_future();
  auto tasm_runner = devtool_mediator_->tasm_task_runner_;
  devtool_mediator_->GetLayoutTree(1, [tasm_runner, promise](std::string) {
    promise->set_value(tasm_runner->RunsTasksOnCurrentThread());
  });

  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());
}

TEST_F(DevToolMediatorTest, GetBoxModelHandlesMissingUIStateCase) {
  devtool::InspectorBoxModelQuery query;
  std::promise<bool> promise;
  auto ui_task_runner = devtool_mediator_->ui_task_runner_;
  auto ui_executor = devtool_mediator_->ui_executor_;

  auto future = promise.get_future();

  devtool_mediator_->ui_task_runner_ = nullptr;
  devtool_mediator_->GetBoxModel(
      query, [&promise](std::vector<double> result) mutable {
        EXPECT_TRUE(result.empty());
        promise.set_value(true);
      });
  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());

  promise = {};
  devtool_mediator_->ui_task_runner_ = ui_task_runner;
  devtool_mediator_->ui_executor_ = nullptr;
  future = promise.get_future();
  devtool_mediator_->GetBoxModel(
      query, [&promise](std::vector<double> result) mutable {
        EXPECT_TRUE(result.empty());
        promise.set_value(true);
      });
  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());
  devtool_mediator_->ui_executor_ = ui_executor;
}

TEST_F(DevToolMediatorTest, GetLayoutTreeHandlesMissingUIStateCase) {
  std::promise<bool> promise;
  auto ui_task_runner = devtool_mediator_->ui_task_runner_;
  auto ui_executor = devtool_mediator_->ui_executor_;

  auto future = promise.get_future();

  devtool_mediator_->ui_task_runner_ = nullptr;
  devtool_mediator_->GetLayoutTree(1, [&promise](std::string layout_tree) {
    EXPECT_TRUE(layout_tree.empty());
    promise.set_value(true);
  });
  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());

  promise = {};
  devtool_mediator_->ui_task_runner_ = ui_task_runner;
  devtool_mediator_->ui_executor_ = nullptr;
  future = promise.get_future();
  devtool_mediator_->GetLayoutTree(1, [&promise](std::string layout_tree) {
    EXPECT_TRUE(layout_tree.empty());
    promise.set_value(true);
  });
  ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(future.get());
  devtool_mediator_->ui_executor_ = ui_executor;
}

TEST_F(DevToolMediatorTest, BoxModelDomCommandsRunOnTASMThreadCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 51;
  message["params"]["nodeId"] = 0;
  devtool_mediator_->GetDocumentWithBoxModel(message_sender_, message);
  devtool_mediator_->DOM_GetBoxModel(message_sender_, message);
  devtool_mediator_->GetComputedStyleForNode(message_sender_, message);
  tasm_thread_->Join();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 51);
}

TEST_F(DevToolMediatorTest, LayerTreeCommandsRunOnTASMThreadCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 52;
  Json::Value compositing_message(Json::ValueType::objectValue);
  compositing_message["id"] = 53;
  compositing_message["params"]["layerId"] = "0";

  devtool_mediator_->LayerTreeEnable(message_sender_, message);
  devtool_mediator_->LayerTreeDisable(message_sender_, message);
  devtool_mediator_->SendLayerTreeDidChangeEvent();
  devtool_mediator_->CompositingReasons(message_sender_, compositing_message);
  tasm_thread_->Join();

  EXPECT_FALSE(devtool_mediator_->element_executor_->layer_tree_enabled_);
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
  request["id"] = 123;

  devtool_mediator_->LynxSetTraceMode(message_sender_, request);
  devtool_thread_->Join();

  // The new behavior is to log a warning and return an error object
  // to explicitly tell the client this method is dead.
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"error\" : {\n      \"code\" : -32601,\n      \"message\" "
            ": \"SetTraceMode is deprecated. Use global messages.\"\n   },\n   "
            "\"id\" : 123\n}\n");
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
  lynx::runtime::js::ConsoleMessage param("test", 2, 0);
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

TEST_F(DevToolMediatorTest, GetAllPerformanceEntriesTest) {
  Json::Value param;
  param["id"] = 1;
  devtool_mediator_->getAllPerformanceEntries(message_sender_, param);
  ui_thread_->Join();
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"id\" : 1\n}\n");
}

TEST_F(DevToolMediatorTest, AddCDPEventListener) {
  auto listener_sender =
      std::make_shared<devtool::CDPEventListenerSenderMock>();
  devtools_ng_->AddCDPEventListener("test", listener_sender);
  EXPECT_TRUE(devtool_mediator_->cdp_event_listener_map_.find("test") !=
              devtool_mediator_->cdp_event_listener_map_.end());
  EXPECT_EQ(
      listener_sender.get(),
      devtool_mediator_->cdp_event_listener_map_.find("test")->second.get());
}

TEST_F(DevToolMediatorTest, RemoveCDPEventListener) {
  auto listener_sender =
      std::make_shared<devtool::CDPEventListenerSenderMock>();
  devtools_ng_->AddCDPEventListener("test", listener_sender);
  EXPECT_TRUE(devtool_mediator_->cdp_event_listener_map_.find("test") !=
              devtool_mediator_->cdp_event_listener_map_.end());
  devtools_ng_->RemoveCDPEventListener("test");
  EXPECT_TRUE(devtool_mediator_->cdp_event_listener_map_.find("test") ==
              devtool_mediator_->cdp_event_listener_map_.end());
}

TEST_F(DevToolMediatorTest, SendCDPEventJson) {
  auto listener_sender =
      std::make_shared<devtool::CDPEventListenerSenderMock>();
  devtools_ng_->AddCDPEventListener("test", listener_sender);

  Json::Value msg_json(Json::ValueType::objectValue);
  msg_json["method"] = "DOM.documentUpdated";
  devtool_mediator_->SendCDPEvent(msg_json);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"method\" : \"DOM.documentUpdated\"\n}\n");

  cdp_event_listener_thread_->Join();
  EXPECT_EQ(listener_sender->received_msg_.second,
            "{\n   \"method\" : \"DOM.documentUpdated\"\n}\n");
}

TEST_F(DevToolMediatorTest, SendCDPEventString) {
  auto listener_sender =
      std::make_shared<devtool::CDPEventListenerSenderMock>();
  devtools_ng_->AddCDPEventListener("test", listener_sender);

  Json::Value msg_json(Json::ValueType::objectValue);
  msg_json["method"] = "DOM.documentUpdated";
  std::string msg_str = msg_json.toStyledString();
  devtool_mediator_->SendCDPEvent(msg_str);
  EXPECT_EQ(devtool::MockReceiver::GetInstance().received_message_.second,
            "{\n   \"method\" : \"DOM.documentUpdated\"\n}\n");

  cdp_event_listener_thread_->Join();
  EXPECT_EQ(listener_sender->received_msg_.second,
            "{\n   \"method\" : \"DOM.documentUpdated\"\n}\n");
}

TEST_F(DevToolMediatorTest, NetworkCaptureEnqueuedAfterDisableIsDropped) {
  auto observer = devtool_mediator_->devtool_executor_->network_observer_;
  devtool::MockReceiver::GetInstance().ResetAll();

  Json::Value enable_message;
  enable_message["id"] = 1;
  devtool_mediator_->NetworkEnable(message_sender_, enable_message);
  FlushDevToolTasks();
  ASSERT_TRUE(observer->IsEnabled());

  // Block the DevTool runner so the queue order below is fixed.
  auto blocker_started = std::make_shared<std::promise<void>>();
  auto blocker_started_future = blocker_started->get_future();
  auto release_blocker = std::make_shared<std::promise<void>>();
  auto release_blocker_future = release_blocker->get_future().share();
  devtool_mediator_->default_task_runner_->PostTask(
      [blocker_started, release_blocker_future]() {
        blocker_started->set_value();
        release_blocker_future.wait();
      });
  ASSERT_EQ(blocker_started_future.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);

  // The producer side may still pass the enabled fast-path while a disable is
  // racing: the capture is enqueued behind the disable command and must be
  // dropped by the execution-time enabled check.
  Json::Value disable_message;
  disable_message["id"] = 2;
  devtool_mediator_->NetworkDisable(message_sender_, disable_message);

  devtool::NetworkRequestInfo request;
  request.url = "https://example.com/network-after-disable";
  request.method = "GET";
  ASSERT_FALSE(observer->RequestWillBeSent(request).empty());

  release_blocker->set_value();
  FlushDevToolTasks();

  EXPECT_FALSE(observer->IsEnabled());
  // The disable OK response is the last message; the capture was dropped.
  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 2);
  EXPECT_TRUE(res["result"].isObject());
}

TEST_F(DevToolMediatorTest, SetTag) {
  std::string test_tag = "test";
  // Test when js_debugger_ is nullptr
  devtool_mediator_->SetTag(test_tag);

  // Test when js_debugger_ is not nullptr
  auto js_debugger = std::make_shared<devtool::InspectorJavaScriptDebuggerImpl>(
      devtool_mediator_, devtool_mediator_->view_id_);
  devtool_mediator_->js_debugger_ = js_debugger;
  devtool_mediator_->SetTag(test_tag);
  EXPECT_EQ(js_debugger->GetInspectorRuntimeObserver()->GetTag(), test_tag);
}

TEST_F(DevToolMediatorTest, GlobalPropsCommandsRunOnTasmThreadCase) {
  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 1;

  devtool_mediator_->GlobalPropsEnable(message_sender_, message);
  FlushTasmTasks();
  EXPECT_TRUE(devtool_mediator_->element_executor_->IsGlobalPropsEnabled());
  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 1);
  EXPECT_TRUE(res["result"].isObject());

  devtool_mediator_->GlobalPropsDisable(message_sender_, message);
  FlushTasmTasks();
  EXPECT_FALSE(devtool_mediator_->element_executor_->IsGlobalPropsEnabled());

  message["id"] = 2;
  devtool_mediator_->GlobalPropsGet(message_sender_, message);
  FlushTasmTasks();
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 2);
  // Fixture constructs the executor without a tasm, so global props is empty.
  EXPECT_TRUE(res["result"]["globalProps"].isObject());
  EXPECT_TRUE(res["result"]["globalProps"].empty());
  EXPECT_EQ(res["result"]["timestamp"].asUInt64(), 0u);

  Json::Value replace_message(Json::ValueType::objectValue);
  replace_message["id"] = 3;
  replace_message["params"]["globalProps"]["key"] = "value";
  devtool_mediator_->GlobalPropsReplace(message_sender_, replace_message);
  FlushTasmTasks();
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["id"], 3);
  // Fixture's executor has no tasm, so replace reports the unavailable error.
  EXPECT_EQ(res["error"]["code"].asInt(), devtool::kServerError);
  EXPECT_EQ(res["error"]["message"], "GlobalProps.replace is unavailable");
}

TEST_F(DevToolMediatorTest, GlobalPropsCommandsWhenTasmRunnerUnavailableCase) {
  auto runner = devtool_mediator_->tasm_task_runner_;
  devtool_mediator_->tasm_task_runner_ = nullptr;

  Json::Value message(Json::ValueType::objectValue);
  message["id"] = 7;
  std::vector<std::function<void()>> commands = {
      [&] { devtool_mediator_->GlobalPropsEnable(message_sender_, message); },
      [&] { devtool_mediator_->GlobalPropsDisable(message_sender_, message); },
      [&] { devtool_mediator_->GlobalPropsGet(message_sender_, message); },
      [&] { devtool_mediator_->GlobalPropsReplace(message_sender_, message); },
  };

  for (auto& command : commands) {
    command();
    Json::Value res;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(
        devtool::MockReceiver::GetInstance().received_message_.second, res));
    EXPECT_EQ(res["error"]["code"].asInt(), devtool::kServerError);
    EXPECT_EQ(res["error"]["message"], "GlobalProps target is unavailable");
    EXPECT_EQ(res["id"], 7);
    devtool::MockReceiver::GetInstance().ResetAll();
  }

  devtool_mediator_->tasm_task_runner_ = runner;
}

TEST_F(DevToolMediatorTest, GlobalPropsChangedUpdatesTimestampCase) {
  // The timestamp advances even when the domain is disabled.
  devtool_mediator_->GlobalPropsChanged();
  FlushTasmTasks();
  EXPECT_GT(devtool_mediator_->element_executor_->GetLastGlobalPropsTimestamp(),
            0u);

  // When enabled, a GlobalProps.changed event is emitted.
  devtool::MockReceiver::GetInstance().ResetAll();
  devtool_mediator_->element_executor_->global_props_enabled_ = true;
  devtool_mediator_->GlobalPropsChanged();
  FlushTasmTasks();

  Json::Value res;
  Json::Reader reader;
  ASSERT_TRUE(reader.parse(
      devtool::MockReceiver::GetInstance().received_message_.second, res));
  EXPECT_EQ(res["method"], "GlobalProps.changed");
  EXPECT_GT(res["params"]["timestamp"].asUInt64(), 0u);
  EXPECT_EQ(res["params"]["changes"].size(), 1u);
  EXPECT_EQ(res["params"]["changes"][0]["operation"], "replace");
}

}  // namespace testing
}  // namespace lynx
