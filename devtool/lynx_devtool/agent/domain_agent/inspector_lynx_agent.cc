// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent.h"

#include <utility>

#include "devtool/base_devtool/native/public/cdp_responder.h"

namespace lynx {
namespace devtool {

InspectorLynxAgent::InspectorLynxAgent(
    std::shared_ptr<MessageSender> event_sender)
    : event_sender_(std::move(event_sender)) {
  functions_map_["LynxAgent.sendMessage"] = &InspectorLynxAgent::SendMessage;
}

void InspectorLynxAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  (this->*(iter->second))(responder, message["params"]);
}

void InspectorLynxAgent::SetMessageHandler(MessageHandler handler) {
  std::lock_guard<std::mutex> lock(handler_mutex_);
  message_handler_.swap(handler);
}

void InspectorLynxAgent::SendMessage(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!params.isObject() || !params["message"].isString()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "Expected string message");
    return;
  }

  MessageHandler handler;
  {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    handler = message_handler_;
  }
  if (!handler) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Runtime message handler is not installed");
    return;
  }
  if (!handler(params["message"].asString())) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Runtime did not accept the message");
    return;
  }
  responder->SendSuccess();
}

void InspectorLynxAgent::SendMessageReceived(const std::string& message) {
  Json::Value event(Json::objectValue);
  event["method"] = "LynxAgent.messageReceived";
  event["params"]["message"] = message;
  if (event_sender_) {
    event_sender_->SendMessage("CDP", event);
  }
}

}  // namespace devtool
}  // namespace lynx
