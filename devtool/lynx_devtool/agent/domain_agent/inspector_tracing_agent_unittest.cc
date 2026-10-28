// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "third_party/jsoncpp/include/json/value.h"
#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_tracing_agent.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

#undef protected
#undef private

#include "base/include/fml/thread.h"
#include "base/trace/native/trace_controller.h"
#include "core/renderer/utils/lynx_env.h"
#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/base/file_stream.h"
#include "devtool/testing/mock/global_devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {
namespace {

constexpr int64_t kRequestId = 4294967297LL;

class TracingTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type,
                   const Json::Value& message) override {
    EXPECT_EQ(type, "CDP");
    StoreMessage(message);
  }

  void SendMessage(const std::string& type,
                   const std::string& message) override {
    EXPECT_EQ(type, "CDP");
    Json::Value parsed;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(message, parsed, false));
    StoreMessage(std::move(parsed));
  }

  void Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
  }

  Json::Value WaitForResponse() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!condition_.wait_for(lock, std::chrono::seconds(2),
                             [this] { return !messages_.empty(); })) {
      ADD_FAILURE() << "Missing Tracing response";
      return {};
    }
    EXPECT_EQ(messages_.size(), 1u);
    Json::Value response = messages_.front();
    messages_.clear();
    return response;
  }

 private:
  void StoreMessage(Json::Value message) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      messages_.push_back(std::move(message));
    }
    condition_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<Json::Value> messages_;
};

class FakeTraceController : public lynx::trace::TraceController {
 public:
  int StartTracing(
      const std::shared_ptr<lynx::trace::TraceConfig>& config) override {
    last_config = config;
    config->file_path = trace_file_path;
    return start_result;
  }

  bool StopTracing(int session_id) override {
    stopped_session_id = session_id;
    return true;
  }

  void AddCompleteCallback(int session_id,
                           const std::function<void()> callback) override {
    callback_session_id = session_id;
    complete_callback = callback;
  }

  void SetStartupTracingConfig(std::string config) override {
    startup_config = std::move(config);
  }

  std::string GetStartupTracingConfig() override { return startup_config; }

  std::string GetStartupTracingFilePath() override { return startup_file_path; }

  bool IsTracingStarted() override { return startup_tracing_started; }

  int start_result = 37;
  int stopped_session_id = -1;
  int callback_session_id = -1;
  bool startup_tracing_started = false;
  std::string startup_config;
  std::string startup_file_path;
  std::string trace_file_path;
  std::shared_ptr<lynx::trace::TraceConfig> last_config;
  std::function<void()> complete_callback;
};

Json::Value ParseMockReceiverMessage() {
  Json::Value message;
  Json::Reader reader;
  EXPECT_TRUE(reader.parse(MockReceiver::GetInstance().received_message_.second,
                           message, false));
  return message;
}

}  // namespace

class InspectorTracingAgentTest : public ::testing::Test {
 public:
  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    sender_ = std::make_shared<TracingTestMessageSender>();
    agent_ = std::make_unique<InspectorTracingAgent>();
    devtool_thread_ = std::make_unique<fml::Thread>("tracing-test");

    auto& mediator = LynxGlobalDevToolMediator::GetInstance();
    saved_task_runner_ = mediator.default_task_runner_;
    saved_session_id_ = mediator.tracing_session_id_;
    saved_notification_callback_ =
        std::move(mediator.tracing_notification_callback_);
    mediator.default_task_runner_ = devtool_thread_->GetTaskRunner();
    mediator.tracing_session_id_ = -1;

