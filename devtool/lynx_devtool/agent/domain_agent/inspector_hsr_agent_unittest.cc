// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_hsr_agent.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"
#include "devtool/testing/mock/global_devtool_platform_facade_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {
namespace {

class HSRMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string& type, const Json::Value& msg) override {
    EXPECT_EQ(type, "CDP");
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(msg);
    response_thread_ = std::this_thread::get_id();
    condition_.notify_all();
  }

  void SendMessage(const std::string& type, const std::string& msg) override {
    Json::Value parsed;
    Json::Reader reader;
    ASSERT_TRUE(reader.parse(msg, parsed));
    SendMessage(type, parsed);
  }

  Json::Value WaitForResponse() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!condition_.wait_for(lock, std::chrono::seconds(2),
                             [this] { return !messages_.empty(); })) {
      ADD_FAILURE() << "Missing HSR response";
      return {};
    }
    EXPECT_EQ(messages_.size(), 1u);
    auto response = messages_.front();
    messages_.clear();
    return response;
  }

  size_t MessageCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.size();
  }

  std::thread::id ResponseThread() {
    std::lock_guard<std::mutex> lock(mutex_);
    return response_thread_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<Json::Value> messages_;
  std::thread::id response_thread_;
};

class HSRMessageDispatcher : public DevToolMessageDispatcher {
 public:
  std::shared_ptr<MessageSender> GetSender() const override { return sender; }
  std::shared_ptr<HSRMessageSender> sender =
      std::make_shared<HSRMessageSender>();
};

class HSRFacade : public lynx::testing::GlobalDevToolPlatformFacadeMock {
 public:
  void HandleHSRScript(HSRScriptRequest request,
                       HSRScriptCallback callback) override {
    last_request = request;
    ++calls;
    request_thread = std::this_thread::get_id();
    if (handler) {
      handler(std::move(callback));
    } else {
      GlobalDevToolPlatformFacade::HandleHSRScript(std::move(request),
                                                   std::move(callback));
    }
  }

  HSRScriptRequest last_request;
  int calls = 0;
  std::thread::id request_thread;
  std::function<void(HSRScriptCallback)> handler;
};

Json::Value Parse(const char* json) {
  Json::Value value;
  Json::Reader reader;
  EXPECT_TRUE(reader.parse(json, value));
  return value;
}

}  // namespace

class InspectorHSRAgentTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    dispatcher_.RegisterAgent("HSR",
                              std::make_unique<InspectorHSRAgent>(facade_));
  }

  void Drain() {
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();
    LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner()->PostTask(
        [done] { done->set_value(); });
    ASSERT_EQ(future.wait_for(std::chrono::seconds(2)),
              std::future_status::ready);
  }

  void Send(const std::string& method, const Json::Value& params) {
    Json::Value request(Json::objectValue);
    request["id"] = Json::Int64(4294967297LL);
    request["method"] = method;
    request["params"] = params;
    dispatcher_.DispatchMessage(dispatcher_.sender, "CDP",
                                request.toStyledString());
  }

  Json::Value Dispatch(const std::string& method, const Json::Value& params) {
    Send(method, params);
    auto response = dispatcher_.sender->WaitForResponse();
    Drain();
    EXPECT_EQ(response["id"].asInt64(), 4294967297LL);
    EXPECT_EQ(dispatcher_.sender->MessageCount(), 0u);
    return response;
  }

  void ExpectError(const Json::Value& response, CDPErrorCode code) {
    EXPECT_EQ(response["error"]["code"].asInt(), static_cast<int>(code));
    EXPECT_FALSE(response.isMember("result"));
  }

  HSRFacade facade_;
  HSRMessageDispatcher dispatcher_;
};

TEST_F(InspectorHSRAgentTest, ReportsMissingRuntimeForBothMethods) {
  for (const auto& entry :
       {std::make_pair("HSR.loadScript",
                       R"({"source":{"type":"inline","script":""}})"),
        std::make_pair("HSR.evaluate", R"({"expression":"2 + 2"})")}) {
    auto response = Dispatch(entry.first, Parse(entry.second));
    ExpectError(response, CDPErrorCode::ServerError);
    EXPECT_EQ(response["error"]["message"].asString(),
              "HSR runtime is not connected");
  }
}

