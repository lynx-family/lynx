// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#define private public
#define protected public
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#undef protected
#undef private
#include "devtool/testing/mock/devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::devtool {
namespace {
class LogLevelSender : public MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& value) override {
    EXPECT_EQ(type, "CDP");
    response = value;
    ++count;
  }
  void SendMessage(const std::string&, const std::string&) override {
    ADD_FAILURE();
  }
  Json::Value response;
  int count = 0;
};

class InspectorLynxLogLevelTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saved_level_ = base::logging::GetMinLogLevel();
    if (saved_level_ == base::logging::LOG_INFO) {
      saved_level_ |= base::logging::GetInfoLogLevel() << 16;
    }
    base::logging::SetPlatformMinLogLevel(base::logging::LOG_INFO);
  }
  void TearDown() override {
    ui_thread_.GetTaskRunner()->PostSyncTask([] {});
    base::logging::SetPlatformMinLogLevel(saved_level_);
  }
  Json::Value Dispatch(const char* method, Json::Value params = Json::Value(),
                       bool with_ui_thread = true) {
    Json::Value message;
    message["id"] = 123;
    message["method"] = method;
    message["params"] = params;
    auto sender = std::make_shared<LogLevelSender>();
    auto mediator = std::make_shared<LynxDevToolMediator>();
    mediator->ui_executor_ = std::make_shared<InspectorUIExecutor>(mediator);
    if (with_ui_thread) {
      mediator->ui_task_runner_ = ui_thread_.GetTaskRunner();
    }
    // Destroy the agent before draining queued tasks. State and pending replies
    // must survive the originating view/session.
    {
      InspectorLynxAgentNG agent(mediator);
      agent.CallMethod(sender, message);
    }
    ui_thread_.GetTaskRunner()->PostSyncTask([] {});
    EXPECT_EQ(sender->count, 1);
    EXPECT_EQ(sender->response["id"].asInt(), 123);
    return sender->response;
  }
  Json::Value SetLevel(const char* name) {
    Json::Value params;
    params["level"] = name;
    return Dispatch("Lynx.setLogLevel", params);
  }

  fml::Thread ui_thread_{"log_agent_ui"};
  int saved_level_ = 0;
};

TEST_F(InspectorLynxLogLevelTest, SetsAllLevelsAndQueriesSharedState) {
  const char* names[] = {"VERBOSE", "DEBUG",   "MONITOR", "OBSERVE",
                         "INFO",    "WARNING", "ERROR"};
  const int native_levels[] = {0, 1, 2, 2, 2, 3, 4};
  const char* compiled_levels[] = {"VERBOSE", "DEBUG", "MONITOR",
                                   "WARNING", "ERROR", "FATAL"};
  for (int index = 0; index < 7; ++index) {
    SCOPED_TRACE(names[index]);
    auto response = SetLevel(names[index]);
    EXPECT_FALSE(response.isMember("error"));
    EXPECT_EQ(response["result"], Json::Value(Json::objectValue));
    auto query = Dispatch("Lynx.getLogLevel");
    EXPECT_FALSE(query.isMember("error"));
    EXPECT_FALSE(query["result"].isMember("businessLevel"));
    EXPECT_EQ(query["result"]["nativeCompileLevel"].asString(),
              compiled_levels[LYNX_MIN_LOG_LEVEL]);
    EXPECT_EQ(query["result"]["runtimeLevel"].asString(), names[index]);
    EXPECT_EQ(base::logging::GetMinLogLevel(), native_levels[index]);
    int m = 0, o = 0, i = 0;
    LOGM(++m);
    LOGO(++o);
    LOGI(++i);
    const bool compiled = LYNX_MIN_LOG_LEVEL <= LYNX_LOG_LEVEL_INFO;
    EXPECT_EQ(m, compiled && index <= 2);
    EXPECT_EQ(o, compiled && index <= 3);
    EXPECT_EQ(i, compiled && index <= 4);
  }
}

TEST_F(InspectorLynxLogLevelTest, HostUpdatesReplaceCDPConfiguration) {
  SetLevel("MONITOR");
  base::logging::SetPlatformMinLogLevel(base::logging::LOG_INFO);
  EXPECT_EQ(Dispatch("Lynx.getLogLevel")["result"]["runtimeLevel"].asString(),
            "INFO");
  base::logging::SetPlatformMinLogLevel(base::logging::LOG_WARNING);
  EXPECT_EQ(Dispatch("Lynx.getLogLevel")["result"]["runtimeLevel"].asString(),
            "WARNING");
}

