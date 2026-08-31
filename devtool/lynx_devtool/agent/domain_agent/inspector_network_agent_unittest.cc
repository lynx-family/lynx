// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/agent/domain_agent/inspector_network_agent.h"

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/thread.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/inspector_default_executor.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/agent/network_request_observer.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {

namespace testing {

class NetworkTestMessageSender : public MessageSender {
 public:
  void SendMessage(const std::string&, const Json::Value& message) override {
    messages_.push_back(message);
  }

  void SendMessage(const std::string&, const std::string&) override {}

  const Json::Value& LastMessage() const { return messages_.back(); }
  const std::vector<Json::Value>& Messages() const { return messages_; }
  void Clear() { messages_.clear(); }

 private:
  std::vector<Json::Value> messages_;
};

class InspectorNetworkAgentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sender_ = std::make_shared<NetworkTestMessageSender>();
    devtool_thread_ = std::make_unique<fml::Thread>("devtools");
    mediator_ = std::make_shared<LynxDevToolMediator>();
    mediator_->default_task_runner_ = devtool_thread_->GetTaskRunner();
    mediator_->devtool_executor_ =
        std::make_shared<InspectorDefaultExecutor>(mediator_);
    // Route CDP events produced by the observer through the real chain
    // (observer -> DevTool thread -> mediator -> NG mock) back to sender_.
    devtools_ng_ = std::make_shared<lynx::testing::LynxDevToolNGMock>();
    devtools_ng_->message_sender_ = sender_;
    devtools_ng_->devtool_mediator_ = mediator_;
    mediator_->devtool_wp_ = devtools_ng_;
    observer_ = mediator_->devtool_executor_->network_observer_;
    agent_ = std::make_unique<InspectorNetworkAgent>(mediator_);
  }

  void TearDown() override {
    FlushDevToolTasks();
    devtool_thread_.reset();
  }

  // Captures and commands are dispatched to the DevTool runner; drain it
  // before asserting on events or command responses.
  void FlushDevToolTasks() {
    std::promise<void> p;
    auto future = p.get_future();
    mediator_->RunOnDevToolThread([&p]() { p.set_value(); });
    ASSERT_EQ(future.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
  }

  void Call(int64_t id, const std::string& method,
            const Json::Value& params = Json::Value()) {
    Json::Value message;
    message["id"] = id;
    message["method"] = method;
    if (!params.isNull()) {
      message["params"] = params;
    }
    agent_->CallMethod(sender_, message);
    FlushDevToolTasks();
  }

  void Enable(const Json::Value& params = Json::Value()) {
    Call(1, "Network.enable", params);
    ASSERT_TRUE(observer_->IsEnabled());
    ASSERT_TRUE(sender_->LastMessage()["result"].isObject());
    sender_->Clear();
  }

  size_t RequestBodyCapacity(const std::string& request_id) {
    auto it = observer_->records_.find(request_id);
    return it == observer_->records_.end() ? 0
                                           : it->second.request_body.capacity();
  }

  size_t ResponseBodyCapacity(const std::string& request_id) {
    auto it = observer_->records_.find(request_id);
    return it == observer_->records_.end()
               ? 0
               : it->second.response_body.capacity();
  }

  std::shared_ptr<NetworkTestMessageSender> sender_;
  std::shared_ptr<LynxDevToolMediator> mediator_;
  std::shared_ptr<lynx::testing::LynxDevToolNGMock> devtools_ng_;
  std::shared_ptr<NetworkRequestObserver> observer_;
  std::unique_ptr<InspectorNetworkAgent> agent_;
  std::unique_ptr<fml::Thread> devtool_thread_;
};

TEST_F(InspectorNetworkAgentTest, EnableUsesDefaultsAndStoresPostData) {
  Enable();
  NetworkRequestInfo request;
  request.url = "https://example.com/echo";
  request.method = "POST";
  request.headers["Content-Type"] = "text/plain";
  request.body.assign(64 * 1024 + 1, 'a');

  const std::string request_id = observer_->RequestWillBeSent(request);
  ASSERT_FALSE(request_id.empty());
  FlushDevToolTasks();
  ASSERT_EQ(sender_->Messages().size(), 1u);
  const Json::Value& event = sender_->LastMessage();
  EXPECT_EQ(event["method"].asString(), "Network.requestWillBeSent");
  EXPECT_EQ(event["params"]["requestId"].asString(), request_id);
  EXPECT_EQ(event["params"]["request"]["method"].asString(), "POST");
  EXPECT_TRUE(event["params"]["request"]["hasPostData"].asBool());
  EXPECT_FALSE(event["params"]["request"].isMember("postData"));

  Json::Value params;
  params["requestId"] = request_id;
  Call(2, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["postData"].asString().size(),
            request.body.size());
  EXPECT_FALSE(sender_->LastMessage()["result"]["base64Encoded"].asBool());
}

TEST_F(InspectorNetworkAgentTest, EmitsAllSupportedEventShapes) {
  Enable();
  NetworkRequestInfo request;
  request.url = "https://example.com/events";
  const std::string request_id = observer_->RequestWillBeSent(request);

  NetworkResponseInfo response;
  response.status = 200;
  response.status_text = "OK";
  response.headers["Content-Type"] = "text/event-stream";
  observer_->ResponseReceived(request_id, response);
  const uint8_t chunk[] = {'o', 'k'};
  observer_->DataReceived(request_id,
                          std::vector<uint8_t>(chunk, chunk + sizeof(chunk)));
  observer_->EventSourceMessageReceived(request_id, "message", "7", "ok");
  observer_->LoadingFinished(request_id);

  NetworkRequestInfo failed_request;
  failed_request.url = "https://invalid.example";
  const std::string failed_id = observer_->RequestWillBeSent(failed_request);
  observer_->LoadingFailed(failed_id, "cancelled", true);
  FlushDevToolTasks();

  std::vector<std::string> methods;
  for (const auto& message : sender_->Messages()) {
    methods.push_back(message["method"].asString());
  }
  EXPECT_EQ(methods,
            (std::vector<std::string>{
                "Network.requestWillBeSent", "Network.responseReceived",
                "Network.dataReceived", "Network.eventSourceMessageReceived",
                "Network.loadingFinished", "Network.requestWillBeSent",
                "Network.loadingFailed"}));
  EXPECT_EQ(
      sender_->Messages()[1]["params"]["response"]["securityState"].asString(),
      "unknown");
  EXPECT_EQ(sender_->Messages()[3]["params"]["eventId"].asString(), "7");
  EXPECT_TRUE(sender_->Messages()[6]["params"]["canceled"].asBool());
}

TEST_F(InspectorNetworkAgentTest, ReturnsTextAndBinaryResponseBodies) {
  Enable();
  NetworkRequestInfo request;
  request.url = "https://example.com/body";
  std::string request_id = observer_->RequestWillBeSent(request);
  NetworkResponseInfo response;
  response.status = 200;
  observer_->ResponseReceived(request_id, response);
  const uint8_t text[] = {'h', 'i'};
  observer_->DataReceived(request_id,
                          std::vector<uint8_t>(text, text + sizeof(text)));
  observer_->LoadingFinished(request_id);

  Json::Value params;
  params["requestId"] = request_id;
  Call(2, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["body"].asString(), "hi");
  EXPECT_FALSE(sender_->LastMessage()["result"]["base64Encoded"].asBool());

  request_id = observer_->RequestWillBeSent(request);
  observer_->ResponseReceived(request_id, response);
  const uint8_t binary[] = {0xff, 0x00};
  observer_->DataReceived(
      request_id, std::vector<uint8_t>(binary, binary + sizeof(binary)));
  observer_->LoadingFinished(request_id);
  params["requestId"] = request_id;
  Call(3, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["body"].asString(), "/wA=");
  EXPECT_TRUE(sender_->LastMessage()["result"]["base64Encoded"].asBool());
}

TEST_F(InspectorNetworkAgentTest, HonorsResourceLimitAndBodyFifoEviction) {
  Json::Value params;
  params["maxTotalBufferSize"] = 8;
  params["maxResourceBufferSize"] = 10;
  params["maxPostDataSize"] = 2;
  Enable(params);

  NetworkRequestInfo first;
  first.url = "https://example.com/first";
  first.method = "POST";
  first.body = {'1', '2', '3', '4'};
  const std::string first_id = observer_->RequestWillBeSent(first);
  NetworkRequestInfo second = first;
  second.url = "https://example.com/second";
  second.body = {'a', 'b', 'c', 'd'};
  const std::string second_id = observer_->RequestWillBeSent(second);

  // Reading the oldest body must not refresh its FIFO position.
  params.clear();
  params["requestId"] = first_id;
  Call(2, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["postData"].asString(), "1234");

  NetworkResponseInfo response;
  response.status = 200;
  observer_->ResponseReceived(first_id, response);
  const uint8_t response_body[] = {'r', 'e', 's', 'p'};
  observer_->DataReceived(
      first_id, std::vector<uint8_t>(response_body,
                                     response_body + sizeof(response_body)));
  observer_->LoadingFinished(first_id);

  params["requestId"] = first_id;
  Call(3, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["code"].asInt(), -32000);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "Body was evicted from the Network buffer");
  EXPECT_EQ(RequestBodyCapacity(first_id), 0u);

  params["requestId"] = second_id;
  Call(4, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["postData"].asString(), "abcd");

  params["requestId"] = first_id;
  Call(5, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["body"].asString(), "resp");

  NetworkRequestInfo oversized = first;
  oversized.url = "https://example.com/oversized";
  oversized.body.assign(11, 'x');
  const std::string oversized_id = observer_->RequestWillBeSent(oversized);
  params["requestId"] = oversized_id;
  Call(6, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "Body exceeded maxResourceBufferSize");
}