TEST_F(InspectorHSRAgentTest, ForwardsAllLoadSourcesWithoutChangingText) {
  facade_.handler = [](auto callback) {
    callback(Json::Value(Json::objectValue), "");
  };
  const std::vector<std::pair<const char*, HSRScriptRequest::SourceType>> inputs =
      {{R"({"source":{"type":"inline","script":"globalThis.x = '😀';\n"}})",
        HSRScriptRequest::SourceType::kInline},
       {R"({"source":{"type":"inline","script":""}})",
        HSRScriptRequest::SourceType::kInline},
       {R"({"source":{"type":"url","url":"https://example.com/host.js?q=a%20b"}})",
        HSRScriptRequest::SourceType::kUrl},
       {R"({"source":{"type":"url","url":"file://host:/data/host.js"}})",
        HSRScriptRequest::SourceType::kUrl},
       {R"({"source":{"type":"url","url":"file:///data/host%2520script.js"}})",
        HSRScriptRequest::SourceType::kUrl},
       {R"({"source":{"type":"url","url":"assets://host.js"}})",
        HSRScriptRequest::SourceType::kUrl},
       {R"({"source":{"type":"url","url":"content://scripts/host.js"}})",
        HSRScriptRequest::SourceType::kUrl},
       // URL validity and supported schemes belong to the resource fetcher.
       {R"({"source":{"type":"url","url":"file://"}})",
        HSRScriptRequest::SourceType::kUrl}};
  for (const auto& input : inputs) {
    auto params = Parse(input.first);
    auto response = Dispatch("HSR.loadScript", params);
    EXPECT_TRUE(response["result"].isObject());
    EXPECT_TRUE(response["result"].empty());
    EXPECT_EQ(facade_.last_request.operation,
              HSRScriptRequest::Operation::kLoadScript);
    EXPECT_EQ(facade_.last_request.source_type, input.second);
    EXPECT_EQ(
        facade_.last_request.source,
        params["source"]
              [input.second == HSRScriptRequest::SourceType::kInline ? "script"
                                                                     : "url"]
                  .asString());
  }
}

TEST_F(InspectorHSRAgentTest, ReturnsJsonValuesAndUndefined) {
  for (const auto* result :
       {R"({"valueType":"json","value":4})",
        R"({"valueType":"json","value":null})",
        R"({"valueType":"json","value":{"ok":true,"items":[1,"😀"]}})",
        R"({"valueType":"undefined"})"}) {
    facade_.handler = [result](auto callback) { callback(Parse(result), ""); };
    auto response = Dispatch("HSR.evaluate",
                             Parse(R"({"expression":"globalThis.result"})"));
    EXPECT_EQ(response["result"], Parse(result));
    EXPECT_EQ(facade_.last_request.operation,
              HSRScriptRequest::Operation::kEvaluate);
    EXPECT_EQ(facade_.last_request.source, "globalThis.result");
  }
}