TEST_F(InspectorLynxLogLevelTest, InvalidParamsDoNotChangeConfiguration) {
  SetLevel("OBSERVE");
  for (const auto& params :
       {Json::Value(), Json::Value(2), Json::Value(Json::arrayValue),
        Json::Value(Json::objectValue)}) {
    EXPECT_EQ(Dispatch("Lynx.setLogLevel", params)["error"]["code"].asInt(),
              -32602);
  }
  for (const auto& value :
       {Json::Value("OFF"), Json::Value("FATAL"), Json::Value("info"),
        Json::Value(""), Json::Value(2)}) {
    Json::Value params;
    params["level"] = value;
    EXPECT_EQ(Dispatch("Lynx.setLogLevel", params)["error"]["code"].asInt(),
              -32602);
  }
  EXPECT_EQ(base::logging::GetMinLogLevel(), base::logging::LOG_INFO);
  EXPECT_EQ(base::logging::GetInfoLogLevel(),
            base::logging::detail::INFO_LEVEL_OBSERVE);
}

TEST_F(InspectorLynxLogLevelTest, MissingUIThreadRejectsSetterButAllowsQuery) {
  Json::Value params;
  params["level"] = "MONITOR";
  auto response = Dispatch("Lynx.setLogLevel", params, false);
  EXPECT_EQ(response["error"]["code"].asInt(), -32603);
  EXPECT_EQ(Dispatch("Lynx.getLogLevel", Json::Value(),
                     false)["result"]["runtimeLevel"]
                .asString(),
            "INFO");
}

class LynxPlatformFacadeMock
    : public ::lynx::testing::DevToolPlatformFacadeMock {
 public:
  struct VMEvent {
    std::string vm_type;
    std::string name;
    std::string data;
  };

  std::string GetLynxVersion() const override { return "3.0.0"; }
  std::vector<float> GetRectToWindow() const override { return rect; }
  std::vector<int32_t> GetViewLocationOnScreen() const override {
    return location;
  }
  void GetLynxScreenShot() override { ++screenshot_count; }
  void OnReceiveTemplateFragment(const std::string& data, bool eof) override {
    fragments.emplace_back(data, eof);
  }
  void SendEventToVM(const std::string& vm_type, const std::string& event_name,
                     const std::string& data) override {
    events.push_back({vm_type, event_name, data});
  }

  std::vector<float> rect{1.5f, 2.5f, 300.0f, 400.0f};
  std::vector<int32_t> location{10, 20};
  int screenshot_count = 0;
  std::vector<std::pair<std::string, bool>> fragments;
  std::vector<VMEvent> events;
};

class InspectorLynxPlatformTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mediator_ = std::make_shared<LynxDevToolMediator>();
    mediator_->ui_executor_ = std::make_shared<InspectorUIExecutor>(mediator_);
    mediator_->devtool_executor_ =
        std::make_shared<InspectorDefaultExecutor>(mediator_);
    mediator_->ui_task_runner_ = ui_thread_.GetTaskRunner();
    mediator_->default_task_runner_ = devtool_thread_.GetTaskRunner();
    facade_ = std::make_shared<LynxPlatformFacadeMock>();
    mediator_->SetDevToolPlatformFacade(facade_);
  }

  void TearDown() override { DrainTasks(); }

  void DrainTasks() {
    ui_thread_.GetTaskRunner()->PostSyncTask([] {});
    devtool_thread_.GetTaskRunner()->PostSyncTask([] {});
  }

  Json::Value Dispatch(const char* method,
                       const Json::Value& params = Json::Value()) {
    Json::Value message(Json::objectValue);
    message["id"] = static_cast<Json::Int64>(kRequestId);
    message["method"] = method;
    message["params"] = params;
    auto sender = std::make_shared<LogLevelSender>();
    {
      InspectorLynxAgentNG agent(mediator_);
      // Use the dispatcher entry so the legacy sender adapter is exercised.
      static_cast<CDPDomainAgentBase&>(agent).CallMethod(
          std::make_shared<CDPResponder>(sender, kRequestId), message);
    }
    DrainTasks();
    EXPECT_EQ(sender->count, 1);
    EXPECT_EQ(sender->response["id"].asInt64(), kRequestId);
    EXPECT_EQ(sender->response.size(), 2u);
    return sender->response;
  }

  void ExpectSuccess(const char* method,
                     const Json::Value& params = Json::Value()) {
    auto response = Dispatch(method, params);
    EXPECT_FALSE(response.isMember("error"));
    EXPECT_EQ(response["result"], Json::Value(Json::objectValue));
  }

  void ExpectError(const char* method, const Json::Value& params,
                   CDPErrorCode code, const char* error_message) {
    auto response = Dispatch(method, params);
    EXPECT_FALSE(response.isMember("result"));
    EXPECT_EQ(response["error"]["code"].asInt(), static_cast<int>(code));
    EXPECT_EQ(response["error"]["message"], error_message);
  }

  void ExpectUnavailableCommands() {
    for (const char* method :
         {"Lynx.getVersion", "Lynx.getRectToWindow", "Lynx.getScreenshot",
          "Lynx.getViewLocationOnScreen", "Lynx.setTraceMode"}) {
      SCOPED_TRACE(method);
      ExpectError(method, Json::Value(), CDPErrorCode::ServerError,
                  "Lynx target is unavailable");
    }
    ExpectError("Lynx.transferData", TemplateParams(),
                CDPErrorCode::ServerError, "Lynx target is unavailable");
    ExpectError("Lynx.sendVMEvent", EventParams(), CDPErrorCode::ServerError,
                "Lynx target is unavailable");
    EXPECT_EQ(facade_->screenshot_count, 0);
    EXPECT_TRUE(facade_->fragments.empty());
    EXPECT_TRUE(facade_->events.empty());
  }

  static Json::Value TemplateParams() {
    Json::Value params(Json::objectValue);
    params["dataType"] = "template";
    params["data"] = "fragment";
    params["eof"] = false;
    return params;
  }

  static Json::Value EventParams() {
    Json::Value params(Json::objectValue);
    params["vmType"] = "lepus";
    params["event"] = "refresh";
    params["data"] = "payload";
    return params;
  }

  static constexpr int64_t kRequestId = 1LL << 40;
  fml::Thread ui_thread_{"lynx_platform_ui"};
  fml::Thread devtool_thread_{"lynx_platform_devtool"};
  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<LynxPlatformFacadeMock> facade_;
};

TEST_F(InspectorLynxPlatformTest, QueriesPreservePlatformResults) {
  auto response = Dispatch("Lynx.getVersion");
  EXPECT_FALSE(response.isMember("error"));
  EXPECT_EQ(response["result"], "3.0.0");

  Json::Value rect(Json::objectValue);
  rect["left"] = 1.5f;
  rect["top"] = 2.5f;
  rect["width"] = 300.0f;
  rect["height"] = 400.0f;
  response = Dispatch("Lynx.getRectToWindow");
  EXPECT_FALSE(response.isMember("error"));
  EXPECT_EQ(response["result"], rect);

  Json::Value location(Json::objectValue);
  location["x"] = 10;
  location["y"] = 20;
  response = Dispatch("Lynx.getViewLocationOnScreen");
  EXPECT_FALSE(response.isMember("error"));
  EXPECT_EQ(response["result"], location);
}

TEST_F(InspectorLynxPlatformTest, MissingFacadeReturnsServerError) {
  mediator_->SetDevToolPlatformFacade(nullptr);
  for (const char* method :
       {"Lynx.getVersion", "Lynx.getRectToWindow", "Lynx.getScreenshot",
        "Lynx.getViewLocationOnScreen"}) {
    SCOPED_TRACE(method);
    ExpectError(method, Json::Value(), CDPErrorCode::ServerError,
                "Lynx target is unavailable");
  }
  ExpectError("Lynx.transferData", TemplateParams(), CDPErrorCode::ServerError,
              "Lynx target is unavailable");
  ExpectError("Lynx.sendVMEvent", EventParams(), CDPErrorCode::ServerError,
              "Lynx target is unavailable");
  EXPECT_EQ(facade_->screenshot_count, 0);
  EXPECT_TRUE(facade_->fragments.empty());
  EXPECT_TRUE(facade_->events.empty());
}

TEST_F(InspectorLynxPlatformTest, MissingExecutorsReturnServerError) {
  mediator_->ui_executor_.reset();
  mediator_->devtool_executor_.reset();
  ExpectUnavailableCommands();
}

TEST_F(InspectorLynxPlatformTest, MissingTaskRunnersReturnServerError) {
  mediator_->ui_task_runner_ = nullptr;
  mediator_->default_task_runner_ = nullptr;
  ExpectUnavailableCommands();
}

