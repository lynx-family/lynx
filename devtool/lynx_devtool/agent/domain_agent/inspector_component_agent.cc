// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_component_agent.h"

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace devtool {

using lynx::tasm::LynxEnv;

InspectorComponentAgent::InspectorComponentAgent() {
  functions_map_["Component.uselessUpdate"] =
      &InspectorComponentAgent::UselessUpdate;
}

InspectorComponentAgent::~InspectorComponentAgent() = default;

void InspectorComponentAgent::UselessUpdate(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  Json::Value content(Json::ValueType::objectValue);
  content["method"] = "Component.uselessUpdate";
  content["params"] = message["params"];
  sender->SendMessage("CDP", content);
}

void InspectorComponentAgent::CallMethod(
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
