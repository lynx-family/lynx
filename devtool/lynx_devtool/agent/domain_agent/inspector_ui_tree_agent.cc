// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_ui_tree_agent.h"

#include <queue>

#include "devtool/lynx_devtool/element/element_inspector.h"

namespace lynx {
namespace devtool {

InspectorUITreeAgent::InspectorUITreeAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["UITree.enable"] = &InspectorUITreeAgent::Enable;
  functions_map_["UITree.disable"] = &InspectorUITreeAgent::Disable;
  functions_map_["UITree.getLynxUITree"] = &InspectorUITreeAgent::GetLynxUITree;
  functions_map_["UITree.getUIInfoForNode"] =
      &InspectorUITreeAgent::GetUIInfoForNode;
  functions_map_["UITree.setUIStyle"] = &InspectorUITreeAgent::SetUIStyle;
}

void InspectorUITreeAgent::Enable(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  devtool_mediator_->UITree_Enable(sender, message);
}

void InspectorUITreeAgent::Disable(const std::shared_ptr<MessageSender>& sender,
                                   const Json::Value& message) {
  devtool_mediator_->UITree_Disable(sender, message);
}

void InspectorUITreeAgent::GetLynxUITree(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetLynxUITree(sender, message);
}

void InspectorUITreeAgent::GetUIInfoForNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetUIInfoForNode(sender, message);
}

void InspectorUITreeAgent::SetUIStyle(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetUIStyle(sender, message);
}

void InspectorUITreeAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["error"]["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

}  // namespace devtool
}  // namespace lynx