TEST_F(InspectorLynxPlatformTest, DeprecatedTraceModeReturnsMethodNotFound) {
  ExpectError("Lynx.setTraceMode", Json::Value(), CDPErrorCode::MethodNotFound,
              "SetTraceMode is deprecated. Use global messages.");
}

TEST_F(InspectorLynxPlatformTest, IncompleteRectReturnsMethodNotFound) {
  for (size_t size = 0; size < 4; ++size) {
    SCOPED_TRACE(size);
    facade_->rect.resize(size);
    ExpectError("Lynx.getRectToWindow", Json::Value(),
                CDPErrorCode::MethodNotFound,
                "Lynx.getRectToWindow is unavailable");
  }
}

TEST_F(InspectorLynxPlatformTest, IncompleteLocationReturnsInvalidPosition) {
  for (size_t size = 0; size < 2; ++size) {
    SCOPED_TRACE(size);
    facade_->location.resize(size);
    auto response = Dispatch("Lynx.getViewLocationOnScreen");
    EXPECT_FALSE(response.isMember("error"));
    EXPECT_EQ(response["result"]["x"], -1);
    EXPECT_EQ(response["result"]["y"], -1);
  }
}

TEST_F(InspectorLynxPlatformTest, ScreenshotAcknowledgesWithoutFrameEvent) {
  ExpectSuccess("Lynx.getScreenshot");
  EXPECT_EQ(facade_->screenshot_count, 1);
}

TEST_F(InspectorLynxPlatformTest,
       TemplateFragmentsAreForwardedAndAcknowledged) {
  auto params = TemplateParams();
  ExpectSuccess("Lynx.transferData", params);
  params["data"] = "last fragment";
  params["eof"] = true;
  ExpectSuccess("Lynx.transferData", params);
  ASSERT_EQ(facade_->fragments.size(), 2u);
  EXPECT_EQ(facade_->fragments[0],
            std::make_pair(std::string("fragment"), false));
  EXPECT_EQ(facade_->fragments[1],
            std::make_pair(std::string("last fragment"), true));
}

TEST_F(InspectorLynxPlatformTest, InvalidTemplateFragmentsAreAcknowledged) {
  ExpectSuccess("Lynx.transferData");
  ExpectSuccess("Lynx.transferData", Json::Value(Json::objectValue));
  auto params = TemplateParams();
  params.removeMember("dataType");
  ExpectSuccess("Lynx.transferData", params);
  params["dataType"] = "unknown";
  ExpectSuccess("Lynx.transferData", params);
  params["dataType"] = "template";
  params.removeMember("data");
  ExpectSuccess("Lynx.transferData", params);
  params["data"] = 1;
  ExpectSuccess("Lynx.transferData", params);
  params["data"] = "fragment";
  params.removeMember("eof");
  ExpectSuccess("Lynx.transferData", params);
  params["eof"] = "true";
  ExpectSuccess("Lynx.transferData", params);
  EXPECT_TRUE(facade_->fragments.empty());
}

TEST_F(InspectorLynxPlatformTest, VMEventsForwardStringsAndDefaultOtherData) {
  auto params = EventParams();
  ExpectSuccess("Lynx.sendVMEvent", params);
  params["data"] = Json::Value(Json::objectValue);
  ExpectSuccess("Lynx.sendVMEvent", params);
  params.removeMember("data");
  ExpectSuccess("Lynx.sendVMEvent", params);
  ASSERT_EQ(facade_->events.size(), 3u);
  for (const auto& event : facade_->events) {
    EXPECT_EQ(event.vm_type, "lepus");
    EXPECT_EQ(event.name, "refresh");
  }
  EXPECT_EQ(facade_->events[0].data, "payload");
  EXPECT_EQ(facade_->events[1].data, "");
  EXPECT_EQ(facade_->events[2].data, "");
}

TEST_F(InspectorLynxPlatformTest, InvalidVMEventNamesAreAcknowledged) {
  ExpectSuccess("Lynx.sendVMEvent");
  ExpectSuccess("Lynx.sendVMEvent", Json::Value(Json::objectValue));
  auto params = EventParams();
  params.removeMember("vmType");
  ExpectSuccess("Lynx.sendVMEvent", params);
  params["vmType"] = 1;
  ExpectSuccess("Lynx.sendVMEvent", params);
  params["vmType"] = "lepus";
  params.removeMember("event");
  ExpectSuccess("Lynx.sendVMEvent", params);
  params["event"] = false;
  ExpectSuccess("Lynx.sendVMEvent", params);
  EXPECT_TRUE(facade_->events.empty());
}
}  // namespace
}  // namespace lynx::devtool
