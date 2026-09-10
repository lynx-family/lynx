// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_default_executor.h"

#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorDefaultExecutor::InspectorDefaultExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator),
      console_msg_manager_(
          std::make_unique<ConsoleMessageManager>(devtool_mediator)),
      network_observer_(
          std::make_shared<NetworkRequestObserver>(devtool_mediator)) {}

void InspectorDefaultExecutor::Reset() {
  console_msg_manager_->ClearConsoleMessages();
}

// start inspector protocl
void InspectorDefaultExecutor::InspectorEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("InspectorEnable");
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::InspectorDetached(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("InspectorDetached");
  Json::Value content;
  content["method"] = "Inspector.detached";
  content["params"] = Json::ValueType::objectValue;
  content["params"]["reason"] = "";
  sender->SendMessage("CDP", content);
}

void InspectorDefaultExecutor::LynxSetTraceMode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  // TODO(mitchilling): remove this protocol, including agent and mediator.
  LOGW(
      "SetTraceMode is no longer supported. Please use global messages to "
      "access certain settings.");
  Json::Value response(Json::ValueType::objectValue);
  Json::Value error(Json::ValueType::objectValue);
  error["code"] = -32601;  // JSON-RPC standard code for Method not found
  error["message"] = "SetTraceMode is deprecated. Use global messages.";
  response["error"] = error;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::LynxGetVersion(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  response["result"] = devtool_platform_facade_->GetLynxVersion();
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  devtool_platform_facade_ = devtool_platform_facade;
}

// end inspector protocl

// start log protocol
void InspectorDefaultExecutor::LogEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("LogEnable");
  console_msg_manager_->EnableConsoleLog(sender);
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::LogDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("LogDisable");
  console_msg_manager_->DisableConsoleLog();
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::LogClear(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  console_msg_manager_->ClearConsoleMessages();
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::SendLogEntryAddedEvent(
    const lynx::runtime::js::ConsoleMessage& message) {
  console_msg_manager_->LogEntryAdded(message);
}

// end log protocol

void InspectorDefaultExecutor::NetworkEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const int64_t id = message["id"].asInt64();
  if (!network_observer_->Enable(message["params"])) {
    // Network.enable(maxPostDataSize) is called with invalid params, e.g.
    // maxPostDataSize is negative.
    sender->SendErrorResponse(id, kInvalidParams, "Invalid params");
    return;
  }
  sender->SendOKResponse(id);
}

void InspectorDefaultExecutor::NetworkDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  network_observer_->Disable();
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::NetworkGetResponseBody(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const int64_t id = message["id"].asInt64();
  const Json::Value& request_id = message["params"]["requestId"];
  if (!request_id.isString() || request_id.asString().empty()) {
    sender->SendErrorResponse(id, kInvalidParams,
                              "Invalid params: requestId must be a string");
    return;
  }
  std::string body;
  bool base64_encoded = false;
  const auto result = network_observer_->GetResponseBody(request_id.asString(),
                                                         body, base64_encoded);
  if (result != NetworkRequestObserver::BodyResult::OK) {
    sender->SendErrorResponse(
        id, kServerError,
        NetworkRequestObserver::NetworkBodyResultMessage(result));
    return;
  }
  Json::Value response;
  response["id"] = id;
  response["result"]["body"] = body;
  response["result"]["base64Encoded"] = base64_encoded;
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::NetworkGetRequestPostData(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  const int64_t id = message["id"].asInt64();
  const Json::Value& request_id = message["params"]["requestId"];
  if (!request_id.isString() || request_id.asString().empty()) {
    sender->SendErrorResponse(id, kInvalidParams,
                              "Invalid params: requestId must be a string");
    return;
  }
  std::string post_data;
  bool base64_encoded = false;
  const auto result = network_observer_->GetRequestPostData(
      request_id.asString(), post_data, base64_encoded);
  if (result != NetworkRequestObserver::BodyResult::OK) {
    sender->SendErrorResponse(
        id, kServerError,
        NetworkRequestObserver::NetworkBodyResultMessage(result));
    return;
  }
  Json::Value response;
  response["id"] = id;
  response["result"]["postData"] = post_data;
  response["result"]["base64Encoded"] = base64_encoded;
  sender->SendMessage("CDP", response);
}

}  // namespace devtool
}  // namespace lynx
