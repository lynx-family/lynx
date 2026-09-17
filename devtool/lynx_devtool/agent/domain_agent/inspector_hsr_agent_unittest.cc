// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_hsr_agent.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
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

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::vector<Json::Value> messages_;
};

class HSRMessageDispatcher : public DevToolMessageDispatcher {
 public:
  std::shared_ptr<MessageSender> GetSender() const override { return sender; }
  std::shared_ptr<HSRMessageSender> sender =
      std::make_shared<HSRMessageSender>();
};

using HSRFacade = lynx::testing::GlobalDevToolPlatformFacadeMock;

Json::Value Parse(const char* json) {
  Json::Value value;
  Json::Reader reader;
  EXPECT_TRUE(reader.parse(json, value));
  return value;
}

}  // namespace

class InspectorHSRAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dispatcher_.RegisterAgent("HSR", std::make_unique<InspectorHSRAgent>());
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
    EXPECT_EQ(response["id"].asInt64(), 4294967297LL);
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
  for (const auto operation : {HSRScriptRequest::Operation::kLoadScript,
                               HSRScriptRequest::Operation::kEvaluate}) {
    HSRScriptRequest request;
    request.operation = operation;
    bool completed = false;
    facade_.HandleHSRScript(
        request, [&completed](const auto& result, const auto& error) {
          completed = true;
          EXPECT_TRUE(result.empty());
          EXPECT_EQ(error, "HSR runtime is not connected");
        });
    EXPECT_TRUE(completed);
  }
}

TEST_F(InspectorHSRAgentTest, ForwardsAllLoadSourcesWithoutChangingText) {
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
    HSRScriptRequest request;
    std::string error;
    ASSERT_TRUE(ParseHSRLoadScript(params, request, error));
    EXPECT_EQ(request.operation, HSRScriptRequest::Operation::kLoadScript);
    EXPECT_EQ(request.source_type, input.second);
    EXPECT_EQ(
        request.source,
        params["source"]
              [input.second == HSRScriptRequest::SourceType::kInline ? "script"
                                                                     : "url"]
                  .asString());
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
    HSRScriptRequest request;
    std::string error;
    EXPECT_FALSE(ParseHSRLoadScript(Parse(params), request, error));
    EXPECT_FALSE(error.empty());
  }
}

TEST_F(InspectorHSRAgentTest,
       RejectsInvalidExpressionsAndAllowsEmptyExpression) {
  HSRScriptRequest request;
  std::string error;
  for (const auto* params : {"null", "{}", R"({"expression":12})"}) {
    EXPECT_FALSE(ParseHSREvaluate(Parse(params), request, error));
    EXPECT_FALSE(error.empty());
  }
  for (const auto* expression : {"", "globalThis.result"}) {
    Json::Value params(Json::objectValue);
    params["expression"] = expression;
    ASSERT_TRUE(ParseHSREvaluate(params, request, error));
    EXPECT_EQ(request.operation, HSRScriptRequest::Operation::kEvaluate);
    EXPECT_EQ(request.source, expression);
  }
}

TEST_F(InspectorHSRAgentTest, RejectsRemovedAndUnknownMethods) {
  for (const auto* method : {"HSR.sendMessage", "HSR.unknown"}) {
    ExpectError(Dispatch(method, Json::Value()), CDPErrorCode::MethodNotFound);
  }
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