TEST_F(InspectorNetworkAgentTest, OversizedRequestBodiesAreNotRetained) {
  Json::Value params;
  params["maxTotalBufferSize"] = 4;
  params["maxResourceBufferSize"] = 4;
  Enable(params);

  NetworkRequestInfo request;
  request.url = "https://example.com/oversized";
  request.method = "POST";
  request.body.assign(5, 'x');
  const std::string first_id = observer_->RequestWillBeSent(request);
  request.url = "https://example.com/oversized-again";
  const std::string second_id = observer_->RequestWillBeSent(request);

  request.url = "https://example.com/small";
  request.body.assign(4, 'a');
  const std::string small_id = observer_->RequestWillBeSent(request);
  FlushDevToolTasks();

  EXPECT_EQ(RequestBodyCapacity(first_id), 0u);
  EXPECT_EQ(RequestBodyCapacity(second_id), 0u);

  EXPECT_GE(RequestBodyCapacity(small_id), 4u);
}

TEST_F(InspectorNetworkAgentTest, ResponseBodyRequiresSuccessfulCompletion) {
  Enable();
  NetworkRequestInfo request;
  request.url = "https://example.com/stream";
  NetworkResponseInfo response;
  response.status = 200;

  const std::string pending_id = observer_->RequestWillBeSent(request);
  observer_->ResponseReceived(pending_id, response);
  const uint8_t partial[] = {'p', 'a', 'r', 't'};
  observer_->DataReceived(
      pending_id, std::vector<uint8_t>(partial, partial + sizeof(partial)));

  Json::Value params;
  params["requestId"] = pending_id;
  Call(2, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "Response body is not available until loading is finished");

  observer_->LoadingFailed(pending_id, "stream failed");
  Call(3, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "Response body is unavailable because the request failed");
  EXPECT_EQ(ResponseBodyCapacity(pending_id), 0u);

  const std::string empty_id = observer_->RequestWillBeSent(request);
  observer_->ResponseReceived(empty_id, response);
  observer_->LoadingFinished(empty_id);
  params["requestId"] = empty_id;
  Call(4, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["body"].asString(), "");
  EXPECT_FALSE(sender_->LastMessage()["result"]["base64Encoded"].asBool());
}

