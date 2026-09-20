// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"

#include "base/include/fml/thread.h"
#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#define private public
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#undef private
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::devtool {
namespace {
class LogLevelSender : public MessageSender {
 public:
  void SendMessage(const std::string&, const Json::Value& value) override {
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
}  // namespace
}  // namespace lynx::devtool
