// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "devtool/base_devtool/native/public/cdp_domain_agent_base.h"
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
  std::string method = msg["method"].asString();
  std::string domain = method.substr(0, method.find(kDomainDot));
  Json::Value content;

  auto iter = agent_map_.find(domain);
  if (iter == agent_map_.end()) {
    Json::Value error;
    error["code"] = kInspectorErrorCode;
    error["message"] = "Not implemented: " + method;
    content["error"] = error;
    content["id"] = msg["id"].asInt64();
    sender->SendMessage("CDP", content);
  } else {
    iter->second->CallMethod(sender, msg);
  }
}

void DevToolMessageDispatcher::DispatchJsonMessage(
    const std::shared_ptr<MessageSender>& sender, const std::string& type,
    const Json::Value& msg) {
  if (type == "CDP") {
    DispatchCDPMessage(sender, msg);
    return;
  }
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
  auto it = handler_map_.find(type);
  if (it != handler_map_.end()) {
    LOGI("RegisterMessageHandler has exists:" << it->first);
  }
  handler_map_[type] = std::move(handler);
}

void DevToolMessageDispatcher::RegisterAgent(
    const std::string& agent_name,
    std::unique_ptr<CDPDomainAgentBase>&& agent) {
  agent_map_.emplace(agent_name, std::move(agent));
}

CDPDomainAgentBase* DevToolMessageDispatcher::GetAgent(
    const std::string& agent_name) {
  auto iter = agent_map_.find(agent_name);
  if (iter == agent_map_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

}  // namespace devtool
}  // namespace lynx