TEST_F(InspectorNetworkAgentTest, DisableClearsBufferedBodies) {
  Enable();
  NetworkRequestInfo request;
  request.url = "https://example.com";
  request.method = "POST";
  request.body = {'o', 'k'};
  const std::string request_id = observer_->RequestWillBeSent(request);
  Call(2, "Network.disable");
  EXPECT_FALSE(observer_->IsEnabled());

  Json::Value params;
  params["requestId"] = request_id;
  Call(3, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "No resource with given identifier found");
}

TEST_F(InspectorNetworkAgentTest, RepeatedEnableKeepsBufferedBodies) {
  Enable();
  NetworkRequestInfo request;
  request.url = "https://example.com";
  request.method = "POST";
  request.body = {'o', 'k'};
  const std::string request_id = observer_->RequestWillBeSent(request);
  FlushDevToolTasks();

  // A repeated enable while already enabled is a no-op: buffered bodies are
  // kept even when the new params differ.
  Json::Value enable_params;
  enable_params["maxTotalBufferSize"] = 1;
  Call(2, "Network.enable", enable_params);

  Json::Value query;
  query["requestId"] = request_id;
  Call(3, "Network.getRequestPostData", query);
  EXPECT_EQ(sender_->LastMessage()["result"]["postData"].asString(), "ok");
}