TEST_F(InspectorHSRAgentTest, RejectsInvalidLoadSourcesBeforeCallingRuntime) {
  for (
      const auto* params :
      {"null", "{}", R"({"source":0})", R"({"source":{}})",
       R"({"source":{"type":"unknown"}})",
       R"({"source":{"type":"inline","script":5}})",
       R"({"source":{"type":"inline","script":"x","url":"u"}})",
       R"({"source":{"type":"url","url":""}})",
       R"({"source":{"type":"url","url":false}})",
       R"({"source":{"type":"file","url":"file:///x.js"}})",
       R"({"source":{"type":"url","url":"https://example.com/x.js","script":""}})"}) {
    ExpectError(Dispatch("HSR.loadScript", Parse(params)),
                CDPErrorCode::InvalidParams);
  }
  EXPECT_EQ(facade_.calls, 0);
}

TEST_F(InspectorHSRAgentTest,
       RejectsInvalidExpressionsAndAllowsEmptyExpression) {
  for (const auto* params : {"null", "{}", R"({"expression":12})"}) {
    ExpectError(Dispatch("HSR.evaluate", Parse(params)),
                CDPErrorCode::InvalidParams);
  }
  EXPECT_EQ(facade_.calls, 0);
  facade_.handler = [](auto callback) {
    callback(Parse(R"({"valueType":"undefined"})"), "");
  };
  EXPECT_TRUE(Dispatch("HSR.evaluate", Parse(R"({"expression":""})"))
                  .isMember("result"));
}

TEST_F(InspectorHSRAgentTest, PreservesDispatcherValidationOfRequestEnvelope) {
  for (const auto* method : {"HSR.loadScript", "HSR.evaluate"}) {
    ExpectError(Dispatch(method, Parse("[]")), CDPErrorCode::InvalidRequest);
  }
  EXPECT_EQ(facade_.calls, 0);
}

TEST_F(InspectorHSRAgentTest, RejectsRemovedAndUnknownMethods) {
  for (const auto* method : {"HSR.sendMessage", "HSR.unknown"}) {
    ExpectError(Dispatch(method, Json::Value()), CDPErrorCode::MethodNotFound);
  }
}

TEST_F(InspectorHSRAgentTest, DoesNotAcknowledgeBeforeAsynchronousCompletion) {
  auto accepted = std::make_shared<
      std::promise<GlobalDevToolPlatformFacade::HSRScriptCallback>>();
  auto future = accepted->get_future();
  facade_.handler = [accepted](auto callback) {
    accepted->set_value(std::move(callback));
  };
  Send("HSR.evaluate", Parse(R"({"expression":"2 + 2"})"));
  ASSERT_EQ(future.wait_for(std::chrono::seconds(2)),
            std::future_status::ready);
  auto callback = future.get();
  Drain();
  EXPECT_EQ(dispatcher_.sender->MessageCount(), 0u);
  std::thread worker([callback = std::move(callback)]() mutable {
    callback(Parse(R"({"valueType":"json","value":4})"), "");
    callback(Json::Value(), "late duplicate failure");
  });
  worker.join();
  auto response = dispatcher_.sender->WaitForResponse();
  Drain();
  EXPECT_EQ(response["id"].asInt64(), 4294967297LL);
  EXPECT_EQ(response["result"]["value"].asInt(), 4);
  EXPECT_EQ(dispatcher_.sender->ResponseThread(), facade_.request_thread);
  EXPECT_EQ(dispatcher_.sender->MessageCount(), 0u);
}

TEST_F(InspectorHSRAgentTest, ReportsRuntimeErrorsBeforeValidatingResults) {
  facade_.handler = [](auto callback) {
    callback(Json::Value(), "Script initialization failed");
  };
  for (const auto& entry :
       {std::make_pair("HSR.loadScript",
                       R"({"source":{"type":"inline","script":"throw 1"}})"),
        std::make_pair("HSR.evaluate", R"({"expression":"throw 1"})")}) {
    auto response = Dispatch(entry.first, Parse(entry.second));
    ExpectError(response, CDPErrorCode::ServerError);
    EXPECT_EQ(response["error"]["message"].asString(),
              "Script initialization failed");
  }
}

TEST_F(InspectorHSRAgentTest, ReportsCallbackAbandonedOnWorkerThread) {
  auto accepted = std::make_shared<
      std::promise<GlobalDevToolPlatformFacade::HSRScriptCallback>>();
  auto future = accepted->get_future();
  facade_.handler = [accepted](auto callback) {
    accepted->set_value(std::move(callback));
  };
  Send("HSR.evaluate", Parse(R"({"expression":"0"})"));
  ASSERT_EQ(future.wait_for(std::chrono::seconds(2)),
            std::future_status::ready);
  auto callback = future.get();
  Drain();
  EXPECT_EQ(dispatcher_.sender->MessageCount(), 0u);
  std::thread worker(
      [callback = std::move(callback)]() mutable { callback = nullptr; });
  worker.join();
  auto response = dispatcher_.sender->WaitForResponse();
  Drain();
  EXPECT_EQ(response["id"].asInt64(), 4294967297LL);
  ExpectError(response, CDPErrorCode::ServerError);
  EXPECT_EQ(response["error"]["message"].asString(),
            "HSR request was abandoned before completion");
  EXPECT_EQ(dispatcher_.sender->ResponseThread(), facade_.request_thread);
  EXPECT_EQ(dispatcher_.sender->MessageCount(), 0u);
}

TEST_F(InspectorHSRAgentTest, RejectsInvalidCompletionValues) {
  for (const auto* result : {"null", "false", "0", R"("text")", "[]"}) {
    facade_.handler = [result](auto callback) { callback(Parse(result), ""); };
    ExpectError(Dispatch("HSR.evaluate", Parse(R"({"expression":"0"})")),
                CDPErrorCode::InternalError);
  }
  for (const auto* result : {"null", "false", "0", R"("text")", "[]"}) {
    facade_.handler = [result](auto callback) { callback(Parse(result), ""); };
    ExpectError(Dispatch("HSR.loadScript",
                         Parse(R"({"source":{"type":"inline","script":""}})")),
                CDPErrorCode::InternalError);
  }
}

TEST_F(InspectorHSRAgentTest, OwnsCompletionValuesAcrossThreads) {
  auto expected = Parse(
      R"({"valueType":"json","value":{"items":[null,true,-1,1.5,{"text":"😀"}]}})");
  expected["value"]["text"] = std::string("a\0b", 3);
  expected["value"]["integer"] = Json::Int64(-4294967297LL);
  facade_.handler = [expected](auto callback) {
    std::thread worker([expected, &callback] {
      auto result = expected;
      callback(result, "");
      result["value"] = "changed after completion";
    });
    // Queue success before destroying the callback on the DevTool thread.
    // Its destructor must not reply with an error ahead of the queued success.
    worker.join();
  };
  auto response =
      Dispatch("HSR.evaluate", Parse(R"({"expression":"globalThis.result"})"));
  EXPECT_EQ(response["result"], expected);
  EXPECT_EQ(dispatcher_.sender->ResponseThread(), facade_.request_thread);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
