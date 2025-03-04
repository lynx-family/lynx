// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_memory_agent.h"

#include "core/runtime/vm/lepus/json_parser.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {
InspectorMemoryAgent::InspectorMemoryAgent() {
  functions_map_["Memory.startTracing"] = &InspectorMemoryAgent::StartTracing;
  functions_map_["Memory.stopTracing"] = &InspectorMemoryAgent::StopTracing;
}

InspectorMemoryAgent::~InspectorMemoryAgent() = default;

void InspectorMemoryAgent::StartTracing(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().MemoryStartTracing(sender, message);
}

void InspectorMemoryAgent::StopTracing(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().MemoryStopTracing(sender, message);
}

void InspectorMemoryAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& content) {
  std::string method = content["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(sender, content);
  } else {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = content["id"].asInt64();
    sender->SendMessage("CDP", res);
  }
}

}  // namespace devtool
}  // namespace lynx