TEST_F(InspectorNetworkAgentTest,
       ReturnsBinaryPostDataAndReportsBodyAvailabilityErrors) {
  Json::Value enable_params;
  enable_params["maxResourceBufferSize"] = 2;
  Enable(enable_params);

  NetworkRequestInfo request;
  request.url = "https://example.com/body-errors";
  request.method = "POST";
  const std::string empty_body_id = observer_->RequestWillBeSent(request);

  Json::Value params;
  params["requestId"] = empty_body_id;
  Call(2, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "No body was sent or received for this request");

  request.body = {0xff};
  const std::string binary_body_id = observer_->RequestWillBeSent(request);
  params["requestId"] = binary_body_id;
  Call(3, "Network.getRequestPostData", params);
  EXPECT_EQ(sender_->LastMessage()["result"]["postData"].asString(), "/w==");
  EXPECT_TRUE(sender_->LastMessage()["result"]["base64Encoded"].asBool());

  request.body.clear();
  const std::string response_id = observer_->RequestWillBeSent(request);
  NetworkResponseInfo response;
  response.status = 200;
  observer_->ResponseReceived(response_id, response);
  const uint8_t oversized_response[] = {'1', '2', '3'};
  observer_->DataReceived(
      response_id,
      std::vector<uint8_t>(oversized_response,
                           oversized_response + sizeof(oversized_response)));
  observer_->LoadingFinished(response_id);
  params["requestId"] = response_id;
  Call(4, "Network.getResponseBody", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "Body exceeded maxResourceBufferSize");
  EXPECT_EQ(ResponseBodyCapacity(response_id), 0u);
}

TEST_F(InspectorNetworkAgentTest, RejectsInvalidParametersAndUnknownMethods) {
  Json::Value params;
  params["maxTotalBufferSize"] = -1;
  Call(1, "Network.enable", params);
  EXPECT_EQ(sender_->LastMessage()["error"]["code"].asInt(), -32602);
  EXPECT_FALSE(observer_->IsEnabled());

  Call(2, "Network.getResponseBody", Json::Value(Json::objectValue));
  EXPECT_EQ(sender_->LastMessage()["error"]["code"].asInt(), -32602);

  Call(3, "Network.unknown");
  EXPECT_EQ(sender_->LastMessage()["error"]["message"].asString(),
            "Not implemented: Network.unknown");
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
