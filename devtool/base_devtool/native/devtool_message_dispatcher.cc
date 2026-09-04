// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/jsoncpp/include/json/reader.h"

namespace lynx {
namespace devtool {

void DevToolMessageDispatcher::DispatchMessage(
    const std::shared_ptr<MessageSender>& sender, const std::string& type,
    const std::string& msg) {
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(msg, root, false)) {
    return;
  }
  DispatchJsonMessage(sender, type, root);
}

void DevToolMessageDispatcher::DispatchCDPMessage(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& msg) {
  // A CDP command must carry an integer "id". When it is missing or not an
  // integer we cannot echo it back, so the error response id stays null. This
  // matches Chromium's crdtp::Dispatchable, which rejects such a message with
  // InvalidRequest instead of dispatching it.
  std::optional<int64_t> id;
  if (msg.isMember("id") && msg["id"].isIntegral()) {
    id = msg["id"].asInt64();
  }

  auto responder = std::make_shared<CDPResponder>(sender, id);

  if (!id.has_value()) {
    responder->SendErrorResponse(CDPErrorCode::kInvalidRequest,
                                 "message must have integer 'id' property");
    return;
  }

  // "method" must be present and be a string.
  if (!msg.isMember("method") || !msg["method"].isString()) {
    responder->SendErrorResponse(CDPErrorCode::kInvalidRequest,
                                 "message must have string 'method' property");
    return;
  }

  // "params" is optional, but when present it must be an object. A malformed
  // envelope is InvalidRequest; InvalidParams is reserved for semantic checks
  // of a well-formed params object, which the domain agent performs.
  if (msg.isMember("params") && !msg["params"].isObject()) {
    responder->SendErrorResponse(CDPErrorCode::kInvalidRequest,
                                 "'params' must be an object");
    return;
  }

  const std::string method = msg["method"].asString();
  const std::string domain = method.substr(0, method.find(kDomainDot));

  std::shared_lock<std::shared_mutex> lock(agent_mutex_);
  auto iter = agent_map_.find(domain);
  if (iter == agent_map_.end()) {
    // Unknown domain/method: reply through CDPResponder so the error
    // envelope is assembled in one place instead of being hand-written here.
    responder->SendErrorResponse(CDPErrorCode::kMethodNotFound,
                                 "'" + method + "' wasn't found");
    return;
  }

  // Ownership of the response is handed back to the legacy agent path; further
  // migration to CDPResponder requires future refactorings.
  iter->second->CallMethod(responder->RetrieveSender(), msg);
}

void DevToolMessageDispatcher::DispatchJsonMessage(
    const std::shared_ptr<MessageSender>& sender, const std::string& type,
    const Json::Value& msg) {
  if (type == "CDP") {
    DispatchCDPMessage(sender, msg);
    return;
  }
  std::shared_lock<std::shared_mutex> lock(handler_mutex_);
  auto it = handler_map_.find(type);
  if (it != handler_map_.end()) {
    it->second->handle(sender, type, msg);
    return;
  }
}

// TODO(zhoumingsong.smile): Add a task_runner for devtool, at now use
// DebugRouter Thread
void DevToolMessageDispatcher::RegisterMessageHandler(
    const std::string& type, std::unique_ptr<DevToolMessageHandler>&& handler) {
  std::unique_lock<std::shared_mutex> lock(handler_mutex_);
  auto it = handler_map_.find(type);
  if (it != handler_map_.end()) {
    LOGI("RegisterMessageHandler has exists:" << it->first);
  }
  handler_map_[type] = std::move(handler);
}

void DevToolMessageDispatcher::UnregisterMessageHandler(
    const std::string& type) {
  std::unique_lock<std::shared_mutex> lock(handler_mutex_);
  handler_map_.erase(type);
}

void DevToolMessageDispatcher::RegisterAgent(
    const std::string& agent_name,
    std::unique_ptr<CDPDomainAgentBase>&& agent) {
  std::unique_lock<std::shared_mutex> lock(agent_mutex_);
  agent_map_.emplace(agent_name, std::move(agent));
}

CDPDomainAgentBase* DevToolMessageDispatcher::GetAgent(
    const std::string& agent_name) {
  std::shared_lock<std::shared_mutex> lock(agent_mutex_);
  auto iter = agent_map_.find(agent_name);
  if (iter == agent_map_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

}  // namespace devtool
}  // namespace lynx