    saved_trace_controller_ = lynx::testing::GlobalDevToolPlatformFacadeMock::
        TraceControllerForTesting();
    lynx::testing::GlobalDevToolPlatformFacadeMock::
        TraceControllerForTesting() = &trace_controller_;
    temp_trace_file_ =
        (std::filesystem::temp_directory_path() /
         ("lynx_tracing_agent_test_" +
          std::to_string(reinterpret_cast<uintptr_t>(this)) + ".trace"))
            .string();
    std::ofstream trace_file(temp_trace_file_);
    ASSERT_TRUE(trace_file.is_open());
    trace_file << "trace";
    MockReceiver::GetInstance().ResetAll();
  }

  void TearDown() override {
    DrainTasks();
    auto& mediator = LynxGlobalDevToolMediator::GetInstance();
    mediator.default_task_runner_ = saved_task_runner_;
    mediator.tracing_session_id_ = saved_session_id_;
    mediator.tracing_notification_callback_ =
        std::move(saved_notification_callback_);
    lynx::testing::GlobalDevToolPlatformFacadeMock::
        TraceControllerForTesting() = saved_trace_controller_;
    std::error_code error;
    std::filesystem::remove(temp_trace_file_, error);
    MockReceiver::GetInstance().ResetAll();
  }

 protected:
  Json::Value Dispatch(const std::string& method,
                       const Json::Value& params = Json::Value()) {
    sender_->Reset();
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(kRequestId);
    message["method"] = method;
    if (!params.isNull()) {
      message["params"] = params;
    }
    agent_->CallMethod(std::make_shared<CDPResponder>(sender_, kRequestId),
                       message);
    Json::Value response = sender_->WaitForResponse();
    EXPECT_EQ(response["id"].asInt64(), kRequestId);
    return response;
  }

  void DrainTasks() {
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();
    devtool_thread_->GetTaskRunner()->PostTask([done] { done->set_value(); });
    EXPECT_EQ(future.wait_for(std::chrono::seconds(2)),
              std::future_status::ready);
  }

  void ExpectError(const Json::Value& response, CDPErrorCode code,
                   const std::string& message) {
    EXPECT_EQ(response["error"]["code"].asInt(), static_cast<int>(code));
    EXPECT_EQ(response["error"]["message"].asString(), message);
    EXPECT_FALSE(response.isMember("result"));
  }

  FakeTraceController trace_controller_;
  std::shared_ptr<TracingTestMessageSender> sender_;
  std::unique_ptr<InspectorTracingAgent> agent_;
  std::unique_ptr<fml::Thread> devtool_thread_;
  fml::RefPtr<fml::TaskRunner> saved_task_runner_;
  int saved_session_id_ = -1;
  std::unique_ptr<base::NotificationCallback> saved_notification_callback_;
  lynx::trace::TraceController* saved_trace_controller_ = nullptr;
  std::string temp_trace_file_;
};

TEST_F(InspectorTracingAgentTest, UnknownMethodReturnsMethodNotFound) {
  Json::Value response = Dispatch("Tracing.unknown");

  ExpectError(response, CDPErrorCode::MethodNotFound,
              "'Tracing.unknown' wasn't found");
}

TEST_F(InspectorTracingAgentTest, StoresAndReturnsStartupConfig) {
  Json::Value params(Json::objectValue);
  params["config"] = R"({"traceConfig":{"includedCategories":["*"]}})";

  Json::Value response = Dispatch("Tracing.setStartupTracingConfig", params);
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_EQ(trace_controller_.startup_config, params["config"].asString());

  response = Dispatch("Tracing.getStartupTracingConfig");
  EXPECT_EQ(response["result"]["config"], params["config"]);
}

TEST_F(InspectorTracingAgentTest, RejectsInvalidStartupConfig) {
  Json::Value params(Json::objectValue);
  params["config"] = 1;

  Json::Value response = Dispatch("Tracing.setStartupTracingConfig", params);

  ExpectError(response, CDPErrorCode::InvalidParams,
              "Invalid config: expected string");
}

TEST_F(InspectorTracingAgentTest, ReportsUnavailableTraceController) {
  lynx::testing::GlobalDevToolPlatformFacadeMock::TraceControllerForTesting() =
      nullptr;

  Json::Value response = Dispatch("Tracing.getStartupTracingConfig");

  ExpectError(response, CDPErrorCode::ServerError,
              "Failed to get trace controller");
}

TEST_F(InspectorTracingAgentTest, ReportsUnavailableTaskRunner) {
  auto& mediator = LynxGlobalDevToolMediator::GetInstance();
  mediator.default_task_runner_ = nullptr;

  Json::Value response = Dispatch("Tracing.getStartupTracingConfig");

  ExpectError(response, CDPErrorCode::ServerError,
              "Cannot find default task runner");
  mediator.default_task_runner_ = devtool_thread_->GetTaskRunner();
}

TEST_F(InspectorTracingAgentTest, ReportsStartupTracingFileState) {
  trace_controller_.startup_tracing_started = true;
  Json::Value response = Dispatch("Tracing.getStartupTracingFile");
  ExpectError(response, CDPErrorCode::ServerError,
              "Startup Tracing is running");

  trace_controller_.startup_tracing_started = false;
  response = Dispatch("Tracing.getStartupTracingFile");
  ExpectError(response, CDPErrorCode::ServerError,
              "Failed to get startup tracing file");
}

TEST_F(InspectorTracingAgentTest,
       SendsStartupTracingCompleteAsIndependentEvent) {
  trace_controller_.startup_file_path = temp_trace_file_;
  MockReceiver::GetInstance().ResetAll();

  Json::Value response = Dispatch("Tracing.getStartupTracingFile");
  EXPECT_TRUE(response["result"].isObject());
  DrainTasks();

  Json::Value event = ParseMockReceiverMessage();
  EXPECT_EQ(event["method"], "Tracing.tracingComplete");
  EXPECT_TRUE(event["params"]["isStartupTracing"].asBool());
  EXPECT_FALSE(event.isMember("id"));
  int stream_handle = std::stoi(event["params"]["stream"].asString());
  EXPECT_GT(stream_handle, 0);
  FileStream::Close(stream_handle);
}

TEST_F(InspectorTracingAgentTest, ValidatesVMSnapshotRequest) {
  auto& mediator = LynxGlobalDevToolMediator::GetInstance();
  mediator.tracing_session_id_ = 37;

  Json::Value invalid_params(Json::objectValue);
  invalid_params["url"] = "https://example.com/template.js";
  Json::Value response =
      Dispatch("Tracing.takeVMSnapshotByUrl", invalid_params);
  ExpectError(response, CDPErrorCode::InvalidParams,
              "Invalid url or type: expected strings");

  Json::Value params(Json::objectValue);
  params["url"] = "https://example.com/template.js";
  params["type"] = "bts";
  response = Dispatch("Tracing.takeVMSnapshotByUrl", params);
  EXPECT_TRUE(response["result"].isObject());

  mediator.tracing_session_id_ = -1;
  response = Dispatch("Tracing.takeVMSnapshotByUrl", params);
  ExpectError(response, CDPErrorCode::ServerError, "Tracing is not started");
}

TEST_F(InspectorTracingAgentTest, StartsCompletesAndEndsTracing) {
  trace_controller_.trace_file_path = temp_trace_file_;
  Json::Value params(Json::objectValue);
  params["traceConfig"]["includedCategories"].append("test");
  params["traceConfig"]["enableSystrace"] = true;

  Json::Value response = Dispatch("Tracing.start", params);
  if (!lynx::tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
    ExpectError(response, CDPErrorCode::ServerError, "Tracing not enabled");
    return;
  }
  EXPECT_TRUE(response["result"].isObject());
  ASSERT_NE(trace_controller_.last_config, nullptr);
  EXPECT_EQ(trace_controller_.last_config->included_categories,
            std::vector<std::string>({"test"}));
  EXPECT_TRUE(trace_controller_.last_config->enable_systrace);
  EXPECT_EQ(trace_controller_.callback_session_id, 37);

  response = Dispatch("Tracing.start", params);
  ExpectError(response, CDPErrorCode::ServerError, "Tracing already started");

  MockReceiver::GetInstance().ResetAll();
  ASSERT_TRUE(trace_controller_.complete_callback);
  trace_controller_.complete_callback();
  Json::Value event = ParseMockReceiverMessage();
  EXPECT_EQ(event["method"], "Tracing.tracingComplete");
  EXPECT_EQ(event["params"]["traceFormat"], "proto");
  EXPECT_FALSE(event.isMember("id"));
  int stream_handle = std::stoi(event["params"]["stream"].asString());
  EXPECT_GT(stream_handle, 0);
  FileStream::Close(stream_handle);

  response = Dispatch("Tracing.end");
  DrainTasks();
  EXPECT_TRUE(response["result"].isObject());
  EXPECT_EQ(trace_controller_.stopped_session_id, 37);

  response = Dispatch("Tracing.end");
  ExpectError(response, CDPErrorCode::ServerError, "Tracing is not started");
}

TEST_F(InspectorTracingAgentTest, ReportsTraceStartFailure) {
  if (!lynx::tasm::LynxEnv::GetInstance().IsDebugModeEnabled()) {
    GTEST_SKIP() << "Tracing is disabled in this build";
  }
  trace_controller_.start_result = -1;

  Json::Value response = Dispatch("Tracing.start");

  ExpectError(response, CDPErrorCode::ServerError, "Failed to start tracing");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
